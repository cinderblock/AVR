#pragma once

/**
 * @file BDShot.cpp
 * @author Cameron Tacklind <cameron@tacklind.com>
 * @brief The implementation of the BDShot class
 * @version 0.1
 * @date 2022-10-07
 * @note This file is part of the AVR++ library.
 * @copyright Copyright (c) 2022
 *
 * @see The comments in getResponse() for more information on the internal workings of this implementation.
 */

#include "BDShot.hpp"
#include "Const.hpp"
#include "Core.hpp"
#include "GCR.hpp"
#include <avr/interrupt.h>

// Yes, we're including the cpp
#include "DShot.cpp"

// cSpell:ignore datasheet
// cSpell:ignore TCCR TIMSK TIFR TCNT OCRA COMPA vect
// cSpell:ignore SREG EIMSK PCMSK PCICR UCSR RXCIE TXCIE UDRIE USBCON VBUSTE UDIEN UEIENX SPMCSR SPMIE ACSR ACIE SPCR
// cSpell:ignore SPIE EECR ADCSRA ADIE ADSC ADIF TWCR TWIE TWINT FPIE WDTCSR WDIE WDIF
// cSpell:ignore subi breq sbic sbis rjmp rcall andi reti brcc brcs ijmp

constexpr static bool AssemblyComments = true;

namespace AVR {
namespace DShot {
namespace BDShotTimer {
namespace {
static constexpr auto BitOverflowShortFlagMask = 1 << OCF0A;
static constexpr auto BitOverflowMaxFlagMask = 1 << TOV0;
} // namespace

static inline void setCounter(u1 value) {
  if (AssemblyComments) asm("; setCounter(u1 value)");
  TCNT0 = value;
}

static inline void enableOverflowShortInterrupt() {
  if (AssemblyComments) asm("; enableOverflowShortInterrupt()");
  TIMSK0 = BitOverflowShortFlagMask;
}

static inline void disableOverflowShortInterrupt() {
  if (AssemblyComments) asm("; disableOverflowShortInterrupt()");
  TIMSK0 = 0;
}

static inline void clearOverflowShortFlag() {
  if (AssemblyComments) asm("; clearOverflowShortFlag()");
  TIFR0 |= BitOverflowShortFlagMask;
}

static inline void clearOverflowMaxFlag() {
  if (AssemblyComments) asm("; clearOverflowMaxFlag()");
  TIFR0 |= BitOverflowMaxFlagMask;
}

static inline bool hasOverflowMaxFlagged() {
  if (AssemblyComments) asm("; hasOverflowMaxFlagged()");
  return TIFR0 & BitOverflowMaxFlagMask;
}

static inline void setMaxTimeout() {
  if (AssemblyComments) asm("; setMaxTimeout();");

  // Normal mode
  u1 wgm = 0b000;

  // wgm2 doesn't change

  TCCR0A = wgm & 0b11;
}

static inline void setShortTimeout() {
  if (AssemblyComments) asm("; setShortTimeout();");

  // CTC Mode (clear counter at OCR0A)
  u1 wgm = 0b010;

  // wgm2 doesn't change

  TCCR0A = wgm & 0b11;
}
static inline void start() {
  if (AssemblyComments) asm("; start()");
  // Both modes use same value
  u1 const wgm2 = 0;
  u1 const prescaler = 1;
  TCCR0B = wgm2 << WGM02 | prescaler << CS00;
}
static inline void stop() {
  if (AssemblyComments) asm("; stop()");
  TCCR0B = 0;
}

static inline void init(u1 shortPeriod) {
  if (AssemblyComments) asm("; Setup Timer");

  // Set TOP value
  OCR0A = shortPeriod - 1;

  // Set up timer that we use internally
  TIMSK0 = 0; // Ensure timer interrupts are disabled

  stop();
}
} // namespace BDShotTimer
} // namespace DShot
} // namespace AVR

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
void AVR::DShot::BDShot<Port, Pin, Speed>::exitBootloader() {
  if (AssemblyComments) asm("; Waiting for bootloader exit");

  // Output needs to be low long enough to get out of bootloader and start main program
  Parent::IO::clr();
  Parent::IO::output();

  // TODO: This is a long delay. Allow other things to happen while we wait.
  _delay_ms(BDShotConfig::exitBootloaderDelay);

  if (AssemblyComments) asm("; Done waiting for bootloader exit");

  // Set output high
  Parent::IO::set();
}

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
void AVR::DShot::BDShot<Port, Pin, Speed>::init() {
  BDShotTimer::init(Periods::delayPeriodTicks);

  if (AssemblyComments) asm("; Init BDShot");

  // Set output high
  Parent::IO::set();
}

