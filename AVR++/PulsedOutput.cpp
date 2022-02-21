#pragma once

#include "PulsedOutput.hpp"
#include "Const.hpp"
#include "Nop.hpp"
#include <util/delay.h>

using namespace AVR;

template <Ports Port, unsigned Pin, unsigned ShortPulseNanos, bool InvertedOutput, unsigned LongPulseNanos,
          bool LittleEndian, bool InvertBits, unsigned MinimumRecoveryNanos, bool BalanceRecoveryTimes>
void PulsedOutput<Port, Pin, ShortPulseNanos, InvertedOutput, LongPulseNanos, LittleEndian, InvertBits,
                  MinimumRecoveryNanos, BalanceRecoveryTimes>::send(u1 byte, u1 bits) {
  asm volatile("; PulsedOutput::send(u1 byte, u1 bits)");

  // Automatically shift bits over for BigEndian?
  // if (!LittleEndian) for (u1 i = bits; i < 8; i++) byte <<= 1;

  // TODO: Multiple implementations based on minimum pulse length.
  // Current implementation optimized for enabling a 2 cycle pulse minimum.
  // If not needed, some instructions could be saved and complexity reduced.

  // if (ih == InterruptHandling::Byte) cli();

  while (bits--) {
    // Mutates carry flag which we check later
    if (LittleEndian)
      asm volatile("lsr %0" : "+r"(byte)); // byte >>= 1;
    else
      asm volatile("lsl %0" : "+r"(byte)); // byte <<= 1;

    // if (ih == InterruptHandling::Bit) cli();

    on();

    asm("; PulsedOutput Delay A = %0 cycles" : : "I"(delayCyclesA));
    nopCycles(delayCyclesA);
    asm("; End of PulsedOutput Delay A");

    // BRanch if Carry is Clear/Set (sending a long pulse)
    // delayB();
    if (InvertBits)
      asm goto("brcc %l[C_DELAY_B]" :: ::C_DELAY_B);
    else
      asm goto("brcs %l[C_DELAY_B]" :: ::C_DELAY_B);

    // We use brcc/brcs because when it falls through (sending a short pulse), it only takes 1 clock cycle.
    // This enables the minimum pulse time of 2 clock cycles.

  OFF_JUMP:
    off();

    // if (ih == InterruptHandling::Bit) sei();

    asm("; PulsedOutput Delay C = %0 cycles" : : "I"(delayCyclesC));
    nopCycles(delayCyclesC);
    asm("; End of PulsedOutput Delay C");
  }

  // if (ih == InterruptHandling::Byte) asm volatile("reti");
  return;

  // Jump to here for the long delay
C_DELAY_B:
  asm("; PulsedOutput Delay B = %0 cycles" : : "I"(delayCyclesB));
  nopCycles(delayCyclesB);
  asm("; End of PulsedOutput Delay B");

  if (BalanceRecoveryTimes) {
    off();

    // TODO: Alternate path if BalanceRecoveryTimes is true
    static_assert(!BalanceRecoveryTimes, "Not yet implemented");
    const unsigned delayCyclesD = 0; // TODO: Calculate this and move to class

    asm("; PulsedOutput Delay D = %0 cycles" : : "I"(delayCyclesD));
    nopCycles(delayCyclesD);
    asm("; End of PulsedOutput Delay D");
  } else {
    // Get back to the normal loop
    asm goto("rjmp %l[OFF_JUMP]" :: ::OFF_JUMP);
  }

  asm volatile("; PulsedOutput::send(u1 byte, u1 bits)#end");
}
