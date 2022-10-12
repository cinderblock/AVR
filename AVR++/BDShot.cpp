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
#include "GCR.hpp"
#include <avr/interrupt.h>

// Yes, we're including the cpp
#include "DShot.cpp"

constexpr auto BitOverflowFlag = OCF0A;

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
void AVR::DShot::BDShot<Port, Pin, Speed>::init() {
  using Basic::u1;

  if (Parent::IO::isHigh()) {
    // Output needs to be low long enough to get out of bootloader and start main program
    Parent::IO::clr();
    Parent::IO::output();

    // TODO: This is a long delay. Allow other things to happen while we wait.
    _delay_ms(AVR::DShot::BDShotConfig::exitBootloaderDelay);
  }

  // Alternate timer mode that is needed to use the OCR debug outputs
  constexpr bool useFastPWM = Debug::OutputOCR;

  // CTC Mode (clear counter at OCR0A)
  u1 wgm = 0b010;
  u1 const prescaler = 1;

  u1 com0A = 0;
  u1 com0B = 0;

  // Fast PWM is similar enough to CTC Mode that we don't care which one we use
  if (useFastPWM) { wgm = 0b111; }

  asm("; Setup Timer Output Compare Modules");

  // Sample on overflows
  OCR0A = Periods::delayPeriodTicks - 1;

  if (Debug::OutputOCR) {
    static_assert(Debug::OutputOCR <= useFastPWM, "DebugUseOCR requires useFastPWM");

    OCR0B = Periods::delayHalfPeriodTicks;

    com0A = 0b01; // Toggle on compare match
    // com0B = 0b01; // Toggle on compare match
    com0B = 0b11; // Set on compare match, clear at TOP

// Known pins for ATmega32u4
#ifdef __AVR_ATmega32U4__
    // IO Setup OCR0A (PB7, OC1C)
    Output<AVR::Ports::B, 7>::init();

    // IO Setup OCR0B (PD0, SCL)IDR
    Output<AVR::Ports::D, 0>::init();
#endif
  }

  asm("; Init BDShot");

  // TODO: Make timer configurable?

  // Set up and start timer that we use internally
  TIMSK0 = 0; // Ensure timer interrupts are disabled
  TCCR0A = (com0A & 0b11) << COM0A0 | (com0B & 0b11) << COM0B0 | (wgm & 0b11);
  // Start the timer
  TCCR0B = ((wgm >> 2) & 1) << WGM02 | (prescaler & 0b111) << CS00;

  // Set output high
  Parent::IO::set();
}

// Make sure these functions are always optimized
#pragma GCC optimize "O3"

// Pick 3 "call-used" or "call-clobbered" general purpose registers.
// Order/adjacency does not matter.
// We'll use these to store the response as we read it.
// Since no other code is running during the read, we don't need to protect these registers either.
#define Result0Reg "r18"
#define Result1Reg "r19"
#define Result2Reg "r20"

constexpr bool const_string_equal(char const *a, char const *b) {
  while (*a || *b)
    if (*a++ != *b++) return false;
  return true;
}

static_assert(!const_string_equal(Result0Reg, Result1Reg), "Result0Reg must not be equal to Result1Reg");
static_assert(!const_string_equal(Result1Reg, Result2Reg), "Result1Reg must not be equal to Result2Reg");
static_assert(!const_string_equal(Result2Reg, Result0Reg), "Result2Reg must not be equal to Result0Reg");

static_assert(!const_string_equal(Result0Reg, "r0"), "Result0Reg must not be __tmp_reg__");
static_assert(!const_string_equal(Result0Reg, "r1"), "Result0Reg must not be __zero_reg__");
static_assert(!const_string_equal(Result0Reg, "r24"), "Result0Reg must not be A register that GCC uses");
static_assert(!const_string_equal(Result0Reg, "r30"), "Result0Reg must not be Z register");
static_assert(!const_string_equal(Result0Reg, "r31"), "Result0Reg must not be Z register");

