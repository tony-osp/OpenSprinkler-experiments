// tftp.h
// main AVR entry 
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//


#include <SdVolume.h>
#include <SdStream.h>
#include <SdSpi.h>
#include <SdInfo.h>
#include <SdFile.h>
#include <SdFatUtil.h>
#include <SdFatStructs.h>
#include <SdFatmainpage.h>
#include <SdFatConfig.h>
#include <SdBaseFile.h>
#include <Sd2Card.h>
#include <ostream.h>
#include <MinimumSerial.h>
#include <istream.h>
#include <iostream.h>
#include <ios.h>
#include <bufstream.h>
#include <ArduinoStream.h>
//#include <SD.h>
#include "settings.h"
#include "core.h"
#include <Ethernet.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Time.h>
#include <SDFat.h>
#include <LiquidCrystal.h>
#include "LocalUI.h"
#include "sdlog.h"
#include <SFE_BMP180.h>
#include <Wire.h>

OSLocalUI localUI;

SdFat sd;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAD};

void setup() {
    Serial.begin(115200); 
    Serial.println("Start!");

    localUI.begin();
    localUI.lcd_print_line_clear_pgm(PSTR("Connecting..."), 1);
    
	if (IsFirstBoot())
		ResetEEPROM();
    // start the Ethernet connection and the server:
    Ethernet.begin(mac, GetIP(), INADDR_NONE, GetGateway(), GetNetmask());

	if (!sd.begin(4, SPI_HALF_SPEED)) 
		Serial.println("Could not Initialize Card");

    // give the Ethernet shield time to set up:
    delay(1000);
    
   localUI.set_mode(OSUI_MODE_HOME);  // set to HOME mode, page 0
   localUI.resume();
}

void loop() {
    mainLoop();
    localUI.loop();
}

