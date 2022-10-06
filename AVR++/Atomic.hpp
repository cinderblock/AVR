#pragma once

#include <avr/io.h>
#include <util/atomic.h>

// Just in case
#undef AVR

namespace AVR {

template <class T>
class Atomic {
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

  inline T getAndSet(const T next) {
    T ret;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      ret = value;
      value = next;
    }
    return ret;
  }

  inline T getAndSet(const T next) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      const auto ret = value;
      value = next;
      return ret;
    }
    // Should not get here. Prevent warning.
    return value;
  }

  // The useful bits that abstract around atomic reads and writes

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
  inline operator T() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
  }

  inline operator const T() const {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
  }

  inline operator T() volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
  }

  inline operator const T() const volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value; }
  }
#pragma GCC diagnostic pop

  inline Atomic<T> &operator++() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value++; }
    return *this;
  }

  inline Atomic<T> &operator++() volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value++; }
    return *this;
  }

  inline T operator++(int) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value++; }
  }

  inline T operator++(int) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value++; }
  }

  inline Atomic<T> &operator--() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value--; }
    return *this;
  }

  inline Atomic<T> &operator--() volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value--; }
    return *this;
  }

  inline T operator--(int) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value--; }
  }

  inline T operator--(int) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return value--; }
  }

  inline Atomic<T> &operator+=(const T v) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value += v; }
    return *this;
  }

  inline Atomic<T> &operator+=(const T v) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value += v; }
    return *this;
  }

  inline Atomic<T> &operator-=(const T v) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value -= v; }
    return *this;
  }

  inline Atomic<T> &operator-=(const T v) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value -= v; }
    return *this;
  }

  inline Atomic<T> &operator*=(const T v) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value *= v; }
    return *this;
  }

  inline Atomic<T> &operator*=(const T v) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value *= v; }
    return *this;
  }

  inline Atomic<T> &operator/=(const T v) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value /= v; }
    return *this;
  }

  inline Atomic<T> &operator/=(const T v) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value /= v; }
    return *this;
  }

  inline Atomic<T> &operator=(const T &next) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = next; }
    return *this;
  }

  inline Atomic<T> &operator=(const T &next) volatile {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = next; }
    return *this;
  }

  inline T getForceInterruptsOn() {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
  }

  inline const T getForceInterruptsOn() const {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
  }

  inline T getForceInterruptsOn() volatile {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
  }

  inline const T getForceInterruptsOn() const volatile {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { return value; }
  }

  inline Atomic<T> &setForceInterruptsOn(const T next) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { value = next; }
    return *this;
  }

  inline Atomic<T> &setForceInterruptsOn(const T next) volatile {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { value = next; }
    return *this;
  }
};

} // namespace AVR