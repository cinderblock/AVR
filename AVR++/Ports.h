/* 
 * File:   Ports.h
 * Author: Cameron
 *
 */

#ifndef PORTS_H_
#define	PORTS_H_

// #include <avr/io.h>
#include "basicTypes.h"

namespace AVR {
  namespace Ports {
    #ifdef PORTA
    struct A {
      constexpr static volatile u1* PORT() {return &PORTA;}
      constexpr static volatile u1* PIN() {return &PINA;}
      constexpr static volatile u1* DDR() {return &DDRA;}
      #ifdef PUEA
      constexpr static volatile u1* PUE() {return &PUEA;}
      #endif
    };
    #endif
    #ifdef PORTB
    struct B {
      constexpr static volatile u1* PORT() {return &PORTB;}
      constexpr static volatile u1* PIN() {return &PINB;}
      constexpr static volatile u1* DDR() {return &DDRB;}
      #ifdef PUEB
      constexpr static volatile u1* PUE() {return &PUEB;}
      #endif
    };
    #endif
    #ifdef PORTC
    struct C {
      constexpr static volatile u1* PORT() {return &PORTC;}
      constexpr static volatile u1* PIN() {return &PINC;}
      constexpr static volatile u1* DDR() {return &DDRC;}
      #ifdef PUEC
      constexpr static volatile u1* PUE() {return &PUEC;}
      #endif
    };
    #endif
    #ifdef PORTD
    struct D {
      constexpr static volatile u1* PORT() {return &PORTD;}
      constexpr static volatile u1* PIN() {return &PIND;}
      constexpr static volatile u1* DDR() {return &DDRD;}
      #ifdef PUED
      constexpr static volatile u1* PUE() {return &PUED;}
      #endif
    };
    #endif
    #ifdef PORTE
    struct E {
      constexpr static volatile u1* PORT() {return &PORTE;}
      constexpr static volatile u1* PIN() {return &PINE;}
      constexpr static volatile u1* DDR() {return &DDRE;}
      #ifdef PUEE
      constexpr static volatile u1* PUE() {return &PUEE;}
      #endif
    };
    #endif
    #ifdef PORTF
    struct F {
      constexpr static volatile u1* PORT() {return &PORTF;}
      constexpr static volatile u1* PIN() {return &PINF;}
      constexpr static volatile u1* DDR() {return &DDRF;}
      #ifdef PUEF
      constexpr static volatile u1* PUE() {return &PUEF;}
      #endif
    };
    #endif
    #ifdef PORTG
    struct G {
      constexpr static volatile u1* PORT() {return &PORTG;}
      constexpr static volatile u1* PIN() {return &PING;}
      constexpr static volatile u1* DDR() {return &DDRG;}
      #ifdef PUEG
      constexpr static volatile u1* PUE() {return &PUEG;}
      #endif
    };
    #endif
  };
};
#endif	/* PORTS_H_ */
