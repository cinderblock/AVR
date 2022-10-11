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
template <bool SkipBufferEmptyCheck = false>
void AVR::DShot::Debug::WriteByte(Basic::u1 const byte) __attribute__((weak));

template <bool SkipBufferEmptyCheck = false>
void AVR::DShot::Debug::WriteByte(Basic::u1 const byte) {
  if (!SkipBufferEmptyCheck)
    while (~UCSR1A & (1 << UDRE1)) {}
  UDR1 = byte;
}

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

  // We use assembly comments to mark important points in the generated assembly to aid in analyzing the generated
  // assembly
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

constexpr unsigned responseTimeoutTicks = (((long long)(F_CPU)) * AVR::DShot::BDShotConfig::responseTimeout) / 1e6;
static inline void shiftInCarryLeft(Basic::u1 &byte) { asm("rol %0" : "+r"(byte)); }
static inline void shiftInCarryLeft(Basic::u2 &word) {
  asm("rol %A0\n\t"
      "rol %B0"
      : "+r"(word));
}
static inline void shiftInCarryLeft(Basic::u3 &word) {
  asm("rol %A0\n\t"
      "rol %B0\n\t"
      "rol %C0"
      : "+r"(word));
}

static inline void setCarry(bool value) {
  if (value)
    asm("sec \t;Set Carry Flag");
  else
    asm("clc \t;Clear Carry Flag");
}

namespace MakeResponse {

/**
 * @return false if correct
 */
inline static constexpr bool checkResponseChecksum(Basic::u1 n3, Basic::u1 n2, Basic::u1 n1, Basic::u1 n0) {
  return 0xf ^ n0 ^ n1 ^ n2 ^ n3;
}

inline static constexpr AVR::DShot::Response fromGcrNibbles(Basic::u1 n3, Basic::u1 n2, Basic::u1 n1, Basic::u1 n0) {
  using AVR::DShot::Response;

  if ((n3 = GCR::decode(n3)) == 0xff) return Response::Error::BadDecodeFirstNibble;
  if ((n2 = GCR::decode(n2)) == 0xff) return Response::Error::BadDecodeSecondNibble;
  if ((n1 = GCR::decode(n1)) == 0xff) return Response::Error::BadDecodeThirdNibble;
  if ((n0 = GCR::decode(n0)) == 0xff) return Response::Error::BadDecodeFourthNibble;

  // asm("; done decoding");

  if (checkResponseChecksum(n3, n2, n1, n0)) return Response::Error::BadChecksum;

  // asm("; done checksum");

  // We've checked the checksum, so we can safely ignore the checksum nibble
  return {n3, n2, n1};
}

inline static constexpr AVR::DShot::Response fromDecodedNibbles(Basic::u1 n3, Basic::u1 n2, Basic::u1 n1,
                                                                Basic::u1 n0) {
  using AVR::DShot::Response;

  if (n3 == 0xff) return Response::Error::BadDecodeFirstNibble;
  if (n2 == 0xff) return Response::Error::BadDecodeSecondNibble;
  if (n1 == 0xff) return Response::Error::BadDecodeThirdNibble;
  if (n0 == 0xff) return Response::Error::BadDecodeFourthNibble;

  // asm("; done decoding");

  if (checkResponseChecksum(n3, n2, n1, n0)) return Response::Error::BadChecksum;

  // asm("; done checksum");

  // We've checked the checksum, so we can safely ignore the checksum nibble
  return {n3, n2, n1};
}

inline static AVR::DShot::Response fromRawWord(Basic::u3 raw) {
  using namespace AVR::DShot;
  if (Debug::EmitEncodedNibblesToUART) {
    _delay_us(10);
    Debug::WriteByte<false>(raw >> 16);
    Debug::WriteByte<false>(raw >> 8);
    Debug::WriteByte(raw);
  }

  asm("; undo shifting");

  // Undo the "shift-encoding"
  raw ^= raw >> 1;

  asm("; done shifting");

  if (Debug::EmitEncodedNibblesToUART) {
    _delay_us(10);
    Debug::WriteByte<false>(raw >> 16);
    Debug::WriteByte<false>(raw >> 8);
    Debug::WriteByte(raw);
  }

  asm("; decoding nibbles");

  u1 n3 = raw >> (GCR::inBits * 3) & GCR::inMask;
  u1 n2 = raw >> (GCR::inBits * 2) & GCR::inMask;
  u1 n1 = raw >> (GCR::inBits * 1) & GCR::inMask;
  u1 n0 = raw >> (GCR::inBits * 0) & GCR::inMask;

  asm("; done decoding nibbles");

  if (Debug::EmitEncodedNibblesToUART) {
    _delay_us(10);
    Debug::WriteByte<false>(n3);
    Debug::WriteByte<false>(n2);
    Debug::WriteByte(n1);
    Debug::WriteByte(n0);
  }

  return fromGcrNibbles(n3, n2, n1, n0);
}

inline static AVR::DShot::Response fromRawBytes(Basic::u1 raw2, Basic::u1 raw1, Basic::u1 raw0) {
  // Collect bytes in the correct order for shifting
  return fromRawWord(u3(raw2) << 16 | u2(raw1) << 8 | raw0);
}

static AVR::DShot::Response fromResult();

} // namespace MakeResponse

