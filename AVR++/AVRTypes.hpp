#pragma once

/*
 * File:   AVRTypes.h
 * Author: Cameron
 *
 * Created on September 17, 2014, 3:47 PM
 */

#undef AVR

#include "basicTypes.hpp"

namespace AVR {
using namespace Basic;
/**
 * Shortcut for a pointer to a hardware mapped (volatile) byte
 */
typedef volatile u1 *Byte;
/**
 * Shortcut for a pointer to a hardware mapped (volatile) word
 */
typedef volatile u2 *Word;
}; // namespace AVR
