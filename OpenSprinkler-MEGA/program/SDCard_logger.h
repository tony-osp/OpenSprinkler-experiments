/*
	SD Card Logger class for OpenSprinkler
	***AP***

*/
#ifndef _SDLogger_h
#define _SDLogger_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "OpenSprinklerGen2.h"
#include <SD.h>
#include <Time.h>

class SDLogger {
public:

  // ====== Member Functions ======
  // -- Setup --
  static byte begin(void);    // initialization, must call this function before calling other functions

  static byte start_logger(char *str);  // initialize logger, this will open or create new log file and add "start" log record. Note: uses time/date data (so clock should be running)
                                                         // Note: takes input string from PROGMEM!!!

  // -- Operation --
  static byte log_str(char *str);           // add log record using input string from RAM
  static byte log_str_P(char *str);       // add log record using input string from PROGRAM MEMORY

  static byte log_web_rq(char *str);     // handle web requests, str is the input request string

private:
//nothing
};

#endif

