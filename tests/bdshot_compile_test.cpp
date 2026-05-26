// CI smoke test for the BDShot driver. BDShot.cpp is intended to be
// `#include`d by the consumer (yes, a .cpp file) — it has no separate
// translation unit. To exercise it in CI we follow the README pattern: the
// consumer includes the .cpp, declares an explicit template instantiation
// for the pin combination they want, then references the type.
//
// BDShotConfig::Debug::Pin defaults to AVR::Output<Ports::B, 8> which is
// the sentinel "dummy pin" (Pin 8 → no-op IOpin), so no stub config is
// required here.

#include <AVR++/BDShot.cpp>

template class AVR::DShot::BDShot<AVR::Ports::C, 7>;

namespace {
[[maybe_unused]] inline void touch() {
  using ESC = AVR::DShot::BDShot<AVR::Ports::C, 7>;
  ESC::init();
}
} // namespace
