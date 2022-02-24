#pragma once

#include "PulsedOutput.hpp"
#include <util/delay.h>

namespace AVR {

template <Ports port, u1 pin, bool StrictTiming = false, bool HandleInterrupts = true, unsigned ResetMicroseconds = 300,
          bool InvertedOutput = false, bool LittleEndian = false>
class WS2812 : protected PulsedOutput<port, pin, 400, InvertedOutput, StrictTiming> {
  using Parent = PulsedOutput<port, pin, 400, InvertedOutput, StrictTiming>;

  using Parent::send;

protected:
  /**
   * @brief Shift an array of bytes out the specified pin using the WS2812 protocol.
   *
   * Does not include reset pulse.
   *
   * @param bytes the bytes to send
   * @param length the number of bytes to send
   */
  static void sendBytes(u1 const *const bytes, u2 length);
  // naked, in our case, just removes the automatic "ret" which we do ourselves.

public:
  using Parent::init;

  struct RGB {
    // This order matches the order of the WS2812 protocol

    u1 g;
    u1 r;
    u1 b;
    RGB() : g(0), r(0), b(0) {}
    RGB(u1 w) : g(w), r(w), b(w) {}
    RGB(u1 r, u1 g, u1 b) : g(g), r(r), b(b) {}

    inline u1 const *data() const { return (u1 const *)this; }
    inline operator u1 const *() const { return data(); }
    static constexpr u1 size = sizeof(RGB);
  };
  struct RGBW {
    // This order matches the order of the WS2812 protocol

    u1 g;
    u1 r;
    u1 b;
    u1 w;
    RGBW() : g(0), r(0), b(0), w(0) {}
    RGBW(u1 w) : g(0), r(0), b(0), w(w) {}
    RGBW(u1 r, u1 g, u1 b, u1 w = 0) : g(g), r(r), b(b), w(w) {}

    inline u1 const *data() const { return (u1 const *)this; }
    inline operator u1 const *() const { return data(); }
    static constexpr u1 size = sizeof(RGBW);
  };

  template <bool doLatchDelay = true> inline static void setLEDs(RGB const *leds, u2 pixels) {
    sendBytes(*leds, pixels * leds->size);
    if (doLatchDelay)
      _delay_us(ResetMicroseconds);
  }
  template <bool doLatchDelay = true> inline static void setLEDs(RGBW const *leds, u2 pixels) {
    sendBytes(*leds, pixels * leds->size);
    if (doLatchDelay)
      _delay_us(ResetMicroseconds);
  }
};

}; // namespace AVR