/* 
 * File:   USART.cpp
 * Author: cameron
 * 
 * Created on March 17, 2011, 11:00 AM
 */

#include "USART.h"

using namespace AVR;


#ifdef UCSRA
 USART<(size_t)&UCSRA> usart;
#elif defined(UCSR0A)
 USART<(size_t)&UCSR0A> usart;
#elif defined(UCSR1A)
 USART<(size_t)&UCSR1A> usart;
#endif
 
#if defined(UCSR1A) && (defined(UCSRA) || defined(UCSR0A))
  USART<(size_t)&UCSR1A> usart1;
#endif
 
#ifdef UCSR2A
 USART<(size_t)&UCSR2A> usart2;
#endif
 
#ifdef UCSR3A
 USART<(size_t)&UCSR3A> usart3;
#endif
 
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
USART<A>& USART<A>::operator>>(u2 &word) {
 word = get() << 8 | get();
 return *this;
}

template <size_t A>
void USART<A>::getLittleEndian(u2 &word) {
 word = get() | get() << 8;
}

template <size_t A>
USART<A>& USART<A>::operator<<(const u1 byte) {
 send(byte);
 return *this;
}
template <size_t A>
USART<A>& USART<A>::operator<<(const u2 word) {
 send(word >> 8);
 send(word);
 
 return *this;
}