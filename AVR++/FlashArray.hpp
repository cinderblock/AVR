/*
 * File:   FlashArray.h
 * Author: Cameron
 *
 */
#pragma once

#include "undefAVR.hpp"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stddef.h>

namespace AVR {
template <size_t location, unsigned int count, class T = uint8_t>
class FlashArray {
  static_assert(sizeof(T) * count + location <= FLASHEND, "Referencing data outside of addressable space!");

public:
  constexpr static auto length = count;
  inline T const operator[](const int index) const {
    union {
      T ret;
      unsigned char bytes[sizeof(T)];
    };

    for (size_t i = 0; i < sizeof(T); i++)
      bytes[i] = pgm_read_byte(location + index * sizeof(T) + i);

    return ret;
  };
};
}; // namespace AVR
