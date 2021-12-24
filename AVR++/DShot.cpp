#include "DSHOT.hpp"
#include "TimerTimeout.hpp"

using namespace AVR;
using namespace DShot;

inline static constexpr u1 decodeGCR(u1 const gcr) {
  // clang-format off
  switch (gcr) {
    case 0b11001: return 0b0000;
    case 0b11011: return 0b0001;
 // case 0b10010: return 0b0010;
 // case 0b10011: return 0b0011;
    case 0b11101: return 0b0100;
 // case 0b10101: return 0b0101;
 // case 0b10110: return 0b0110;
 // case 0b10111: return 0b0111;
    case 0b11010: return 0b1000;
 // case 0b01001: return 0b1001;
 // case 0b01010: return 0b1010;
 // case 0b01011: return 0b1011;
    case 0b11110: return 0b1100;
 // case 0b01101: return 0b1101;
 // case 0b01110: return 0b1110;
 // case 0b01111: return 0b1111;
    // clang-format on
  }
  return gcr & 0b1111;
}

template <Ports port, u1 pin, Speeds speed, bool bidirectional>
Response DShot<port, pin, speed, bidirectional>::getResponse() {
  if (!bidirectional)
    return 0;

  input();

  // 4 nibbles come in as 5 bits
  constexpr unsigned nibbles = 4;
  constexpr unsigned gcrBits = 5;

  union {
    u2 result;
    Response response;
  };

  u1 crc = 0;

  while (isHigh()) {
  }

  constexpr auto delayPeriodNanos = samplePeriodNanos(speed);

  // Delay 0.5 bit time
  delayNanos(delayPeriodNanos / 2);

  bool lastBit = false;

  for (unsigned i = 0; i < nibbles; i++) {
    unsigned j = gcrBits;
    u1 temp = 0;
    while (j--) {
      temp <<= 1;

      // Delay 1 bit time
      delayNanos(delayPeriodNanos);

      // Sample pin
      const bool bit = isHigh();

      // Step 1 of GCR decoding
      temp |= bit ^ lastBit;

      lastBit = bit;
    }

    // Step 2 of GCR decoding
    const auto nibble = decodeGCR(temp);

    // compute crc every step instead of at the end
    crc ^= nibble;

    // Handle each nibble as we get them instead of at the end
    switch (i) {
    case 0:
      // Move bits around into an easier to use order that matches the Response struct
      result = __builtin_avr_insert_bits(0xf321fff0ul, nibble, 0) << 8;
      break;
    case 1:
      // swap nibbles in nibble
      nibble = __builtin_avr_swap(nibble);
    //   asm volatile("swap %0" : "+r"(nibble));
    case 2:
      result |= nibble;
      break;
    case 3:
      // Don't shift. Return the nibles and let user shift later (after inversion)
      // result <<= shift;
      break;
    }
  }

  if (crc) {
    // Since we're getting 1/RPM, we can safely use 0 as an error value
    return 0;
  }

  return response;
}