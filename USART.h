/* 
 * File:   USART.h
 * Author: cameron
 *
 * Created on March 17, 2011, 11:00 AM
 */

#ifndef USART_H
#define	USART_H

#include <avr/io.h>
#include "basicTypes.h"

#undef AVR

namespace AVR {

class USART {
public:
 inline static void setBRR(u2 const BBR) {UBRR0 = BBR;}
 inline static u2   getBRR(      ) {return UBRR0;}

 inline static void set2X() {UCSR0A |=  _BV(U2X0);}
 inline static void clr2X() {UCSR0A &= ~_BV(U2X0);}

 inline static void setDataRegister(u1 const byte) {UDR0 = byte;}
 inline static u1   getDataRegister(       ) {return UDR0;}

 inline static bool dataRegisterEmpty() {return UCSR0A & _BV(UDRE0);}
 inline static bool isTxComplete     () {return UCSR0A & _BV(TXC0);}
 inline static bool isRxComplete     () {return UCSR0A & _BV(RXC0);}

 inline static void disableTxInt() {UCSR0B &= ~_BV(UDRIE0);}
 inline static void  enableTxInt() {UCSR0B |=  _BV(UDRIE0);}

 inline static void disableRxInt() {UCSR0B &= ~_BV(RXCIE0);}
 inline static void  enableRxInt() {UCSR0B |=  _BV(RXCIE0);}

 inline static void enableTx () {UCSR0B |=  _BV(TXEN0);}
 inline static void disableTx() {UCSR0B &= ~_BV(TXEN0);}
 
 inline static void enableRx () {UCSR0B |=  _BV(RXEN0);}
 inline static void disableRx() {UCSR0B &= ~_BV(RXEN0);}
 
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

extern USART usart;

};

#endif	/* USART_H */

