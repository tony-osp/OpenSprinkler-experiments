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

// Watering activity log directory and file name format (wat-yyyy.nnn)
#define WATERING_LOG_DIR			"/watering.log"
#define WATERING_LOG_FNAME_FORMAT "/watering.log/wat-%4.4u.%3.3u"

// Water flow data directory and file name format (wfl-yyyy.nnn)
#define WFLOW_LOG_DIR			"/wflow.log"
#define WFLOW_LOG_FNAME_FORMAT "/wflow.log/wfl-%4.4u.%3.3u"

// Temperature data directory and file name format (tem-yyyy.nnn)
//#define TEMPERATURE_LOG_DIR		 	 "/tempr.log"
//#define TEMPERATURE_LOG_FNAME_FORMAT "/tempr.log/tem-%4.4u.%3.3u"
// temporary put temperature logs into /logs directory
#define TEMPERATURE_LOG_DIR		 	 "/logs"
#define TEMPERATURE_LOG_FNAME_FORMAT "/logs/tem-%4.4u.%3.3u"

// Humidity data directory and file name format (hum-yyyy.nnn)
#define HUMIDITY_LOG_DIR		  "/humid.log"
#define HUMIDITY_LOG_FNAME_FORMAT "/humid.log/hum-%4.4u.%3.3u"

// Atmospheric pressure data directory and file name format (pre-yyyy.nnn)
//#define PRESSURE_LOG_DIR		  "/pressure.log"
//#define PRESSURE_LOG_FNAME_FORMAT "/pressure.log/pre-%4.4u.%3.3u"
// temporary put pressure files in system logs directory
#define PRESSURE_LOG_DIR		  "/logs"
#define PRESSURE_LOG_FNAME_FORMAT "/logs/pre-%4.4u.%3.3u"


//
// Event types for system log
//
#define SYSEVENT_ERROR       1
#define SYSEVENT_WARNING  2
#define SYSEVENT_INFO           3


//
// Sensor types
//
#define SENSOR_TYPE_TEMPERATURE    1
#define SENSOR_TYPE_PRESSURE            2
#define SENSOR_TYPE_HUMIDITY              3
#define SENSOR_TYPE_WATERFLOW        4


//
// Log types
//
#define LOG_TYPE_SYSTEM                        1
#define LOG_TYPE_WATERING                   2
#define LOG_TYPE_WATERFLOW               3
#define LOG_TYPE_TEMPERATURE           4
#define LOG_TYPE_HUMIDITY                     5
#define LOG_TYPE_PRESSURE                  6


// max log record size
#define MAX_LOG_RECORD_SIZE          32

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

        // Sensors logging. It covers all types of basic sensors (e.g. temperature, pressure etc) that provide momentarily (immediate) readings
        bool LogSensorReading(char sensor_type, int sensor_id, int sensor_reading);

        // add event to the system log with the string str
        byte syslog_str(char evt_type, char *str);
        // add event to the system log with the string str in PROGMEM
        byte syslog_str_P(char evt_type, char *str);
        
        void HandleWebRq(char *sPage, FILE *pFile);

private:

        bool   logger_ready;
        
        byte syslog_str_internal(char evt_type, char *str, char flag);
        int getZoneBins( int zone, time_t start, time_t end, long int *bin_data, int bins, GROUPING grouping);

};

#endif /* SD-LOG_H_ */
