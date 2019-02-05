/* 
 * File:   I2C.h
 * Author: Cameron
 *
 * Created on January 12, 2015, 12:15 AM
 */

#ifndef I2C_H
#define	I2C_H

#include <avr/io.h>
#include "bitTypes.hpp"

namespace AVR {
 namespace I2C {
  
  typedef union {
   struct {
    /** TWIE (bit0)
     * 
     * When this bit is written to one, and the I-bit in SREG is set, the TWI
     * interrupt request will be activated for as long as the TWINT Flag is high.
     */
    b1 InterruptEnable:1;
    /**
     * Reserved
     */
    u1:1;
    /** TWEN (bit2)
     * 
     * The TWEN bit enables TWI operation and activates the TWI interface. When
     * TWEN is written to one, the TWI takes control over the I/O pins connected
     * to the SCL and SDA pins, enabling the slew-rate limiters and spike
     * filters. If this bit is written to zero, the TWI is switched off and all
     * TWI transmissions are terminated, regardless of any ongoing operation.
     */
    b1 Enable:1;
    /** TWWC (bit3)
     * 
     * The TWWC bit is set when attempting to write to the TWI Data Register –
     * TWDR when TWINT is low. This flag is cleared by writing the TWDR Register
     * when TWINT is high.
     */
    b1 WriteCollisionFlag:1;
    /** TWSTO (bit4)
     * 
     * Writing the TWSTO bit to one in Master mode will generate a STOP
     * condition on the 2-wire Serial Bus. When the STOP condition is executed
     * on the bus, the TWSTO bit is cleared automatically. In Slave mode,
     * setting the TWSTO bit can be used to recover from an error condition.
     * This will not generate a STOP condition, but the TWI returns to a
     * well-defined unaddressed Slave mode and releases the SCL and SDA lines to
     * a high impedance state.
     */
    b1 StopCondition:1;
    /** TWSTA (bit5)
     * 
     * The application writes the TWSTA bit to one when it desires to become a
     * Master on the 2-wire Serial Bus. The TWI hardware checks if the bus is
     * available, and generates a START condition on the bus if it is free.
     * However, if the bus is not free, the TWI waits until a STOP condition is
     * detected, and then generates a new START condition to claim the bus
     * Master status. TWSTA must be cleared by software when the START condition
     * has been transmitted.
     */
    b1 StartCondition:1;
    /** TWEA (bit6)
     * 
     * The TWEA bit controls the generation of the acknowledge pulse. If the
     * TWEA bit is written to one, the ACK pulse is generated on the TWI bus if
     * the following conditions are met:
     *  1. The device’s own slave address has been received.
     *  2. A general call has been received, while the TWGCE bit in the TWAR is set.
     *  3. A data byte has been received in Master Receiver or Slave Receiver mode.
     * By writing the TWEA bit to zero, the device can be virtually disconnected
     * from the 2-wire Serial Bus temporarily. Address recognition can then be
     * resumed by writing the TWEA bit to one again.
     */
    b1 EnableAcknowledge:1;
    /** TWINT (bit7)
     * 
     * This bit is set by hardware when the TWI has finished its current job and
     * expects application software response. If the I-bit in SREG and TWIE in
     * TWCR are set, the MCU will jump to the TWI Interrupt Vector. While the
     * TWINT Flag is set, the SCL low period is stretched. The TWINT Flag must
     * be cleared by software by writing a logic one to it. Note that this flag
     * is not automatically cleared by hardware when executing the interrupt
     * routine. Also note that clearing this flag starts the operation of the
     * TWI, so all accesses to the TWI Address Register (TWAR), TWI Status
     * Register (TWSR), and TWI Data Register (TWDR) must be complete before
     * clearing this flag.
     */
    b1 InterruptFlag:1;
   };
   u1 byte;
  } CRt;

  typedef union {
   struct {
    b2 Prescaler:2;
    u1:1;
    b5 Status:5;
   };
   u1 byte;
  } SRt;

  typedef union {
   struct {
    b1 GeneralCallRecognition:1;
    b7 SlaveAddress:7;
   };
   u1 byte;
  } ARt;

  constexpr volatile u1  * const BR = &TWBR;

