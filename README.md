# Standardized C++ AVR Library

This library aims to provide a high performance C++ library for dealing with AVR hardware in an abstract manner. Initial support geared towards 8-bit AVRs.

## Work In Progress

While I have high hopes for this project, it's nowhere near the standardized library that it could be. It currently only works for some hardware on a few chips.

It also needs some help in certain placed for performance. Eventually, most functions will compile down to the equivalent single AVR instructions where possible. This for instance is not yet done with `IOpin.h`.

## Recommended Usage

- Use a git submodule to load a version of AVR++ into a project. Since we have yet to standardize basically anything, semantic versioning is useless for now. Instead, using submodules enables easily trying new features with the safety of getting
- Add the submodule folder to your Includes (`-I`) directories. _Note: Point to the main submodule directory._
- Include the header files as necessary: `#include "AVR++/USART.h"`
- Compile the needed .cpp files and link them into your final build.

## Modules

### [`IOpin.hpp`](AVR++/IOpin.hpp)

A header only library for dealing with I/O pins.
Exports a `IOpin<port, pin>` class that can be used to abstract away the details of dealing with a single IO pin in a high performance manner.
All set/clear/toggle operations are done in a single instruction.
Unfortunately, this means that the pin number must be known at compile time and they aren't interchangeable classes that can be parameters as you might expect.

```C++
template <Ports port, unsigned pin> class IOpin {
  inline static void output();        // Set pin to output
  inline static void input();         // Set pin to input
  inline static void enablePullUp();  // Enable pull-up resistor
  inline static void disablePullUp(); // Disable pull-up resistor
  inline static void setPullUp(bool); // Set pull-up resistor
  inline static void set();           // Set pin high
  inline static void clr();           // Set pin low
  inline static void tgl();           // Toggle pin via hardware
  inline static void set(bool);       // Set pin to value

  inline static bool isHigh();          // Check if pin is high
  inline static bool isDriveHigh();     // Check if we're driving the pin high
  inline static bool isOutputEnabled(); // Check if pin is output
}
```

Usage:

```C++
#include <AVR++/IOpin.hpp>
using MyPin = IOpin<Ports::B, 0>;
void setup() {
  MyPin::init();
  MyPin::output();
  MyPin::set();
  // ...
}
```

There are also more advance child classes that automatically deal with many common use cases:

- **`Output<Port, Pin, inverted = false, startOn = false>`** - A mode that gives logical meaning to `on()` and `off()`.
  `inverted` will automatically invert normal logic and map `on()` to `clr()` (LOW).
  Useful to abstract away the hardware implementation of the logical function of a pin.
  It also automatically sets the pin to output mode and initial stat on startup, before `main()`.
- **`WeakOutput<Port, Pin, inverted = false, startOn = false>`** - A mode like `Output` that leaves the pin as an **Input**.
  Uses the internal pull-up resistor to drive the pin high.
- **`OpenDrain<Port, Pin, activeLow, pullUp>`** - A mode that allows the pin to be driven low, but not high.
  `activeLow` will invert the logic and map `on()` to `clr()` (LOW).
  `pullUp` will enable the internal pull-up resistor when not driving low.

### [`USART.hpp`](AVR++/USART.hpp)

A library for dealing with the USART hardware.

_TODO: Fill in details here._

### [`SPI.hpp`](AVR++/SPI.hpp)

A header only library for dealing with the SPI hardware.

_TODO: Fill in details here._

### [`Atomic.hpp`](AVR++/Atomic.hpp)

A header only library for dealing with Atomic operations.
Makes it easy to wrap any type with interrupt safe operations and accessors.

_TODO: Fill in details here._

### [`ADC.hpp`](AVR++/ADC.hpp)

A header only library for dealing with the ADC hardware.

_TODO: Fill in details here._

### [`basicTypes.hpp`](AVR++/basicTypes.hpp), [`bigTypes.hpp`](AVR++/bigTypes.hpp), [`bitTypes.hpp`](AVR++/bitTypes.hpp), & [`AVRTypes.hpp`](AVR++/AVRTypes.hpp)

Header only libraries for dealing with various sized variables in a clean way.

_TODO: Fill in details here._

### [`WDT.hpp`](AVR++/WDT.hpp)

A header only library for dealing with the Watchdog hardware.

_TODO: Fill in details here._

### [`PulsedOutput.hpp`](AVR++/PulsedOutput.hpp)

A library to bit-bang out pulsed signals on IOpins.
Useful for high speed single-wire protocols like WS2812 and DShot.

### [`WS2812.hpp`](AVR++/WS2812.hpp)

A library to bit-bang out streams of bytes intended to be used with WS2812 (and relate) LEDs.

### [`DShot.hpp`](AVR++/DShot.hpp)

A library to bit-bang out DShot packets for use with modern inexpensive BLDC ESCs.

### [`BDShot.hpp`](AVR++/BDShot.hpp)

A library to add Bidirectional support to DShot packets to allow for reading back telemetry data from ESCs.
