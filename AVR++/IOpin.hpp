#pragma once

/*
 * File: IOpin.h
 * Author: Cameron Tacklind
 *
 * Created on May 5, 2010, 3:23 PM
 */

#include "Ports.hpp"
#include <avr/io.h>

namespace AVR {

using namespace Basic;

template <Ports Port, unsigned Pin>
class IOpin {
  constexpr static auto PUE = u1(Port) - 0;
  constexpr static auto PORT = u1(Port);
  constexpr static auto DDR = u1(Port) - 1;
  constexpr static auto PIN = u1(Port) - 2;
  constexpr static u1 mask = u1(1 << Pin);

  static_assert(Pin <= 8, "AVR has 8-bit IO Ports. '8' is allowed to disable a Pin");

public:
  constexpr static bool isDummy = Pin == 8;

  constexpr inline IOpin() {}

  /**
   * Sets bit in DDRx
   */
  inline static void output() {
    if (!mask) {
      asm("; IOpin::output dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    *(volatile uint8_t *)DDR |= mask;
  }
  /**
   * Clears bit in DDRx
   */
  inline static void input() {
    if (!mask) {
      asm("; IOpin::input dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    *(volatile uint8_t *)DDR &= ~mask;
  }

  /**
   * Sets bit in PUEx (or PORT)
   */
  inline static void enablePullUp() {
    if (!mask) {
      asm("; IOpin::enablePullUp dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    *(volatile uint8_t *)PUE |= mask;
  }
  /**
   * Clears bit in PUEx (or PORT)
   */
  inline static void disablePullUp() {
    if (!mask) {
      asm("; IOpin::disablePullUp dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    *(volatile uint8_t *)PUE &= ~mask;
  }
  /**
   * Clears bit in PUEx (or PORT)
   */
  inline static void setPullUp(bool v) {
    if (!mask) {
      asm("; IOpin::setPullUp dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    v ? enablePullUp() : disablePullUp();
  }

  /**
   * Sets bit in PORTx
   */
  inline static void set() {
    if (!mask) {
      asm("; IOpin::set dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    *(volatile uint8_t *)PORT |= mask;
  }

  /**
   * Clears bit in PORTx
   */
  inline static void clr() {
    if (!mask) {
      asm("; IOpin::clr dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    *(volatile uint8_t *)PORT &= ~mask;
  }

  /**
   * Sets bit in PINx
   */
  inline static void tgl() {
    if (!mask) {
      asm("; IOpin::tgl dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    *(volatile uint8_t *)PIN = mask;
  }

  /**
   * Returns value of bit in PINx
   */
  inline static bool isHigh() {
    if (!mask) {
      asm("; IOpin::isHigh dummy. %[PORT]" ::[PORT] "I"(Port));
      return false;
    }

    return *(volatile uint8_t *)PIN & mask;
  }

  /**
   * Returns value of bit in PORTx
   */
  inline static bool isDriveHigh() {
    if (!mask) {
      asm("; IOpin::isDriveHigh dummy. %[PORT]" ::[PORT] "I"(Port));
      return false;
    }

    return *(volatile uint8_t *)PORT & mask;
  }

  /**
   * Returns value of bit in DDRx
   */
  inline static bool isOutputEnabled() {
    if (!mask) {
      asm("; IOpin::isOutputEnabled dummy. %[PORT]" ::[PORT] "I"(Port));
      return false;
    }

    return *(volatile uint8_t *)DDR & mask;
  }

  /**
   * set() or clr() based on v
   */
  inline static void set(bool v) {
    if (!mask) {
      asm("; IOpin::set dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    v ? set() : clr();
  }

  /**
   * set() or clr() based on v
   */
  inline bool operator=(bool v) {
    if (!mask) {
      asm("; IOpin::operator=(bool v) dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    set(v);
    return v;
  }

  /**
   * Toggles the output
   */
  inline bool operator++(int) {
    if (!mask) {
      asm("; IOpin::operator++(int) dummy. %[PORT]" ::[PORT] "I"(Port));
      return;
    }

    tgl();
    return isDriveHigh();
  }
};

template <Ports Port, unsigned Pin, bool inverted = false, bool startOn = false>
class WeakOutput : public IOpin<Port, Pin> {
protected:
  using IOpin<Port, Pin>::input;
  using IOpin<Port, Pin>::isDriveHigh;

public:
  inline WeakOutput() {}

  /**
   * Turns on output, whatever logic level that is
   */
  inline static void on() { IOpin<Port, Pin>::set(!inverted); }

  /**
   * Turns off output, whatever logic level that is
   */
  inline static void off() { IOpin<Port, Pin>::set(inverted); }

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

  inline static void init() {
    input();
    set(startOn);
  }
};

template <Ports Port, unsigned Pin, bool inverted = false, bool startOn = false>
class Output : public WeakOutput<Port, Pin, inverted, startOn> {
protected:
  using IOpin<Port, Pin>::output;
  using WeakOutput<Port, Pin, inverted, startOn>::input;
  using WeakOutput<Port, Pin, inverted, startOn>::output;
  using WeakOutput<Port, Pin, inverted, startOn>::isDriveHigh;

public:
  inline Output() {}

  using WeakOutput<Port, Pin, inverted, startOn>::on;
  using WeakOutput<Port, Pin, inverted, startOn>::off;
  using WeakOutput<Port, Pin, inverted, startOn>::isOn;
  using WeakOutput<Port, Pin, inverted, startOn>::set;
  using WeakOutput<Port, Pin, inverted, startOn>::operator=;
  using WeakOutput<Port, Pin, inverted, startOn>::operator bool;

  inline static void init() {
    set(startOn);
    output();
  }
};

template <Ports Port, unsigned Pin, bool activeLow = true, bool pullUp = activeLow>
class Input : public IOpin<Port, Pin> {
  using IOpin<Port, Pin>::setPullUp;
  using IOpin<Port, Pin>::isHigh;
  using IOpin<Port, Pin>::input;

public:
  inline Input() {}
  inline static bool isActive() { return isHigh() != activeLow; }

  inline operator bool() const { return isActive(); }

  inline static void init() {
    input();
    setPullUp(pullUp);
  }
};

template <Ports Port, unsigned Pin, bool activeLow = true, bool pullUp = false>
class OpenDrain : public IOpin<Port, Pin> {
  using IOpin<Port, Pin>::setPullUp;
  using IOpin<Port, Pin>::isHigh;
  using IOpin<Port, Pin>::output;
  using IOpin<Port, Pin>::input;
  using IOpin<Port, Pin>::clr;
  using IOpin<Port, Pin>::set;
  using IOpin<Port, Pin>::isOutputEnabled;

public:
  inline OpenDrain() {}
  inline static bool isSink() { return isOutputEnabled(); }
  inline static bool isOpen() { return !isOutputEnabled(); }

  inline static bool isActive() { return isHigh() != activeLow; }

  inline operator bool() const { return isActive(); }

  inline static void sink() {
    if (pullUp) { clr(); }
    output();
  }

  inline static void open() {
    input();
    if (pullUp) { set(); }
  }

  inline static void tgl() {
    if (isSink()) {
      open();
    } else {
      sink();
    }
  }

  /**
   * sink() or open() based on v
   */
  inline static void set(bool v) { (v == activeLow) ? sink() : open(); }

  /**
   * sink() or open() based on v
   */
  inline bool operator=(bool v) {
    set(v);
    return v;
  }

  inline static void init() {
    input();
    if (pullUp) { setPullUp(pullUp); }
  }
};
}; // namespace AVR