// Pick 3 "call-used" or "call-clobbered" general purpose registers.
// Order/adjacency does not matter.
// We'll use these to store the response as we read it.
// Since no other code is running during the read, we don't need to protect these registers either.
#define ResultReg0 "r18"
#define ResultReg1 "r19"
#define ResultReg2 "r20"

static_assert(!Const::equal(ResultReg0, ResultReg1), "ResultReg0 must not be equal to ResultReg1");
static_assert(!Const::equal(ResultReg1, ResultReg2), "ResultReg1 must not be equal to ResultReg2");
static_assert(!Const::equal(ResultReg2, ResultReg0), "ResultReg2 must not be equal to ResultReg0");

#define CheckRegister(n)                                                                                               \
  static_assert(!Const::equal(ResultReg##n, "r0"), "ResultReg" #n " must not be __tmp_reg__");                         \
  static_assert(!Const::equal(ResultReg##n, "r1"), "ResultReg" #n " must not be __zero_reg__");                        \
  static_assert(!Const::equal(ResultReg##n, "r30"), "ResultReg" #n " must not be Z register");                         \
  static_assert(!Const::equal(ResultReg##n, "r31"), "ResultReg" #n " must not be Z register");                         \
  static_assert(!Const::equal(ResultReg##n, "r24"), "ResultReg" #n " must not be a register that GCC uses");

CheckRegister(0);
CheckRegister(1);
CheckRegister(2);

#undef CheckRegister

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
AVR::DShot::Response AVR::DShot::BDShot<Port, Pin, Speed>::getResponse() {
  /**
   * @brief Resync with interrupts
   *
   * We're receiving a NRZ(L) RLL(GCR) shift-encoded* bit stream; a start bit and 20 raw bits to be decoded.
   *
   * *Shift-encoded: gcrEncodedData = receivedBits ^ (receivedBits >> 1);
   *
   * One way to receive this data would be to use a standard TTL level hardware serial port with the capability of
   * receiving 20 bits, or even implemented in software. This, however, has significant issues with clock drift.
   *
   * Instead, we try to recover clocking information on *any* transition in a fast (~3 clock cycles) spin loop.
   *
   * Timer overflows mean it's time to sample a bit.
   * We can service this safely since we expect no transitions mid bit.
   *
   * Notably, we don't use interrupts in the case of the initial waiting period because the recovery time from the
   * interrupt would hurt our initial clock recovery too much.
   *
   * Pseudo code:
   * - Wait for initial high-to-low transition
   *   - Timeout if no response in >50us
   * - Start timer with overflow in 1.5 bit periods
   *   - We're skipping the first bit because it's *always* 0
   * - Enable our Timer Overflow Interrupt
   * - Fast Loop waiting for Transition
   *   - On Transition
   *     - Set new Timer Value (resync)
   * - On Timer Overflow Interrupt
   *   - Sample Pin State
   *   - Store Pin State
   *   - If 20 bits received
   *     - Parse
   *     - return Response
   *
   * Assumptions:
   * - All other interrupts are off
   * - Global interrupts are on
   */

  // cSpell:ignore eeex

  // clang-format off
  /**
   * Let's walk through an example waveform. The bit periods are not quite uniform intentionally.
   * 
   * We're receiving this standard waveform that basically means "not spinning" (period is at maximum).
   * Notice the timer counts up and overflows every approximate bit period. On overflow, an interrupt is triggered and we sample the bit.
   * In the "main" loop, we're watching for transitions. If we see one, reset the timer counter to an ideal middle value. This takes care of clock drift.
   * 
   * --------------- _____ ______ ------ _______ ------ ______ ______ ------- ______ ------ ______ _______ ------ ______ ----- _____ ------ _____ _____ _____ ------ --------------
   * Idle...             0      1|     2|      3|     4|     5      6|      7|     8|     9|    10      11|    12|    13|   14|   15|    16|   17    18    19|    20 Idle...
   * Transition Resync           |      |       |      |             |       |      |      |              |      |      |     |     |      |                 |       
   *                |      _-*   |_-*   |_-*    |_-*   |_-*    _-*   |_-*    |_-*   |_-*   |_-*    _-*    |_-*   |_-*   |_-*  | _-* |_-*   |_-*    _-*    _-*|_-*
   * Timer          |   _-*    _-*    _-*    _-**    _-*    _-*    _-*    _-**    _-*    _-*    _-*    _-**    _-*    _-*    _-*    *    _-*    _-*    _-*   *   
   *                |_-*      *      *      *       *      *      *      *       *      *      *      *       *      *      *      *    *      *      *          *
   * Sample Bits    |         0      1      0       1      0      0      1       0      1      0      0       1      0      1      0    1      0      0      0   1
   * 
   * Sampled bits end up in the result registers
   * 
   * ResultReg:   2        1        0 - MSB first
   * Result :  0101 00101001 01010001 - 20 bits
   * Shifted:  0010 10010100 10101000 - Shift result right by 1
   * Xor'd  :  0111 10111101 11111001 - 20 bits of gcr encoded data
   * regroup:  //// //////|| ///||||| - group 20 bits as 4 5-bit words (now how assembly is implemented)
   * gcr    : 01111 01111 01111 11001 - 4 5-bit words
   * Nibble :     3     2     1     0 - decode each 5-bit word into nibbles of data
   * data   :  1111  1111  1111  0000 - 4 nibbles of data
   * Meaning:  eeex  bbbb  bbbb  cccc - 3 bits exponent, 1 bit to mark EDT(0) or normal(1), 8 bits base, 4 bits checksum
   * Checksum: n3 ^ n2 ^ n1 ^ n0 ^ 0xf == 0
   * Spin period = (base << exponent) microseconds
   */
  // clang-format on

  using namespace BDShotConfig;

  // These are probably defunct
  static_assert(Periods::delayPeriodTicks == 43, "delayPeriodTicks is not what we expect!");
  static_assert(Periods::delayHalfPeriodTicks == 21, "delayHalfPeriodTicks is not what we expect!");

  // We're expecting 4 nibbles in the response, each encoded with GCR
  // We don't count the extra bit because it's handled manually as it's the trigger to start reading the response and is
  // *always* 0.
  static constexpr unsigned expectedNibbles = 4;
  static constexpr Basic::u1 ExpectedBits = expectedNibbles * GCR::inBits;

  constexpr unsigned responseTimeoutTicks = (((long long)(F_CPU)) * responseTimeout) / 1e6;

  /**
   * Number of ticks from timer overflow to when we execute the instruction that samples the pin's input state.
   *
   * This comes mostly from 3 separate places:
   * - The AVR datasheet which specified ISR handling takes 5 clock cycles to execute.
   * - The de facto standard JMP table as the first instructions in the ISR (could be eliminated with a custom .init)
   * - Other instructions in the ISR before sampling the pin into a temp register
   */
  constexpr unsigned ticksFromOverflowToISRSample =
      AVR::Core::Ticks::Hardware::ISR +     // ISR time
      AVR::Core::Ticks::Instruction::Jmp +  // Initial jmp table, could be eliminated with a custom .init table but as
                                            // long as it's predictable, do we care?
      Debug::EmitPulseAtISR +               // Pin toggle adds a clock cycle
      AVR::Core::Ticks::Instruction::IJmp + // Our jmp to Z
      0;

  /**
   * Number of ticks it takes the initial spin loop to check for the initial high-to-low transition.
   *
   * Most of the time, this number is used.
   *
   * Sometimes, the timer has overflowed and we need to check that our timeout counter hasn't reached zero.
   * In this case, we chance being slightly more out of sync than normal. So we use the largest Timer period possible.
   * In any case, it will resync on the next bit transition.
   *
   * @see `ticksInitialSpinLoopWorstCase`
   *
   * @note Maximum accuracy we can achieve of timing on initial transition
   * @note These lists are generated from looking at the generated assembly for these functions.
   */
  constexpr unsigned ticksInitialSpinLoop =
      AVR::Core::Ticks::Instruction::Skip1Word +  // Check Pin, skip over rjmp to normal
      ResetWatchdog::WaitingFirstTransitionFast + // WDR
      1 +                                         // Didn't overflow
      AVR::Core::Ticks::Instruction::RJmp +       // Loop
      0;

  /**
   * Number of ticks it takes the initial spin loop to check for the initial high-to-low transition while also
   * decrementing and checking the timer overflow counter.
   *
   * This value is not used but computed for reference.
   *
   * @note Maximum accuracy we can achieve of timing on initial transition
   * @note These lists are generated from looking at the generated assembly for these functions.
   */
  constexpr unsigned ticksInitialSpinLoopWorstCase =
      AVR::Core::Ticks::Instruction::Skip1Word +           // Check Pin, skip over rjmp to normal
      ResetWatchdog::WaitingFirstTransitionFast +          // WDR
      AVR::Core::Ticks::Instruction::Skip1Word +           // Check Overflow Flag
      1 +                                                  // subi; i--
      1 +                                                  // breq; if (i == 0) {<exit path>} else ...
      1 +                                                  // sbi; clear timer flag by setting it
      ResetWatchdog::WaitingFirstTransitionTimerOverflow + // WDR
      AVR::Core::Ticks::Instruction::RJmp +                // Back to main loop
      0;

  static_assert(ticksInitialSpinLoopWorstCase < ticksInitialSpinLoop * 3,
                "ticksInitialSpinLoopWorstCase is curiously large");

  /**
   * Number of ticks from the initial high-to-low transition to when we can set the timer to some value.
   *
   * @note Part of the phase adjustment of reading bits on timer overflow
   * @note These lists are generated from looking at the generated assembly for these functions.
   */
  constexpr unsigned ticksFromTransitionToInitialTimerSync =
      AVR::Core::Ticks::Instruction::Skip1Word * useDebounce + // Debounce compensation
      1 +                                                      // Check Pin  with `sbis`, it's low, don't skip
      AVR::Core::Ticks::Instruction::RJmp +                    // Jump to Initial Ticks
      ResetWatchdog::ReceivedFirstTransition +                 // WDR
      AVR::Core::Ticks::Instruction::LoaDImediate +            // Set register to immediate
      AVR::Core::Ticks::Instruction::Out +                     // Set timer counter from register
      0;

  /**
   * Number of ticks it takes the ultra fast main spin loop to check for a transition
   *
   * @note Maximum accuracy we can achieve of timing on transition
   */
  constexpr unsigned ticksSpinLoop = Debug::EmitPulsesAtIdle +             // Pin toggle adds a clock cycle to the loop
                                     1 +                                   // Reading the pin state takes 1 clock cycle
                                     AVR::Core::Ticks::Instruction::RJmp + // Jump to start of loop
                                     0;

  /**
   * Number of ticks from a transition to when we can set the timer to some value.
   * Receiving bits has a different code path that the initial transition.
   *
   * @note Part of the phase adjustment of reading bits on timer overflow
   */
  constexpr unsigned ticksFromTransitionToTimerSync =
      AVR::Core::Ticks::Instruction::Skip1Word * useDebounce + // Debounce compensation
      AVR::Core::Ticks::Instruction::Skip1Word +               // Read + skip rjmp for loop[]
      AVR::Core::Ticks::Instruction::Out +                     // Set timer counter from register
      0;

  // Larger numbers will make the samples happen sooner
  constexpr int fudgeInitialTicks = 0;
  constexpr int fudgeSyncTicks = 0;

  // constexpr unsigned adjustSyncTicks = ticksFromOverflowToISRSample + 1;
  // constexpr unsigned adjustInitialTicks = adjustSyncTicks + 10;
  constexpr unsigned adjustInitialTicks = ticksInitialSpinLoop / 2 +              // Half of the normal loop time
                                          ticksFromTransitionToInitialTimerSync + // Initial sync
                                          ticksFromOverflowToISRSample +          // Compensate for ISR service time
                                          fudgeInitialTicks +                     // Let us easily fudge the numbers
                                          0;

  constexpr unsigned adjustSyncTicks = ticksSpinLoop / 2 +              // Half of the loop time
                                       ticksFromTransitionToTimerSync + // Continuous sync
                                       ticksFromOverflowToISRSample +   // Compensate for ISR service time
                                       fudgeSyncTicks +                 // Let us easily fudge the numbers
                                       0;

  constexpr u1 timerCounterValueInitial = u1(adjustInitialTicks - Periods::delayHalfPeriodTicks);
  constexpr u1 timerCounterValueSync = Periods::delayHalfPeriodTicks + adjustSyncTicks;

  // Assumptions of these implementations
  static_assert(Periods::delayPeriodTicks > adjustInitialTicks, "delayPeriodTicks is too small");
  static_assert(Periods::delayPeriodTicks < 200, "delayPeriodTicks is too large");

  if (AssemblyComments) asm("; Starting timer with max timeout");

  constexpr unsigned counterUpperByte = responseTimeoutTicks >> 8;
  u1 overflowsWhileWaiting = counterUpperByte + 1;

  static_assert(counterUpperByte < u4(1) << (8 * sizeof(overflowsWhileWaiting)),
                "counterUpperByte is too large for this implementation");

  BDShotTimer::setMaxTimeout();
  BDShotTimer::setCounter(u1(-responseTimeoutTicks));
  BDShotTimer::clearOverflowMaxFlag();
  BDShotTimer::start();

  if (AssemblyComments) asm("; Waiting for first transition");

  // Wait for initial high-to-low transition, or timeout while waiting
  while (isHigh() || (useDebounce && isHigh())) {
    if (ResetWatchdog::WaitingFirstTransitionFast) asm("wdr");

    if (!BDShotTimer::hasOverflowMaxFlagged()) continue;
    // If timer overflows, see if we've overflowed enough to know we're not getting a response.

    if (!--overflowsWhileWaiting) {
      if (AssemblyComments) asm("; Return timeout");
      // Timeout waiting for response
      return AVR::DShot::Response::Error::ResponseTimeout;
    }

    // Clear the flag so we can wait for the next overflow
    BDShotTimer::clearOverflowMaxFlag();

    if (ResetWatchdog::WaitingFirstTransitionTimerOverflow) asm("wdr");
  }

  // Yay! We're getting a response. Try and receive it!

  if (ResetWatchdog::ReceivedFirstTransition) asm("wdr");

  if (AssemblyComments) asm("; Initial Ticks");
  // Set timer so that it matches trigger register in 1.5 bit periods
  BDShotTimer::setCounter(timerCounterValueInitial);
  BDShotTimer::setShortTimeout();
  BDShotTimer::clearOverflowShortFlag();

  if (AssemblyOptimizations::saveZRegister) {
    // Save the contents of the call-saved result registers
    asm("push r30");
    asm("push r31");
  }

  if (AssemblyComments) asm("; Setting up our Z register (r30 and r31)");

  // The magic that lets this work on *any* pin
  AVR::Core::setZ(&ReadBitISR);
  // Just make sure nothing else uses the Z registers locally (with a hope and educated guesses)
  // Also consider building with "-S" to see the generated assembly and inspect it manually.
  // It will include many comments :)

  if (AssemblyOptimizations::saveResultRegisters) {
    // Save the contents of the call-saved result registers
    asm("push " ResultReg0);
    asm("push " ResultReg1);
    asm("push " ResultReg2);
  }

  if (AssemblyComments) asm("; Setting up ResultReg: (" ResultReg0 " " ResultReg1 " " ResultReg2 ") and Carry");

  // Use a set bit in result to indicate we've gotten all the bits.
  // That bit will roll into Carry bit, which we test to break out of our spin loop.
  constexpr u3 FinishedMarker = 1 << (8 * sizeof(FinishedMarker) - ExpectedBits);

  asm("ldi " ResultReg0 ", %0 ; result0" ::"M"(FinishedMarker >> (8 * 0)) : ResultReg0); // lsb
  asm("ldi " ResultReg1 ", %0 ; result1" ::"M"(FinishedMarker >> (8 * 1)) : ResultReg1);
  asm("ldi " ResultReg2 ", %0 ; result2" ::"M"(FinishedMarker >> (8 * 2)) : ResultReg2); // msb

  // Make sure Carry starts in expected state
  asm("clc \t;Clear Carry Flag");

  if (AssemblyComments) asm("; DON'T MESS WITH: " ResultReg0 " " ResultReg1 " " ResultReg2 " r30 r31 or Carry!");

  BDShotTimer::enableOverflowShortInterrupt();

  // The ultra fast loop implementation
  // Relies on extra weird code at the end of the ISR to save us
  if (AssemblyComments) asm("; Ultra Fast Loop Start");
  while (true) {
    asm("" ::: ResultReg0, ResultReg1, ResultReg2, "r30", "r31"); // Remind the compiler

    do {
      if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
      if (AssemblyComments) asm("; Ultra Fast Loop. Waiting for transition to high.");
    } while (!isHigh() || (useDebounce && !isHigh()));

    BDShotTimer::setCounter(timerCounterValueSync);

    if (ResetWatchdog::ReceivedTransition) asm("wdr");

    if (Debug::EmitPulseAtSync) {
      Debug::Pin::on();
      Debug::Pin::off();
    }

    do {
      if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
      if (AssemblyComments) asm("; Ultra Fast Loop. Waiting for transition to low.");
    } while (isHigh() || (useDebounce && isHigh()));

    BDShotTimer::setCounter(timerCounterValueSync);

    if (ResetWatchdog::ReceivedTransition) asm("wdr");

    if (Debug::EmitPulseAtSync) {
      Debug::Pin::on();
      Debug::Pin::off();
    }
  }

  // GCC thinks code execution can't reach here, which is fine.
  // The ISR messes with the call stack and will "return" for us.
  asm volatile("; Ultra Fast Loop Loop End");
}

namespace MakeResponse {

inline static constexpr bool isBadChecksum(Basic::u1 n3, Basic::u1 n2, Basic::u1 n1, Basic::u1 n0) {
  return 0xf ^ n0 ^ n1 ^ n2 ^ n3;
}

static void interruptReturn() __attribute__((naked));
void interruptReturn() { asm("reti"); }

static AVR::DShot::Response bitByBit() {
  using namespace AVR::DShot;
  using namespace BDShotConfig;

  if (ResetWatchdog::SampledBit) asm("wdr");

  asm("; Store Carry into Result\n\t"
      "rol " ResultReg0 "\n\t"
      "rol " ResultReg1 "\n\t"
      "rol " ResultReg2 "\n\t"
      "; And get Carry from Result. If set, it indicates we're done." ::
          : ResultReg0, ResultReg1, ResultReg2);

  // Reti if Carry is clear to continue receiving bits
  // Carry will eventually be set by the initial bit set in ResultReg when we started receiving bits
  asm("brcc %x[DoneSamplingPin]; Branch to reti if Carry cleared" : : [DoneSamplingPin] "p"(&interruptReturn));
  // We could invert the logic and use `brcs` instead, to get back to the main loop 1 cycle sooner.
  // But in practice that doesn't matter as we always wait a loop cycles to see the transition.
  // So instead we clean up 1 cycle faster when we're done!

  // All done!

  if (AssemblyComments) asm("; DONE WITH REGISTERS: r30 r31 and Carry");

  BDShotTimer::stop();

  BDShotTimer::disableOverflowShortInterrupt();

  /**
   * Here is where we need the "magic" to happen.
   *
   * In getResponse(), in our spin loop, waiting for transitions, GCC thinks that it will never get out of it.
   * If we returned from this interrupt like normal, we'd go right back into that infinite loop.
   *
   * Fortunately getResponse() needs to return a Response.
   * So if we can just get some other function to return that Response for us...
   *
   * First we remove the last return pointer from the stack, which currently is somewhere in the middle of that
   * loop. Now, if we "return", we'll basically be returning from getResponse() instead. To seal the deal, we jump
   * to a function that parses Result into a Response and returns it as we need.
   */

  // Pop the interrupt return location off the stack (to get out of the ultra fast main loop)
  // Use a register we're about to use for other stuff
  asm("pop r24; Break out of Ultra Fast Loop when we reti");
  asm("pop r24; Break out of Ultra Fast Loop when we reti");
  // We also need to re-enable interrupts
  asm("sei");

  if (ResetWatchdog::BeforeProcessing) asm("wdr");

  Basic::u1 n0, n1, n2, n3;

  asm(
      // First we undo the shifting

      // Use r24 instead of __temp_reg__ because we can

      "mov  " /*        */ "r24, " ResultReg2 "\t; Copy byte 2\n\t"
      "lsr  " /*        */ "r24" /*        */ "\t; Shift the copy\n\t"
      "eor  " ResultReg2 ", r24" /*        */ "\t; XOR the copy back\n\t"

      "mov  " /*        */ "r24, " ResultReg1 "\t; Copy byte 1\n\t"
      "ror  " /*        */ "r24" /*        */ "\t; Shift the copy\n\t"
      "eor  " ResultReg1 ", r24" /*        */ "\t; XOR the copy back\n\t"

      "mov  " /*        */ "r24, " ResultReg0 "\t; Copy byte 0\n\t"
      "ror  " /*        */ "r24" /*        */ "\t; Shift the copy\n\t"
      "eor  " ResultReg0 ", r24" /*        */ "\t; XOR the copy back\n\t"

      // Now we turn 3 bytes into 4 quintets

      // Layout:                              Result 2 | Result 1 | Result 0|Carry
      "mov  r24, " /**/ ResultReg0 /**/ "\t; _--- 3333  3222 2211  1110 0000 ?\n\t" // move n0 to r24 for later

      "lsl  " /**/ ResultReg1 /**/ /**/ "\t; _--- 3333  2222 211_  1110 0000 3\n\t"
      "rol  " ResultReg2 /**/ /**/ /**/ "\t; ---3 3333  2222 211_  1110 0000 _\n\t" // n3 is ready in ResultReg2

      "lsr  " /**/ ResultReg1 /**/ /**/ "\t; ---3 3333  -222 2211  1110 0000 _\n\t"
      "lsr  " /**/ ResultReg1 /**/ /**/ "\t; ---3 3333  --22 2221  1110 0000 1\n\t"
      "ror  " /**/ /**/ ResultReg0 /**/ "\t; ---3 3333  --22 2221  1111 0000 0\n\t"
      "lsr  " /**/ ResultReg1 /**/ /**/ "\t; ---3 3333  ---2 2222  1111 0000 1\n\t" // n2 is ready in ResultReg1

      "andi " /**/ /**/ ResultReg0 ",0xf0\t; ---3 3333  ---2 2222  1111 ---- 1\n\t" // Ensure lower nibble is 0
      "adc  " /**/ /**/ ResultReg0 ",r1  \t; ---3 3333  ---2 2222  1111 ---1 _\n\t" // Add Carry to lower nibble
      "swap " /**/ /**/ ResultReg0 /**/ "\t; ---3 3333  ---2 2222  ---1 1111 _\n\t" // n1 is ready in ResultReg0

      // Decode the GCR encoded quintets into nibbles
      // We don't need to worry about trash in the upper nibbles because decodeNibble() masks them out

      "andi r24, 0x1f\n\t" // r24 has some of n1 in it still. Mask it out.

      "rcall %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n0], r24\n\t" // n0 was in r24 already

      "mov  r24, " /**/ /**/ ResultReg0 "\n\t"
      "rcall %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n1], r24\n\t" // n1 was in ResultReg0

      "mov  r24, " /**/ ResultReg1 /**/ "\n\t"
      "rcall %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n2], r24\n\t" // n2 was in ResultReg1

      "mov  r24, " ResultReg2 /**/ /**/ "\n\t"
      "rcall %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n3], r24\n\t" // n3 was in ResultReg2

      // Let the compiler do the rest

      : [n0] "=r"(n0), [n1] "=r"(n1), [n2] "=r"(n2), [n3] "=r"(n3)
      : [decodeNibble] "p"(&GCR::decode)
      : "r24", ResultReg0, ResultReg1, ResultReg2);

  if (AssemblyComments) asm("; DONE WITH REGISTERS: " ResultReg0 " " ResultReg1 " " ResultReg2);

  if (AssemblyOptimizations::saveResultRegisters) {
    // Restore the contents of the call-saved result registers
    asm("pop " ResultReg2 ::: ResultReg2);
    asm("pop " ResultReg1 ::: ResultReg1);
    asm("pop " ResultReg0 ::: ResultReg0);
  }

  if (AssemblyOptimizations::saveZRegister) {
    // Restore the contents of the call-saved result registers
    asm("pop r31" ::: "r31");
    asm("pop r30" ::: "r30");
  }

  // Yes, this order is correct. They are numbered by significance, and MSB was first.
  if (n3 == 0xff) return Response::Error::BadDecodeFirstNibble;
  if (n2 == 0xff) return Response::Error::BadDecodeSecondNibble;
  if (n1 == 0xff) return Response::Error::BadDecodeThirdNibble;
  if (n0 == 0xff) return Response::Error::BadDecodeFourthNibble;

  if (isBadChecksum(n3, n2, n1, n0)) return Response::Error::BadChecksum;

  // Efficient combination of nibbles
  asm("swap %0" : "=r"(n2) : "0"(n2));
  n1 |= n2;

  return {n1, n3};
}
} // namespace MakeResponse

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
void AVR::DShot::BDShot<Port, Pin, Speed>::ReadBitISR() {
  /**
   * High level goal:
   *  - Read pin
   *  - Put pin state into result
   *  - Use Carry to mark done reading all bits for main loop
   *
   * Tricks:
   *  - Use `sbic` to help set Carry bit to pin state
   *  - Use Carry bit and `rol` to do all the hard work
   *
   * Detailed implementation, simplified:
   *  - Carry starts as 0 (from main loop requirements)
   *  - `sbic` Test input. If high, skip next instruction
   *  - `sec ` Set Carry to 1 (unless skipped)
   *  - `rjmp` Jump to shared remainder of implementation
   *  - `rol ` Move Carry into result word, first byte
   *  - `rol ` Rotate result word, middle byte
   *  - `rol ` Rotate Carry out of result word, last byte
   *  - `reti` Return from interrupt (unless we're done)
   */

  // Carry is always zero at this point
  asm("sbic %[PIN], %[N]\n\tsec; Set Carry, unless input high"
      // If input was low, Carry is still low
      // If was high, Carry is now high
      :                          // Carry is our "output"
      : [PIN] "M"(unsigned(Port) // Convert Port to integer
                  - __SFR_OFFSET // Offset because of AVR internal workings
                  - 2            // Distance from PINx to PORTx
                  ),
        [N] "I"(Pin));

  // Share the bit handling logic between all pins
  asm("rjmp %x[HandleBit]" ::[HandleBit] "p"(&MakeResponse::bitByBit));
}

// Don't pollute
#undef ResultReg0
#undef ResultReg1
#undef ResultReg2

// We need to mark this as Naked for maximum performance because the generated entry/exit sequences are unnecessary in
// our known execution path.
ISR(TIMER0_COMPA_vect, ISR_NAKED) {
  if (AssemblyComments) asm("; Start of TIMER0_COMPA_vect");

  using AVR::DShot::BDShotConfig::Debug::EmitPulseAtISR;
  using AVR::DShot::BDShotConfig::Debug::Pin;
  if (EmitPulseAtISR) Pin::on();

  // If we got *extra* fancy, we could save 3 clock cycles (and a word of flash) by putting this "ijmp" directly in the
  // interrupt table. But this doesn't improve our resolution in any way.
  asm("ijmp ; Jump to Z register, set in getResponse() with setZ()");
}

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
AVR::DShot::Response AVR::DShot::BDShot<Port, Pin, Speed>::sendCommand(Command<true> c) {
  // Set output mode only while sending command
  Parent::output();

  // We don't want DShot implementation to touch interrupts
  Parent::sendCommand(c, false);
  if (AVR::DShot::BDShotConfig::ResetWatchdog::AfterSend) asm("wdr");

  // Return pin to input mode
  Parent::input();

  return getResponse();
}
