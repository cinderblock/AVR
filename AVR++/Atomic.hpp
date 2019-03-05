#pragma once

#include <avr/io.h>
#include <util/atomic.h>

// Just in case
#undef AVR

namespace AVR {

template <class T> class Atomic {
  T value;

public:
  inline Atomic(T v) : value(v) {}
  inline Atomic() {}

  inline T &getUnsafe() { return value; }

  inline const T &getUnsafe() const { return value; }

  inline volatile T &getUnsafe() volatile { return value; }

  inline const volatile T &getUnsafe() const volatile { return value; }

  inline void setUnsafe(const T next) { value = next; }

  inline void setUnsafe(const T next) volatile { value = next; }

  // The useful bits that abstract around atomic reads and writes

  inline operator T() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
    return value;
  }

  inline operator const T() const {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
    return value;
  }

  inline operator T() volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
    return value;
  }

  inline operator const T() const volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
    return value;
  }

  inline T &operator=(T &next) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = next; }
    return next;
  }

  inline T &operator=(T &next) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = next; }
    return next;
  }

  inline const T &operator=(const T &next) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = next; }
    return next;
  }

  inline const T &operator=(const T &next) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = next; }
    return next;
  }

  inline T getForceInterruptsOn() {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
    return value;
  }

  inline const T getForceInterruptsOn() const {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
    return value;
  }

  inline T getForceInterruptsOn() volatile {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
    return value;
  }

  inline const T getForceInterruptsOn() const volatile {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
    return value;
  }

  inline T &setForceInterruptsOn(T &next) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { value = next; }
    return next;
  }

  inline T &setForceInterruptsOn(T &next) volatile {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { value = next; }
    return next;
  }

  inline const T &setForceInterruptsOn(const T &next) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { value = next; }
    return next;
  }

  inline const T &setForceInterruptsOn(const T &next) volatile {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { value = next; }
    return next;
  }
};

} // namespace AVR