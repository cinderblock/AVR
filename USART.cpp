/* 
 * File:   USART.cpp
 * Author: cameron
 * 
 * Created on March 17, 2011, 11:00 AM
 */

#include "USART.h"

using namespace AVR;

USART usart;
 
void USART::send(const u1 byte) {
 while (!dataRegisterEmpty());
 setDataRegister(byte);
}

u1 USART::get() {
 while (!isRxComplete());
 return getDataRegister();
}

void USART::skip(u1 num) {
 while (num--) get();
}

void USART::getBlock(u1 * array, u2 len) {
 while (len--) *array++ = get();
}
 
USART& USART::operator>>(u2 &word) {
 word = get() << 8 | get();
 return *this;
}
 
void USART::getLittleEndian(u2 &word) {
 word = get() | get() << 8;
}
 
USART& USART::operator<<(const u1 byte) {
 send(byte);
 return *this;
}
USART& USART::operator<<(const u2 word) {
 send(word >> 8);
 send(word);
 
 return *this;
}