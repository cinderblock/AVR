/*
* File: IOpin.h
* Author: chtacklind
*
* Created on May 5, 2010, 3:23 PM
*/

#ifndef _IOPIN_H
#define _IOPIN_H

#include <avr/io.h>
#include "Ports.h"

namespace AVR {

template <class port, u1 pin>
class IOpin {
  constexpr static volatile u1 * DDR = port::DDR();
  constexpr static volatile u1 * PIN = port::PIN();
  constexpr static volatile u1 * PUE = port::PUE();
  constexpr static volatile u1 * PORT = port::PORT();
  constexpr static u1 mask = 1 << pin;
public:
inline IOpin() {}

/**
 * Sets bit in DDRx
 */
inline static void output() {*DDR |= mask;}
/**
 * Clears bit in DDRx
 */
inline static void input() {*DDR &= ~mask;}

/**
 * Sets bit in PUEx (or PORT)
 */
inline static void enablePullUp() {*PUE |= mask;}
/**
 * Clears bit in PUEx (or PORT)
 */
inline static void disablePullUp() {*PUE &= ~mask;}
/**
 * Clears bit in PUEx (or PORT)
 */
inline static void setPullUp(bool v) {v ? enablePullUp() : disablePullUp();}

/**
 * Sets bit in PORTx
 */
inline static void on() {*PORT |= mask;}

/**
 * Clears bit in PORTx
 */
inline static void off() {*PORT &= ~mask;}

/**
 * Sets bit in PINx
 */
inline static void tgl() {*PIN = mask;}

/**
 * Returns value of bit in PINx
 */
inline static bool isHigh() {return *PIN & mask;}

/**
 * Returns value of bit in PORTx
 */
inline static bool isDriveHigh() {return *PORT & mask;}

/**
 * Turns on() or off() based on v
 */
inline static void set(bool v) {if (v) on(); else off();}

/**
 * Turns on() or off() based on v
 */
inline bool operator= (bool v) {set(v); return v;}

/**
 * Toggles the output
 */

inline void operator++ (int) {tgl();}
};

};

#endif /* _IOPIN_H */
