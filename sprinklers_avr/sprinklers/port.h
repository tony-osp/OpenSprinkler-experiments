// port.h
// This file defines certain functions necessary to abstract out the AVR (e.g. Arduino) based functions
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include <Ethernet.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <SdFat.h>
#include "freeMemory.h"


void trace(const char * fmt, ...);
void trace(const __FlashStringHelper * fmt, ...);

#define EXIT_FAILURE 1
