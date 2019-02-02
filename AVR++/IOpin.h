/*
 * File: IOpin.h
 * Author: chtacklind
 *
 * Created on May 5, 2010, 3:23 PM
 */

#ifndef _IOPIN_H
#define _IOPIN_H

#include "Ports.h"
#include <avr/io.h>

namespace AVR {

using namespace Basic;

template <class port, u1 pin> class IOpin {
  constexpr static volatile u1 *const DDR = port::DDR();
  constexpr static volatile u1 *const PIN = port::PIN();
  constexpr static volatile u1 *const PUE = port::PUE();
  constexpr static volatile u1 *const PORT = port::PORT();
  constexpr static u1 mask = 1 << pin;

public:
  inline IOpin() {}

  /**
   * Sets bit in DDRx
   */
  inline static void output() { *DDR |= mask; }
  /**
   * Clears bit in DDRx
   */
  inline static void input() { *DDR &= ~mask; }

  /**
   * Sets bit in PUEx (or PORT)
   */
  inline static void enablePullUp() { *PUE |= mask; }
  /**
   * Clears bit in PUEx (or PORT)
   */
  inline static void disablePullUp() { *PUE &= ~mask; }
  /**
   * Clears bit in PUEx (or PORT)
   */
  inline static void setPullUp(bool v) { v ? enablePullUp() : disablePullUp(); }

  /**
   * Sets bit in PORTx
   */
  inline static void set() { *PORT |= mask; }

  /**
   * Clears bit in PORTx
   */
  inline static void clr() { *PORT &= ~mask; }

  /**
   * Sets bit in PINx
   */
  inline static void tgl() { *PIN = mask; }

  /**
   * Returns value of bit in PINx
   */
  inline static bool isHigh() { return *PIN & mask; }

  /**
   * Returns value of bit in PORTx
   */
  inline static bool isDriveHigh() { return *PORT & mask; }

  /**
   * set() or clr() based on v
   */
  inline static void set(bool v) { v ? set() : clr(); }

  /**
   * set() or clr() based on v
   */
  inline bool operator=(bool v) {
    set(v);
    return v;
  }

  /**
   * Toggles the output
   */
  inline bool operator++(int) {
    tgl();
    return isDriveHigh();
  }
};

template <class port, u1 pin, bool inverted = false, bool startOn = false> class Output : public IOpin<port, pin> {
  using IOpin<port, pin>::output;
  using IOpin<port, pin>::isDriveHigh;

public:
  inline Output() {}

  /**
   * Turns on output, whatever logic level that is
   */
  inline static void on() { IOpin<port, pin>::set(!inverted); }

  /**
   * Turns off output, whatever logic level that is
   */
  inline static void off() { IOpin<port, pin>::set(inverted); }

  /**
   * Checks output state, based on if this Output is inverted or not
   */
  inline static bool isOn() { return isDriveHigh() != inverted; }

  /**
   * Turns on() or off() based on v
   */
  inline static void set(bool v) { v ? on() : off(); }

  /**
   * Turns on() or off() based on v
   */
  inline bool operator=(bool v) {
    set(v);
    return v;
  }

  inline operator bool() const { return isOn(); }

private:
  static void init() __attribute__((constructor, used)) {
    set(startOn);
    output();
  }
};

template <class port, u1 pin, bool activeLow = true, bool pullUp = activeLow> class Input : public IOpin<port, pin> {
  using IOpin<port, pin>::setPullUp;
  using IOpin<port, pin>::isHigh;
  using IOpin<port, pin>::input;

public:
  inline Input() {}
  inline static bool isActive() { return isHigh() != activeLow; }

  inline operator bool() const { return isActive(); }

private:
  static void init() __attribute__((constructor, used)) {
    input();
    setPullUp(pullUp);
  }
};

}; // namespace AVR

#endif /* _IOPIN_H */
