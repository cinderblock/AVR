#include "basicTypes.h"
#include <stddef.h>

#undef AVR

namespace AVR {
using namespace Basic;
u2 FlashCRC16(size_t startAddress, size_t const endAddress);
} // namespace AVR