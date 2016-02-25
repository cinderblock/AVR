# Standardized C++ AVR Library

This library aims to provide a high performance C++ library for dealing with AVR hardware in an abstract manner. Initial support geared towards 8-bit AVRs.

## Work In Progress

While I have high hopes for this project, it's nowhere near the standardized library that it could be. It currently only works for some hardware on a few chips.

It also needs some help in certain placed for performance. Eventually, most functions will compile down to the equivalent single AVR instructions where possible. This for instance is not yet done with `IOpin.h`.

## Recommended Usage

- Use a git submodule to load a version of AVR++ into a project. Since we have yet to standardize basically anything, semantic versioning is useless for now. Instead, using submodules enables easily trying new features with the safety of getting
- Add the submodule folder to your Includes (`-I`) directories. *Note: Point to the main submodule directory.*
- Include the header files as necessary: `#include "AVR++/USART.h"`
- Compile the needed .cpp files and link them into your final build.
