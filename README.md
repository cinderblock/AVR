# Standardized C++ AVR Library

This library aims to provide a high performance C++ library for dealing with AVR hardware in an abstract manner. Initial support geared towards 8-bit AVRs.

## Work In Progress

While I have high hopes for this project, it's nowhere near the standardized library that it could be. It currently only works for some hardware on a few chips.

It also needs some help in certain placed for performance. Eventually, most functions will compile down to the equivalent single AVR instructions where possible. This for instance is not yet done with `IOpin.h`.
