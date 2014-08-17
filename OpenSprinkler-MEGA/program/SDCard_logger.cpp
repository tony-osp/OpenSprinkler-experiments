/*
	SD Card Logger class implementation for OpenSprinkler
	***AP***

*/

#include "SDCard_logger.h"

#define ENABLE_SERIAL_DEBUG 1

// External references
extern BufferFiller bfill;
extern EthernetClient client;

// local forward definitions
byte emit_logs_listing(void);
byte emit_log_file(char *path);
byte log_str_internal(char *str, char flag);           // internal log_str worker, second parameter indicates string type. false == RAM, true==PROGMEM

  // ====== Data Members ======
static byte    logger_ready = false;

#define CL_TMPB_SIZE  255    // size of the local temporary buffer
#define LOG_FNAME_FORMAT "/logs/%2.2u-%4.4u.log"

static char tmp_buf[CL_TMPB_SIZE];

// Initialization. Returns true on success and false on failure

byte SDLogger::begin(void)
{
  byte r;

#ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(115200);  //*** debug output, just for now
  Serial.println(F("Logger - begin."));
#endif
  
  pinMode(PIN_SD_CS, OUTPUT); 
  pinMode(PIN_ETHER_CS, OUTPUT);
  digitalWrite(PIN_ETHER_CS, HIGH); // davekw7x: If it's low, the Wiznet chip corrupts the SPI bus
  digitalWrite(PIN_SD_CS, LOW); 

  r = SD.begin(PIN_SD_CS);
  digitalWrite(PIN_SD_CS, HIGH); 

#ifdef ENABLE_SERIAL_DEBUG
  if( !r )Serial.println(F("SD init failed."));
  else  Serial.println(F("SD init - success."));
#endif

  return r;
}


// Start logging - open/create log file, create initial "start" record. Note: uses time/date data from OpenSprinkler, time/date should be available by now
// Takes input string that will be used in the first log record
// Returns true on success and false on failure

byte SDLogger::start_logger(char *str)
{
  char    log_fname[20];
  time_t t=now();

//  generate log file name
  sprintf_P(log_fname, PSTR(LOG_FNAME_FORMAT), month(t), year(t) );

#ifdef ENABLE_SERIAL_DEBUG
  Serial.print(F("Logger - starting. Log file name: ")); Serial.println(log_fname); 
#endif

  File logfile = SD.open(log_fname, FILE_WRITE);
  if( !logfile ) return false;    // failed to open/create log file

  logfile.close();
  logger_ready = true;      // we are good to go
  return log_str_P(str);
}

// Add new log record
// Takes input string, returns true on success and false on failure
//
byte SDLogger::log_str(char *str)
{
   return log_str_internal(str, false);
}

// Add new log record
// Takes input string FROM PROGRAM MEMORY, returns true on success and false on failure
//
byte SDLogger::log_str_P(char *str)
{
   return log_str_internal(str, true);
}

// Internal worker for log_str
// First parameter is the string, second parameter is a flag indicating string location
//     false == RAM
//     true   == PROGMEM
//
byte log_str_internal(char *str, char flag)
{
   char    log_fname[20];
   time_t t=now();
  
   if( strlen(str) > (CL_TMPB_SIZE-20) ) return false;   // input string too long, reject it. Note: we need almost 20 bytes for the date/time etc
  
   if( flag ) sprintf_P(tmp_buf, PSTR("%2.2u-%2.2u-%4.4u %2.2u:%2.2u  %S"), month(t), day(t), year(t), hour(t), minute(t), str );
   else      sprintf_P(tmp_buf, PSTR("%2.2u-%2.2u-%4.4u %2.2u:%2.2u  %s"), month(t), day(t), year(t), hour(t), minute(t), str );
  
   if( !logger_ready ) return false;  //check if the logger is ready

//  generate log file name
  sprintf_P(log_fname, PSTR(LOG_FNAME_FORMAT), month(t), year(t) );

  File logfile = SD.open(log_fname, FILE_WRITE);
  if( !logfile ) return false;    // failed to open/create log file
  logfile.println(tmp_buf);
  logfile.close();

#ifdef ENABLE_SERIAL_DEBUG
  Serial.print(F("Logger - log string <<<")); Serial.print(tmp_buf); Serial.println(F(">>>"));
#endif

   return true;   
}


