/* 
 * File:   USART.cpp
 * Author: cameron
 * 
 * Created on March 17, 2011, 11:00 AM
 */

#include "USART.h"

using namespace AVR;

USART<&UCSR1A> usart;
 
template <size_t A>
void USART<A>::send(const u1 byte) {
 while (!dataRegisterEmpty());
 setDataRegister(byte);
}

template <size_t A>
u1 USART<A>::get() {
 while (!isRxComplete());
 return getDataRegister();
}

template <size_t A>
void USART<A>::skip(u1 num) {
 while (num--) get();
}

template <size_t A>
void USART<A>::getBlock(u1 * array, u2 len) {
 while (len--) *array++ = get();
}

template <size_t A>
USART& USART<A>::operator>>(u2 &word) {
 word = get() << 8 | get();
 return *this;
}

template <size_t A>
void USART<A>::getLittleEndian(u2 &word) {
 word = get() | get() << 8;
}

template <size_t A>
USART& USART<A>::operator<<(const u1 byte) {
 send(byte);
 return *this;
}
template <size_t A>
USART& USART<A>::operator<<(const u2 word) {
 send(word >> 8);
 send(word);
 
 return *this;
}