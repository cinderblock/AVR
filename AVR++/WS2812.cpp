#pragma once

#include "WS2812.hpp"
#include <util/delay.h>

// Yes, cpp is intentional
// #include "PulsedOutput.cpp"

using namespace AVR;

/**
 * A single WS2812 bit is a high "pulse" period and low "pulse" period.
 *
 * A longer high pulse is a "1" bit and a shorter high pulse is a "0" bit.
 *
 * The spec says that the pulses are 0.4µs and 0.8µs (+/- 150ns) respectively.
 * It also specifies a negative time to make each bit the same length overall.
 *
 *  ________400ns________
 * |                     |
 * |                     |                                         = 0
 * |                     |_________________850ns__________________
 *
 *  ___________________800ns_________________
 * |                                         |
 * |                                         |                     = 1
 * |                                         |________450ns_______
 *
 * In practice, the length of time the data line is low does not matter as long as it's long enough that pixels "see" it
 * and not so long that it's "seen" as a reset pulse. To put it another way, there is no reason to wait the full 850ns
 * for a "0". This simplification of the protocol is easier to implement. This also lets the code gracefully take a
 * different amount of time when at the end of a byte.
 *
 * Simplified explanation:
 * Each bit is sampled a fixed time period after a rising edge.
 * After the falling edge, which could be before the sample, we need to hold the line low for some minimum time for the
 * pixel circuitry to recover.
 *  ______________________________s_________                       ...
 * |                    \         ^         \                     |
 * | < rising edge       \   sample strobe   \                    | < next rising edge
 * |                      \___________________\___Recovery Time___|
 *
 * If the line is held low for a longer time period (> ~40us), all pixels will latch their latest values.
 */

template <Ports Port, u1 Pin, bool StrictTiming, bool HandleInterrupts, unsigned ResetMicroseconds, bool InvertedLogic,
          bool LittleEndian>
void WS2812<Port, Pin, StrictTiming, HandleInterrupts, ResetMicroseconds, InvertedLogic, LittleEndian>::sendBytes(
    u1 const *const data, u2 length) {
  if (HandleInterrupts) asm("cli");

  send(data, length);

  if (HandleInterrupts)
    asm("reti");
  else
    return;

  static_assert(Parent::PulseMath::realHighNanosecondsShort <= 400 + 150,
                "Short pulse period is too long. Check F_CPU and WS2812 timing.");

  // This test depends on `ResetMicroseconds` so it needs to be inside a templated function.
  static_assert(Parent::PulseMath::realLowMicrosecondsMax < ResetMicroseconds / 2,
                "Low period is too long and could be considered a \"reset\". "
                "Check F_CPU and WS2812 timing.");
}
