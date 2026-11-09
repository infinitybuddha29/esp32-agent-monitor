#pragma once
#include <cstdint>
// Persistence must be unreachable in the filming build.
#include <cstdlib>
struct Preferences {
    bool begin(const char*, bool) { std::abort(); }
    bool getBool(const char*, bool) { std::abort(); }
    uint8_t getUChar(const char*, uint8_t) { std::abort(); }
    void putBool(const char*, bool) { std::abort(); }
    void putUChar(const char*, uint8_t) { std::abort(); }
    void end() { std::abort(); }
};
