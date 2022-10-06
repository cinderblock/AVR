#pragma once

#include "ADC.hpp"
#include "avr/interrupt.h"

ISR(ADC_vect, ISR_NOBLOCK);

namespace AVR {
using namespace Basic;

template <u1 N>
class ScanningADC {

  friend void ::ADC_vect();

public:
  /**
   * A function that will be called when the most recent data is available in ADC register.
   *
   * Executed in interrupt context, but interrupts are on.
   */
  typedef void (*Handler)();

  /**
   * The ADC MUX value and a function to call when the conversion is complete
   */
  typedef struct {
    ADC::RegularInput mux;
    Handler handle;
  } Input;

private:
  static Input inputs[N];

  /**
   * Current index in the table.
   */
  static u1 current;

  /**
   * Call this from ISR(ADC_vect). Something like:
   * ```C++
   * #include <AVR++/ScanningADC.cpp>
   * template <u1 N> typename ScanningADC<N>::Input ScanningADC<N>::inputs[] = {...};
   * template class ScanningADC<numberOfScannedAnalogInputs>;
   * ISR(ADC_vect) { ScanningADC<numberOfScannedAnalogInputs>::interrupt(); }
   * ```
   */
  static void interrupt();

  static bool selectNext();

public:
  static void init() __attribute__((constructor));
};

} // namespace AVR
