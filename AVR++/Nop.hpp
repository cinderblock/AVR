#pragma once

// VS Code complains without this. GCC doesn't need it.
extern "C" void __builtin_avr_delay_cycles(long unsigned int);

namespace AVR {
static inline void nopCycles(unsigned cycles) {
  // GCC provides an excellent builtin for this.
  // It's a lot faster than the loops below but it's not always available.
#ifdef __BUILTIN_AVR_DELAY_CYCLES
  __builtin_avr_delay_cycles(cycles);
#else
  if (cycles & 1)
    asm volatile("nop");

  cycles /= 2;

  while (cycles--)
    asm volatile("rjmp .");
#endif
}
} // namespace AVR