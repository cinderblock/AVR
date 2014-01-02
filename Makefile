## Creator: Cameron Tacklind
##
## This is the main Makefile for building AVR projects. It should only be edited
## to add generic (and maybe project specific) build features. It should not
## include configuration (but should provide defaults or error if not set), or 
## support for external tools such as avrdude. Support for those will be in other
## Makefiles that are included from this one automatically. (Silent fail if missing).
##


# Load local settings
-include local.mk

CPP ?= USART

CPPFILES=$(CPP:%=%.cpp)
	
CPPFILES=USART.cpp

OPT ?= -O2

TARGET = AVR

MCU ?= atmega328p

-include build.mk

all: debug $(LIBOUT)

debug:
	$(ECO) AVRPP!

.PHONY: all debug