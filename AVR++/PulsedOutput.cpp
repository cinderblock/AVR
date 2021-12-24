#include "PulsedOutput.hpp"
#include "Const.hpp"
#include "Nop.hpp"
#include <util/delay.h>

using namespace AVR;

template <Ports Port, u1 Pin, unsigned shortPulseNanos, bool InvertedOutput, unsigned longPulseNanos, bool LittleEndian,
          bool InvertBits, unsigned minRecoveryNanos, bool balanceRecoveryTimes>
void PulsedOutput<Port, Pin, shortPulseNanos, InvertedOutput, longPulseNanos, LittleEndian, InvertBits,
                  minRecoveryNanos, balanceRecoveryTimes>::send(u1 byte, u1 bits) {
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

    asm("; Delay A = %0 cycles" : : "I"(delayCyclesA));
    nopCycles(delayCyclesA);
    asm("; End of Delay A");

    // BRanch if Carry is Clear/Set (sending a long pulse)
    // delayB();
    if (InvertBits)
      asm volatile("brcc DELAYB");
    else
      asm volatile("brcs DELAYB");
    // We use brcc/brcs because when it falls through (sending a short pulse), it only takes 1 clock cycle.
    // This enables the minimum pulse time of 2 clock cycles.

    asm("OFF_JUMP:");
    off();

    // if (ih == InterruptHandling::Bit) sei();

    asm("; Delay C = %0 cycles" : : "I"(delayCyclesC));
    nopCycles(delayCyclesC);
    asm("; End of Delay C");

    continue;

    // Jump to here for the long delay
    asm("DELAYB:");
    asm("; Delay B = %0 cycles" : : "I"(delayCyclesB));
    nopCycles(delayCyclesB);
    asm("; End of Delay B");

    if (balanceRecoveryTimes) {
      off();

      // TODO: Alternate path if balanceRecoveryTimes is true
      static_assert(!balanceRecoveryTimes, "Not yet implemented");
      const unsigned delayCyclesD = 0; // TODO: Calculate this and move to class

      asm("; Delay D = %0 cycles" : : "I"(delayCyclesD));
      nopCycles(delayCyclesD);
      asm("; End of Delay D");
    } else {
      // Get back to the normal loop
      asm volatile("rjmp OFF_JUMP");
    }
  }

  // if (ih == InterruptHandling::Byte) sei();
}

template <Ports Port, u1 Pin, unsigned shortPulseNanos, bool InvertedOutput, unsigned longPulseNanos, bool LittleEndian,
          bool InvertBits, unsigned minRecoveryNanos, bool balanceRecoveryTimes>
void PulsedOutput<Port, Pin, shortPulseNanos, InvertedOutput, longPulseNanos, LittleEndian, InvertBits,
                  minRecoveryNanos, balanceRecoveryTimes>::send(u1 const *bytes, u2 bits) {
  // if (ih == InterruptHandling::Block) cli();

  while (bits) {
    const auto b = min<unsigned>(bits, 8u);
    send(*bytes++, b);
    bits -= b;
  }

  // if (ih == InterruptHandling::Block) sei();
}