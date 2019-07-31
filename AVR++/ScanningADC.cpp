

#include "ScanningADC.hpp"

using namespace AVR;
using namespace Basic;

// Start at -1 so that first run starts at 0
template <u1 N> u1 ScanningADC<N>::current = -1;

template <u1 N> void ScanningADC<N>::init() {
  // Enable "High Speed Mode". Doesn't actually seem to do anything.
  ADCSRB = (1 << ADHSM);

  // Enable ADC with interrupt and suggested prescaler
  // *ADC::ControlStatusRegisterA = {true};
  ADCSRA = 0b10011000 | (u1)ADC::suggestedPrescaler;

  if (selectNext()) {
    inputs[current].mux.select();

    ADC::startConversion();
  }
}

template <u1 N> bool ScanningADC<N>::selectNext() {
  auto const start = current;
  do {
    current++;
    if (current >= N)
      current = 0;

    if (start == current)
      return false;
  } while (!inputs[current].handle);

  return true;
}

template <u1 N> void ScanningADC<N>::interrupt() {
  inputs[current].handle();

  if (!selectNext())
    return;

  inputs[current].mux.select();

  ADC::startConversion();
}
