#pragma once

#include "basicTypes.hpp"

namespace AVR {

typedef union {
 struct {
  u1 A;
  u1 B;
 };
 u2 WORD;
} f2;

typedef union {
 struct {
  u1 A;
  u1 B;
  u1 C;
  u1 D;
 };
 u4 LONG;
} f4;

};