static_assert(!const_string_equal(Result1Reg, "r0"), "Result1Reg must not be __tmp_reg__");
static_assert(!const_string_equal(Result1Reg, "r1"), "Result1Reg must not be __zero_reg__");
static_assert(!const_string_equal(Result1Reg, "r24"), "Result1Reg must not be A register that GCC uses");
static_assert(!const_string_equal(Result1Reg, "r30"), "Result1Reg must not be Z register");
static_assert(!const_string_equal(Result1Reg, "r31"), "Result1Reg must not be Z register");

static_assert(!const_string_equal(Result2Reg, "r0"), "Result2Reg must not be __tmp_reg__");
static_assert(!const_string_equal(Result2Reg, "r1"), "Result2Reg must not be __zero_reg__");
static_assert(!const_string_equal(Result2Reg, "r24"), "Result2Reg must not be A register that GCC uses");
static_assert(!const_string_equal(Result2Reg, "r30"), "Result2Reg must not be Z register");
static_assert(!const_string_equal(Result2Reg, "r31"), "Result2Reg must not be Z register");

/**
 * This is the only way to tell GCC to use fixed registers for certain bytes.
 * This will generate warnings for other functions.
 * But we don't actually pollute anything that GCC doesn't expect to be mangled outside of a short un-interruptable
 * block.
 *
 * Ignore the warnings:
 *  - warning: call-clobbered register used for global register variable
 *  - warning: fixed register r19 used to pass parameter to function
 *
 * These registers are not used outside of processing a single response and will be reset on every run.
 * So, contrary to many standards, we want to use a "call-clobbered" register because these aren't really global.
 */
register Basic::u1 result0 asm(Result0Reg);
register Basic::u1 result1 asm(Result1Reg);
register Basic::u1 result2 asm(Result2Reg);

