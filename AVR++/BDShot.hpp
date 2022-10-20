#pragma once

/**
 * @brief BDShot protocol implementation for AVRs
 * @file BDShot.hpp
 * @author Cameron Tacklind <cameron@tacklind.com>
 *
 * This is a bit-bang implementation of a BDShot master for AVRs.
 * It supports as any motors as you wish but can only access them in a round-robin fashion and interrupts must be
 * disabled. This implementation requires _full control_ of interrupts.
 *
 * The DShot and BDShot protocols are described well here: https://brushlesswhoop.com/dshot-and-bidirectional-dshot
 *
 * For performance reasons, most of the configuration is locked at compile time with template parameters.
 *
 * While this implementation is a "classes", it is not intended to be instantiated.
 * `using` is the preferred method of access.
 *
 * Watch out for the ESC detecting a long "high" state (BDShot idles high) as a command to enter the bootloader.
 *
 * Simplified API:
 *
 * template <AVR::Ports Port, int Pin, AVR::DShot::Speeds Speed = [150/300]>
 * class AVR::DShot::BDShot {
 *   static void init();
 *   static Response sendCommand(Command);
 * }
 *
 * Simplest Usage BDShot:
 *
 * ```main.cpp
 * #include <AVR++/BDShot.cpp> // Yes, a cpp file
 *
 * using ESC = AVR::DShot::BDShot<Ports::C, 7>;
 *
 * // Wish this worked...
 * // template class ESC;
 * // But no, it doesn't. So we have to do this instead:
 * template class AVR::DShot::BDShot<Ports::C, 7>;
 *
 * int main() {
 *   ESC::init();
 *
 *   // Implementation requires interrupts to be enabled, but all other interrupts must be disabled individually
 *   sei();
 *
 *   while (true) {
 *     // Ensure regular communication with ESC to prevent launching bootloader
 *     _delay_ms(1);
 *
 *     // Disable other interrupts individually
 *     // Disable USART interrupts, for instance
 *     UCSR0B &= 0b0001'1111;
 *
 *     // 250us maximum execution time for DSHOT150
 *     auto res = ESC::sendCommand(0.5); // See DShot.hpp for other Command constructors
 *
 *     // Reenable other interrupts individually
 *     UCSR0B |= (1 << RXCIE0) | (0 << TXCIE0) | (1 << UDRIE0);
 *
 *     // Check for transmission errors first
 *     if (res.isError()) {
 *       auto err = res.getError();
 *       // Do something with the error
 *       continue;
 *     }
 *
 *     // Required if EDT is enabled
 *     if (res.isExtendedTelemetry()) {
 *       auto et = res.getTelemetryType();
 *       auto val = res.getTelemetryValue();
 *       // Do something with the extended telemetry
 *       continue;
 *     }
 *
 *     // Optional
 *     if (res.isStopped()) {
 *       // Do something with the stopped state
 *       continue;
 *     }
 *
 *     // Get useful numbers
 *     u2 period = res.getPeriodMicros();
 *     float rpm = res.getRPM();
 *
 *     // Do something with the motor telemetry
 *   }
 * }
 * ```
 *
 * @see An explanation of using template classes in cpp files:
 * https://stackoverflow.com/questions/115703/storing-c-template-function-definitions-in-a-cpp-file/41292751#41292751
 *
 * Other APIs:
 *
 * @see `AVR::DShot::Command` in DShot.hpp
 *
 * The response from the ESC in BDShot.
 * Has a few helpful parsing methods.
 * class AVR::DShot::Response {
 *   inline constexpr bool      isError()             const;
 *   inline constexpr Error     getError()            const;
 *   inline constexpr bool      isExtendedTelemetry() const;
 *   inline constexpr Telemetry getTelemetryType()    const;
 *   inline constexpr u1        getTelemetryValue()   const;
 *   inline constexpr u2        getPeriodMicros()     const;
 *   inline constexpr float     getRPM()              const;
 *   inline constexpr bool      isStopped()           const;
 *   inline constexpr operator  u2()                  const { return getPeriodMicros(); }
 * }
 *
 */

