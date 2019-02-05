/*
 * File:   FlashData.h
 * Author: Cameron
 *
 */
#pragma once

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stddef.h>

// Clear out conflicting gcc defines
#undef AVR

namespace AVR {
template <size_t location, class T = uint8_t> class FlashData {
  static_assert(sizeof(T) + location <= FLASHEND, "Referencing data outside of addressable space!");

public:
  inline operator T() const {
    union {
      T ret;
      unsigned char bytes[sizeof(T)];
    };

    for (size_t i = 0; i < sizeof(T); i++)
      bytes[i] = pgm_read_byte(location + i);

    return ret;
  };
};
}; // namespace AVR
