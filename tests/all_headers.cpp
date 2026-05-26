// Auto-generated header-aggregation test for AVR++. Including every public
// header here catches parse / template-body errors that wouldn't surface
// without instantiation (e.g. C++17 stricter `constexpr` checks).
//
// Update this list when adding or renaming files in AVR++/.

#include <AVR++/ADC.hpp>
#include <AVR++/Atomic.hpp>
#include <AVR++/AVRTypes.hpp>
#include <AVR++/basicTypes.hpp>
#include <AVR++/BDShot.hpp>
#include <AVR++/bigTypes.hpp>
#include <AVR++/bitTypes.hpp>
#include <AVR++/Const.hpp>
#include <AVR++/Core.hpp>
#include <AVR++/DShot.hpp>
#include <AVR++/FlashArray.hpp>
#include <AVR++/FlashCRC.hpp>
#include <AVR++/FlashData.hpp>
#include <AVR++/gccGuard.hpp>
#include <AVR++/GCR.hpp>
#include <AVR++/I2C.hpp>
#include <AVR++/IOpin.hpp>
#include <AVR++/Nop.hpp>
#include <AVR++/Ports.hpp>
#include <AVR++/PulsedOutput.hpp>
#include <AVR++/ResetReason.hpp>
#include <AVR++/ScanningADC.hpp>
#include <AVR++/SPI.hpp>
#include <AVR++/TimerTimeout.hpp>
#include <AVR++/undefAVR.hpp>
#include <AVR++/USART.hpp>
#include <AVR++/WDT.hpp>
#include <AVR++/WS2812.hpp>

// Force template-body checking for a few of the trickier classes.
namespace {
using SamplePin = AVR::IOpin<AVR::Ports::B, 0>;
using SampleOutput = AVR::Output<AVR::Ports::B, 0>;
using SampleWeakOutput = AVR::WeakOutput<AVR::Ports::B, 0>;
using SampleInput = AVR::Input<AVR::Ports::B, 0>;
using SampleOpenDrain = AVR::OpenDrain<AVR::Ports::B, 0>;

[[maybe_unused]] inline void touch() {
  SamplePin::output();
  SampleOutput::init();
  SampleWeakOutput::init();
  SampleInput::init();
  SampleOpenDrain::init();
}
} // namespace
