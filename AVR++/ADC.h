/* 
 * File:   ADC.h
 * Author: Cameron
 *
 * Created on March 12, 2015, 1:44 PM
 */

#ifndef ADC_H
#define	ADC_H

#include <avr/io.h>

#include "bitTypes.h"

// Clear out conflicting Atmel defines
#undef ADC

// Clear out conflicting gcc defines
#undef AVR

namespace AVR {
 namespace ADC {
  
  enum class Reference : b2 {
   AREF, AVcc, Reserved, Internal
  };
  
  typedef union {
   struct {
    b5 mux:5;
    bool LeftAdjust: 1;
    Reference ref: 2;
   };
   u1 byte;
  } MUXt;
  
  enum class Prescaler : b3 {
   D_, D2, D4, D8, D16, D32, D64, D128
  };
  
  typedef union {
   struct {
    Prescaler Prescale :3;
    bool InterruptEnable :1;
    bool InterruptFlag :1;
    bool AutoTriggerEnable :1;
    bool StartConversion :1;
    bool Enable :1;
   };
   u1 byte;
  } SRAt;
  
  enum class AutoTriggerSource : b4 {
   FreeRunning, AnalogComparator, ExternalInterrupt0,
   Timer0CompareMatch, Timer0Overflow,
   Timer1CompareMatchB, Timer1Overflow, Timer1Capture,
   Timer4Overflow, Timer4CompareMatchA, Timer4CompareMatchB, Timer4CompareMatchD
  };
  
  typedef union {
   struct {
    AutoTriggerSource TriggerSource :4;
    bool :1;
    bool MUX5 :1;
    bool AnalogComparatorMultiplexerEnable :1;
    bool HighSpeedMode :1;
   };
   u1 byte;
  } SRBt;

  constexpr volatile MUXt * const MUX  = (volatile MUXt * const)&ADMUX;
  
  constexpr volatile SRAt * const ControlStatusRegisterA  = (volatile SRAt * const)&ADCSRA;
  
  constexpr volatile u1  * const DataRegisterLow  = &ADCL;
  constexpr volatile u1  * const DataRegisterHigh = &ADCH;
  constexpr volatile u2  * const DataRegister =     &ADCW;
  
  constexpr volatile SRBt * const ControlStatusRegisterB  = (volatile SRBt * const)&ADCSRB;
  
  constexpr volatile u1  * const DigitalInputDisableRegister0 = &DIDR0;
  constexpr volatile u1  * const DigitalInputDisableRegister1 = &DIDR1;
  constexpr volatile u2  * const DigitalInputDisableRegister  = (volatile u2 * const)&DIDR0;
  
  class RegularInput {
   u1 mux;
  public:
   inline RegularInput(u1 muxValue, Reference ref = Reference::AVcc, bool leftAdjust = false)
   : mux((u1)ref << 6 | (u1)leftAdjust << 5 | muxValue)
   {}
   
   inline void selectWithMux() {MUX->byte = mux;}
  };
  
  void init();
 };
};

#endif	/* ADC_H */

