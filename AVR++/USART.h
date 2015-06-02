/* 
 * File:   USART.h
 * Author: cameron
 *
 * Created on March 17, 2011, 11:00 AM
 */

#ifndef USART_H
#define	USART_H

#include <avr/io.h>
#include <stddef.h>
#include "basicTypes.h"

#undef AVR

#define UCSRA  _MMIO_BYTE(A + 0)
#define UCSRB  _MMIO_BYTE(A + 1)
#define UCSRC  _MMIO_BYTE(A + 2)
#define UBRR   _MMIO_WORD(A + 4)
#define UBRRL  _MMIO_BYTE(A + 4)
#define UBRRH  _MMIO_BYTE(A + 5)
#define UDR    _MMIO_BYTE(A + 6)

namespace AVR {

template <size_t A>
class USART {
public:
 inline static void setBRR(u2 const BBR) {UBRR = BBR;}
 inline static u2   getBRR(     ) {return UBRR;}

 inline static void set2X() {UCSRA |=  0b10;}
 inline static void clr2X() {UCSRA &= ~0b10;}

 inline static void setDataRegister(u1 const byte) {UDR = byte;}
 inline static u1   getDataRegister(      ) {return UDR;}

 inline static bool dataRegisterEmpty() {return UCSRA & 0b00100000;}
 inline static bool isTxComplete     () {return UCSRA & 0b01000000;}
 inline static bool isRxComplete     () {return UCSRA & 0b10000000;}
 
 inline static void clearTxCompleteFlag() {UCSRA = UCSRA;}

 inline static void disableReInt() {UCSRB &= ~0b00100000;}
 inline static void  enableReInt() {UCSRB |=  0b00100000;}

 inline static void disableTxInt() {UCSRB &= ~0b01000000;}
 inline static void  enableTxInt() {UCSRB |=  0b01000000;}

 inline static void disableRxInt() {UCSRB &= ~0b10000000;}
 inline static void  enableRxInt() {UCSRB |=  0b10000000;}

 inline static void enableTx () {UCSRB |=  0b00001000;}
 inline static void disableTx() {UCSRB &= ~0b00001000;}
 
 inline static void enableRx () {UCSRB |=  0b00010000;}
 inline static void disableRx() {UCSRB &= ~0b00010000;}
 
 static void send(const u1 byte);
 static u1 get();
 
 static void skip(u1 num);
 
 static void getBlock(u1 * array, u2 len);
 
 USART& operator<<(const char  byte) {send(byte);   return *this;}
 
 USART& operator<<(USART& (*callback)(USART&)) {return callback(*this);}
 
 bool operator== (const char byte) {return get() == byte;}
 bool operator!= (const char byte) {return get() != byte;}
 
 USART& operator>>(u1 &byte) {byte = get(); return *this;}
 USART& operator>>(u2 &word);

 void getLittleEndian(u2   &word);
 
 USART& operator<<(const u1 byte);
 USART& operator<<(const u2 byte);

 private:

};


#undef UCSRA
#undef UCSRB
#undef UCSRC
#undef UBRR
#undef UBRRL
#undef UBRRH
#undef UDR

#ifdef UCSRA
 extern USART<(size_t)&UCSRA> usart;
#elif defined(UCSR0A)
 extern USART<(size_t)&UCSR0A> usart;
#elif defined(UCSR1A)
// extern USART<(size_t)&UCSR1A> usart;
#endif
 
#if defined(UCSR1A) && (defined(UCSRA) || defined(UCSR0A))
  extern USART<(size_t)&UCSR1A> usart1;
#endif
 
#ifdef UCSR2A
 extern USART<(size_t)&UCSR2A> usart2;
#endif
 
#ifdef UCSR3A
 extern USART<(size_t)&UCSR3A> usart3;
#endif

};

#endif	/* USART_H */

