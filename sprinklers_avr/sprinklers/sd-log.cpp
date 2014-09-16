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

#define __STDC_FORMAT_MACROS
#include "sd-log.h"
#include "port.h"
#include "settings.h"

extern SdFat sd;

#define CL_TMPB_SIZE  256    // size of the local temporary buffer

Logging::Logging()
{
// initialize internal state

  logger_ready = false;

}

Logging::~Logging()
{
;
}


// Start logging - open/create system log file, create initial "start" record. Note: uses time/date data, time/date should be available by now
// Takes input string that will be used in the first log record
// Returns true on success and false on failure
//


bool Logging::begin(char *str)
{
  SdFile  lfile;
  char    log_fname[20];
  time_t  curr_time = nntpTimeServer.LocalNow();

// Ensure log folders are there, if not - create it.

  sprintf_P(log_fname, PSTR(SYSTEM_LOG_DIR));   // system log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("System log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating System log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(WATERING_LOG_DIR));   // watering log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Watering log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Watering log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(WFLOW_LOG_DIR));   // Waterflow log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Waterflow log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Waterflow log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(TEMPERATURE_LOG_DIR));   // Temperature log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Temperature log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Temperature log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(HUMIDITY_LOG_DIR));   // Temperature log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Humidity log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Humidity log directory.\n"));
        }
  }
  lfile.close();      // close the directory


//  generate system log file name
  sprintf_P(log_fname, PSTR(SYSTEM_LOG_FNAME_FORMAT), month(curr_time), year(curr_time) );


  if( !lfile.open(log_fname, O_WRITE | O_APPEND | O_CREAT) ){

        trace(F("Cannot open system log file (%s)\n"), log_fname);
        logger_ready = false;

        return false;    // failed to open/create log file
  }

  lfile.close();

  logger_ready = true;      // we are good to go

  return syslog_str_P(SYSEVENT_INFO, str);
}

void Logging::Close()
{
   logger_ready = false;
}



//
// internal helpers
//

// Add new log record
// Takes input string, returns true on success and false on failure
//
byte Logging::syslog_str(char evt_type, char *str)
{
   return syslog_str_internal(evt_type, str, false);
}

// Add new log record
// Takes input string FROM PROGRAM MEMORY, returns true on success and false on failure
//
byte Logging::syslog_str_P(char evt_type, char *str)
{
   return syslog_str_internal(evt_type, str, true);
}


// Internal worker for syslog_str
// First parameter is the string, second parameter is a flag indicating string location
//     false == RAM
//     true   == PROGMEM
//

// Note: We open/write/close log file on each event. It is kind of expensive, but it allows to keep RAM
//               usage low. Also close operation flushes buffers (acts as sync()).
//
byte Logging::syslog_str_internal(char evt_type, char *str, char flag)
{
   time_t t = nntpTimeServer.LocalNow();
   SdFile  system_logfile;
// temp buffer for log strings processing
   char tmp_buf[20];

   if( !logger_ready ) return false;  //check if the logger is ready

   sprintf_P(tmp_buf, PSTR(SYSTEM_LOG_FNAME_FORMAT), month(t), year(t) );

   if( !system_logfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

            trace(F("Cannot open system log file (%s)\n"), tmp_buf);

            logger_ready = false;      // something is wrong with the log file, mark logger as "not ready"
            return false;    // failed to open/create log file
   }

   if( flag ){   // progmem or regular string

      if( strlen_P(str) > (CL_TMPB_SIZE-20) ) return false;   // input string too long, reject it. Note: we need almost 20 bytes for the date/time etc
   }
   else {
      if( strlen(str) > (CL_TMPB_SIZE-20) ) return false;   // input string too long, reject it. Note: we need almost 20 bytes for the date/time etc
   }

   if( flag ){

      sprintf_P(tmp_buf, PSTR("%u,%u:%u:%u,"), day(t), hour(t), minute(t), second(t) );


      system_logfile.print(tmp_buf);

// Because this is PROGMEM string we have to output it manually. But SdFat library will buffer output anyway, so it is OK.
// SdFat will flush the buffer on close().
      char c;
      while((c = pgm_read_byte(str++))){

         system_logfile.write(c);
      }
      system_logfile.write('\n');
   }
   else {

      sprintf_P(tmp_buf, PSTR("%u,%u:%u:%u,"), day(t), hour(t), minute(t), second(t) );

      system_logfile.print(tmp_buf);
      system_logfile.println(str);
   }

   system_logfile.close();

   return true;
}


