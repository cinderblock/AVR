#pragma once

/*
 * File:   WDT.h
 *
 * Created on January 25, 2012, 11:33 AM
 */

#include "basicTypes.hpp"
#include "undefAVR.hpp"

namespace AVR {

enum class GlobalInterruptHandlingStrategy {
  None,
  AlwaysOn,
  Maintain,
};

using Basic::u1;

class WDT {
public:
  enum class AutomaticTick {
    Off,
    On,
  };

  enum class Timeout : u1 {
    T__15ms = 0,
    T__30ms = 1,
    T__60ms = 2,
    T_120ms = 3,
    T_250ms = 4,
    T_500ms = 5,
    T1000ms = 6,
    T2000ms = 7,
    T4000ms = 8,
    T8000ms = 9
  };

  inline constexpr static u1 timeoutBits(Timeout t) {
    const u1 v = static_cast<u1>(t);
    const u1 low = v & 0b111;
    const u1 high = (v >> 3) & 0b1;
    return low | (high << WDP3);
  }

  /**
   * Resets the WDT (call this more often than the timeout!)
   */
  static inline void tick() { asm volatile("wdr"); }

  /**
   * Starts the watchdog timer with timeout t
   * @param t the timeout to wait for
   */
  template <GlobalInterruptHandlingStrategy interruptStrategy = GlobalInterruptHandlingStrategy::None,
            AutomaticTick handleTick = AutomaticTick::On>
  static inline void start(Timeout t, bool reset = true, bool interrupt = false) {
    u1 sreg;

    switch (interruptStrategy) {
    case GlobalInterruptHandlingStrategy::Maintain:
      sreg = SREG;
    // Fall through
    case GlobalInterruptHandlingStrategy::AlwaysOn:
      asm volatile("cli");
    case GlobalInterruptHandlingStrategy::None:;
    }

    // Ensure a reset doesn't happen while we're stopping
    if (handleTick == AutomaticTick::On) tick();

    // Clear interrupt flag, enable changes, and maybe enable
    WDTCSR = (interrupt << WDIF) | (1 << WDCE) | (reset << WDE);
    // Maybe enable interrupt, keep changes enabled, and set timeout
    WDTCSR = (interrupt << WDIE) | (1 << WDCE) | (reset << WDE) | timeoutBits(t);

    switch (interruptStrategy) {
    case GlobalInterruptHandlingStrategy::Maintain:
      SREG = sreg;
      return;
    case GlobalInterruptHandlingStrategy::AlwaysOn:
      asm volatile("sei");
    case GlobalInterruptHandlingStrategy::None:;
    }
  }

  /**
   * Stops the WDT. May not always succeed as WDT can be forced on by fuses
   */
  template <GlobalInterruptHandlingStrategy interruptStrategy = GlobalInterruptHandlingStrategy::None,
            AutomaticTick handleTick = AutomaticTick::On>
  static inline void stop() {
    u1 sreg;

    switch (interruptStrategy) {
    case GlobalInterruptHandlingStrategy::Maintain:
      sreg = SREG;
    // Fall through
    case GlobalInterruptHandlingStrategy::AlwaysOn:
      asm volatile("cli");
    case GlobalInterruptHandlingStrategy::None:;
    }

    // Ensure a reset doesn't happen while we're stopping
    if (handleTick == AutomaticTick::On) tick();

    // Clear WDRF in MCUSR
    MCUSR &= ~(1 << WDRF);

    // Write logical one to WDCE and WDE
    // Keep old prescaler setting to prevent unintentional time-out
    WDTCSR |= (1 << WDCE) | (1 << WDE);

    // Turn off WDT
    WDTCSR = 0x00;

    switch (interruptStrategy) {
    case GlobalInterruptHandlingStrategy::Maintain:
      SREG = sreg;
      return;
    case GlobalInterruptHandlingStrategy::AlwaysOn:
      asm volatile("sei");
    case GlobalInterruptHandlingStrategy::None:;
    }
  }

  static inline bool isEnabled() { return bit_is_set(WDTCSR, WDE); }
  static inline bool isInterruptEnabled() { return bit_is_set(WDTCSR, WDIE); }
  static inline void clearResetFlag() { MCUSR = ~_BV(WDRF); }
};

}; // namespace AVR