  /**
   * The TWCR is used to control the operation of the TWI. It is used to enable
   * the TWI, to initiate a Master access by applying a START condition to the
   * bus, to generate a Receiver acknowledge, to generate a stop condition, and
   * to control halting of the bus while the data to be written to the bus are
   * written to the TWDR. It also indicates a write collision if data is
   * attempted written to TWDR while the register is inaccessible.
   */
  constexpr volatile CRt * const CR  = (volatile CRt * const)&TWCR;
  constexpr volatile SRt * const SR  = (volatile SRt * const)&TWSR;
  constexpr volatile u1  * const DR  =                       &TWDR;
  constexpr volatile ARt * const AR  = (volatile ARt * const)&TWAR;
  constexpr volatile u1  * const AMR =                       &TWAMR;

  enum class Status : u1 {
   MasterStartTransmitted = 0x08,
   MasterStartRepeatTransmitted = 0x10,
   MasterWriteAcked = 0x18,
   MasterWriteNacked = 0x20,
   MasterDataTransmittedAcked = 0x28,
   MasterDataTransmittedNacked = 0x30,
   MasterArbitrationLost = 0x38,

   MasterReadAcked = 0x40,
   MasterReadNacked = 0x48,
   MasterDataReceivedAcked = 0x50,
   MasterDataReceivedNacked = 0x58,
   
   SlaveWriteAcked = 0x60,
   SlaveWriteAckedMasterLost = 0x68,
   SlaveGeneralAcked = 0x70,
   SlaveGeneralAckedMasterLost = 0x78,
   SlaveDataReceivedAcked = 0x80,
   SlaveDataReceivedNacked = 0x88,
   SlaveGeneralDataReceivedAcked = 0x90,
   SlaveGeneralDataReceivedNacked = 0x98,
   SlaveStopped = 0xA0,
   
   SlaveReadAcked = 0xA8,
   SlaveReadAckedMasterLost = 0xB0,
   SlaveDataTransmittedAcked = 0xB8,
   SlaveDataTransmittedNacked = 0xC0,
   SlaveDataTransmittedAckedDone = 0xC8,
   
   NoState = 0xF8,
   BusError = 0x00
  };
  
  volatile inline Status getStatus() {
   return (Status)(SR->Status << 3);
  }
 
  inline static bool isFlagSet() {return CR->InterruptFlag;}
  
  class Control {
   const u1 crValueBase;
  public:
   inline Control(bool useInterrupt = true, bool ack = false)
    : crValueBase(0b00000100 | (useInterrupt << TWIE))
    {
     CR->byte = crValueBase | (ack << TWEA);
    }
   inline void clearFlag() {
    CR->byte = crValueBase | (1 << TWINT);
   }
   inline void clearFlagEnableAck() {
    CR->byte = crValueBase | (1 << TWINT) | (1 << TWEA);
   }
   const inline void enableAck() {
    CR->byte = crValueBase | (1 << TWEA);
   }
  };
  
  inline static void sendByte(u1 const b) {
   *DR = b;
  }
  
  inline static u1 getByte() {
   return *DR;
  }
  
  inline static void sendSTART() {
   CR->byte = 1<<TWINT | 1<<TWEN | 1<<TWSTA;
  }
  
  inline static void sendSTOP() {
   CR->byte = 1<<TWINT | 1<<TWEN | 1<<TWSTO;
  }
  
  inline static void setupACK() {
   CR->byte = 1<<TWINT | 1<<TWEN | 1<<TWEA;
  }
  
  inline static void setupNACK() {
   CR->byte = 1<<TWINT | 1<<TWEN;
  }
  
  inline static void setPrescaler(b2 bits) {
   // We can be lazy and assign the whole byte since only two bits are writable
   SR->byte = bits;
  }
  
  inline static void setBitRateRegister(u1 twbr) {
   *BR = twbr;
  }
  
  inline static bool isWriteAddress(u1 const addr) {
   return (addr & 1) == 0;
  }
  
  inline static bool isReadAddress(u1 const addr) {
   return (addr & 1) == 1;
  }
  
  inline static u1 makeWriteAddress(u1 const addr) {
   return addr & ~1;
  }
  
  inline static u1 makeReadAddress(u1 const addr) {
   return addr | 1;
  }
 };
};

#endif	/* I2C_H */

