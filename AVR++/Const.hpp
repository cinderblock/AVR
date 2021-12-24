#pragma once

template <typename T> static constexpr int const_round(T f) { return f > 0.0 ? int(f + 0.5) : int(f - 0.5); }
template <typename T> static constexpr T max(T a, T b) { return a > b ? a : b; }
template <typename T> static constexpr T min(T a, T b) { return a < b ? a : b; }