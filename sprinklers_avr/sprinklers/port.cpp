// port.cpp
// This file defines certain functions necessary to abstract out the AVR (e.g. Arduino) based functions
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "port.h"
#include <stdio.h>

static int serial_putchar(char c, FILE *stream)
{
	return Serial.write(c);
}

static FILE serial;

static void serial_setup()
{
	fdev_setup_stream(&serial, serial_putchar, NULL, _FDEV_SETUP_WRITE);
}

static bool bSerialSetup = false;

void trace(const char * fmt, ...)
{
	if (!bSerialSetup)
	{
		serial_setup();
		bSerialSetup = true;
	}
	va_list parms;
	va_start(parms, fmt);
	vfprintf(&serial, fmt, parms);
	va_end(parms);
}

void trace(const __FlashStringHelper * fmt, ...)
{
        if (!bSerialSetup)
        {
                serial_setup();
                bSerialSetup = true;
        }
        va_list parms;
        va_start(parms, fmt);
        vfprintf_P(&serial, reinterpret_cast<const char *>(fmt), parms);
        va_end(parms);
}

