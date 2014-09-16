/*

Logging sub-system implementation for Sprinklers control program. This module handles both regular system logging
as well as temperature/humidity/waterflow/etc logging.

Note: Log operation signature is implemented to be compatible with the Sql-based logging in sprinklers_pi control program.


Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/



#ifndef SD-LOG_H_
#define SD-LOG_H_

#include "port.h"
#include <Time.h>

//
// Log directories
//

// System log directory and file name format (mm-yyyy.log)
#define SYSTEM_LOG_DIR			"/logs"
#define SYSTEM_LOG_FNAME_FORMAT "/logs/%2.2u-%4.4u.log"

// Watering activity log directory and file name format (mm-yyyy.wat)
#define WATERING_LOG_DIR			"/watering.log"
#define WATERING_LOG_FNAME_FORMAT "/watering.log/%2.2u-%4.4u.wat"

// Water flow data directory and file name format (mm-yyyy.wtr)
#define WFLOW_LOG_DIR			"/wflow.log"
#define WFLOW_LOG_FNAME_FORMAT "/wflow.log/%2.2u-%4.4u.wfl"

// Temperature data directory and file name format (mm-yyyy.tem)
#define TEMPERATURE_LOG_DIR		 	 "/tempr.log"
#define TEMPERATURE_LOG_FNAME_FORMAT "/tempr.log/%2.2u-%4.4u.tem"

// Humidity data directory and file name format (mm-yyyy.hum)
#define HUMIDITY_LOG_DIR		  "/humid.log"
#define HUMIDITY_LOG_FNAME_FORMAT "/humid.log/%2.2u-%4.4u.hum"

//
// Event types for system log
//
#define SYSEVENT_ERROR    1
#define SYSEVENT_WARNING  2
#define SYSEVENT_INFO     3




class Logging
{
public:
        enum GROUPING {NONE, HOURLY, DAILY, MONTHLY};
        Logging();
        ~Logging();
        bool begin(char *str);
        void Close();
        // Watering activity logging. Note: signature is deliberately compatible with sprinklers_pi control program
        bool LogZoneEvent(time_t start, int zone, int duration, int schedule, int sadj, int wunderground);

        // Retrieve data sutible for graphing
        bool GraphZone(FILE * stream_file, time_t start, time_t end, GROUPING group);

        // Retrieve data suitble for putting into a table
        bool TableZone(FILE* stream_file, time_t start, time_t end);

        // add event to the system log with the string str
        byte syslog_str(char evt_type, char *str);
        // add event to the system log with the string str in PROGMEM
        byte syslog_str_P(char evt_type, char *str);
        
        void HandleWebRq(char *sPage, FILE *pFile);

private:

        bool   logger_ready;
        
        byte syslog_str_internal(char evt_type, char *str, char flag);

};

#endif /* SD-LOG_H_ */
