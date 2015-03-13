/* 
 * File:   ADC.cpp
 * Author: Cameron
 * 
 * Created on March 12, 2015, 1:44 PM
 */

#include "ADC.h"

void AVR::ADC::init() {
 // Ensure default value
 ControlStatusRegisterB->byte = 0;
 
 // Enable the ADC
 ControlStatusRegisterA->byte = 0b10111111;
}