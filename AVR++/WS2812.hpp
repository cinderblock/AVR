#pragma once

#include "PulsedOutput.hpp"

namespace AVR {

template <Ports port, u1 pin, bool HandleInterrupts = true, unsigned resetMicroseconds = 300,
          bool InvertedOutput = false, bool LittleEndian = false>
class WS2812 : protected PulsedOutput<port, pin, 400, InvertedOutput> {
  using PulsedOutput<port, pin, 400, InvertedOutput>::send;
  using PulsedOutput<port, pin, 400, InvertedOutput>::realHighNanosecondsShort;
  using PulsedOutput<port, pin, 400, InvertedOutput>::realLowMicrosecondsMax;

protected:
  /**
   * @brief Shift an array of bytes out the specified pin using the WS2812 protocol.
   *
   * Does not include reset pulse.
   *
   * @param bytes the bytes to send
   * @param length the number of bytes to send
   */
  static void sendBytes(u1 const *bytes, u2 length) __attribute__((naked));
  // naked, in our case, just removes the automatic "ret" which we do ourselves.

public:
  struct RGB {
    u1 g;
    u1 r;
    u1 b;
  };
  struct RGBW {
    u1 g;
    u1 r;
    u1 b;
    u1 w;
  };

  static void setLEDs(RGB const *leds, u2 pixels);
  static void setLEDs(RGBW const *leds, u2 pixels);
};

}; // namespace AVR