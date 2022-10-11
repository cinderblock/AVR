#pragma once

#include "basicTypes.hpp"

namespace GCR {
constexpr static unsigned inBits = 5;
constexpr static unsigned inCeiling = 1 << inBits;
constexpr static unsigned inMax = inCeiling - 1;
constexpr static unsigned inMask = inCeiling - 1;

constexpr static unsigned outBits = 4;
constexpr static unsigned outCeiling = 1 << outBits;
constexpr static unsigned outMax = outCeiling - 1;
constexpr static unsigned outMask = outCeiling - 1;

inline static constexpr Basic::u1 decode(Basic::u1 const gcr) {
  // clang-format off
  switch (gcr) {
    case 0b11001: return 0b0000;
    case 0b11011: return 0b0001;
    case 0b10010: return 0b0010; // gcr & 0b1111
    case 0b10011: return 0b0011; // gcr & 0b1111
    case 0b11101: return 0b0100;
    case 0b10101: return 0b0101; // gcr & 0b1111
    case 0b10110: return 0b0110; // gcr & 0b1111
    case 0b10111: return 0b0111; // gcr & 0b1111
    case 0b11010: return 0b1000;
    case 0b01001: return 0b1001; // gcr & 0b1111
    case 0b01010: return 0b1010; // gcr & 0b1111
    case 0b01011: return 0b1011; // gcr & 0b1111
    case 0b11110: return 0b1100;
    case 0b01101: return 0b1101; // gcr & 0b1111
    case 0b01110: return 0b1110; // gcr & 0b1111
    case 0b01111: return 0b1111; // gcr & 0b1111
    default: return 0xff;
  }
  // clang-format on
}

} // namespace GCR