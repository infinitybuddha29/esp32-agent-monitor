#pragma once
#include <stdint.h>

// Character-aligned movement, with reading pauses at both ends.
inline uint32_t marqueeOffset(uint32_t length, uint32_t width, uint32_t elapsed) {
  if (length <= width) return 0;
  const uint32_t steps = length - width;
  const uint32_t hold = 1800, stepMs = 350;
  const uint32_t phase = elapsed % (hold * 2 + steps * stepMs);
  if (phase < hold) return 0;
  const uint32_t offset = (phase - hold) / stepMs + 1;
  return offset > steps ? steps : offset;
}
