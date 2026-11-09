#pragma once
#include <cstdlib>
#include <cstdint>
#include <cstdio>
inline long random(long maximum) { return maximum > 0 ? std::rand() % maximum : 0; }
inline uint32_t millis() { return 0; }
