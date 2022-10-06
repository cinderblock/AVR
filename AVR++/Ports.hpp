#pragma once

/*
 * File:   Ports.h
 * Author: Cameron
 *
 */

#include "basicTypes.hpp"
#include "undefAVR.hpp"
#include <avr/io.h>

namespace AVR {
using namespace Basic;
enum class Ports : u1 {
#ifdef __AVR_ATmega32U4__
  B = 0x20 - 1 + 3 * 2,
  C = 0x20 - 1 + 3 * 3,
  D = 0x20 - 1 + 3 * 4,
  E = 0x20 - 1 + 3 * 5,
  F = 0x20 - 1 + 3 * 6,
#else
#error "Unsupported MCU"
#endif
}; // namespace Ports
} // namespace AVR
