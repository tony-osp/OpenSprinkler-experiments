// Arduino library code for OpenSprinkler Generation 2

/* OpenSprinkler Class Implementation
 Creative Commons Attribution-ShareAlike 3.0 license
 Dec 2012 @ Rayshobby.net
 */

#include "OpenSprinklerGen2.h"
#include "SDCard_logger.h"
#include "LocalUI.h"

extern OSLocalUI  localUI;    // reference to the localUI (defined in interval_program_v2

StatusBits OpenSprinkler::status;
byte OpenSprinkler::nboards;
byte OpenSprinkler::nstations;
byte OpenSprinkler::station_bits[MAX_EXT_BOARDS+1];
byte OpenSprinkler::masop_bits[MAX_EXT_BOARDS+1];
unsigned long OpenSprinkler::raindelay_stop_time;

//===== Digital Outputs =====// 
//***AP*** Note: station pins are in reverse order
int OpenSprinkler::station_pins[8] = {
  PIN_STN_S1, PIN_STN_S2, PIN_STN_S3, PIN_STN_S4, PIN_STN_S5, PIN_STN_S6, PIN_STN_S7, PIN_STN_S8};

// Option names
prog_char _str_fwv [] PROGMEM = "Firmware ver.";
prog_char _str_tz  [] PROGMEM = "Time zone:";
prog_char _str_ntp [] PROGMEM = "NTP Sync:";
prog_char _str_dhcp[] PROGMEM = "Use DHCP:";
prog_char _str_ip1 [] PROGMEM = "Static.ip1:";
prog_char _str_ip2 [] PROGMEM = "ip2:";
prog_char _str_ip3 [] PROGMEM = "ip3:";
prog_char _str_ip4 [] PROGMEM = "ip4:";
prog_char _str_gw1 [] PROGMEM = "Gateway.ip1:";
prog_char _str_gw2 [] PROGMEM = "ip2:";
prog_char _str_gw3 [] PROGMEM = "ip3:";
prog_char _str_gw4 [] PROGMEM = "ip4:";
prog_char _str_hp0 [] PROGMEM = "HTTP port:";
prog_char _str_hp1 [] PROGMEM = "";
prog_char _str_ar  [] PROGMEM = "Auto reconnect:";
prog_char _str_ext [] PROGMEM = "Exp. boards:";
prog_char _str_seq [] PROGMEM = "Sequential:";
prog_char _str_sdt [] PROGMEM = "Station delay:";
prog_char _str_mas [] PROGMEM = "Master station:";
prog_char _str_mton[] PROGMEM = "Mas. on adj.:";
prog_char _str_mtof[] PROGMEM = "Mas. off adj.:";
prog_char _str_urs [] PROGMEM = "Use rain sensor:";
prog_char _str_rso [] PROGMEM = "Normally open:";
prog_char _str_wl  [] PROGMEM = "Watering level:";
prog_char _str_stt [] PROGMEM = "Selftest time:";
prog_char _str_ipas[] PROGMEM = "Ignore password:";
prog_char _str_devid[]PROGMEM = "Device ID:";
prog_char _str_con [] PROGMEM = "LCD Contrast:";
prog_char _str_lit [] PROGMEM = "LCD Backlight:";
prog_char _str_reset[] PROGMEM = "Reset all?";