// Pick 3 "call-used" or "call-clobbered" general purpose registers.
// Order/adjacency does not matter.
// We'll use these to store the response as we read it.
// Since no other code is running during the read, we don't need to protect these registers either.
#define Result0Reg "r18"
#define Result1Reg "r19"
#define Result2Reg "r20"

inline Basic::u3 getResult() {
  Basic::u3 result;
  asm("mov %A0, " Result0Reg "\t; Get result byte 0\n\t"
      "mov %B0, " Result1Reg "\t; Get result byte 1\n\t"
      "mov %C0, " Result2Reg "\t; Get result byte 2"
      : "=r"(result));
  return result;
}

template <Basic::u1 v>
inline void initResult() {
  asm("ldi " Result0Reg ", %[byte0]\t; Set result byte 0\n\t"
      "ldi " Result1Reg ", %[byte1]\t; Set result byte 1\n\t"
      "ldi " Result2Reg ", %[byte2]\t; Set result byte 2"
      :
      : [byte0] "I"(1 << v), [byte1] "I"((1 << v) >> 8), [byte2] "I"((1 << v) >> 16));
}

inline Basic::u3 getResultUnShifted() {
  Basic::u3 result;
  asm("mov %C[result],\t" Result2Reg "\t; Get result byte 2\n\t"
      "lsr   \t" Result2Reg "\n\t"
      "eor %C[result],\t" Result2Reg "\n\t"

      "mov %B[result],\t" Result1Reg "\t; Get result byte 1\n\t"
      "ror   \t" Result1Reg "\n\t"
      "eor %B[result],\t" Result1Reg "\n\t"

      "mov %A[result],\t" Result0Reg "\t; Get result byte 0\n\t"
      "ror   \t" Result0Reg "\n\t"
      "eor %A[result],\t" Result0Reg
      : [result] "=r"(result)
      :
      : Result0Reg Result1Reg Result2Reg);
  return result;
}

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

enum class ResultProcedure {
  RawBytes,
  FromWord,
  FromGcrNibbles,
};

constexpr ResultProcedure resultProcedure = ResultProcedure::FromGcrNibbles;

Basic::u1 decodeNibble(Basic::u1 b) { return GCR::decode(b & 0x1f); }

