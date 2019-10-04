/*
 * File:   ADC.h
 * Author: Cameron
 *
 * Created on March 12, 2015, 1:44 PM
 */

#ifndef ADC_H
#define ADC_H

#include <avr/io.h>

#include "bitTypes.hpp"

// Clear out conflicting Atmel defines
#undef ADC

// Clear out conflicting gcc defines
#undef AVR

namespace AVR {
namespace ADC {
using namespace Basic;

enum class Reference : b2 { AREF, AVcc, Reserved, Internal };

typedef union {
  struct {
    b5 MultiplexerLowBits : 5;
    bool LeftAdjust : 1;
    Reference ref : 2;
  };
  u1 byte;
} MUXt;

enum class Prescaler : b3 { D_, D2, D4, D8, D16, D32, D64, D128 };

u1 constexpr divider(Prescaler const p) {
  return p == Prescaler::D_
             ? 2
             : p == Prescaler::D2
                   ? 2
                   : p == Prescaler::D4
                         ? 4
                         : p == Prescaler::D8
                               ? 8
                               : p == Prescaler::D16
                                     ? 16
                                     : p == Prescaler::D32 ? 32
                                                           : p == Prescaler::D64 ? 64 : p == Prescaler::D128 ? 128 : 0;
}

constexpr unsigned long operator/(unsigned long f, Prescaler const p) { return f / divider(p); }
// constexpr int value(Prescaler const p) { return (u1)p; }

#ifdef F_CPU
constexpr unsigned long adcFreq(Prescaler const p) { return F_CPU / p; }

constexpr bool isValid(Prescaler const p) { return adcFreq(p) > 50000 && adcFreq(p) < 200000; }

constexpr Prescaler suggestedPrescaler =
    isValid(Prescaler::D2)
        ? Prescaler::D2
        : isValid(Prescaler::D4)
              ? Prescaler::D4
              : isValid(Prescaler::D8)
                    ? Prescaler::D8
                    : isValid(Prescaler::D16)
                          ? Prescaler::D16
                          : isValid(Prescaler::D32) ? Prescaler::D32
                                                    : isValid(Prescaler::D64) ? Prescaler::D64 : Prescaler::D128;

#endif

inline void startConversion() { ADCSRA |= 1 << ADSC; }

typedef union SRA {
  struct {
    Prescaler Prescale : 3;
    bool InterruptEnable : 1;
    bool InterruptFlag : 1;
    bool AutoTriggerEnable : 1;
    bool StartConversion : 1;
    bool Enable : 1;
  };
  u1 byte;

#ifdef F_CPU
  SRA(bool interruptEnable, bool autoTrigger = false, Prescaler p = suggestedPrescaler, bool clearFlag = true,
      bool start = false)
      : Prescale(p), InterruptEnable(interruptEnable), InterruptFlag(clearFlag), AutoTriggerEnable(autoTrigger),
        StartConversion(start), Enable(true) {}
#endif

  SRA(bool interruptEnable, bool autoTrigger, Prescaler p, bool clearFlag = true, bool start = false)
      : Prescale(p), InterruptEnable(interruptEnable), InterruptFlag(clearFlag), AutoTriggerEnable(autoTrigger),
        StartConversion(start), Enable(true) {}

  SRA(bool interruptEnable, Prescaler p, bool clearFlag = true, bool start = false)
      : Prescale(p), InterruptEnable(interruptEnable), InterruptFlag(clearFlag), AutoTriggerEnable(false),
        StartConversion(start), Enable(true) {}

  SRA(Prescaler p, bool interruptEnable, bool clearFlag = true, bool start = false)
      : Prescale(p), InterruptEnable(interruptEnable), InterruptFlag(clearFlag), AutoTriggerEnable(false),
        StartConversion(start), Enable(true) {}

} SRAt;

static_assert(sizeof(SRAt) == 1, "Invalid struct");

enum class AutoTriggerSource : b4 {
  FreeRunning,
  AnalogComparator,
  ExternalInterrupt0,
  Timer0CompareMatch,
  Timer0Overflow,
  Timer1CompareMatchB,
  Timer1Overflow,
  Timer1Capture,
  Timer4Overflow,
  Timer4CompareMatchA,
  Timer4CompareMatchB,
  Timer4CompareMatchD
};

typedef union {
  struct {
    AutoTriggerSource TriggerSource : 4;
    bool : 1;
    bool MultiplexerBit5 : 1;
    bool AnalogComparatorMultiplexerEnable : 1;
    bool HighSpeedMode : 1;
  };
  u1 byte;
} SRBt;

constexpr volatile MUXt *const MUX = (volatile MUXt *const) & ADMUX;

constexpr volatile SRAt *const ControlStatusRegisterA = (volatile SRAt *const) & ADCSRA;

constexpr volatile u1 *const DataRegisterLow = &ADCL;
constexpr volatile u1 *const DataRegisterHigh = &ADCH;
constexpr volatile u2 *const DataRegister = &ADCW;

constexpr volatile SRBt *const ControlStatusRegisterB = (volatile SRBt *const) & ADCSRB;

constexpr volatile u1 *const DigitalInputDisableRegister0 = &DIDR0;
constexpr volatile u1 *const DigitalInputDisableRegister1 = &DIDR1;
constexpr volatile u2 *const DigitalInputDisableRegister = (volatile u2 *const) & DIDR0;

class RegularInput {
  const u1 mux;
  const bool mux5;

public:
  constexpr inline RegularInput(u1 muxValue, Reference ref = Reference::AVcc, bool leftAdjust = false)
      : mux((u1)ref << 6 | (u1)leftAdjust << 5 | (muxValue & ((1 << 5) - 1))), mux5(muxValue & (1 << 5)) {}

  inline void select() {
    MUX->byte = mux;
    ControlStatusRegisterB->MultiplexerBit5 = mux5;
  }
};
}; // namespace ADC
}; // namespace AVR

#endif /* ADC_H */
