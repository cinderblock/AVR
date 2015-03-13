/* 
 * File:   DecPrintFormatter.h
 * Author: Cameron Tacklind
 *
 * Created on December 29, 2011, 3:54 PM
 */

#ifndef DECPRINTFORMATTER_H
#define	DECPRINTFORMATTER_H

#include "basictypes.h"

#include <avr/pgmspace.h>

#undef AVR

namespace AVR {
 
class DecPrintFormatter {
private:
 void (* const destination)(u1 const byte);
public:
 DecPrintFormatter(void (* const dest)(u1 const byte));
 
 void print(u1 const byte) {print((u2)byte);}
 void print(u2 const word);
 void print(u4 const dword);
 
 void print(s1 const byte);
 void print(s2 const word);
 void print(s4 const dword);
 
 void print(char const * str);
 void print(u1 const * ptr);
 
 inline void print(const bool bl) {print(bl ? '1' : '0');}
 
 inline void print(         char c) {destination(c);}
// inline void print(unsigned char c) {destination(c);};
 
 inline DecPrintFormatter& operator<< (const bool bl) {print(bl); return *this;}

 inline DecPrintFormatter& operator<< (const char  c) {print(c); return *this;}
 
 inline DecPrintFormatter& operator<< (const u1 byte) {print(byte); return *this;}
 inline DecPrintFormatter& operator<< (const u2 word) {print(word); return *this;}
 inline DecPrintFormatter& operator<< (const u4 word) {print(word); return *this;}
 
 inline DecPrintFormatter& operator<< (const s1 byte) {print(byte); return *this;}
 inline DecPrintFormatter& operator<< (const s2 word) {print(word); return *this;}
 inline DecPrintFormatter& operator<< (const s4 word) {print(word); return *this;}
 
 inline DecPrintFormatter& operator<< (char const * str) {print(str); return *this;}
 
 inline DecPrintFormatter& operator<< (u1 const * ptr) {print(ptr); return *this;}

 inline DecPrintFormatter& operator<< (DecPrintFormatter&(*const func)(DecPrintFormatter&)) {return func(*this);}
 inline DecPrintFormatter& operator<< (void              (*const func)(DecPrintFormatter&)) {func(*this); return *this;}

private:
 inline static const u1 dec(u1 const c) {
  if (c < 10) return c + '0';
  return ' ';
 }
};

#endif	/* DECPRINTFORMATTER_H */

};