AVR::DShot::Response MakeResponse::fromResult() {
  asm("; fromResult");

  if (resultProcedure == ResultProcedure::RawBytes) {
    return fromRawBytes(result2, result1, result0);
  } else if (resultProcedure == ResultProcedure::FromWord) {
    return fromRawWord(getResult());
  } else if (resultProcedure == ResultProcedure::FromGcrNibbles) {
    Basic::u1 n0, n1, n2, n3;

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

    return fromDecodedNibbles(n3, n2, n1, n0);
  }
}

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
  constexpr bool useDebounce = true;

  static_assert(Periods::delayPeriodTicks == 43, "delayPeriodTicks is not what we expect!");
  static_assert(Periods::delayHalfPeriodTicks == 21, "delayHalfPeriodTicks is not what we expect!");

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
  constexpr unsigned ticksInitialSpinLoopNormal = 1                                           // Check Pin
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
      + 2 * useDebounce                             // Debounce compensation
      + ResetWatchdog::ReceivedFirstTransition      // WDR
      + AVR::Core::Ticks::Instruction::LoaDImediate // Set register to immediate
      + AVR::Core::Ticks::Instruction::Out          // Set timer counter from register
      ;

  /**
   * Number of ticks it takes the main spin loop to check for a transition.
   *
   * @note Maximum accuracy we can achieve of timing on transition
   */
  constexpr unsigned ticksSpinLoopNormal =
      (Debug::EmitPulseAtSample || Debug::EmitPulseAtSync) // Pin toggle adds a clock cycle
      + AVR::Core::Ticks::Instruction::Skip1Word           // Check input
      + AVR::Core::Ticks::Instruction::Branch              // Loop
      ;

  /**
   * Number of ticks it takes the main spin loop to check for a transition in "ultra" mode
   *
   * @note Maximum accuracy we can achieve of timing on transition
   */
  constexpr unsigned ticksSpinLoopUltra =
      Debug::EmitPulsesAtIdle               // Pin toggle adds a clock cycle to the loop sasassssaa
      + 1                                   // Reading the pin state takes 1 clock cycle
      + AVR::Core::Ticks::Instruction::RJmp // Jump to start of loop
      ;

  constexpr unsigned ticksSpinLoop = BDShotConfig::useUltraLoop ? ticksSpinLoopUltra : ticksSpinLoopNormal;

  /**
   * Number of ticks from a transition to when we can set the timer to some value.
   * Receiving bits has a different code path that the initial transition.
   *
   * @note Part of the phase adjustment of reading bits on timer overflow
   */
  constexpr unsigned ticksFromTransitionToTimerSyncNormal =
      1                                     // Initial read
      + AVR::Core::Ticks::Instruction::RJmp // Jump to set timer
      + AVR::Core::Ticks::Instruction::Out  // Set timer counter from register
      ;
  /**
   * Number of ticks from a transition to when we can set the timer to some value.
   * Receiving bits has a different code path that the initial transition.
   *
   * @note Part of the phase adjustment of reading bits on timer overflow
   */
  constexpr unsigned ticksFromTransitionToTimerSyncUltra =
      1                                    // Initial read
      + 2 * useDebounce                    // Debounce compensation
      + 1                                  // Skip over rjmp
      + AVR::Core::Ticks::Instruction::Out // Set timer counter from register
      ;

  constexpr unsigned ticksFromTransitionToTimerSync =
      BDShotConfig::useUltraLoop ? ticksFromTransitionToTimerSyncUltra : ticksFromTransitionToTimerSyncNormal;

  // Larger numbers will make the samples happen sooner
  constexpr int fudgeInitialTicks = 0;
  constexpr int fudgeSyncTicks = 0;

  // constexpr unsigned adjustSyncTicks = ticksFromOverflowToISRSample + 1;
  // constexpr unsigned adjustInitialTicks = adjustSyncTicks + 10;
  constexpr unsigned adjustInitialTicks =
      // Half of the normal loop time
      ticksInitialSpinLoopNormal / 2
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
  while (isHigh() || useDebounce && isHigh()) {
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
  setCarry(false);

  asm("; DON'T MESS WITH REGISTERS: " Result0Reg " " Result1Reg " " Result2Reg " r30 r31 or Carry\n\t");

  // Enable interrupt
  TIMSK0 = _BV(BitOverflowFlag);

  register u1 const syncValue = timerCounterValueSync;

  constexpr bool useOneLoop = false;
  constexpr bool useTwoLoop = true;
  constexpr bool useInlineAssembly = true;

  if (BDShotConfig::useUltraLoop) {
    // The fastest implementation, but relies on extra weird code at the end of the ISR to save us
    asm("; UltraLoop Start");
    while (true) {
      do {
        if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
      } while (!isHigh() || useDebounce && !isHigh());
      TCNT0 = syncValue;
      if (Debug::EmitPulseAtSync) {
        Debug::Pin::on();
        Debug::Pin::off();
      }
      do {
        if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
      } while (isHigh() || useDebounce && isHigh());
      TCNT0 = syncValue;
      if (Debug::EmitPulseAtSync) {
        Debug::Pin::on();
        Debug::Pin::off();
      }
    }
    // GCC doesn't think code execution can reach here, which is fine.
    // The ISR messes with the call stack and will "return" for us.
    asm volatile("; UltraLoop End");
  } else if (useOneLoop) {
    // The DRY implementation that reuses the same loop. Compiles into something fat
    bool curr = false;

    while (true) {
      do {
        asm goto("brcs %l[DoneReadingBits]; branch to DoneReadingBits" :: ::DoneReadingBits);
      } while (curr == isHigh());
      TCNT0 = syncValue;
      curr = !curr;
    }
  } else if (useTwoLoop) {
    // A compact loop that exits on its own.
    while (true) {
      do {
        asm goto("brcs %l[DoneReadingBits]; branch to DoneReadingBits" :: ::DoneReadingBits);
      } while (!isHigh());
      TCNT0 = syncValue;
      do {
        asm goto("brcs %l[DoneReadingBits]; branch to DoneReadingBits" :: ::DoneReadingBits);
      } while (isHigh());
      TCNT0 = syncValue;
    }

  } else if (useInlineAssembly) {
    // A compact version of the "TwoLoop" implementation above
    asm goto(
        // clang-format off
      "ldi r24, %[syncValue] ; Put timerCounterValueSync into a register for use\n"

      "KeepReadingBitsLow:\n"
      "  brcs %l[DoneReadingBits] ; Branch to DoneReadingBits when Carry set\n"
      "  sbis %[PIN], %[N] ; Skip if pin is set\n"
      "  rjmp KeepReadingBitsLow\n"

      "PinNowHigh:\n"
      "  out %[TCNT],r24 ; Update timer\n"

      "KeepReadingBitsHigh:\n"
      "  brcs %l[DoneReadingBits] ; Branch to DoneReadingBits when Carry set\n"
      "  sbic %[PIN], %[N] ; Skip if pin is clear\n"
      "  rjmp KeepReadingBitsHigh\n"

      "PinNowLow:\n"
      "  out %[TCNT],r24 ; Update timer\n"
      "  rjmp KeepReadingBitsLow\n"
      :
      : [syncValue]"I" (timerCounterValueSync),
        [PIN] "I"(unsigned(Port) // Convert Port to integer
                  - __SFR_OFFSET // Offset because of AVR internal workings
                  - 2            // Distance from PINx to PORTx
                  ),
        [N] "I"(Pin), // The pin number
        [TCNT] "I"(_SFR_IO_ADDR(TCNT0)) // The timer counter register
      : "r24"
      : DoneReadingBits
        // clang-format on
    );

  } else {
    // The original implementation, unoptimized, but works.

    // We can't use normal loop structures because we need to use the `asm goto("brcc ...")` statement to test for the
    // end condition: 20-bits received. We use two separate loops to optimize the pin state checks in exchange for a
    // small amount of program space.

    // Main loop while line is low
  KeepReadingBitsLow:
    // Turn off Debug IO every loop checking pin state
    if (Debug::EmitPulseAtSample || Debug::EmitPulseAtSync) Debug::Pin::clr();
    // Wait for input to go high
    if (isHigh()) {
      TCNT0 = syncValue;
      // Debug:: IO pulse after seeing a state change
      if (Debug::EmitPulseAtSync) Debug::Pin::on();

      if (ResetWatchdog::ReceivedTransition) asm("wdr");

      // Jump to other pin loop
      goto KeepReadingBitsHigh;
    }
    // Toggle Debug IO state when idle
    if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
    // We use the carry bit that was set in the interrupt to determine if we've received 20 bits.
    asm goto("brcc %l[KeepReadingBitsLow]; branch to KeepReadingBitsLow" :: ::KeepReadingBitsLow);

    // If we get here, we've received 20 bits
    goto DoneReadingBits;

  KeepReadingBitsHigh:
    // Turn off Debug IO every loop checking pin state
    if (Debug::EmitPulseAtSample || Debug::EmitPulseAtSync) Debug::Pin::clr();
    // Wait for input to go low
    if (!isHigh()) {
      TCNT0 = syncValue;
      // Debug:: IO pulse after seeing a state change
      if (Debug::EmitPulseAtSync) Debug::Pin::on();

      if (ResetWatchdog::ReceivedTransition) asm("wdr");

      // Jump to other pin loop
      goto KeepReadingBitsLow;
    }
    // Toggle Debug IO state when idle
    if (Debug::EmitPulsesAtIdle) Debug::Pin::tgl();
    // We use the carry bit that was set in the interrupt to determine if we've received 20 bits.
    asm goto("brcc %l[KeepReadingBitsHigh]; branch to KeepReadingBitsHigh" :: ::KeepReadingBitsHigh);
  }

DoneReadingBits:
  // It's important to tell GCC that we've modified these registers elsewhere
  asm("; Done reading bits!" ::: Result0Reg, Result1Reg, Result2Reg);

  // Disable interrupt
  TIMSK0 = 0;

  if (ResetWatchdog::BeforeProcessing) asm("wdr");

  if (Debug::EmitRawResultToUART) {
    Debug::WriteByte<false>(result2);
    Debug::WriteByte<false>(result1);
    Debug::WriteByte(result0);
  }

  asm("; DONE WITH REGISTERS: " Result0Reg " " Result1Reg " " Result2Reg " r30 r31 and Carry");

  // Result is in registers, but we still need to unshift and decode the GCR encoding
  // Bit number:     1   5 6  9  12 14 17 21
  // On the wire:    xxxxx|xxxxxxxx|xxxxxxxx
  // Results:     result 2|result 1|result 0

  return MakeResponse::fromResult();
}

