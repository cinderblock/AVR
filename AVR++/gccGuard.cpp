/* 
 * File:   gccGuard.cpp
 * Author: Cameron
 * 
 * Created on January 4, 2016, 4:56 PM
 */

#include "gccGuard.hpp"

int __cxa_guard_acquire(__guard *g) {
 return !*(char *)(g);
}

void __cxa_guard_release (__guard *g) {
 *(char *)g = 1;
}

void __cxa_guard_abort (__guard *) {}

void __cxa_pure_virtual(void) {}