OptionStruct OpenSprinkler::options[NUM_OPTIONS] = {
  {SVC_FW_VERSION, 0, _str_fwv, OPFLAG_NONE  } ,                    // firmware version
  {32,  108, _str_tz,   OPFLAG_WEB_EDIT | OPFLAG_SETUP_EDIT  },     // default time zone: GMT-4
  {1,   1,   _str_ntp,  OPFLAG_SETUP_EDIT  },                       // use NTP sync
  {0,   1,   _str_dhcp, OPFLAG_SETUP_EDIT  },                       // 0: use static ip, 1: use dhcp
  {STATIC_IP_1, 255, _str_ip1,  OPFLAG_SETUP_EDIT  },               // this and next 3 bytes define static ip
  {STATIC_IP_2, 255, _str_ip2,  OPFLAG_SETUP_EDIT  },
  {STATIC_IP_3,   255, _str_ip3,  OPFLAG_SETUP_EDIT  },
  {STATIC_IP_4,  255, _str_ip4,  OPFLAG_SETUP_EDIT },
  {STATIC_GW_1, 255, _str_gw1,  OPFLAG_SETUP_EDIT  },               // this and next 3 bytes define static gateway ip
  {STATIC_GW_2, 255, _str_gw2,  OPFLAG_SETUP_EDIT  },
  {STATIC_GW_3,   255, _str_gw3,  OPFLAG_SETUP_EDIT  },
  {STATIC_GW_4,   255, _str_gw4,  OPFLAG_SETUP_EDIT  },
  {STATIC_PORT0,  255, _str_hp0,  OPFLAG_WEB_EDIT  },               // this and next byte define http port number
  {STATIC_PORT1,   255, _str_hp1,  OPFLAG_WEB_EDIT  },
  {1,   1,   _str_ar,   OPFLAG_SETUP_EDIT  },                       // network auto reconnect
  {0,   MAX_EXT_BOARDS, _str_ext, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  }, // number of extension board. 0: no extension boards
  {1,   1,   _str_seq,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // sequential mode. 1: stations run sequentially; 0: concurrently
  {0,   240, _str_sdt,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // station delay time (0 to 240 seconds).
  {0,   8,   _str_mas,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // index of master station. 0: no master station
  {0,   60,  _str_mton, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // master on time [0,60] seconds
  {60,  120, _str_mtof, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // master off time [-60,60] seconds
  {0,   1,   _str_urs,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // rain sensor control bit. 1: use rain sensor input; 0: ignore
  {1,   1,   _str_rso,  OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // rain sensor type. 0: normally closed; 1: normally open.
  {100, 250, _str_wl,   OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // water level (default 100%),
  {10,  240, _str_stt,  OPFLAG_SETUP_EDIT  },                       // self-test time (in seconds)
  {0,   1,   _str_ipas, OPFLAG_SETUP_EDIT | OPFLAG_WEB_EDIT  },     // 1: ignore password; 0: use password
  {0,   255, _str_devid,OPFLAG_SETUP_EDIT  },                       // device id
  {110, 255,  _str_con,  OPFLAG_SETUP_EDIT  },                      // lcd contrast
  {200, 255,  _str_lit,  OPFLAG_SETUP_EDIT  },                      // lcd backlight
  {0,   1,   _str_reset,OPFLAG_SETUP_EDIT  }
};


//***AP***
// static data to keep track of the old station bits state
byte  old_station_bits[8] = {0}; 

// External references
extern SDLogger  sdlog;

// ===============
// Setup Functions
// ===============

// Arduino software reset function
void(* resetFunc) (void) = 0;

// Initialize network with the given mac address and http port
byte OpenSprinkler::start_network(byte mymac[], int http_port) {

  mymac[5] = options[OPTION_DEVICE_ID].value;
  if(!ether.begin(ETHER_BUFFER_SIZE, mymac, PIN_ETHER_CS))  return 0;
  ether.hisport = http_port;    

  if (options[OPTION_USE_DHCP].value) {
    // register with domain name "opensprinkler"
    if (!ether.dhcpSetup("opensprinkler")) return 0;
  } 
  else {
    byte staticip[] = {
      options[OPTION_STATIC_IP1].value,
      options[OPTION_STATIC_IP2].value,
      options[OPTION_STATIC_IP3].value,
      options[OPTION_STATIC_IP4].value        };

    byte gateway[] = {
      options[OPTION_GATEWAY_IP1].value,
      options[OPTION_GATEWAY_IP2].value,
      options[OPTION_GATEWAY_IP3].value,
      options[OPTION_GATEWAY_IP4].value        };
    if (!ether.staticSetup(staticip, gateway))  return 0;
  }
  return 1;
}

// Reboot controller
void OpenSprinkler::reboot() {
  resetFunc();
}

// OpenSprinkler init function
void OpenSprinkler::begin() {

  
  //===== Shift Register =====//
  /*
  // shift register setup
   pinMode(PIN_SR_LATCH, OUTPUT);
   pinMode(PIN_SR_OE, OUTPUT);
   pinMode(PIN_SR_CLOCK, OUTPUT);
   pinMode(PIN_SR_DATA,  OUTPUT);
   
   digitalWrite(PIN_SR_LATCH, HIGH);
   // pull shift register OE high to disable output
   digitalWrite(PIN_SR_OE, HIGH);
   */

  //===== Digital Outputs =====// 
  // initialize the pins as outputs:
  pinMode(PIN_STN_S1, OUTPUT); 
  pinMode(PIN_STN_S2, OUTPUT);
  pinMode(PIN_STN_S3, OUTPUT); 
  pinMode(PIN_STN_S4, OUTPUT); 
  pinMode(PIN_STN_S5, OUTPUT); 
  pinMode(PIN_STN_S6, OUTPUT); 
  pinMode(PIN_STN_S7, OUTPUT); 
  pinMode(PIN_STN_S8, OUTPUT); 
  //===========================//

  // Reset all stations
  clear_all_station_bits();
  apply_all_station_bits();

  //===== Shift Register =====//
  /*  
   // pull shift register OE low to enable output
   digitalWrite(PIN_SR_OE, LOW);
   */

  // Rain sensor port set up
  pinMode(PIN_RAINSENSOR, INPUT);
  digitalWrite(PIN_RAINSENSOR, HIGH); // enabled internal pullup

  // Init I2C
  Wire.begin();

  // Reset status variables
  status.enabled = 1;
  status.rain_delayed = 0;
  status.rain_sensed = 0;
  status.program_busy = 0;
  status.manual_mode = 0;
  status.has_rtc = 0;
  status.display_board = 0;
  status.network_fails = 0;

  nboards = 1;
  nstations = 8;
  raindelay_stop_time = 0;

  // set rf data pin
//  pinMode(PIN_RF_DATA, OUTPUT);
//  digitalWrite(PIN_RF_DATA, LOW);

  // detect if DS1307 RTC exists
  if (RTC.chipPresent()==0) {
    status.has_rtc = 1;
  }
  
//    status.has_rtc = 0;
}

// Self_test function
void OpenSprinkler::self_test(unsigned long ms) {
  byte sid;
  while(1) {
    for(sid=0; sid<nstations; sid++) {
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print((int)sid+1);
      clear_all_station_bits();
      set_station_bit(sid, 1);
      apply_all_station_bits();
      // run each station for designated amount of time
      delay(ms);	
    }
  }
}

// Get station name from eeprom
void OpenSprinkler::get_station_name(byte sid, char tmp[]) {
  int i=0;
  int start = ADDR_EEPROM_STN_NAMES + (int)sid * STATION_NAME_SIZE;
  tmp[STATION_NAME_SIZE]=0;
  while(1) {
    tmp[i] = eeprom_read_byte((unsigned char *)(start+i));
    if (tmp[i]==0 || i==(STATION_NAME_SIZE-1)) break;
    i++;
  }
  return;
}

// Set station name to eeprom
void OpenSprinkler::set_station_name(byte sid, char tmp[]) {
  int i=0;
  int start = ADDR_EEPROM_STN_NAMES + (int)sid * STATION_NAME_SIZE;
  tmp[STATION_NAME_SIZE]=0;
  while(1) {
    eeprom_write_byte((unsigned char *)(start+i), tmp[i]);
    if (tmp[i]==0 || i==(STATION_NAME_SIZE-1)) break;
    i++;
  }
  return;  
}

// Save station master operation bits to eeprom
void OpenSprinkler::masop_save() {
  byte i;
  for(i=0;i<=MAX_EXT_BOARDS;i++) {
    eeprom_write_byte((unsigned char *)ADDR_EEPROM_MAS_OP+i, masop_bits[i]);
  }
}

// Load station master operation bits from eeprom
void OpenSprinkler::masop_load() {
  byte i;
  for(i=0;i<=MAX_EXT_BOARDS;i++) {
    masop_bits[i] = eeprom_read_byte((unsigned char *)ADDR_EEPROM_MAS_OP+i);
  }
}

// ==================
// Schedule Functions
// ==================

// Index of today's weekday (Monday is 0)
byte OpenSprinkler::weekday_today() {
  return ((byte)weekday()+5)%7; // Time::weekday() assumes Sunday is 1
//  return ((byte)5)%7; // hardcode Monday
}

// Set station bit
void OpenSprinkler::set_station_bit(byte sid, byte value) {
  byte bid = (sid>>3);  // board index
  byte s = sid % 8;     // station bit index
  if (value) {
    station_bits[bid] = station_bits[bid] | ((byte)1<<s);
  } 
  else {
    station_bits[bid] = station_bits[bid] &~((byte)1<<s);
  }
}	

// Clear all station bits
void OpenSprinkler::clear_all_station_bits() {
  byte bid;
  for(bid=0;bid<=MAX_EXT_BOARDS;bid++) {
    station_bits[bid] = 0;
  }
}

// Apply all station bits
// !!! This will activate/deactivate valves !!!
void OpenSprinkler::apply_all_station_bits() {

  //===== Shift Register =====//
  /*
  digitalWrite(PIN_SR_LATCH, LOW);
   
   byte bid, s;
   byte bitvalue;
   
   // Shift out all station bit values
   // from the highest bit to the lowest
   for(bid=0;bid<=MAX_EXT_BOARDS;bid++) {
   bitvalue = 0;
   if (status.enabled && (!status.rain_delayed) && !(options[OPTION_USE_RAINSENSOR].value && status.rain_sensed))
   bitvalue = station_bits[MAX_EXT_BOARDS-bid];
   for(s=0;s<8;s++) {
   digitalWrite(PIN_SR_CLOCK, LOW);
   digitalWrite(PIN_SR_DATA, (bitvalue & ((byte)1<<(7-s))) ? HIGH : LOW );
   digitalWrite(PIN_SR_CLOCK, HIGH);          
   }
   }
   digitalWrite(PIN_SR_LATCH, HIGH); 
   */
  //===== Digital Pins =====//

  byte bid, s;
  byte bitvalue;

  // Shift out all station bit values
  // from the highest bit to the lowest
  for(bid=0;bid<=MAX_EXT_BOARDS;bid++) {
    bitvalue = 0;
    if (status.enabled && (!status.rain_delayed) && !(options[OPTION_USE_RAINSENSOR].value && status.rain_sensed))
      bitvalue = station_bits[MAX_EXT_BOARDS-bid];

      if( bid==MAX_EXT_BOARDS ){  // ***AP*** We should output parallel data only for the main board (not extension board - for now)
          for(s=0;s<8;s++) {
             byte outv = (bitvalue & ((byte)1<<(s))) ? LOW : HIGH;
             digitalWrite(station_pins[s], outv );         
             
             if( outv != old_station_bits[s]) {  // check if anything changed
               char tmps[25];
               
               if( outv ) sprintf_P(tmps, PSTR("Channel %i disabled."), s+1);    //N.B. we are using reverse station bits - values are negative
               else        sprintf_P(tmps, PSTR("Channel %i enabled."), s+1);
               
               sdlog.log_str(tmps);
               old_station_bits[s] = outv;
             }
             
////             Serial.print("Writing station :"); Serial.print(s); Serial.print(", pin: "); Serial.print(station_pins[s]); Serial.print(" value="); Serial.println(outv+'0'); 
          }
      }
  }
}		

// =================
// Options Functions
// =================

void OpenSprinkler::options_setup() {

  // add 0.5 second delay to allow EEPROM to stablize
  delay(500);

  // check reset condition: either firmware version has changed, or reset flag is up
  byte curr_ver = eeprom_read_byte((unsigned char*)(ADDR_EEPROM_OPTIONS+OPTION_FW_VERSION));
  if (curr_ver<100) curr_ver = curr_ver*10; // adding a default 0 if version number is the old type
  if (curr_ver != SVC_FW_VERSION || eeprom_read_byte((unsigned char*)(ADDR_EEPROM_OPTIONS+OPTION_RESET))==0xAA) {

    //======== Reset EEPROM data ========
    options_save(); // write default option values
    eeprom_string_set(ADDR_EEPROM_PASSWORD, DEFAULT_PASSWORD);  // write default password
    eeprom_string_set(ADDR_EEPROM_LOCATION, DEFAULT_LOCATION);  // write default location

    localUI.lcd_print_line_clear_pgm(PSTR("Resetting EEPROM"), 0);
    localUI.lcd_print_line_clear_pgm(PSTR("Please Wait..."), 1);  

    int i, sn;
    for(i=ADDR_EEPROM_STN_NAMES; i<INT_EEPROM_SIZE; i++) {
      eeprom_write_byte((unsigned char *) i, 0);      
    }

    // reset station names
    for(i=ADDR_EEPROM_STN_NAMES, sn=1; i<ADDR_EEPROM_RUNONCE; i+=STATION_NAME_SIZE, sn++) {
      eeprom_write_byte((unsigned char *)i    ,'S');
      eeprom_write_byte((unsigned char *)(i+1),'0'+(sn/10));
      eeprom_write_byte((unsigned char *)(i+2),'0'+(sn%10)); 
    }

    // reset master operation bits
    for(i=ADDR_EEPROM_MAS_OP; i<ADDR_EEPROM_MAS_OP+(MAX_EXT_BOARDS+1); i++) {
      // default master operation bits on
      eeprom_write_byte((unsigned char *)i, 0xff);
    }
    //======== END OF EEPROM RESET CODE ========

    // restart after resetting EEPROM.
    delay(500);
    reboot();
  } 
  else {
    options_load(); // load option values
    masop_load();   // load master operation bits
  }

}

// Load options from internal eeprom
void OpenSprinkler::options_load() {
  for (byte i=0; i<NUM_OPTIONS; i++) {
    options[i].value = eeprom_read_byte((unsigned char *)(ADDR_EEPROM_OPTIONS + i));
  }
  nboards = options[OPTION_EXT_BOARDS].value+1;
  nstations = nboards * 8;
}

// Save options to internal eeprom
void OpenSprinkler::options_save() {
  // save options in reverse order so version number is saved the last
  for (int i=NUM_OPTIONS-1; i>=0; i--) {
    eeprom_write_byte((unsigned char *) (ADDR_EEPROM_OPTIONS + i), options[i].value);
  }
  nboards = options[OPTION_EXT_BOARDS].value+1;
  nstations = nboards * 8;
}

// ==============================
// Controller Operation Functions
// ==============================

// Enable controller operation
void OpenSprinkler::enable() {
  status.enabled = 1;
  apply_all_station_bits();
  // write enable bit to eeprom
  options_save();
}

// Disable controller operation
void OpenSprinkler::disable() {
  status.enabled = 0;
  apply_all_station_bits();
  // write enable bit to eeprom
  options_save();
}

void OpenSprinkler::raindelay_start(byte rd) {
  if(rd == 0) return;
  raindelay_stop_time = now() + (unsigned long) rd * 3600;
  status.rain_delayed = 1;
  apply_all_station_bits();
}

void OpenSprinkler::raindelay_stop() {
  status.rain_delayed = 0;
  apply_all_station_bits();
}

void OpenSprinkler::rainsensor_status() {
  // options[OPTION_RS_TYPE]: 0 if normally closed, 1 if normally open
  status.rain_sensed = (digitalRead(PIN_RAINSENSOR) == options[OPTION_RAINSENSOR_TYPE].value ? 0 : 1);
}


// ==================
// String Functions
// ==================
void OpenSprinkler::eeprom_string_set(int start_addr, char* buf) {
  byte i=0;
  for (; (*buf)!=0; buf++, i++) {
    eeprom_write_byte((unsigned char*)(start_addr+i), *(buf));
  }
  eeprom_write_byte((unsigned char*)(start_addr+i), 0);  
}

void OpenSprinkler::eeprom_string_get(int start_addr, char *buf) {
  byte c;
  byte i = 0;
  do {
    c = eeprom_read_byte((unsigned char*)(start_addr+i));
    //if (c==' ') c='+';
    *(buf++) = c;
    i ++;
  } 
  while (c != 0);
}

// verify if a string matches password
byte OpenSprinkler::password_verify(char *pw) { 
  byte i = 0;
  byte c1, c2;
  while(1) {
    c1 = eeprom_read_byte((unsigned char*)(ADDR_EEPROM_PASSWORD+i));
    c2 = *pw;
    if (c1==0 || c2==0)
      break;
    if (c1!=c2) {
      return 0;
    }
    i++;
    pw++;
  }
  return (c1==c2) ? 1 : 0;
}



