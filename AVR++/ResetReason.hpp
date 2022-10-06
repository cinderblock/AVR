#pragma once

#include "basicTypes.hpp"
#include "undefAVR.hpp"
#include <avr/io.h>

namespace AVR {

struct ResetReason {
  union {
    struct {
      /**
       * First power on
       */
      bool PowerOn : 1;

      /**
       * Reset pin
       */
      bool External : 1;

      /**
       * Internal brown-out detector tripped
       */
      bool BrownOut : 1;

      /**
       * Internal watchdog timer reset tripped
       */
      bool WatchDog : 1;

      /**
       * JTAG debugging
       */
      bool JTAG : 1;

      /**
       * USB Reset event
       */
      bool USB : 1;
    };
    Basic::u1 byte;
  };

  /**
   * Initialize from MCUSR and reset it
   */
  inline ResetReason() : byte(MCUSR) { MCUSR = 0; }

  /**
   * Initialize from MCUSR and don't touch it
   */
  inline ResetReason(const bool) : byte(MCUSR) {}

  /**
   * Initialize from byte
   */
  inline ResetReason(const Basic::u1 v) : byte(v) {}

  /**
   * Initialize from copy
   */
  inline ResetReason(const ResetReason &c) : byte(c.byte) {}
};

extern const ResetReason resetReason;
}; // namespace AVR
