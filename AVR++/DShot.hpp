#pragma once

/**
 * @brief DShot (and BDShot) protocol implementation for AVRs
 * @file DShot.hpp
 * @author Cameron Tacklind <cameron@tacklind.com>
 *
 * This is a bit-bang implementation of a DShot master for AVRs.
 * It supports as any motors as you wish but can only access them in a round-robin fashion and interrupts must be
 * disabled.
 *
 * The DShot and BDShot protocols are described well here: https://brushlesswhoop.com/dshot-and-bidirectional-dshot
 *
 * For performance reasons, most of the configuration is locked at compile time with template parameters.
 *
 * While this implementation is a "classes", it is not intended to be instantiated.
 * `using` is the preferred method of access.
 *
 * Simplified API:
 *
 * template <Ports Port, int Pin, AVR::DShot::Speeds Speed = [150/300], bool Inverted = false>
 * class AVR::DShot::DShot {
 *   static void init();
 *   static void sendCommand(Command);
 * }
 *
 * Simplest Usage DShot:
 *
 * ```main.cpp
 * #include <AVR++/DShot.cpp> // Yes, a cpp file
 *
 * using ESC = AVR::DShot::DShot<Ports::C, 7>;
 *
 * // Wish this worked...
 * // template class ESC;
 * // But no, it doesn't. So we have to do this instead:
 * template class AVR::DShot::DShot<Ports::C, 7>;
 *
 * int main() {
 *   ESC::init();
 *   ESC::sendCommand(0.5);
 * }
 * ```
 *
 * @see An explanation of using template classes in cpp files:
 * https://stackoverflow.com/questions/115703/storing-c-template-function-definitions-in-a-cpp-file/41292751#41292751
 *
 * Other APIs:
 *
 * The raw command to send to the ESC.
 * Has a large number of helpful constructors.
 * class AVR::DShot::Command {
 *   // Input values are in the range [0, Command::Max] Command::Max = 1999
 *   constexpr inline Command(u2             command, bool telemetry = false);
 *   constexpr inline Command(int            command, bool telemetry = false);
 *   // Input values are in the range [0, 1]
 *   constexpr inline Command(float          command, bool telemetry = false);
 *   constexpr inline Command(double         command, bool telemetry = false);
 *   // DShot supports 48 special commands
 *   constexpr inline Command(SpecialCommand command, bool telemetry = false);
 * }
 * See also: `AVR::DShot::SpecialCommand`
 *
 */

#include "Const.hpp"
#include "PulsedOutput.hpp"

namespace AVR {
namespace DShot {
using namespace AVR;
using namespace Basic;
enum class Speeds {
  DSHOT150,
  DSHOT300,
  DSHOT600,
  DSHOT1200,
};

constexpr Speeds NominalSpeed = F_CPU == 16000000UL ? Speeds::DSHOT300 : Speeds::DSHOT150;

constexpr unsigned pulseNanos0(Speeds speed) {
  switch (speed) {
  case Speeds::DSHOT150:
    return 2500;
  case Speeds::DSHOT300:
    return 1250;
  case Speeds::DSHOT600:
    return 625;
  case Speeds::DSHOT1200:
    return 313;
  }
  return 0;
}

enum class SpecialCommand : u1 {
  // Commands 0-36 are only executed when motors are stopped.

  Stop = 0,

  /**
   * @note Wait at least length of beep (260ms) before next command
   */
  Beep1 = 1,
  /**
   * @note Wait at least length of beep (260ms) before next command
   */
  Beep2 = 2,
  /**
   * @note Wait at least length of beep (260ms) before next command
   */
  Beep3 = 3,
  /**
   * @note Wait at least length of beep (260ms) before next command
   */
  Beep4 = 4,
  /**
   * @note Wait at least length of beep (260ms) before next command
   */
  Beep5 = 5,

  /**
   * @note Wait at least 12ms before next command
   *
   * Sends info packet over separate telemetry serial port bus wire
   */
  Info = 6,

  // Read more: https://brushlesswhoop.com/dshot-and-bidirectional-dshot/#special-commands
};

// cSpell:ignore tparam dshot

/**
 * Represents the complete packet of data that will be sent to the motor.
 * @tparam Inverted If the CRC calculation is inverted from normal (for Bidirectional DShot)
 */
template <bool Inverted>
class Command {
  static constexpr unsigned Bits = 11;
  static constexpr unsigned BitsLow = Bits - 8;
  static constexpr unsigned RawTop = 1 << Bits;
  static constexpr unsigned RawMax = RawTop - 1;
  static constexpr unsigned SpecialCommands = 48;
  static constexpr unsigned Zero = SpecialCommands;

public:
  static constexpr unsigned Max = RawMax - Zero;

private:
  static constexpr inline u2 scaleToCommandMax(double value) { return Const::round(Const::clamp(value) * Max); }
  static constexpr inline u2 scaleToCommandMax(float value) { return Const::round(Const::clamp(value) * Max); }

