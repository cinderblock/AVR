/* 
 * File:   IOpin.h
 * Author: chtacklind
 *
 * Created on May 5, 2010, 3:23 PM
 */

#ifndef _IOPIN_H
#define	_IOPIN_H

#include <avr/sfr_defs.h>
#include "basicTypes.h"
#include "bitTypes.h"

#define PORT(port) _MMIO_BYTE(port - 0)
#define DDR(port)  _MMIO_BYTE(port - 1)
#define PIN(port)  _MMIO_BYTE(port - 2)

#undef AVR

namespace AVR {

class IOpin {
 volatile u1 * const port;
 u1 const mask;
public:
 IOpin(volatile u1 * const port, b3 const pin): port(port), mask(_BV(pin)) {}

 /* Sets bit in DDRx */
 inline void output() const {DDR(port) |=  mask;}
 /* Clears bit in DDRx */
 inline void input() const {DDR(port) &= ~mask;}

 /* Sets bit in PORTx */
 inline void on() const {PORT(port) |=  mask;}
 /* Clears bit in PORTx */
 inline void off() const {PORT(port) &= ~mask;}
 
 /* Sets bit in PINx */
 inline void tgl() const {PIN(port) = mask;}

 /* Returns value of bit in PINx. Not 0 or 1 but 0 or _BV(pin) */
 inline bool isHigh() const {return PIN(port) & mask;}

 /* Turns on() or off() */
 inline void set(bool const v) const {if (v) on(); else off();}
 inline bool operator=(bool const v) const {set(v); return v;}

 inline void operator++ (int) const {tgl();}
 inline void operator++ (   ) const {tgl();}
 
 inline operator bool () const {return  isHigh();}
 inline bool operator!() const {return !isHigh();}
};

};

#undef PORT
#undef  DDR
#undef  PIN

#endif	/* _IOPIN_H */