#pragma once

#include "basicTypes.hpp"
#include "undefAVR.hpp"

namespace AVR {
using namespace Basic;

typedef union {
  struct {
    u1 A;
    u1 B;
  };
  u1 BYTES[2];
  u2 WORD;
} f2;

typedef union {
  struct {
    u1 A;
    u1 B;
    u1 C;
  };
  u1 BYTES[3];
  u3 WORD;
} f3;

typedef union {
  struct {
    u1 A;
    u1 B;
    u1 C;
    u1 D;
  };
  u1 BYTES[4];
  u4 LONG;
} f4;

}; // namespace AVR
