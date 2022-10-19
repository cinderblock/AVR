#pragma once

namespace AVR {
namespace Core {

/**
 * @brief Set the Z register to the desired location
 *
 * @param z A pointer to the location to jump to
 */
inline static void setZ(void *z) {
  asm("; setZ\n\t"
      "ldi r30, lo8(%x[function])\n\t"
      "ldi r31, hi8(%x[function])\n\t"
      : // No outputs
      : [function] "p"(z)
      : "r30", "r31");
}
/**
 * @brief Set the Z register to the desired function location
 *
 * @param z A pointer to the function to jump to
 */
template <typename T>
inline static void setZ(T (*z)()) {
  setZ((void *)z);
}

namespace Ticks {
namespace Hardware {
constexpr unsigned ISR = 5;
} // namespace Hardware
namespace Instruction {
constexpr unsigned Skip1Word = 2;
constexpr unsigned Skip2Words = 3;
constexpr unsigned LoaDImediate = 1;
constexpr unsigned Out = 1;
constexpr unsigned Branch = 2;
constexpr unsigned Jmp = 3;
constexpr unsigned IJmp = 2;
constexpr unsigned RJmp = 2;
constexpr unsigned RetI = 5; // Or is it 4?
} // namespace Instruction
} // namespace Ticks
} // namespace Core
} // namespace AVR