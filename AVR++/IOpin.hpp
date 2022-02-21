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

template <Ports port, unsigned pin> class IOpin {
  constexpr static auto PUE = u1(port) - 0;
  constexpr static auto PORT = u1(port);
  constexpr static auto DDR = u1(port) - 1;
  constexpr static auto PIN = u1(port) - 2;
  constexpr static u1 mask = 1 << pin;

  static_assert(pin <= 8, "AVR has 8-bit IO ports. '8' is allowed to disable a pin");

public:
  inline IOpin() {}

  /**
   * Sets bit in DDRx
   */
  inline static void output() { *(volatile uint8_t *)DDR |= mask; }
  /**
   * Clears bit in DDRx
   */
  inline static void input() { *(volatile uint8_t *)DDR &= ~mask; }

  /**
   * Sets bit in PUEx (or PORT)
   */
  inline static void enablePullUp() { *(volatile uint8_t *)PUE |= mask; }
  /**
   * Clears bit in PUEx (or PORT)
   */
  inline static void disablePullUp() { *(volatile uint8_t *)PUE &= ~mask; }
  /**
   * Clears bit in PUEx (or PORT)
   */
  inline static void setPullUp(bool v) { v ? enablePullUp() : disablePullUp(); }

  /**
   * Sets bit in PORTx
   */
  inline static void set() { *(volatile uint8_t *)PORT |= mask; }

  /**
   * Clears bit in PORTx
   */
  inline static void clr() { *(volatile uint8_t *)PORT &= ~mask; }

  /**
   * Sets bit in PINx
   */
  inline static void tgl() { *(volatile uint8_t *)PIN = mask; }

  /**
   * Returns value of bit in PINx
   */
  inline static bool isHigh() { return *(volatile uint8_t *)PIN & mask; }

  /**
   * Returns value of bit in PORTx
   */
  inline static bool isDriveHigh() { return *(volatile uint8_t *)PORT & mask; }

  /**
   * Returns value of bit in DDRx
   */
  inline static bool isOutputEnabled() { return *(volatile uint8_t *)DDR & mask; }

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

template <Ports port, unsigned pin, bool inverted = false, bool startOn = false, bool init = true>
class WeakOutput : public IOpin<port, pin> {
protected:
  using IOpin<port, pin>::input;
  using IOpin<port, pin>::isDriveHigh;

public:
  inline WeakOutput() {}

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
  inline static void init_() __attribute__((constructor, used)) {
    if (!init)
      return;
    input();
    set(startOn);
  }
};

template <Ports port, unsigned pin, bool inverted = false, bool startOn = false>
class Output : public WeakOutput<port, pin, inverted, startOn, false> {
protected:
  using WeakOutput<port, pin, inverted, startOn, false>::input;
  using WeakOutput<port, pin, inverted, startOn, false>::output;
  using WeakOutput<port, pin, inverted, startOn, false>::isDriveHigh;

public:
  inline Output() {}

  using WeakOutput<port, pin, inverted, startOn, false>::on;
  using WeakOutput<port, pin, inverted, startOn, false>::off;
  using WeakOutput<port, pin, inverted, startOn, false>::isOn;
  using WeakOutput<port, pin, inverted, startOn, false>::set;
  using WeakOutput<port, pin, inverted, startOn, false>::operator=;
  using WeakOutput<port, pin, inverted, startOn, false>::operator bool;

private:
  inline static void init() __attribute__((constructor, used)) {
    set(startOn);
    output();
  }
};

template <Ports port, unsigned pin, bool activeLow = true, bool pullUp = activeLow>
class Input : public IOpin<port, pin> {
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

template <Ports port, unsigned pin, bool activeLow = true, bool pullUp = false>
class OpenDrain : public IOpin<port, pin> {
  using IOpin<port, pin>::setPullUp;
  using IOpin<port, pin>::isHigh;
  using IOpin<port, pin>::output;
  using IOpin<port, pin>::input;
  using IOpin<port, pin>::clr;
  using IOpin<port, pin>::set;
  using IOpin<port, pin>::isOutputEnabled;

public:
  inline OpenDrain() {}
  inline static bool isSink() { return isOutputEnabled(); }
  inline static bool isOpen() { return !isOutputEnabled(); }

  inline static bool isActive() { return isHigh() != activeLow; }

  inline operator bool() const { return isActive(); }

  inline static void sink() {
    if (pullUp) {
      clr();
    }
    output();
  }

  inline static void open() {
    input();
    if (pullUp) {
      set();
    }
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

private:
  static void init() __attribute__((constructor, used)) {
    input();
    if (pullUp) {
      setPullUp(pullUp);
    }
  }
};
}; // namespace AVR