// handle web requests, str is the input request string
//
byte SDLogger::log_web_rq(char *str)
{
#ifdef ENABLE_SERIAL_DEBUG
  Serial.print(F("Logger - web request, input string <<<")); Serial.print(str); Serial.println(F(">>>"));
#endif

   if( !logger_ready ) return false;  //check if the logger is ready
 
//   let's check what is it - log listing or a specific log file request
   
   if( str[4] == ' ' || (str[4] == '/' && str[5] == ' ')){    // this is log listing - the string is either /logs or logs/ with a spacebar after the last character
       return emit_logs_listing();
   }
   else {         // this is a request to an individual log file
       return emit_log_file(str);
   }
}

// Send null-terminated string to the WEB client from program memory

void client_send_str_p(char *buf)
{

  char c;
  int len = 0;
   
   while( true ){
       c = pgm_read_byte(buf++);
       if( !c ){  //end of the line, send it and exit
         
         if( len>0 ) client.write((uint8_t *)tmp_buf, len);
         return;
       }
       tmp_buf[len++] = c;  //add char to the temp buffer
       if( len == (CL_TMPB_SIZE-1) ){
        
          tmp_buf[len] = 0;
          client.write((uint8_t *)tmp_buf, len);
          len = 0;
       }
   }
}

// ***AP***
//  Sends http-formatted /logs directory listing to the connected WEB client. Uses direct external reference to the client object.
//

byte emit_logs_listing(void)
{
   File logfile = SD.open("/logs", FILE_READ);
   if( !logfile ){

#ifdef ENABLE_SERIAL_DEBUG
      Serial.println(F("Cannot find directory /logs"));
#endif
      return false;    // failed to open logs directory
   }
      client_send_str_p( PSTR("<html>\n<body>\n<h1>Directory listing of /logs</h1>\n<table> <tr> <td><b>File Name</b></td> <td>&nbsp&nbsp</td> <td><b>Size, bytes</b></td> </tr>\n"));

      while(true) {

            File entry =  logfile.openNextFile();
            if (! entry) {
       // no more files
                  logfile.close();
                  client_send_str_p( PSTR("</table> </body>\n</html>"));
                  return true;          // all done, exiting
            }

        client_send_str_p( PSTR("<tr> <td> <a href=\"/logs/"));  client.print(entry.name());  client_send_str_p( PSTR("\">")); client.print(entry.name()); client_send_str_p( PSTR("</a> </td> <td>&nbsp&nbsp</td> <td>"));
        client.print(entry.size(), DEC);   client_send_str_p( PSTR("</td> </tr>"));

#ifdef ENABLE_SERIAL_DEBUG
        Serial.print(entry.name());   Serial.print("\t\t");  Serial.println(entry.size(), DEC);
#endif
         entry.close();
    }
    logfile.close();
    
    return true;
}

// ***AP***
//  Sends individual log file to the connected WEB client. Log file path (as GET request) is provided as the input parameter. Uses direct external reference to the client object.
//

byte emit_log_file(char *path)
{
    char c;
    int    i;

// copy full file name to tmps, looding for a space bar as termination. Null-terminate resulting file name.
    for( i=0; i<(CL_TMPB_SIZE-1); i++){
       
       if( path[i] == ' ')break;
       tmp_buf[i] = path[i];
    }
    tmp_buf[i] = 0;

#ifdef ENABLE_SERIAL_DEBUG
      Serial.print(F("Emit individual log file, path: <<<")); Serial.print(tmp_buf); Serial.println(F(">>>"));
#endif

  File logfile = SD.open(tmp_buf, FILE_READ);
   if( !logfile ){

#ifdef ENABLE_SERIAL_DEBUG
      Serial.println(F("Cannot find required log file."));
#endif
      return false;    // failed to open log file
   }

// send the file in chunks, CL_TMPB_SIZE at a time. 
     while(true) {
       
       i = logfile.read(tmp_buf, CL_TMPB_SIZE);
       if( !i )break;
       client.write( (uint8_t *)tmp_buf, i );
     }
     logfile.close();
    
    return true;
}

