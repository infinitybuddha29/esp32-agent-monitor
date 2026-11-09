"""Build a 7x13 bitmap font: ASCII + Russian; requires Pillow only at build time."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

chars = ''.join(chr(i) for i in range(32, 127)) + ''.join(chr(i) for i in range(0x410, 0x450)) + 'Ёё'
font_path = Path(__file__).resolve().parent / 'fonts/DejaVuSansMono.ttf'
font = ImageFont.truetype(str(font_path), 11)
rows = []
for ch in chars:
    image = Image.new('1', (7, 13))
    # Fixed seven-pixel cells, including Cyrillic. Font license: fonts/LICENSE_DEJAVU.
    # Keep a common baseline and verify the complete glyph fits the cell.
    left, top, right, bottom = font.getbbox(ch)
    dy = min(0, 13-bottom)
    assert left >= 0 and right <= 7 and top+dy >= 0 and bottom+dy <= 13, (ch, font.getbbox(ch))
    ImageDraw.Draw(image).text((0, dy), ch, font=font, fill=1)
    rows.append('{' + ','.join(str(sum((1 << (6-x)) for x in range(7) if image.getpixel((x,y)))) for y in range(13)) + '}')
out = Path(__file__).resolve().parents[1] / 'firmware/AgentMonitor/font_ru.h'
out.write_text('// Bitmap glyphs derived from DejaVu Sans Mono; see tools/fonts/LICENSE_DEJAVU.\n#pragma once\nconst uint8_t RU_FONT[][13] PROGMEM = {\n' + ',\n'.join(rows) + '\n};\n')
