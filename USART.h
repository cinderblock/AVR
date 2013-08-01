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
 static void setBRR(u2 BBR) {UBRR1 = BBR;}
 static u2   getBRR(      ) {return UBRR1;}

 static void set2X() {UCSR1A |=  _BV(U2X1);}
 static void clr2X() {UCSR1A &= ~_BV(U2X1);}

 static void setDataRegister(u1 byte) {UDR1 = byte;}
 static u1   getDataRegister(       ) {return UDR1;}

 static bool dataRegisterEmpty() {return UCSR1A & _BV(UDRE1);}
 static bool isTxComplete     () {return UCSR1A & _BV(TXC1);}
 static bool isRxComplete     () {return UCSR1A & _BV(RXC1);}

 static void disableTxInt() {UCSR1B &= ~_BV(UDRIE1);}
 static void  enableTxInt() {UCSR1B |=  _BV(UDRIE1);}

 static void disableRxInt() {UCSR1B &= ~_BV(RXCIE1);}
 static void  enableRxInt() {UCSR1B |=  _BV(RXCIE1);}

 static void enableTx () {UCSR1B |=  _BV(TXEN1);}
 static void disableTx() {UCSR1B &= ~_BV(TXEN1);}
 
 static void enableRx () {UCSR1B |=  _BV(RXEN1);}
 static void disableRx() {UCSR1B &= ~_BV(RXEN1);}
 
 static void enableRxPullUp() {PORTD |= _BV(2);}
 
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