static AVR::DShot::Response parseResultUltra() {
  TIMSK0 = 0;
  asm("pop r30");
  asm("pop r30");
  return MakeResponse::fromResult();
}

#ifdef BUILD_WIP_STUFF
template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
AVR::DShot::Response AVR::DShot::BDShot<Port, Pin, Speed>::getResponseNoInterrupts() {
  using AVR::DShot::Response;
  using Basic::u1;
  using Basic::u3;

  u1 overflowsWhileWaiting = 0; // for first bit of response

  // Experimental interrupt-less resync. Not working yet

  static_assert(Periods::delayPeriodTicks > 41, "delayPeriodTicks is too small for this implementation");

  // We're overflowing with OCRA now which sets lower TOP than 0xff
  constexpr unsigned responseTimeoutOverflows = responseTimeoutTicks / Periods::delayPeriodTicks;

  static_assert(responseTimeoutOverflows < 0xff, "responseTimeoutOverflows is too large for this implementation");

  u1 temp[ExpectedBits];
  // Use a set bit in result to indicate we've gotten all the bits
  u3 result = 1 << ((sizeof(result) * 8) - ExpectedBits);

  static_assert(ExpectedBits <= sizeof(result) * 8, "ExpectedBits is too large for this implementation");

  // Assumptions of this implementation
  static_assert(Periods::delayPeriodTicks > 10, "delayPeriodTicks is too small");
  static_assert(Periods::delayPeriodTicks < 100, "delayPeriodTicks is too large");
  static_assert(Periods::delay3HalfPeriodTicks < 200, "delay3HalfPeriodTicks is too large");

  /**
   * We're receiving a NRZ(L) RLL "shift-encoded" bit stream; a start bit and 20 gcr.
   *
   * One way to receive this data would be to use a standard TTL level hardware serial port with the capability of
   * receiving 20 bits.
   *
   * gcrEncodedData = receivedBits ^ (receivedBits >> 1)
   *
   * This, however, has issues with likely clock drift.
   *
   * Instead, we try to recover clocking information on any transition.
   *
   * Pseudo code:
   * - Wait for initial high-to-low transition
   *   - Timeout if no response in 50us
   * - Start timer with overflow in 1.5 bit periods
   * - Wait for
   *   - Overflow
   *     - Sample bit
   *     - Loop if not 20 bits
   *   - Transition
   *     - Sync timer (set to 0.5 bit period, overflow in half a bit period)
   *     - Loop
   *
   * This is relatively straightforward to implement...
   */

  // Wait for initial high-to-low transition, or timeout while waiting
  while (true) {
    if (!isHigh()) break;

    if (!(TIFR0 & _BV(BitOverflowFlag))) continue;

    // Timeout waiting for response
    if (overflowsWhileWaiting++ > responseTimeoutOverflows) return Response::Error::ResponseTimeout;

    TIFR0 |= _BV(BitOverflowFlag);
  }

  // We're receiving BDShot data! Continue...
InitialLowEntry:
  constexpr unsigned adjustInitialTicks = 6;
  constexpr unsigned adjustSyncTicks = 6;
  TCNT0 = -u1(Periods::delayHalfPeriodTicks - adjustInitialTicks); // Start below 0. Overflow in 1.5 bit periods
  TIFR0 |= _BV(BitOverflowFlag);

  asm("; Got High-to-Low transition");

  constexpr bool SingleLoopVersion = false;
  if (SingleLoopVersion) {
    bool lastBit = false;
    while (true) {
      bool currentBit = isHigh();
      if (currentBit != lastBit) TCNT0 = Periods::delayHalfPeriodTicks - adjustSyncTicks;
      if (TIFR0 & _BV(BitOverflowFlag)) {
        TIFR0 |= _BV(BitOverflowFlag);
        Debug::Pin::tgl();
        setCarry(lastBit ^ currentBit);
        lastBit = currentBit;
        shiftInCarryLeft(result);
        // branch to end if carry set. Means we've gotten the bit out that we set at the start
        asm goto("brcs %l[ParseGCR]; branch to ParseGCR" :: ::ParseGCR);
      }
    }
  } else {

  FirstLow:
    asm("; FirstLow:");
    while (true) {
      if (TIFR0 & _BV(BitOverflowFlag)) {
        TIFR0 |= _BV(BitOverflowFlag);

        setCarry(1);
        shiftInCarryLeft(result);
        // branch to end if carry set. Means we've gotten the bit out that we set at the start
        asm goto("brcc %l[FirstLow]; Loop to FirstLow if not done" :: ::FirstLow);
        asm goto("rjmp %l[ParseGCR]" :: ::ParseGCR);
      }
      if (isHigh()) {
        TCNT0 = Periods::delayHalfPeriodTicks - adjustSyncTicks;
        Debug::Pin::tgl();
        goto FirstHigh;
      }
    }

  FirstHigh:
    asm("; FirstHigh:");
    while (true) {
      if (TIFR0 & _BV(BitOverflowFlag)) {
        TIFR0 |= _BV(BitOverflowFlag);

        setCarry(1);
        shiftInCarryLeft(result);
        // branch to end if carry set. Means we've gotten the bit out that we set at the start
        asm goto("brcc %l[FirstHigh]; Loop to FirstHigh if not done" :: ::FirstHigh);
        asm goto("rjmp %l[ParseGCR]" :: ::ParseGCR);
      }
      if (!isHigh()) {
        TCNT0 = Periods::delayHalfPeriodTicks - adjustSyncTicks;
        Debug::Pin::tgl();
        goto FirstLow;
      }
    }

  SubsequentLow:
    asm("; SubsequentLow:");
    while (true) {
      if (TIFR0 & _BV(BitOverflowFlag)) {
        TIFR0 |= _BV(BitOverflowFlag);

        setCarry(0);
        shiftInCarryLeft(result);
        // branch to end if carry set. Means we've gotten the bit out that we set at the start
        asm goto("brcc %l[SubsequentLow]; LSubsequentLow to SubsequentLow if SubsequentLow done" :: ::SubsequentLow);
        asm goto("rjmp %l[ParseGCR]" :: ::ParseGCR);
      }
      if (isHigh()) {
        TCNT0 = Periods::delayHalfPeriodTicks - adjustSyncTicks;
        Debug::Pin::tgl();
        goto FirstHigh;
      }
    }

  SubsequentHigh:
    asm("; SubsequentHigh:");
    while (true) {
      if (TIFR0 & _BV(BitOverflowFlag)) {
        TIFR0 |= _BV(BitOverflowFlag);

        setCarry(0);
        shiftInCarryLeft(result);
        // branch to end if carry set. Means we've gotten the bit out that we set at the start
        asm goto(
            "brcc %l[SubsequentHigh]; LSubsequentHigh to SubsequentHigh if SubsequentHigh done" :: ::SubsequentHigh);
        asm goto("rjmp %l[ParseGCR]" :: ::ParseGCR);
      }
      if (!isHigh()) {
        TCNT0 = Periods::delayHalfPeriodTicks - adjustSyncTicks;
        Debug::Pin::tgl();
        goto FirstLow;
      }
    }

  ParseGCR:
    asm("; ParseGCR");
  }
  asm("; Got all the bits!");

  u1 checksum = 0;

  for (u1 i = 0; i < nibbles; i++) {
    u1 nibble = 0;
    for (u1 j = 0; j < GCR::inBits; j++) {
      nibble <<= 1;
      nibble |= temp[i * GCR::inBits + j];
    }
    nibble = GCR::decode(nibble);
    if (nibble == 0xff) return Response::Error::BadDecodeFourthNibble;

    // We can reuse temp at this point
    temp[i] = nibble;
    checksum ^= nibble;
  }

  if (checksum != 0) return Response::Error::BadChecksum;

  return {u2((u2(temp[0] & 0b1) << 8) | temp[1] | temp[2]), u1(temp[0] >> 1)};
}
#endif

