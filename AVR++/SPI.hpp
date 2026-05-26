#pragma once

/*
 * File:   SPI.h
 * Author: Cameron
 *
 * Created on December 10, 2014, 4:10 PM
 */

#include "bitTypes.hpp"
#include "undefAVR.hpp"
#include <AVR++/IOpin.hpp>
#include <avr/io.h>

namespace AVR {
namespace SPI {
typedef union {
  struct {
    b2 Divider : 2;
    // CPHA
    b1 ClockPhase : 1;
    // CPOL
    b1 ClockPolarity : 1;
    b1 Master : 1;
    // True: LSB first, False: MSB first
    b1 Order : 1;
    b1 Enable : 1;
    b1 InterruptEnable : 1;
  };
  u1 byte;
} CRt;

typedef union {
  struct {
    b1 DoubleSpeed : 1;
    u1 : 5;
    b1 Collision : 1;
    b1 InterruptFlag : 1;
  };
  u1 byte;
} SRt;

// `&SPCR` and friends are reinterpret_casts in disguise (libc macros expand
// to `*(volatile uint8_t *)(addr)`), which aren't constexpr. Use C++17
// inline variables — runtime-initialized once, no linker duplicates.
inline volatile CRt *const CR = (volatile CRt *const)&SPCR;
inline volatile SRt *const SR = (volatile SRt *const)&SPSR;
inline volatile u1 *const DR = &SPDR;

#ifdef __AVR_ATmega32U4__
using SS = IOpin<Ports::B, 0>;
using SCLK = IOpin<Ports::B, 1>;
using MOSI = IOpin<Ports::B, 2>;
using MISO = IOpin<Ports::B, 3>;
#endif
// TODO: Support more chips here
}; // namespace SPI
}; // namespace AVR