namespace MakeResponse {

/**
 * @return false if correct
 */
inline static constexpr bool isBadChecksum(Basic::u1 n3, Basic::u1 n2, Basic::u1 n1, Basic::u1 n0) {
  return 0xf ^ n0 ^ n1 ^ n2 ^ n3;
}

Basic::u1 decodeNibble(Basic::u1 b) { return GCR::decode(b & 0x1f); }

static AVR::DShot::Response fromResult() {
  asm("; fromResult");

  // All done!

  // Disable interrupt
  TIMSK0 = 0;

  /**
   * Here is where we need the "magic" to happen.
   *
   * In getResponse(), in our spin loop, waiting for transitions, GCC thinks that it will never get out of it.
   * If we returned from this interrupt like normal, we'd go right back into that infinite loop.
   * Fortunately getResponse() needs to return a Response.
   * So if we can just get some other function to return that Response for us...
   *
   * First we remove the last return pointer from the stack, which currently is somewhere in the middle of that
   * loop. Now, if we "return", we'll basically be returning from getResponse() instead. To seal the deal, we jump
   * to a function that parses Result into a Response and returns it as we need.
   */

  // Pop the interrupt return location off the stack (to get out of the ultra fast main loop)
  // We can also trash the previously set Z register value since we don't need it.
  asm("pop r30");
  asm("pop r30");

  Basic::u1 n0, n1, n2, n3;

  asm("; NOW WE GET TO USE OUR REGISTERS: " Result0Reg " " Result1Reg " " Result2Reg "\n\t");

  asm(
      // First we undo the shifting

      // Use r24 instead of __temp_reg__ because we can

      "mov  " /*        */ "r24, " Result2Reg "\t; Copy byte 2\n\t"
      "lsr  " /*        */ "r24\t; Shift the copy\n\t"
      "eor  " Result2Reg ", r24\t; XOR the copy back\n\t"

      "mov  " /*        */ "r24, " Result1Reg "\t; Copy byte 1\n\t"
      "ror  " /*        */ "r24\t; Shift the copy\n\t"
      "eor  " Result1Reg ", r24\t; XOR the copy back\n\t"

      "mov  " /*        */ "r24, " Result0Reg "\t; Copy byte 0\n\t"
      "ror  " /*        */ "r24\t; Shift the copy\n\t"
      "eor  " Result0Reg ", r24\t; XOR the copy back\n\t"

      // Now we turn 3 bytes into 4 quintets

      // Layout:                 Result 2   Result 1   Result 0 Carry
      "mov  r24, " Result0Reg "\t;   3333  3222 2211  1110 0000 x\n\t" // n0 is ready. Put it directly into r24.

      "rol  " /**/ Result1Reg "\t;   3333  2222 211x  1110 0000 3\n\t"
      "rol  " /**/ Result2Reg "\t; 3 3333  2222 211x  1110 0000 x\n\t" // n3 is ready in Result2Reg for later.

      "ror  " /**/ Result1Reg "\t; 3 3333  x222 2211  1110 0000 x\n\t"
      "ror  " /**/ Result1Reg "\t; 3 3333  xx22 2221  1110 0000 1\n\t"
      "ror  " /**/ Result0Reg "\t; 3 3333  xx22 2221  1111 0000 0\n\t"
      "ror  " /**/ Result1Reg "\t; 3 3333  xxx2 2222  1111 0000 1\n\t" // n2 is ready in Result1Reg for later.

      "ror  " /**/ Result0Reg "\t; 3 3333  xxx2 2222  1111 1000 0\n\t"
      "ror  " /**/ Result0Reg "\t; 3 3333  xxx2 2222  0111 1100 0\n\t"
      "ror  " /**/ Result0Reg "\t; 3 3333  xxx2 2222  0011 1110 0\n\t"
      "ror  " /**/ Result0Reg "\t; 3 3333  xxx2 2222  0001 1111 0\n\t" // n1 is ready in Result0Reg for later.

      // Decode the GCR encoded quintets into nibbles

      "call %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n0], r24\n\t"

      "mov  r24, " Result0Reg "\n\t"
      "call %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n1], r24\n\t"

      "mov  r24, " Result1Reg "\n\t"
      "call %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n2], r24\n\t"

      "mov  r24, " Result2Reg "\n\t"
      "call %x[decodeNibble]\t; Decode nibbles\n\t"
      "mov  %[n3], r24\n\t"

      // Let the compiler do the rest

      : [n0] "=r"(n0), [n1] "=r"(n1), [n2] "=r"(n2), [n3] "=r"(n3)
      : [decodeNibble] "p"(&decodeNibble)
      : "r24", Result0Reg, Result1Reg, Result2Reg);

  using AVR::DShot::Response;

  if (n3 == 0xff) return Response::Error::BadDecodeFirstNibble;
  if (n2 == 0xff) return Response::Error::BadDecodeSecondNibble;
  if (n1 == 0xff) return Response::Error::BadDecodeThirdNibble;
  if (n0 == 0xff) return Response::Error::BadDecodeFourthNibble;

  if (isBadChecksum(n3, n2, n1, n0)) return Response::Error::BadChecksum;

  // We've checked the checksum, so we can safely ignore the checksum nibble
  return {n3, n2, n1};
}
} // namespace MakeResponse

// We need to mark this as Naked for maximum performance because the generated entry/exit sequences are unnecessary in
// our known execution path.
ISR(TIMER0_COMPA_vect, ISR_NAKED) {
  asm("; Start of TIMER0_COMPA_vect");

  if (AVR::DShot::Debug::EmitPulseAtSample) AVR::DShot::Debug::Pin::on();

  // If we got *extra* fancy, we could save 3 clock cycles by putting this "ijmp" directly in the interrupt table
  // Jump to Z register, set in getResponse() with setZ()
  asm("ijmp ; Jump to Z register");
}

/**
 * @brief Set the Z register to the desired location
 *
 * @param z A pointer to the location to jump to
 */
inline static void setZ(void *z) {
  asm("; setZ\n\t"
      "ldi r30, lo8(%x[function])\n\t"
      "ldi r31, hi8(%x[function])\n\t"
      : // No outputs
      : [function] "p"(z)
      : "r30", "r31");
}
template <typename T>
inline static void setZ(T (*z)()) {
  setZ((void *)z);
}

