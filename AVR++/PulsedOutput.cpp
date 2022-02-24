#pragma once

#include "PulsedOutput.hpp"
#include "Const.hpp"
#include "Nop.hpp"
#include <util/delay.h>

using namespace AVR;

template <Ports Port, unsigned Pin, unsigned ShortPulseNanos, bool InvertedOutput, bool BalanceRecoveryTimes,
          bool LittleEndian, bool InvertBits, unsigned MinimumRecoveryNanos, unsigned LongPulseNanos>
void PulsedOutput<Port, Pin, ShortPulseNanos, InvertedOutput, BalanceRecoveryTimes, LittleEndian, InvertBits,
                  MinimumRecoveryNanos, LongPulseNanos>::send(u1 byte, u1 bits) {
  asm volatile("; PulsedOutput::send(u1 byte, u1 bits)");

  // Automatically shift bits over for BigEndian?
  // if (!LittleEndian) for (u1 i = bits; i < 8; i++) byte <<= 1;

  // TODO: Multiple implementations based on minimum pulse length.
  // Current implementation optimized for enabling a 2 cycle pulse minimum.
  // If not needed, some instructions could be saved and complexity reduced.

  // if (ih == InterruptHandling::Byte) cli();

  /**
   * Pseudo code unbalanced pulse timing:
   * for each bit in byte
   *   carry = byte & 1
   *   byte >>= 1
   *   assert()
   *   delay(A) // Short pulse length
   *   if (carry)
   *     delay(B) // Long pulse length
   *   idle()
   *   delay(C) // Short Pulse Recovery Time
   *
   * Pseudo code balanced pulse timing:
   * for each bit in byte
   *   carry = byte & 1
   *   byte >>= 1
   *   assert()
   *   delay(A) // Short pulse length
   *   if (carry)
   *     delay(B) // Long pulse length
   *     idle()
   *   else
   *     idle()
   *     delay(B) // Short Pulse Recovery Time
   *   delay(C) // Minimum Recovery Time
   *
   */

  while (bits--) {
    // Mutates carry flag which we check later
    if (LittleEndian)
      asm volatile("lsr %0 ; byte >>= 1" : "+r"(byte));
    else
      asm volatile("lsl %0 ; byte <<= 1" : "+r"(byte));

    // if (ih == InterruptHandling::Bit) cli();

    on(); // assert()

    asm("; PulsedOutput Delay A = %0 cycles" : : "I"(PulseMath::delayCyclesA));
    nopCycles(PulseMath::delayCyclesA - 1);
    asm("; End of PulsedOutput Delay A");

    // BRanch if Carry is Clear/Set (sending a long pulse)
    // delayB();
    // if (InvertBits)
    //   asm goto("brcc %l[SEND_LONG_PULSE]" :: ::SEND_LONG_PULSE);
    // else
    //   asm goto("brcs %l[SEND_LONG_PULSE]" :: ::SEND_LONG_PULSE);
    if (InvertBits)
      asm goto("brcc %l[SKIP_OFF]" :: ::SKIP_OFF);
    else
      asm goto("brcs %l[SKIP_OFF]" :: ::SKIP_OFF);

    // We use brcc/brcs because when it falls through (sending a short pulse), it only takes 1 clock cycle.
    // This enables the minimum pulse time of 2 clock cycles.

  OFF_JUMP:
    off(); // idle()

    nopCycles(1);

  SKIP_OFF:
    asm("; PulsedOutput Delay B2 = %0 cycles" : : "I"(PulseMath::delayCyclesB));
    nopCycles(PulseMath::delayCyclesB);
    asm("; End of PulsedOutput Delay B2");

    off(); // idle()

  RECOVERY_JUMP:
    // if (ih == InterruptHandling::Bit) sei();

    asm("; PulsedOutput Delay C = %0 cycles" : : "I"(PulseMath::delayCyclesC));
    nopCycles(PulseMath::delayCyclesC);
    asm("; End of PulsedOutput Delay C");
  }

  // if (ih == InterruptHandling::Byte) asm volatile("reti");
  // return;

  // Jump to here for the long delay
  // SEND_LONG_PULSE:
  //   asm("; PulsedOutput Delay B = %0 cycles" : : "I"(PulseMath::delayCyclesB));
  //   nopCycles(PulseMath::delayCyclesB);
  //   asm("; End of PulsedOutput Delay B");

  //   if (BalanceRecoveryTimes) {
  //     off(); // idle()
  //     asm goto("rjmp %l[RECOVERY_JUMP]; RECOVERY_JUMP" :: ::RECOVERY_JUMP);
  //   } else {
  //     // Get back to the normal loop
  //     asm goto("rjmp %l[OFF_JUMP]; OFF_JUMP" :: ::OFF_JUMP);
  //   }

  asm volatile("; PulsedOutput::send(u1 byte, u1 bits)#end");
}
