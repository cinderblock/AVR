#pragma once

#include "PulsedOutput.hpp"
#include <avr/interrupt.h>

namespace AVR {

namespace DShot {
using namespace Basic;
enum class Speeds {
  DSHOT150,
  DSHOT300,
  DSHOT600,
  DSHOT1200,
  // DSHOT2400,
};

constexpr float BitrateToPeriod(float bitrate) { return 1.0f / bitrate; }

constexpr float period = BitrateToPeriod(1200e3 * 5 / 4) * 1e9;

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

constexpr unsigned samplePeriodNanos(Speeds speed) {
  switch (speed) {
  case Speeds::DSHOT150:
    return 1e9f / (150e3 * 5 / 4);
  case Speeds::DSHOT300:
    return 1e9f / (300e3 * 5 / 4);
  case Speeds::DSHOT600:
    return 1e9f / (600e3 * 5 / 4);
  case Speeds::DSHOT1200:
    return 1e9f / (1200e3 * 5 / 4);
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

struct Command {
  u2 command : 11;
  bool telemetry : 1;
  u1 crc : 4;

  constexpr inline static u1 computeCRC(u2 command, bool telemetry) {
    command <<= 1;
    command |= telemetry;
    return (~(command ^ (command >> 4) ^ (command >> 8))) & 0x0F;
  }

  static constexpr unsigned minCommand = 48;

  Command(u2 command, bool telemetry = false)
      : command(max<u2>(command, minCommand)), telemetry(telemetry), crc(computeCRC(this->command, telemetry)) {}

  Command(SpecialCommand command, bool telemetry = false)
      : command(static_cast<u2>(command)), telemetry(telemetry), crc(computeCRC(this->command, telemetry)) {}
};

struct Response {
  u2 rpmPeriodBase : 9;
  u1 : 3;
  u1 rpmPeriodExponent : 3;

  inline bool isError() const { return rpmPeriodBase == 0 && rpmPeriodExponent == 0; }
  inline Response() = default;
  inline constexpr Response(u2 rpmPeriodBase, u1 rpmPeriodExponent)
      : rpmPeriodBase(rpmPeriodBase), rpmPeriodExponent(rpmPeriodExponent) {}

  /**
   * @brief Convert internal notation to float rpm
   * @warning Check there is no error before calling this function
   *
   * @details
   *                1 Electrical Revolution           1e6 us   60 seconds
   * eRPM = --------------------------------------- * ------ * ----------
   *        (rpmPeriodBase << rpmPeriodExponent) us   second     minute
   *
   * @return float eRPM of motor
   */
  inline float rpm() const { return 60e6 / (1 << rpmPeriodExponent) / rpmPeriodBase; }
};

// constexpr Response r{1, 0};
// constexpr float test = r.rpm();

template <Ports port, u1 pin, Speeds speed = Speeds::DSHOT1200, bool bidirectional = false>
class DShot : public PulsedOutput<port, pin, pulseNanos0(speed), bidirectional> {
  using WeakOutput<port, pin, bidirectional, false, false>::input;
  using IOpin<port, pin>::isHigh;
  using IOpin<port, pin>::output;

  using PulsedOutput<port, pin, pulseNanos0(speed), bidirectional>::send;

public:
  inline static void sendCommand(Command c) {
    output();
    sei();
    send((u1 const *)&c, sizeof(c));
    cli();
  }

  static Response getResponse();
};
} // namespace DShot
} // namespace AVR