  static constexpr inline u2 commandToRaw(u2 command) { return Const::min<u2>(command, Max) + Zero; }
  static constexpr inline u2 commandToRaw(int command) { return u2(Const::clamp<int>(command, Max)) + Zero; }
  static constexpr inline u2 commandToRaw(float command) { return commandToRaw(scaleToCommandMax(command)); }
  static constexpr inline u2 commandToRaw(double command) { return commandToRaw(scaleToCommandMax(command)); }
  static constexpr inline u2 commandToRaw(SpecialCommand command) { return static_cast<u2>(command); }

public:
  // Packing order of members needs to match wire protocol
  union {
    u1 bytes[2];
    struct {
      // First byte on the wire
      u1 commandHigh;
      // Second byte on the wire, least significant bits first
      u1 crc : 4;
      bool telemetry : 1;
      u1 commandLow : BitsLow;
    };
  };

  static constexpr inline u1 computeCRC(u2 command, bool telemetry) {
    command <<= 1;
    command |= telemetry;
    return ((Inverted ? 0xff : 0) ^ (command ^ (command >> 4) ^ (command >> 8))) & 0x0F;
  }

  // constexpr Command() : bytes{0, 0} {}

  template <typename T>
  constexpr inline Command(T command, bool telemetry = false)
      : commandHigh(commandToRaw(command) >> BitsLow), crc(computeCRC(commandToRaw(command), telemetry)),
        telemetry(telemetry), commandLow(commandToRaw(command)) {}

  constexpr inline u2 getCommandRaw() const { return (u2(commandHigh) << BitsLow) | commandLow; }
  constexpr inline bool isSpecialCommand() const { return getCommandRaw() < Zero; }

  constexpr inline u1 const *data() const { return bytes; }
  constexpr inline operator u1 const *() const { return data(); }

private:
  /**
   * Some simple tests that should always be true if this program was written correctly.
   */
  static inline void selfTest() {
    constexpr u2 testValue = 1046;
    constexpr u2 push = testValue - Zero;
    constexpr bool telemetry = false;
    constexpr Command c = Command{push, telemetry};
    constexpr auto test2 = c.getCommandRaw();
    constexpr auto test3 = c.telemetry;
    constexpr auto test4 = c.commandHigh;
    constexpr auto test5 = c.commandLow;
    constexpr auto test6 = c.crc;

    static_assert(c.getCommandRaw() == testValue, "Command should handle shifting that DShot expects");
    static_assert(c.telemetry == telemetry, "Telemetry should work");
    static_assert(c.crc == 0b0110, "Command should handle shifting that DShot expects");

    static_assert(sizeof(Command) == 2, "Command is 2 bytes. Something is messed up if not.");
  }
};

/**
 * Some more simple tests that can't be in the class that should always be true if this program was written correctly.
 */
namespace SelfTest {
constexpr Command<true> c{998};
constexpr auto test2 = c.getCommandRaw();

static_assert(c.bytes == &c.commandHigh, "CommandHigh should be first byte");
} // namespace SelfTest

template <Ports Port, int Pin, Speeds Speed = NominalSpeed, bool Inverted = false>
class DShot
    : protected PulsedOutput<Port, Pin, pulseNanos0(Speed), Inverted, true, false, false, pulseNanos0(Speed) * 2 / 3> {
  using Parent = PulsedOutput<Port, Pin, pulseNanos0(Speed), Inverted, true, false, false, pulseNanos0(Speed) * 2 / 3>;

protected:
  using Parent::input;
  using Parent::isHigh;
  using Parent::off;
  using Parent::output;
  using Parent::send;
  using IO = IOpin<Port, Pin>;

public:
  // Exposed for development/debugging
  using Parent::PulseMath;

public:
  using Parent::init;

  inline static void sendCommand(Command<Inverted> c, bool handleInterrupts = true) {
    if (handleInterrupts) asm volatile("cli");
    send(c.bytes, sizeof(c.bytes));
    if (handleInterrupts) asm volatile("sei");
  }
};

} // namespace DShot
} // namespace AVR