// Record watering event
//
// Note: for watering events we open/close file on each event

#define MAX_WATERING_LOG_RECORD_SIZE    64

bool Logging::LogZoneEvent(time_t start, int zone, int duration, int schedule, int sadj, int wunderground)
{
      SdFile  wfile;
      time_t t = nntpTimeServer.LocalNow();
// temp buffer for log strings processing
      char tmp_buf[MAX_WATERING_LOG_RECORD_SIZE];

      sprintf_P(tmp_buf, PSTR(WATERING_LOG_FNAME_FORMAT), month(t), year(t) );

      if( !wfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

            trace(F("Cannot open watering log file (%s)\n"), tmp_buf);
            return false;    // failed to open/create file
      }

      sprintf_P(tmp_buf, PSTR("%u,%u:%u,%u,%u,%u,%i,%i"), day(start), hour(start), minute(start), zone, duration, schedule, sadj, wunderground);

      wfile.println(tmp_buf);
      wfile.close();

      return true;
}


#ifdef notdef

static void DumpData(FILE * stream_file, uint32_t bin_data[], uint32_t bins, uint32_t bin_scale, uint32_t bin_offset, bool bMil)
{
        for (uint32_t i=0; i<bins; i++)
                fprintf(stream_file, "%s[%" PRIu32 "%s, %" PRIu32 "]", (i==0)?"":",", bin_offset + i*bin_scale, bMil?"000":"", bin_data[i]);
}

#endif  // disabled for now

bool Logging::GraphZone(FILE* stream_file, time_t start, time_t end, GROUPING grouping)
{
#ifdef notdef

        if (start == 0)
                start = nntpTimeServer.LocalNow();
        end = max(start,end) + 24*3600;  // add 1 day to end time.

        grouping = max(NONE, min(grouping, MONTHLY));
        char sSQL[200];
        uint16_t bins = 0;
        uint32_t bin_offset = 0;
        uint32_t bin_scale = 1;
        switch (grouping)
        {
        case HOURLY:
                snprintf(sSQL, sizeof(sSQL),
                                "SELECT zone, strftime('%%H', date, 'unixepoch') as hour, SUM(duration)"
                                " FROM zonelog WHERE date BETWEEN %lu AND %lu"
                                " GROUP BY zone,hour ORDER BY zone,hour",
                                start, end);
                bins = 24;
                break;
        case DAILY:
                snprintf(sSQL, sizeof(sSQL),
                                "SELECT zone, strftime('%%w', date, 'unixepoch') as bucket, SUM(duration)"
                                " FROM zonelog WHERE date BETWEEN %lu AND %lu"
                                " GROUP BY zone,bucket ORDER BY zone,bucket",
                                start, end);
                bins = 7;
                break;
        case MONTHLY:
                snprintf(sSQL, sizeof(sSQL),
                                "SELECT zone, strftime('%%m', date, 'unixepoch') as bucket, SUM(duration)"
                                " FROM zonelog WHERE date BETWEEN %lu AND %lu"
                                " GROUP BY zone,bucket ORDER BY zone,bucket",
                                start, end);
                bins = 12;
                break;
        case NONE:
                bins = 100;
                bin_offset = start;
                bin_scale = (end-start)/bins;
                snprintf(sSQL, sizeof(sSQL),
                                "SELECT zone, ((date-%lu)/%" PRIu32 ") as bucket, SUM(duration)"
                                " FROM zonelog WHERE date BETWEEN %lu AND %lu"
                                " GROUP BY zone,bucket ORDER BY zone,bucket",
                                start, bin_scale, start, end);
                break;
        }

        // Make sure we've zero terminated this string.
        sSQL[sizeof(sSQL)-1] = 0;

        trace("%s\n", sSQL);

        sqlite3_stmt * statement;
        // Now determine if we're running V1.0 of the schema.
        int res = sqlite3_prepare_v2(m_db, sSQL, -1, &statement, NULL);
        if (res)
        {
                trace("Prepare Failure (%s)\n", sqlite3_errmsg(m_db));
                return false;
        }

        int current_zone = -1;
        bool bFirstZone = true;
        uint32_t bin_data[bins];
        memset(bin_data, 0, bins*sizeof(uint32_t));
        while (sqlite3_step(statement) == SQLITE_ROW)
        {
                int zone = sqlite3_column_int(statement, 0);
                if (current_zone != zone)
                {
                        current_zone = zone;
                        if (!bFirstZone)
                                DumpData(stream_file, bin_data, bins, bin_scale, bin_offset, grouping == NONE);
                        fprintf(stream_file, "%s\t\"%d\" : [", bFirstZone?"":"],\n", zone);
                        memset(bin_data, 0, bins*sizeof(uint32_t));
                        bFirstZone = false;
                }
                uint32_t bin = sqlite3_column_int(statement, 1);
                uint32_t value = sqlite3_column_int(statement, 2);
                bin_data[bin] = value;
        }
        if (!bFirstZone)
        {
                DumpData(stream_file, bin_data, bins, bin_scale, bin_offset, grouping == NONE);
                fprintf(stream_file, "]\n");
        }

        sqlite3_finalize(statement);

#endif   //disabled for now

        return true;
}


