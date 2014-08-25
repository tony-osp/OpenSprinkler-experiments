// freememory.cpp
// Some helper functions to see how we're using system resources.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "freeMemory.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


extern int __bss_end;
extern int __bss_start;
extern int __data_end;
extern int __data_start;
extern int __heap_start;
extern int *__brkval;

void freeMemory() 
{
    int free_memory;
    if(__brkval == 0)
        free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
        free_memory = ((int)&free_memory) - ((int)__brkval);

//    Serial.print("SP ");
//    Serial.println(SP);
//    Serial.print("__data_start ");
//    Serial.println((int)&__data_start);
//    Serial.print("__data_end ");
//    Serial.println((int)&__data_end);
//    Serial.print("__heap_start ");
//    Serial.println((int)&__heap_start);
//    Serial.print("__bss_start ");
//    Serial.println((int)&__bss_start);
//    Serial.print("__bss_end ");
//    Serial.println((int)&__bss_end);
//    Serial.print("__brkval ");
//    Serial.println((int)__brkval);
    Serial.print("Free Memory ");
    Serial.println(free_memory);
}

//#include <utility/W5100.h>
#include <Ethernet.h>

void ShowSockStatus()
{
	for (int i = 0; i < MAX_SOCK_NUM; i++) {
		Serial.print("Socket#");
		Serial.print(i);
//		uint8_t s = W5100.readSnSR(i);
//		Serial.print(":0x");
//		Serial.print(s,16);
//		Serial.print(" ");
//		Serial.print(W5100.readSnPORT(i));
//		Serial.print(" D:");
//		uint8_t dip[4];
//		W5100.readSnDIPR(i, dip);
//		for (int j=0; j<4; j++) {
//			Serial.print(dip[j],10);
//			if (j<3) Serial.print(".");
//		}
//		Serial.print("(");
//		Serial.print(W5100.readSnDPORT(i));
		Serial.println(")");
	}
}
/*
static const uint8_t CLOSED      = 0x00;
static const uint8_t INIT        = 0x13;
static const uint8_t LISTEN      = 0x14;
static const uint8_t SYNSENT     = 0x15;
static const uint8_t SYNRECV     = 0x16;
static const uint8_t ESTABLISHED = 0x17;
static const uint8_t FIN_WAIT    = 0x18;
static const uint8_t CLOSING     = 0x1A;
static const uint8_t TIME_WAIT   = 0x1B;
static const uint8_t CLOSE_WAIT  = 0x1C;
static const uint8_t LAST_ACK    = 0x1D;
static const uint8_t UDP         = 0x22;
static const uint8_t IPRAW       = 0x32;
static const uint8_t MACRAW      = 0x42;
static const uint8_t PPPOE       = 0x5F;
*/
