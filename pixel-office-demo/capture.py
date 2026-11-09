#!/usr/bin/env python3
"""Capture the demo framebuffer over UART without resetting the board (stdlib only)."""
import argparse
import collections
import os
from pathlib import Path
import select
import struct
import termios
import time
import zlib


def capture(port, output):
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    previous = termios.tcgetattr(fd)
    try:
        settings = termios.tcgetattr(fd)
        settings[0] = settings[1] = settings[3] = 0
        settings[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        settings[4] = settings[5] = termios.B115200
        settings[6][termios.VMIN] = 0
        settings[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, settings)
        termios.tcflush(fd, termios.TCIFLUSH)
        os.write(fd, b's')
        data = bytearray()
        deadline = time.monotonic() + 30
        start = -1
        while time.monotonic() < deadline:
            readable, _, _ = select.select([fd], [], [], max(0, deadline - time.monotonic()))
            if not readable:
                break
            chunk = os.read(fd, 16384)
            if not chunk:
                continue
            data.extend(chunk)
            if start < 0:
                start = data.find(b'PXOF')
            if start >= 0 and len(data) >= start + 8:
                width, height = struct.unpack('>HH', data[start + 4:start + 8])
                if (width, height) != (320, 240):
                    raise RuntimeError(f'Unexpected frame size: {width}x{height}')
                if len(data) >= start + 8 + width * height * 2:
                    break
        else:
            raise TimeoutError('Frame capture exceeded 30 seconds')
        if start < 0 or len(data) < start + 8 + 320 * 240 * 2:
            raise RuntimeError(f'Incomplete framebuffer: {len(data)} bytes received')
        pixels = data[start + 8:start + 8 + width * height * 2]
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, previous)
        os.close(fd)

    scanlines = bytearray()
    colors = collections.Counter()
    for y in range(height):
        scanlines.append(0)  # PNG filter: none
        for x in range(width):
            pixel = struct.unpack_from('<H', pixels, (y * width + x) * 2)[0]
            colors[pixel] += 1
            r, g, b = (pixel >> 11) & 31, (pixel >> 5) & 63, pixel & 31
            scanlines.extend(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))

    image = b'\x89PNG\r\n\x1a\n'
    image += chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    image += chunk(b'IDAT', zlib.compress(scanlines)) + chunk(b'IEND', b'')
    Path(output).write_bytes(image)
    print(f'Saved {width}x{height} framebuffer: {output}; {len(colors)} colors')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True, help='Serial port of the ESP32 running PixelOfficeDemo')
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    capture(args.port, args.output)