namespace AVR {
namespace Core {
namespace Ticks {
namespace Hardware {
constexpr unsigned ISR = 5;
} // namespace Hardware
namespace Instruction {
constexpr unsigned Skip1Word = 2;
constexpr unsigned Skip2Words = 3;
constexpr unsigned LoaDImediate = 1;
constexpr unsigned Out = 1;
constexpr unsigned Branch = 2;
constexpr unsigned Jmp = 3;
constexpr unsigned IJmp = 2;
constexpr unsigned RJmp = 2;
constexpr unsigned RetI = 5; // Or is it 4?
} // namespace Instruction
} // namespace Ticks
} // namespace Core
} // namespace AVR

// We're expecting 4 nibbles in the response, each encoded with GCR
// We don't count the extra bit because it's handled manually as it's the trigger to start reading the response and is
// *always* 0.
static constexpr unsigned expectedNibbles = 4;
static constexpr Basic::u1 ExpectedBits = expectedNibbles * GCR::inBits;

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
   * Instead, we try to recover clocking information on *any* transition in a fast (~8 clock cycles) spin loop.
   *
   * Timer overflows indicate a received bit.
   * We can service this safely since we expect no transitions.
   *
   * Pseudo code:
   * - Enable our interrupts
   * - Wait for initial high-to-low transition
   *   - Timeout if no response in >50us
   * - Start timer with overflow in 1.5 bit periods
   *   - We're skipping the first bit because it's a start bit that is *always* 0
   * - Enable global interrupts
   * - On Overflow Interrupt:
   *   - Sample & store bit
   *   - if 20 bits
   *     - Set Temp bit
   * - Wait for Transition
   *   - Sync timer
   *     - Set to more than 0.5 bit period to accommodate delays. Likely ~0.6.
   *       Overflow, and thus capture, in less than half a bit period
   *   - Loop using Temp bit
   *
   * Assumptions:
   * - All other interrupts are off
   * - Global interrupts are on
   */

  // clang-format off
  /**
   * Let's walk through an example waveform. The bit periods are not quite uniform intentionally.
   * 
   * We're receiving this standard waveform that basically means "not spinning" (period is at maximum).
   * Notice the timer counts up and overflows every ~bit period. On overflow, an interrupt is triggered and we sample the bit.
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
   * regroup:  //// //////|| ///||||| - group 20 bits as 4 5-bit words
   * gcr    : 01111 01111 01111 11001 - 4 5-bit words
   * Nibble :     3     2     1     0 - decode each 5-bit word into nibbles of data
   * data   :  1111  1111  1111  0000 - 4 nibbles of data
   * Meaning:  eeep  bbbb  bbbb  cccc - 3 bits exponent, 1 bit to mark EDT(0) or normal(1), 8 bits base, 4 bits checksum
   * Checksum: n3 ^ n2 ^ n1 ^ n0 ^ 0xf == 0
   * Spin period = (base << exponent) microseconds
   */
  // clang-format on

  // These are probably defunct
  static_assert(Periods::delayPeriodTicks == 43, "delayPeriodTicks is not what we expect!");
  static_assert(Periods::delayHalfPeriodTicks == 21, "delayHalfPeriodTicks is not what we expect!");

  constexpr unsigned responseTimeoutTicks = (((long long)(F_CPU)) * AVR::DShot::BDShotConfig::responseTimeout) / 1e6;

  // We're overflowing with OCRA now which sets lower TOP than 0xff
  constexpr unsigned responseTimeoutOverflows = responseTimeoutTicks / Periods::delayPeriodTicks + 1;
  u1 overflowsWhileWaiting = 0;

  static_assert(responseTimeoutOverflows < u4(1) << (8 * sizeof(overflowsWhileWaiting)),
                "responseTimeoutOverflows is too large for this implementation");

  /**
   * Number of ticks from timer overflow to when we execute the instruction that samples the pin's input state.
   *
   * This comes mostly from 3 separate places:
   * - The AVR datasheet which specified ISR handling takes 5 clock cycles to execute.
   * - The de facto standard JMP table as the first instructions in the ISR, could be eliminated with a custom .init
   * table.
   * - Other instructions in the ISR before sampling the pin into a temp register
   */
  constexpr unsigned ticksFromOverflowToISRSample =
      AVR::Core::Ticks::Hardware::ISR +     // ISR time
      AVR::Core::Ticks::Instruction::Jmp +  // Initial jmp table, could be eliminated with a custom .init table
      AVR::Core::Ticks::Instruction::IJmp + // Our jmp to Z
      Debug::EmitPulseAtSample              // Pin toggle adds a clock cycle
      ;

  using namespace AVR::DShot::BDShotConfig;

  /**
   * Number of ticks it takes the initial spin loop to check for the initial high-to-low transition.
   * Most of the time, this number is used.
   * Sometimes, the timer has overflowed and we need to check that our timeout counter hasn't reached zero.
   *
   * @see `ticksInitialSpinLoopWorstCase`
   *
   * @note Maximum accuracy we can achieve of timing on initial transition
   * @note These lists are generated from looking at the generated assembly for these functions.
   */
  constexpr unsigned ticksInitialSpinLoop = 1                                           // Check Pin
                                            + ResetWatchdog::WaitingFirstTransitionFast // WDR
                                            + AVR::Core::Ticks::Instruction::RJmp       // Loop
                                            + AVR::Core::Ticks::Instruction::Skip1Word  // Didn't overflow
      ;

  /**
   * Number of ticks it takes the initial spin loop to check for the initial high-to-low transition while also
   * decrementing and checking the timer overflow counter.
   *
   * @note Maximum accuracy we can achieve of timing on initial transition
   * @note These lists are generated from looking at the generated assembly for these functions.
   */
  constexpr unsigned ticksInitialSpinLoopWorstCase = 1                                           // Check Pin
                                                     + ResetWatchdog::WaitingFirstTransitionFast // WDR
                                                     + AVR::Core::Ticks::Instruction::RJmp       // Loop
                                                     + 1                                         // Check Overflow flag
                                                     + AVR::Core::Ticks::Instruction::RJmp       // Handle overflow
                                                     + 1                                         // subi; i--
                                                     + 1 // breq; if (i == 0) {<exit path>} else ...
                                                     + 1 // sbi; clear timer flag
                                                     + ResetWatchdog::WaitingFirstTransitionTimerOverflow // WDR
                                                     + AVR::Core::Ticks::Instruction::RJmp // Back to main loop
      ;

  /**
   * Number of ticks from the initial high-to-low transition to when we can set the timer to some value.
   *
   * @note Part of the phase adjustment of reading bits on timer overflow
   * @note These lists are generated from looking at the generated assembly for these functions.
   */
  constexpr unsigned ticksFromTransitionToInitialTimerSync =
      AVR::Core::Ticks::Instruction::Skip1Word      // We test with a `sbic` instruction
      + 2 * BDShotConfig::useDebounce               // Debounce compensation
      + ResetWatchdog::ReceivedFirstTransition      // WDR
      + AVR::Core::Ticks::Instruction::LoaDImediate // Set register to immediate
      + AVR::Core::Ticks::Instruction::Out          // Set timer counter from register
      ;

  /**
   * Number of ticks it takes the ultra fast main spin loop to check for a transition
   *
   * @note Maximum accuracy we can achieve of timing on transition
   */
  constexpr unsigned ticksSpinLoop = Debug::EmitPulsesAtIdle               // Pin toggle adds a clock cycle to the loop
                                     + 1                                   // Reading the pin state takes 1 clock cycle
                                     + AVR::Core::Ticks::Instruction::RJmp // Jump to start of loop
      ;

  /**
   * Number of ticks from a transition to when we can set the timer to some value.
   * Receiving bits has a different code path that the initial transition.
   *
   * @note Part of the phase adjustment of reading bits on timer overflow
   */
  constexpr unsigned ticksFromTransitionToTimerSync =
      1                                    // Initial read
      + 2 * BDShotConfig::useDebounce      // Debounce compensation
      + 1                                  // Skip over rjmp
      + AVR::Core::Ticks::Instruction::Out // Set timer counter from register
      ;

  // Larger numbers will make the samples happen sooner
  constexpr int fudgeInitialTicks = 0;
  constexpr int fudgeSyncTicks = 0;

  // constexpr unsigned adjustSyncTicks = ticksFromOverflowToISRSample + 1;
  // constexpr unsigned adjustInitialTicks = adjustSyncTicks + 10;
  constexpr unsigned adjustInitialTicks =
      // Half of the normal loop time
      ticksInitialSpinLoop / 2
      // Initial sync
      + ticksFromTransitionToInitialTimerSync
      // Compensate for ISR service time
      + ticksFromOverflowToISRSample
      // Let us easily fudge the numbers
      + fudgeInitialTicks;

  constexpr unsigned adjustSyncTicks =
      // Half of the loop time
      ticksSpinLoop / 2
      // Continuous sync
      + ticksFromTransitionToTimerSync
      // Compensate for ISR service time
      + ticksFromOverflowToISRSample
      // Let us easily fudge the numbers
      + fudgeSyncTicks;

  constexpr u1 timerCounterValueInitial = u1(adjustInitialTicks - Periods::delayHalfPeriodTicks);
  constexpr u1 timerCounterValueSync = Periods::delayHalfPeriodTicks + adjustSyncTicks;

  // Assumptions of these implementations
  static_assert(Periods::delayPeriodTicks > adjustInitialTicks, "delayPeriodTicks is too small");
  static_assert(Periods::delayPeriodTicks < 200, "delayPeriodTicks is too large");

  asm("; Waiting for first transition");

  // Wait for initial high-to-low transition, or timeout while waiting
  while (isHigh() || BDShotConfig::useDebounce && isHigh()) {
    if (ResetWatchdog::WaitingFirstTransitionFast) asm("wdr");

    // If timer overflows, see if we've overflowed enough to know we're not getting a response.
    if (!(TIFR0 & _BV(BitOverflowFlag))) continue;

    // Timeout waiting for response
    if (overflowsWhileWaiting++ > responseTimeoutOverflows) return AVR::DShot::Response::Error::ResponseTimeout;

    // Clear the flag so we can wait for the next overflow
    TIFR0 |= _BV(BitOverflowFlag);

    if (ResetWatchdog::WaitingFirstTransitionTimerOverflow) asm("wdr");
  }

  // Yay! We're getting a response. Try and receive it!
  if (ResetWatchdog::ReceivedFirstTransition) asm("wdr");

  // Set timer so that it matches trigger register in 1.5 bit periods
  asm("; Initial Ticks");
  TCNT0 = timerCounterValueInitial;

  // Clear flag by setting it
  TIFR0 |= _BV(BitOverflowFlag);

  // The magic that lets this work on *any* pin
  setZ(&ReadBitISR);
  // Just make sure nothing else uses the Z registers locally (with a hope and educated guesses)

  // Use a set bit in result to indicate we've gotten all the bits.
  // That bit will roll into Carry bit, which we test to break out of our spin loop.
  constexpr u3 FinishedMarker = 1 << (8 * sizeof(FinishedMarker) - ExpectedBits);

  // Save the contents of the call-saved result registers
  // asm("push " Result0Reg "\n\t"
  //     "push " Result1Reg "\n\t"
  //     "push " Result2Reg "\n\t");

  result0 = FinishedMarker >> (8 * 0);
  result1 = FinishedMarker >> (8 * 1);
  result2 = FinishedMarker >> (8 * 2);

  // Make sure Carry starts in expected state
  asm("clc \t;Clear Carry Flag");

  asm("; DON'T MESS WITH REGISTERS: " Result0Reg " " Result1Reg " " Result2Reg " r30 r31 or Carry\n\t");

  // Enable interrupt
  TIMSK0 = _BV(BitOverflowFlag);

  register u1 const syncValue = timerCounterValueSync;

  // The ultra fast loop implementation
  // Relies on extra weird code at the end of the ISR to save us
  asm("; Ultra Fast Loop Start");
  while (true) {
    do {
      if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
      asm("; Ultra Fast Loop. Waiting for transition to high.");
    } while (!isHigh() || BDShotConfig::useDebounce && !isHigh());
    TCNT0 = syncValue;
    if (Debug::EmitPulseAtSync) {
      Debug::Pin::on();
      Debug::Pin::off();
    }
    do {
      if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
      asm("; Ultra Fast Loop. Waiting for transition to low.");
    } while (isHigh() || BDShotConfig::useDebounce && isHigh());
    TCNT0 = syncValue;
    if (Debug::EmitPulseAtSync) {
      Debug::Pin::on();
      Debug::Pin::off();
    }
  }

  // GCC doesn't think code execution can reach here, which is fine.
  // The ISR messes with the call stack and will "return" for us.
  asm volatile("; UltraLoop End");
}

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
void AVR::DShot::BDShot<Port, Pin, Speed>::ReadBitISR() {
  /**
   * High level goal:
   *  - Read pin
   *  - Put pin state into result
   *  - Use carry to mark done reading all bits for main loop
   *
   * Tricks:
   *  - Use `sbic` to help set carry bit to pin state
   *  - Use carry bit and `rol` to do all the hard work
   *
   * Detailed implementation:
   *  - Carry starts as 0 (from main loop requirements)
   *  - `sbic` Test input. If high, skip next instruction
   *  - `sec ` Set carry to 1 (unless skipped)
   *  - `rol ` Move Carry into result word, first byte
   *  - `rol ` Rotate result word, middle byte
   *  - `rol ` Rotate Carry out of result word, last byte
   *  - `reti` Return from interrupt (unless we're done)
   */

  // Carry is always zero at this point
  asm("; Skip next instruction if input is low\n\t"
      "sbic %[PIN], %[N]\n\t"

      "sec ; Set Carry"
      :
      : [PIN] "M"(unsigned(Port) // Convert Port to integer
                  - __SFR_OFFSET // Offset because of AVR internal workings
                  - 2            // Distance from PINx to PORTx
                  ),
        [N] "I"(Pin) // The pin number
  );
  // If input was low, carry is still low.
  // If was high, carry is now high.

  if (AVR::DShot::BDShotConfig::ResetWatchdog::SampledBit) asm("wdr");

  asm("; Store Carry into Result\n\t"
      "rol " Result0Reg "\n\t"
      "rol " Result1Reg "\n\t"
      "rol " Result2Reg "\n\t"
      "; And get Carry from Result. If set, it indicates we're done.");

  // Reti if Carry is clear to continue receiving bits
  asm goto("brcc %l[DoneSamplingPin]; Branch to reti if Carry cleared" : : : : DoneSamplingPin);

  asm("; DONE WITH REGISTERS: r30 r31 and Carry\n\t");

  // Jump to the function [MakeResponse::fromResult()] that makes the result the getResponse() caller wants.
  // When it returns, since we've mucked with the call stack, it'll return in place of getResponse().
  if (BDShotConfig::useRelativeJmpAtEndISR) {
    asm("rjmp %x[Done]; Jump to MakeResponse::fromResult" : : [Done] "p"(&MakeResponse::fromResult));
  } else {
    asm("jmp  %x[Done]; Jump to MakeResponse::fromResult" : : [Done] "p"(&MakeResponse::fromResult));
  }

DoneSamplingPin:
  asm("reti");
}

