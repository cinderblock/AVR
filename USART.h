/* 
 * File:   USART.h
 * Author: cameron
 *
 * Created on March 17, 2011, 11:00 AM
 */

#ifndef USART_H
#define	USART_H

#define __AVR_ATmega328P__

#include <avr/io.h>
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

template <volatile uint8_t * const A>
class USART  {
public:
 inline static void setBRR(u2 const BBR) {UBRR = BBR;}
 inline static u2   getBRR(     ) {return UBRR;}

 inline static void set2X() {UCSRA |=  _BV(U2X0);}
 inline static void clr2X() {UCSRA &= ~_BV(U2X0);}

 inline static void setDataRegister(u1 const byte) {UDR = byte;}
 inline static u1   getDataRegister(      ) {return UDR;}

 inline static bool dataRegisterEmpty() {return UCSRA & _BV(UDRE0);}
 inline static bool isTxComplete     () {return UCSRA & _BV(TXC0);}
 inline static bool isRxComplete     () {return UCSRA & _BV(RXC0);}

 inline static void disableTxInt() {UCSRB &= ~_BV(UDRIE0);}
 inline static void  enableTxInt() {UCSRB |=  _BV(UDRIE0);}

 inline static void disableRxInt() {UCSRB &= ~_BV(RXCIE0);}
 inline static void  enableRxInt() {UCSRB |=  _BV(RXCIE0);}

 inline static void enableTx () {UCSRB |=  _BV(TXEN0);}
 inline static void disableTx() {UCSRB &= ~_BV(TXEN0);}
 
 inline static void enableRx () {UCSRB |=  _BV(RXEN0);}
 inline static void disableRx() {UCSRB &= ~_BV(RXEN0);}
 
 inline static void enableRxPullUp() {PORTD |= _BV(0);}
 
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

extern USART<&UCSR0A> usart;

};

#endif	/* USART_H */