#include "DShot.hpp"

// cSpell:ignore GPIO USART RXCIE TXCIE UDRIE

namespace AVR {
namespace DShot {
using namespace AVR;
using namespace Basic;

namespace BDShotConfig {
constexpr double exitBootloaderDelay = 400 /* or 1300 */; // ms
constexpr unsigned responseTimeout = 50;                  // us

/**
 * @brief Should we support the new Extended DShot Telemetry protocol?
 * @see https://brushlesswhoop.com/dshot-and-bidirectional-dshot/#extended-dshot-telemetry-edt
 */
constexpr bool supportEDT = true;

constexpr bool useDebounce = false;

namespace AssemblyOptimizations {
// Not needed.
// Saves a word of flash and a clock cycle, but this is at the end when speed doesn't matter as much.
// Relative jumps are faster but can't reach whole program space.
// If you see errors about "relocation truncated to fit", try setting this to "false".
constexpr bool useRelativeJmpAtEndISR = true;

// Shouldn't be needed since we use call-clobbered registers.
constexpr bool saveResultRegisters = false;
constexpr bool saveZRegister = false;
} // namespace AssemblyOptimizations

// All of these are way overkill. The minimum watchdog timeout is 15ms and the maximum time here is 250us.
namespace ResetWatchdog {
constexpr bool AfterSend = false;                           // After ~107us, for DSHOT150
constexpr bool WaitingFirstTransitionFast = false;          // Ever <1us while waiting for first transition
constexpr bool WaitingFirstTransitionTimerOverflow = false; // Every 2.7us
constexpr bool ReceivedFirstTransition = false;             // Maximum responseTimeout after sending command
constexpr bool ReceivedTransition = false;                  // Every time we receive a transition, min period 2.7us
constexpr bool SampledBit = false;                          // Every time we sample a bit, every 2.7us for DSHOT150
constexpr bool BeforeProcessing = false;                    // After sampling 20 bits
} // namespace ResetWatchdog

// Debugging/development features
namespace Debug {
// Use PB6 as an extra GPIO to verify timing for various parts of the code
using Pin = AVR::Output<AVR::Ports::B, 8>;

constexpr bool EmitPulseAtISR = false;
constexpr bool EmitPulsesAtIdle = false;
constexpr bool EmitPulseAtSync = false;
} // namespace Debug
} // namespace BDShotConfig

class Response {
public:
  constexpr static unsigned BaseBits = 9;
  constexpr static unsigned ExponentBits = 3;
  constexpr static u1 ExponentMax = 1 << ExponentBits;
  constexpr static u1 ExponentTop = ExponentMax - 1;
  constexpr static u1 ExponentMask = ExponentMax - 1;

private:
  u1 lsb;
  u1 msb;

  constexpr static u1 ErrorMask = 1 << 7;

public:
  enum class Error : u1 {
    None,
    ResponseTimeout,
    BadDecodeFirstNibble,
    BadDecodeSecondNibble,
    BadDecodeThirdNibble,
    BadDecodeFourthNibble,
    BadChecksum,
  };

  /**
   * Error by default
   */
  inline constexpr Response(Error e) : lsb(static_cast<u1>(e)), msb(ErrorMask) {}
  /**
   * @param rpmPeriodBase
   * @param rpmPeriodExponent
   */
  inline constexpr Response(u1 lsb, u1 msb) : lsb(lsb), msb(msb) {}

  /**
   * @return true if there was an error
   */
  inline bool constexpr isError() const { return msb & ErrorMask; }

  inline constexpr operator bool() const { return !isError(); }

  inline Error constexpr getError() const {
    if (!isError()) return Error::None;
    return static_cast<Error>(lsb);
  }

