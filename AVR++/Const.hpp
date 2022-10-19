#pragma once

template <typename T>
inline constexpr int const_round(T f) {
  return f > 0.0 ? int(f + 0.5) : int(f - 0.5);
}
template <typename T>
inline constexpr T max(T a, T b) {
  return a > b ? a : b;
}
template <typename T>
inline constexpr T min(T a, T b) {
  return a < b ? a : b;
}
template <typename T>
inline constexpr T clamp(T x, T a = 1, T b = 0) {
  return min(max(x, min(a, b)), max(a, b));
}

template <typename In, typename Out>
inline constexpr Out map(In in, In inA, Out outA, In inB = 0, Out outB = 0) {
  return (in - inA) * (outB - outA) / (inB - inA) + outA;
}

constexpr bool equal(char const *a, char const *b) {
  while (*a || *b)
    if (*a++ != *b++) return false;
  return true;
}