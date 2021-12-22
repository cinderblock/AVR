#pragma once

/*
 * File:   Ports.h
 * Author: Cameron
 *
 */

#include "basicTypes.hpp"
#include <avr/io.h>

namespace AVR {
namespace Ports {

using namespace Basic;
#ifdef PORTA
struct A {
  constexpr static volatile u1 *const PORT() { return &PORTA; }
  constexpr static volatile u1 *const PIN() { return &PINA; }
  constexpr static volatile u1 *const DDR() { return &DDRA; }
  constexpr static volatile u1 *const PUE() {
#ifdef PUEA
    return &PUEA;
#else
    return &PORTA;
#endif
  }
};
#endif
#ifdef PORTB
struct B {
  constexpr static volatile u1 *const PORT() { return &PORTB; }
  constexpr static volatile u1 *const PIN() { return &PINB; }
  constexpr static volatile u1 *const DDR() { return &DDRB; }
  constexpr static volatile u1 *const PUE() {
#ifdef PUEB
    return &PUEB;
#else
    return &PORTB;
#endif
  }
};
#endif
#ifdef PORTC
struct C {
  constexpr static volatile u1 *const PORT() { return &PORTC; }
  constexpr static volatile u1 *const PIN() { return &PINC; }
  constexpr static volatile u1 *const DDR() { return &DDRC; }
  constexpr static volatile u1 *const PUE() {
#ifdef PUEC
    return &PUEC;
#else
    return &PORTC;
#endif
  }
};
#endif
#ifdef PORTD
struct D {
  constexpr static volatile u1 *const PORT() { return &PORTD; }
  constexpr static volatile u1 *const PIN() { return &PIND; }
  constexpr static volatile u1 *const DDR() { return &DDRD; }
  constexpr static volatile u1 *const PUE() {
#ifdef PUED
    return &PUED;
#else
    return &PORTD;
#endif
  }
};
#endif
#ifdef PORTE
struct E {
  constexpr static volatile u1 *const PORT() { return &PORTE; }
  constexpr static volatile u1 *const PIN() { return &PINE; }
  constexpr static volatile u1 *const DDR() { return &DDRE; }
  constexpr static volatile u1 *const PUE() {
#ifdef PUEE
    return &PUEE;
#else
    return &PORTE;
#endif
  }
};
#endif
#ifdef PORTF
struct F {
  constexpr static volatile u1 *const PORT() { return &PORTF; }
  constexpr static volatile u1 *const PIN() { return &PINF; }
  constexpr static volatile u1 *const DDR() { return &DDRF; }
  constexpr static volatile u1 *const PUE() {
#ifdef PUEF
    return &PUEF;
#else
    return &PORTF;
#endif
  }
};
#endif
#ifdef PORTG
struct G {
  constexpr static volatile u1 *const PORT() { return &PORTG; }
  constexpr static volatile u1 *const PIN() { return &PING; }
  constexpr static volatile u1 *const DDR() { return &DDRG; }
  constexpr static volatile u1 *const PUE() {
#ifdef PUEG
    return &PUEG;
#else
    return &PORTG;
#endif
  }
};
#endif
};     // namespace Ports
};     // namespace AVR
