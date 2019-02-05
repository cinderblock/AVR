/* 
 * File:   USART.cpp
 * Author: cameron
 * 
 * Created on March 17, 2011, 11:00 AM
 */

#include "USART.hpp"

using namespace AVR;


#ifdef UCSRA
 USART<(size_t)&UCSRA> AVR::usart;
#elif defined(UCSR0A)
 USART<(size_t)&UCSR0A> AVR::usart;
#elif defined(UCSR1A)
 template class USART<(size_t)&UCSR1A>;
 USART<(size_t)&UCSR1A> AVR::usart;
#endif
 
#if defined(UCSR1A) && (defined(UCSRA) || defined(UCSR0A))
  USART<(size_t)&UCSR1A> AVR::usart1;
#endif
 
#ifdef UCSR2A
 USART<(size_t)&UCSR2A> AVR::usart2;
#endif
 
#ifdef UCSR3A
 USART<(size_t)&UCSR3A> AVR::usart3;
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