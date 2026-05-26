#pragma once

// Byte-counting aliases for the standard fixed-width integer types: u1 is
// 1 byte, u2 is 2 bytes, etc. — chosen because <stdint.h>'s bit-counting
// names (uint8_t = 8 bits) obscure the actual storage cost on AVR, where
// every byte matters. The same set is also defined in libCameron's
// src/basicTypes.hpp; the two files must stay in sync on the shared types
// so they can be included together (C++ permits identical typedef
// redeclaration in the same namespace). AVR++ additionally defines u3/s3
// (__uint24/__int24, AVR-gcc extensions) which libCameron lacks.

#include <stdint.h>

namespace Basic {

typedef uint8_t u1;
typedef uint16_t u2;
typedef __uint24 u3;
typedef uint32_t u4;
typedef uint64_t u8;

typedef int8_t s1;
typedef int16_t s2;
typedef __int24 s3;
typedef int32_t s4;
typedef int64_t s8;

}; // namespace Basic