#ifdef BUILD_OLD_CRAP

static inline void delayNanos(unsigned) __attribute__((error("Not implemented")));
void delayNanos(unsigned) { asm("; delayNanos"); }

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
AVR::DShot::Response AVR::DShot::BDShot<Port, Pin, Speed>::getResponseOld() {
  using AVR::DShot::Response;
  using Basic::s1;
  using Basic::u1;
  using Basic::u2;

  u1 overflowsWhileWaiting = 0; // for first bit of response
  bool lastBit = false;

  // 256 overflow with TOVF
  constexpr unsigned responseTimeoutOverflows = responseTimeoutTicks >> 8;
  static_assert(responseTimeoutOverflows < 0xff, "responseTimeoutOverflows is too large for this implementation");

  u1 checksum = 0;

  if (Periods::delayPeriodTicks <= 50) {
    /**
     * For very short periods, we need to capture all the bits quickly and then process them at the end.
     */

    asm("; delayPeriodTicks <= 50");

    u1 temp[ExpectedBits];

    for (u1 i = 0; i < ExpectedBits; i++)
      temp[i] = 0;

    // Wait for high-to-low transition, or timeout while waiting
    while (isHigh()) {
      if (!(TIFR0 & _BV(TOV0))) continue;

      if (overflowsWhileWaiting > responseTimeoutOverflows) {
        // Timeout waiting for response
        return Response::Error::ResponseTimeout;
      }

      overflowsWhileWaiting++;

      TIFR0 |= _BV(TOV0);
    }

    // TCNT0 = -u1(Periods::delayHalfPeriodTicks);

    // TCNT0 = -10;

    constexpr bool trustGCC = false;
    if (trustGCC) {
// Give GCC a hand with this one...
#pragma GCC unroll 10000
      for (u1 i = 0; i < ExpectedBits; i++) {

        // TIFR0 |= _BV(BitOverflowFlag);

        // Delay 1 bit time
        // while (!(TIFR0 & (1 << TOV0))) {
        // }
        // asm("; Delay 1 bit time");
        // TCNT0 = u1(-u1(Periods::delayPeriodTicks));
        // TIFR0 = 1 << TOV0;
        asm("; Delay 1 bit time");
        while ((s1(TCNT0) - (s1(u1(const_round(Periods::bitPeriodNanos * (1.5 + i) * F_CPU / 1e9))))) < 0) {}
        // while (!(TIFR0 & _BV(BitOverflowFlag))) {} // 2
        // TCNT0 = 0; // 1
        // OCR0A += Periods::delayPeriodTicks; // 3

        Debug::Pin::tgl();
        // if (isHigh()) {
        temp[i] = isHigh(); // 3
        // }
        Debug::Pin::tgl();
      }
    } else {
      TCNT0 = 20;
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 0) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[0] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 1) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[1] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 2) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[2] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 3) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[3] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 4) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[4] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 5) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[5] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 6) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[6] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 7) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[7] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 8) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[8] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 9) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[9] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 10) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[10] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 11) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[11] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 12) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[12] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 13) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[13] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 14) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[14] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 15) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[15] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 16) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[16] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 17) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[17] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 18) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[18] = isHigh();
      while (s1(TCNT0 - (const_round(Periods::bitPeriodNanos * (1.5 + 19) * F_CPU / 1e9))) < 0) {}
      Debug::Pin::tgl();
      temp[19] = isHigh();
    }

    asm("; Done reading in bits");

    bool lastBit = false;
    for (u1 i = 0; i < nibbles; i++) {
      for (u1 j = 0; j < GCR::inBits; j++) {
        Debug::Pin::on();
        Debug::Pin::clr();
        if (temp[i * GCR::inBits + j] == 1) {
          // Step 1 of GCR decoding
          if (!lastBit) {
            // We can safely start overwriting temp with GCR nibbles
            temp[i] |= 1 << j;
            lastBit = true;
          }
        } else {
          if (lastBit) {
            temp[i] |= 1 << j;
            lastBit = false;
          }
        }
      }

      asm("; Decoding GCR");

      // Decode the nibbles in place
      const auto decoded = GCR::decode(temp[i]);

      asm("; Done decoding GCR");

      // If we get an invalid GCR code, bail
      if (decoded == 0xff) {

        // DebugWriteByte(i);
        // DebugWriteByte(temp[i]);

        return Response::Error::BadDecodeFourthNibble;
      }

      asm("; Done checking nibble");

      checksum ^= decoded;
    }

    asm("; Done parsing");

    if (checksum != 0b1111) return Response::Error::BadChecksum;

    asm("; Done Checksum");

    // swap nibbles in temp[1]. Slightly more efficient than shifting and masking
    temp[1] = (temp[1] & 0b00001111) << 4 | (temp[1] & 0b11110000) >> 4;

    return {u2((u2(temp[0] & 0b1) << 8) | temp[1] | temp[2]), u1(temp[0] >> 1)};

  } else if (Periods::delayPeriodTicks <= 100) {
    /**
     * For slightly longer periods, we can demodulate the raw bit stream and just save gcr encoded nibbles for
     * processing later
     */

    asm("; delayPeriodTicks <= 100");

    // Wait for first high-to-low transition
    while (isHigh()) {
      if (!(TIFR0 & _BV(TOV0))) continue;

      if (overflowsWhileWaiting > responseTimeoutOverflows) {
        // Timeout waiting for response
        return Response::Error::ResponseTimeout;
      }

      overflowsWhileWaiting++;
      TIFR0 |= _BV(TOV0);
    }

    TCNT0 = 0;

    TCNT0 = -(u1(Periods::delayPeriodTicks * 2));
    TIFR0 = 1 << TOV0;

    u1 temp[nibbles];

    // Get all 4 nibbles of GCR encoded data
    for (u1 i = 0; i < nibbles; i++) {
      u1 j = GCR::inBits;
      while (j--) {
        // Delay 1 bit time
        while (!(TIFR0 & (1 << TOV0))) {}
        asm("; Delay 1 bit time");
        TCNT0 = -(u1(Periods::delayPeriodTicks));
        TIFR0 = 1 << TOV0;

        // Sample pin
        if (isHigh()) {
          // Step 1 of GCR decoding
          if (!lastBit) {
            temp[i] |= 1 << j;
            lastBit = true;
          }
        } else {
          if (lastBit) {
            temp[i] |= 1 << j;
            lastBit = false;
          }
        }
      }
    }

    asm("; Done parsing in bits");

    // Decode each GCR encoded nibble
    for (u1 i = 0; i < nibbles; i++) {
      temp[i] = GCR::decode(temp[i]);

      // If we get an invalid GCR code, bail
      if (temp[i] == 0xff) return Response::Error::BadDecodeFourthNibble;
    }

    asm("; Computing Checksum");

    // Compute Checksum. Maybe this should be done in the loop above?
    for (u1 i = 0; i < nibbles; i++) {
      // Compute checksum
      checksum ^= temp[i];
    }

    // Test Checksum
    if (checksum != 0b1111) return Response::Error::BadChecksum;

    asm("; Result!");

    // Return the result
    return {u2((u2(temp[0] & 0b1) << 8) | u1(temp[1] << 4) | temp[2]), u1(temp[0] >> 1)};
  } else {
    /**
     * For longest periods, we can capture the raw bit stream and then process it all at once
     */
    asm("; delayPeriodTicks > 100");

    // Do it all in one loop

    u1 overflowsWhileWaiting = 0; // for first bit of response

    // 256 overflow with TOVF
    constexpr unsigned responseTimeoutOverflows = responseTimeoutTicks >> 8;
    static_assert(responseTimeoutOverflows < 0xff, "responseTimeoutOverflows is too large for this implementation");

    u1 temp;
    u1 exponent;
    u2 base;

    // Wait for high-to-low transition
    while (isHigh()) {
      if (!(TIFR0 & _BV(TOV0))) continue;

      if (overflowsWhileWaiting > responseTimeoutOverflows) {
        // Timeout waiting for response
        return Response::Error::ResponseTimeout;
      }

      overflowsWhileWaiting++;
      TIFR0 |= _BV(TOV0);
    }

    // Delay 0.5 bit time
    delayNanos(Periods::bitPeriodNanos / 2);

    // For each nibble
    for (u1 i = 0; i < nibbles; i++) {
      // Demodulate the bit stream
      for (u1 j = GCR::inBits; j; j--) {
        temp <<= 1;

        // Delay 1 bit time
        delayNanos(Periods::bitPeriodNanos);

        // Sample pin
        const bool bit = isHigh();

        // Step 1 of GCR decoding
        temp |= bit ^ lastBit;

        lastBit = bit;
      }

      // Decode the nibble
      const auto nibble = GCR::decode(temp & 0b11111);

      // If we get an invalid GCR code, bail
      if (nibble == 0xff) return Response::Error::BadDecodeFourthNibble;

      // Compute checksum
      checksum ^= nibble;

      // Build up result as we're running
      if (i == 0) {
        exponent = nibble >> 1;
        base = u2(nibble & 0b1) << 8;
      } else if (i == 1) {
        base |= nibble << 4;
      } else if (i == 2) {
        base |= nibble;
      } else {
        if (checksum != 0b1111) {
          // Check checksum on last nibble
          return Response::Error::BadChecksum;
        }
        // Otherwise we're all set!
        return {base, exponent};
      }
    }
  }
}

