/* 
 * File:   DecPrintFormatter.cpp
 * Author: Cameron Tacklind
 * 
 * Created on December 29, 2011, 3:54 PM
 */

#include <stdlib.h>
#include <avr/boot.h>
#include "DecPrintFormatter.h"
#include "bigTypes.h"

#include "USART.h"

using namespace AVR;

DecPrintFormatter::DecPrintFormatter(void (*const dest)(u1 const byte)) : destination(dest) {
 
}

void DecPrintFormatter::print(s1 byte) {
 if (byte < 0) {
  destination('-');
  print((u1)-byte);
 } else {
  print((u1)byte);
 }
}

void DecPrintFormatter::print(s2 word) {
 if (word < 0) {
  destination('-');
  print((u2)-word);
 } else {
  print((u2)word);
 }
}

void DecPrintFormatter::print(s4 dword) {
 if (dword < 0) {
  destination('-');
  print((u4)-dword);
 } else {
  print((u4)dword);
 }
}

void DecPrintFormatter::print(u2 word) {
 
 ldiv_t divresult = ldiv(word, 10);
 
 if (divresult.quot) print((u2)divresult.quot);
 
 destination(dec(divresult.rem));
}

void DecPrintFormatter::print(u4 dword) {
 
 ldiv_t divresult = ldiv(dword, 10);
 
 if (divresult.quot){
//  if ((u4)divresult.quot <= (u4)0xffffUL) print((u2)divresult.quot); else
  
  print((u4)divresult.quot);
 }
 
 destination(dec(divresult.rem));
}

void DecPrintFormatter::print(char const * str) {
 char c;
 while (1) {
  c = pgm_read_byte(str++);
  if (!c) return;
  print(c);
 }
}

void DecPrintFormatter::print(u1 const * str) {
 char c;
 while (1) {
  c = *str++;
  if (!c) return;
  print(c);
 }
}