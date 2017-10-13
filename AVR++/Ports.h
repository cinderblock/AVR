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
      constexpr static volatile u1* PUE() {
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
      constexpr static volatile u1* PORT() {return &PORTB;}
      constexpr static volatile u1* PIN() {return &PINB;}
      constexpr static volatile u1* DDR() {return &DDRB;}
      constexpr static volatile u1* PUE() {
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
      constexpr static volatile u1* PORT() {return &PORTC;}
      constexpr static volatile u1* PIN() {return &PINC;}
      constexpr static volatile u1* DDR() {return &DDRC;}
      constexpr static volatile u1* PUE() {
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
      constexpr static volatile u1* PORT() {return &PORTD;}
      constexpr static volatile u1* PIN() {return &PIND;}
      constexpr static volatile u1* DDR() {return &DDRD;}
      constexpr static volatile u1* PUE() {
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
      constexpr static volatile u1* PORT() {return &PORTE;}
      constexpr static volatile u1* PIN() {return &PINE;}
      constexpr static volatile u1* DDR() {return &DDRE;}
      constexpr static volatile u1* PUE() {
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
      constexpr static volatile u1* PORT() {return &PORTF;}
      constexpr static volatile u1* PIN() {return &PINF;}
      constexpr static volatile u1* DDR() {return &DDRF;}
      constexpr static volatile u1* PUE() {
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
      constexpr static volatile u1* PORT() {return &PORTG;}
      constexpr static volatile u1* PIN() {return &PING;}
      constexpr static volatile u1* DDR() {return &DDRG;}
      constexpr static volatile u1* PUE() {
        #ifdef PUEG
          return &PUEG;
        #else
          return &PORTG;
        #endif
      }
    };
    #endif
  };
};
#endif	/* PORTS_H_ */