#endif // BUILD_OLD_CRAP

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
void AVR::DShot::BDShot<Port, Pin, Speed>::ReadBitISR() {

  // Too bad this compiles to a lot of instructions
  // setCarry(Parent::isHigh());

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
   *  - `reti` Return from interrupt (unless we're using the "ultra" loop)
   */

  // Carry is always zero at this point
  asm("; Skip next instruction if input is low\n\t"
      "sbic %[PIN], %[N]\n\t"

      "sec ; Set Carry"
      :
      : [PIN] "I"(unsigned(Port) // Convert Port to integer
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

  if (BDShotConfig::useUltraLoop) {
    if (false) {
      asm("brcs %x[branch]; Branch to parseResultUltra if Carry set" : : [branch] "p"(&parseResultUltra));
    } else {
      // Reti if Carry is clear to continue receiving bits
      asm goto("brcc %l[DoneSamplingPin]; Branch to reti if Carry cleared" : : : : DoneSamplingPin);

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

      // Pop the interrupt return location off the stack (to get out of the ultra loop)
      // We can also trash the previously set Z register value since we don't need it.
      asm("pop r30");
      asm("pop r30");

      // Not needed.
      // Saves a word of flash and a clock cycle, but this is at the end when speed doesn't matter as much.
      // Relative jumps are faster but can't reach whole program space.
      constexpr bool useRelativeJmp = true;

      // Jump to the function [MakeResponse::fromResult()] that makes the result the getResponse() caller wants.
      // When it returns, since we've mucked with the call stack, it'll return in place of getResponse().
      if (useRelativeJmp) {
        asm("rjmp %x[Done]; Jump to MakeResponse::fromResult" : : [Done] "p"(&MakeResponse::fromResult));
      } else {
        asm("jmp  %x[Done]; Jump to MakeResponse::fromResult" : : [Done] "p"(&MakeResponse::fromResult));
      }
    }
  }

DoneSamplingPin:
  asm("reti");
}

static constexpr bool HandleInterrupts = 0;

template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed>
AVR::DShot::Response AVR::DShot::BDShot<Port, Pin, Speed>::sendCommand(Command<true> c) {
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
    // A warning for the future. This block needs to be implemented for automatic interrupt handling to work.

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

#ifdef BUILD_OLD_CRAP
  return getResponseOld();
#else
#ifdef BUILD_WIP_STUFF
  return getResponseNoInterrupts();
#else

  const auto r = getResponse();

  if (HandleInterrupts) asm("sei");

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

#endif
#endif
}