  /**
   * @brief Check if this packet is a telemetry packet.
   *
   * Do not use if this is an error Response.
   *
   * @return true
   * @return false
   */
  inline bool constexpr isExtendedTelemetry() const { return BDShotConfig::supportEDT && !(msb & 1); }

  enum class Telemetry : u1 {
    None = 0x00,
    Temperature = 0x02,
    Voltage = 0x04,
    Current = 0x06,
    Debug1 = 0x08,
    Debug2 = 0x0A,
    Debug3 = 0x0C,
    State_Event = 0x0E,
  };
  inline Telemetry constexpr getTelemetryType() const { return static_cast<Telemetry>(msb); }

  inline u1 constexpr getTelemetryValue() const { return BDShotConfig::supportEDT ? lsb : -1; }

  inline constexpr u2 getBase() const { return (u2(BDShotConfig::supportEDT | (msb & 1)) << 8) | lsb; }
  inline constexpr u2 getExponent() const {
    // Don't need to mask because it's not an error
    return msb >> 1;
  }

  inline u2 constexpr getPeriodMicros() const { return getBase() << getExponent(); }

  inline constexpr operator u2() const { return getPeriodMicros(); }

  /**
   * @brief Convert internal notation to float rpm
   * @warning Check there is no error before calling this function
   *
   * @details
   *                 1 Electrical Revolution           1e6 us   60 seconds
   * speed = --------------------------------------- * ------ * ----------
   *         (rpmPeriodBase << rpmPeriodExponent) us   second     minute
   *
   *                       60 * 1e6               Electrical Revolutions
   *       = ------------------------------------ ---------per----------
   *         (rpmPeriodBase << rpmPeriodExponent)         minute
   *
   * @return float eRPM of motor
   */
  inline float constexpr getRPM(u1 polePairs = 1) const { return 60e6 / (u3(polePairs) * getPeriodMicros()); }

  inline bool constexpr isStopped() const { return msb == 0x0f && lsb == 0xff; }
};

template <Ports Port, int Pin, Speeds Speed = NominalSpeed>
class BDShot : protected DShot<Port, Pin, Speed, true> {
  static void ReadBitISR() __attribute__((naked));

protected:
  struct Periods {
    inline static constexpr double samplePeriodNanos(Speeds speed) {
      switch (speed) {
      case Speeds::DSHOT150:
        return 4e9 / 150e3 / 5;
      case Speeds::DSHOT300:
        return 4e9 / 300e3 / 5;
      case Speeds::DSHOT600:
        return 4e9 / 600e3 / 5;
      case Speeds::DSHOT1200:
        return 4e9 / 1200e3 / 5;
      }
      return 0;
    }

    static constexpr double bitPeriodNanos = samplePeriodNanos(Speed);
    static constexpr unsigned delayPeriodTicks = Const::round(bitPeriodNanos * F_CPU / 1e9);
    static constexpr unsigned delayHalfPeriodTicks = Const::round(bitPeriodNanos * F_CPU / 1e9 / 2);
    static constexpr unsigned delay3HalfPeriodTicks = Const::round(bitPeriodNanos * 3 * F_CPU / 1e9 / 2);
  };

  using Parent = DShot<Port, Pin, Speed, true>;
  using Parent::isHigh;

  /**
   * @brief Wait for a response from the ESC and return it
   *
   * Requires all other interrupts to be disabled individually and global interrupts to be enabled
   *
   * @return Response
   */
  static Response getResponse();

public:
  // Exposed for development/debugging
  using Parent::PulseMath;

  /**
   * Other interrupts *must* be disabled but Global interrupts must also be enabled when calling this function.
   *
   * Caller must disable all used interrupts by their own enable bits and restore them as desired.
   *
   * @param c
   * @return Response
   */
  static Response sendCommand(Command<true> c);

  static void init();
  static void exitBootloader();
};

} // namespace DShot
} // namespace AVR