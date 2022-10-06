#pragma once

#include "basicTypes.hpp"
#include "undefAVR.hpp"
#include <stddef.h>

namespace AVR {
using namespace Basic;
u2 FlashCRC16(size_t startAddress, size_t const endAddress);
} // namespace AVR