bool Logging::TableZone(FILE* stream_file, time_t start, time_t end)
{
        char m;
        char tmp_buf[MAX_WATERING_LOG_RECORD_SIZE];
        int    nyear=year(start);

        if (start == 0)
                start = nntpTimeServer.LocalNow();

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        for( m = month(start); m<=month(end); m++ ){  // iterate over months between start and end

//                trace(F("Looping through log files, month=%u\n"), m );

                SdFile lfile;
                sprintf_P(tmp_buf, PSTR(WATERING_LOG_FNAME_FORMAT), m, year(start) );

                if( !lfile.open(tmp_buf, O_READ) ){

                       trace(F("Cannot open watering log file for read (%s)\n"), tmp_buf);
                       return false;    // failed to open file
                }
// OK, we opened required watering log file. Iterate over records, filtering out necessary dates range

                int xmaxzone = 1;    // we will find out maximum zone number on the first pass
                int curr_zone = 255;
                for( int xzone = 1; xzone <= xmaxzone; xzone++ ){
                  
                     char bFirstRow = true;
                     lfile.rewind();

                     while( lfile.available() ){

                            int  nday = 0, nhour = 0, nminute = 0;
                            int nzone = 0, nschedule = 0;
                            int  nduration = 0,  nsadj = 0, nwunderground = 0;

                            int bytes = lfile.fgets(tmp_buf, MAX_WATERING_LOG_RECORD_SIZE);
                            if (bytes <= 0)
                                       break;

// Parse the string into fields. First field (up to two digits) is the day of the month

                            sscanf_P( tmp_buf, PSTR("%u,%u:%u,%u,%i,%i,%i,%i"),
                                                            &nday, &nhour, &nminute, &nzone, &nduration, &nschedule, &nsadj, &nwunderground);

                            if( nday > day(end) )    // check for the end date
                                         break;

                            if( (nzone>xmaxzone) && (nzone<=NUM_ZONES) ) xmaxzone = nzone;    // NUM_ZONES is the upper limit, to avoid blocking device in a case of a bogus log file

                            if( (nday >= day(start)) && (nzone == xzone) ){        // the record is within required range. m is the month, nday is the day of the month, xzone is the zone we are currently emitting

// we have something to output.

                                    if( curr_zone != xzone ){
                                      
                                         if( curr_zone != 255 ) 
                                                   fprintf_P(stream_file, PSTR("\n\t\t\t\t\t]\n\t\t\t\t},\n"));   // if this is not the first zone, close previous one
                                         
                                         fprintf_P(stream_file, PSTR("\n\t\t\t\t { \n\t\t\t\t \"zone\": %i,\n\t\t\t\t \"entries\": ["), xzone);   // JSON zone header
                                         curr_zone = xzone;
                                         bFirstRow = true;
                                    }

                                    tmElements_t tm;   tm.Day = nday;  tm.Year = year(start) - 1970;  tm.Hour = nhour;  tm.Minute = nminute;  tm.Second = 0;
                            
                                    fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t { \"date\":%lu, \"duration\":%i, \"schedule\":%i, \"seasonal\":%i, \"wunderground\":%i}"),
                                                                       bFirstRow ? "":",",
                                                                       makeTime(tm), nduration, nschedule, nsadj, nwunderground );

                                     bFirstRow = false;
                            }
                      }
                }
                if( curr_zone != 255)
                         fprintf_P(stream_file, PSTR("\n\t\t\t\t\t ] \n\t\t\t\t } \n"));    // JSON zone footer
                lfile.close();
        }

        return true;
}