static constexpr bool HandleInterrupts = 0;
static constexpr bool HandleInterruptsAlwaysOn = 0;

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
AVR::DShot::Response AVR::DShot::BDShot<Port, Pin, Speed>::sendCommand(Command<true> c) {

  u1 savedSREG;

#ifdef __AVR_ATmega32U4__
  u1 savedTIMSK4;
  u1 savedTIMSK3;
  u1 savedTIMSK1;
  u1 savedEIMSK;
  u1 savedPCMSK0; // Covers PCICR as well
  u1 savedUCSR1B; // Just RXCIE1 TXCIE1 UDRIE1
  u1 savedUSBCON; // Just VBUSTE
  u1 savedUDIEN;
  u1 savedUEIENX;
  u1 savedSPMCSR; // Just SPMIE
  u1 savedACSR;   // Just ACIE clear ACI
  u1 savedSPCR;   // Just SPIE
  u1 savedEECR;   // Just EERIE
  u1 savedADCSRA; // Just ADIE clear ADSC ADIF
  u1 savedTWCR;   // Just TWIE clear TWINT
  u1 savedTCCR4D; // Just FPIE4 clear FPF4
  u1 savedWDTCSR; // Just WDIE clear WDIF

  if (HandleInterrupts) {

    savedTIMSK4 = TIMSK4;
    TIMSK4 = 0;

    savedTIMSK3 = TIMSK3;
    TIMSK3 = 0;

    savedTIMSK1 = TIMSK1;
    TIMSK1 = 0;

    savedEIMSK = EIMSK;
    EIMSK = 0;

    savedPCMSK0 = PCMSK0;
    PCMSK0 = 0;

    savedUCSR1B = UCSR1B;
    UCSR1B &= ~(_BV(RXCIE1) | _BV(TXCIE1) | _BV(UDRIE1));

    savedUSBCON = USBCON;
    USBCON &= ~_BV(VBUSTE);

    savedUDIEN = UDIEN;
    UDIEN = 0;

    savedUEIENX = UEIENX;
    UEIENX = 0;

    savedSPMCSR = SPMCSR;
    SPMCSR &= ~_BV(SPMIE);

    savedACSR = ACSR;
    ACSR &= ~_BV(ACIE);
    savedACSR &= ~_BV(ACI);

    savedSPCR = SPCR;
    SPCR &= ~_BV(SPIE);

    savedEECR = EECR;
    EECR &= ~_BV(EERIE);

    savedADCSRA = ADCSRA;
    ADCSRA &= ~_BV(ADIE);
    savedADCSRA &= ~(_BV(ADIF) | _BV(ADSC));

    savedTWCR = TWCR;
    TWCR &= ~_BV(TWIE);
    savedTWCR &= ~_BV(TWINT);

    savedTCCR4D = TCCR4D;
    TCCR4D &= ~_BV(FPIE4);
    savedTCCR4D &= ~_BV(FPF4);

    savedWDTCSR = WDTCSR;
    WDTCSR &= ~_BV(WDIE);
    savedWDTCSR &= ~_BV(WDIF);
  }
#else
  static_assert(!HandleInterrupts, "Not yet implemented");
#endif

  // Set output mode only while sending command
  Parent::output();

  // We can't let DShot implementation handle interrupts
  Parent::sendCommand(c, false);
  if (AVR::DShot::BDShotConfig::ResetWatchdog::AfterSend) asm("wdr");

  // Return pin to input mode
  Parent::input();

  // getResponse() requires interrupts to be enabled globally
  // but also requires all other interrupts to be disabled.
  // It also always leaves interrupts disabled globally.

  if (HandleInterrupts && !HandleInterruptsAlwaysOn) {
    savedSREG = SREG;

    asm("sei");
  }

  const auto r = getResponse();

  if (HandleInterrupts) {
    SREG = savedSREG;
  } else if (HandleInterruptsAlwaysOn) {
    asm("sei");
  }

#ifdef __AVR_ATmega32U4__
  if (HandleInterrupts) {
    WDTCSR = savedWDTCSR;
    TCCR4D = savedTCCR4D;
    TWCR = savedTWCR;
    ADCSRA = savedADCSRA;
    EECR = savedEECR;
    SPCR = savedSPCR;
    ACSR = savedACSR;
    SPMCSR = savedSPMCSR;
    UEIENX = savedUEIENX;
    UDIEN = savedUDIEN;
    USBCON = savedUSBCON;
    UCSR1B = savedUCSR1B;
    PCMSK0 = savedPCMSK0;
    EIMSK = savedEIMSK;
    TIMSK1 = savedTIMSK1;
    TIMSK3 = savedTIMSK3;
    TIMSK4 = savedTIMSK4;
  }
#endif

  return r;
}
