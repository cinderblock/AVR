
#include "FlashCRC.hpp"
#include <avr/pgmspace.h>

#include <util/crc16.h>

using namespace Basic;

u2 AVR::FlashCRC16(size_t startAddress, size_t const endAddress) {
  u2 crc = 0xffff;

  while (startAddress < endAddress) {
    crc = _crc16_update(crc, pgm_read_byte(startAddress));
    startAddress++;
  }

  return crc;
}
