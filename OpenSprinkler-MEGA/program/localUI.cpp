/*
        Local UI (for local LCD and buttons) on OpenSprinkler
        ***AP***

*/

#include "localUI.h"
#include "OpenSprinklerGen2.h" 
#include "EtherCard_W5100.h"

// external references

extern OpenSprinkler svc;    // OpenSprinkler object
void manual_station_on(byte sid, int ontimer);


// Data members
byte OSLocalUI::osUI_State = OSUI_STATE_UNDEFINED;
byte OSLocalUI::osUI_Mode  = OSUI_MODE_UNDEFINED;
byte OSLocalUI::osUI_Page  = OSUI_PAGE_UNDEFINED;

// this is 1284p version
//LiquidCrystal OSLocalUI::lcd(23, 22, 27, 26, 25, 24);

// and this is Mega with big LCD version 
LiquidCrystal OSLocalUI::lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);


// Local forward declarations


byte get_button_async(byte mode);

byte weekday_today(void);
int get_key(unsigned int input);
void buttons_loop(void);
unsigned long subt_millis(unsigned long new_millis, unsigned long old_millis);

// button functions

byte button_read_busy(byte pin_butt, byte waitmode, byte butt, byte is_holding);        // Wait for button
byte button_read(byte waitmode);                                                                         // Read button and returns button value 'OR'ed with flag bits
void ui_set_options(int oid);                                                                            // user interface for setting options during startup


//static uint8_t ether_myip[4] = {10, 0, 1, 35};   // my ip address
//static uint8_t ether_gwip[4] = {10, 0, 1, 1};     // gateway
//static unsigned int  ether_port = 80;


// ====== UI defines ======

static char ui_anim_chars[3] = {'.', 'o', 'O'};

// Weekday display strings
prog_char str_day0[] PROGMEM = "Mon";
prog_char str_day1[] PROGMEM = "Tue";
prog_char str_day2[] PROGMEM = "Wed";
prog_char str_day3[] PROGMEM = "Thu";
prog_char str_day4[] PROGMEM = "Fri";
prog_char str_day5[] PROGMEM = "Sat";
prog_char str_day6[] PROGMEM = "Sun";

char* days_str[7] = {
  str_day0,
  str_day1,
  str_day2,
  str_day3,
  str_day4,
  str_day5,
  str_day6
};


// initialization. Intended to be called from setup()
//
// returns TRUE on success and FALSE otherwise
//
byte OSLocalUI::begin(void)
{
     lcd.begin(LOCAL_UI_LCD_X, LOCAL_UI_LCD_Y);         // init LCD with desired dimensions

     osUI_State      = OSUI_STATE_SUSPENDED;
     osUI_Mode       = OSUI_MODE_HOME;                         // home screen by default
     osUI_Page       = 0;                                                           // first page by default (pages start form 0)

// define lcd custom characters
    byte lcd_custom_char[8] = {
      B00000,
      B10100,
      B01000,
      B10101,
      B00001,
      B00101,
      B00101,
      B10101
    };
    lcd.createChar(1, lcd_custom_char);
    lcd_custom_char[1]=0;
    lcd_custom_char[2]=0;
    lcd_custom_char[3]=1;
    lcd.createChar(0, lcd_custom_char);

// set button PINs mode

   pinMode(PIN_BUTTON_1, INPUT);
   pinMode(PIN_BUTTON_2, INPUT);
   pinMode(PIN_BUTTON_3, INPUT);    
   pinMode(PIN_BUTTON_4, INPUT);    
   digitalWrite(PIN_BUTTON_1, HIGH);
   digitalWrite(PIN_BUTTON_2, HIGH);
   digitalWrite(PIN_BUTTON_3, HIGH); 
   digitalWrite(PIN_BUTTON_4, HIGH); 
    
//    Serial.begin(115200);
//    Serial.println("Local UI - started");

// Initialization specific to the native OpenSprinkler hardware. Not relevant for me.

// set PWM frequency for LCD
//  TCCR1B = 0x01;
  // turn on LCD backlight and contrast
//  pinMode(PIN_LCD_BACKLIGHT, OUTPUT);
//  pinMode(PIN_LCD_CONTRAST, OUTPUT);
//  analogWrite(PIN_LCD_CONTRAST, options[OPTION_LCD_CONTRAST].value);
//  analogWrite(PIN_LCD_BACKLIGHT, 255-options[OPTION_LCD_BACKLIGHT].value); 


        return true;    // exit, status - success
}

// -- Operation --

// Main loop. Intended to be called regularly and frequently to handle input and UI. Normally this will be called from Arduino loop()
  byte OSLocalUI::loop(void)
  {
    buttons_loop();  // check buttons regardless of the UI state to be ready to handle HOLD situations etc

    if( osUI_State != OSUI_STATE_ENABLED ) return true; //UI update disabled, nothing to do - exit

// local UI is enabled, call appropriate handler

     return callHandler(0);
  }

  // Force UI refresh.
byte OSLocalUI::refresh(void)
{
   return callHandler(1);
}

byte OSLocalUI::callHandler(byte needs_refresh)
{
   if( osUI_Mode == OSUI_MODE_HOME )                     return modeHandler_Home(needs_refresh);
        else if( osUI_Mode == OSUI_MODE_MANUAL )     return modeHandler_Manual(needs_refresh);
        else if( osUI_Mode == OSUI_MODE_VIEWCONF ) return modeHandler_Viewconf(needs_refresh);
        else if( osUI_Mode == OSUI_MODE_SETUP )        return modeHandler_Setup(needs_refresh);

        return false;   // incorrect UI mode - exit with failure
}


// Stop UI updates and input handling (useful when taking over the screen for custom output)
  byte OSLocalUI::suspend(void)
  {
        osUI_State = OSUI_STATE_SUSPENDED;
        return true;    // exit - success
  }

// Resume UI operation
  byte OSLocalUI::resume(void)
  {
        osUI_State = OSUI_STATE_ENABLED;

        return refresh();
  }

// Switch UI to desired Mode and specific Page within the mode
  byte OSLocalUI::set_mode(char mode)
  {
        osUI_Mode = mode;
        osUI_Page = 0;

       return callHandler(2);    // call MODE handler, indicating that it needs to setup things.
  }


 // Home screen mode handler.
 // It is responsible for updating the UI and handling input keys. All operations are asynchronous - must return right away.
 // Typically it is called from loop(), but could be called from other places as well, usually to force UI refresh.
 // Parameter indicates whether UI refresh is required (e.g. if the screen was previously modified by some other code).
 // TRUE means that refresh is required, FALSE means nobody touched LCD since the last call to this handler and UI updates could be more targeted.
 //
 byte OSLocalUI::modeHandler_Home(byte forceRefresh)
 {
   static unsigned long last_time = 0;

// assert
   if( osUI_Mode != OSUI_MODE_HOME )  return false;  // Basic protection to ensure current UI mode is actually HOME mode.

   char btn = get_button_async(0);

// handle input
   if( btn == BUTTON_MODE ){

       set_mode( OSUI_MODE_MANUAL ); // change mode to "view settings" which is the next mode
       return true;
   }
   else if( btn == BUTTON_CONFIRM ){

      if( svc.status.enabled ) svc.disable();  // currently enabled, disable it
      else                              svc.enable();   // currently disabled, enable it
   }

 // Show time and station status
   // if 1 second has passed
  time_t curr_time = now();
  if( (last_time != curr_time) || forceRefresh !=0 ) {  // update UI once a second, OR if explicit refresh is required

    last_time = curr_time;
    lcd_print_time(0);       // print time

    // process LCD display
    if(SHOW_MEMORY)
      lcd_print_memory(1);
    else
      lcd_print_station(1, ui_anim_chars[curr_time%3]);
  }

  return true;
}

// Manual mode loop() handler

byte OSLocalUI::modeHandler_Manual(byte forceRefresh)
{
  static byte sel_manual_ch = 0;
  static time_t last_time = 0;
  static byte man_state = 0;                    // this flag indicates the UI state within MANUAL mode. Valid states are:
                                                                        // 0 - initial screen, select the channel
                                                                        // 1 - entering number of minutes to run
  static byte num_min = 0;

// assert
  if( osUI_Mode != OSUI_MODE_MANUAL )  return false;  // Basic protection to ensure current UI mode is actually HOME mode.

  if( forceRefresh == 2 ){   // entering MANUAL mode, refresh things

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd_print_pgm(PSTR("Manual Start:"));

          man_state = 0;
          sel_manual_ch = 0;
          forceRefresh == 1;    // need to update the screen
  }
  time_t curr_time = now();

  char btn = get_button_async(1);    // Note: we allow Autorepeat in this mode, to help enter data quickly

  if( man_state == 0){          // select channel screen

         if( btn == BUTTON_UP ){

                if( sel_manual_ch < 7 ) sel_manual_ch++;
                else                            sel_manual_ch = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

            if( sel_manual_ch > 0 ) sel_manual_ch--;
                else                            sel_manual_ch = 7;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_MODE ){

            set_mode( OSUI_MODE_VIEWCONF ); // change mode to "view settings" which is the next mode
            return true;
         }
         else if( btn == BUTTON_CONFIRM ){

            man_state = 1;              // channel selected, need to enter the number of minutes to run
                num_min = 0;            // when entering the "number of minutes" screen, start from 0 minutes

// let's display the number of minutes prompt
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd_print_pgm(PSTR("Minutes to run:"));

                return  true;    // exit. Actual minutes display will happen on the next loop();
         }

     if( (last_time != curr_time) || (forceRefresh != 0) ) {  // update UI once a second (for blinking), OR if explicit refresh is required

       last_time = curr_time;
       lcd_print_station(1, '_', sel_manual_ch, (curr_time%2) ? '#':'_');       // this will blink selected station
     }
  }

 min_to_run_sel:
  if( man_state == 1){          // enter the number of minutes to run

         if( btn == BUTTON_UP ){

                if( num_min < 99 ) num_min++;
                else                       num_min = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

            if( num_min > 0 ) num_min--;
                else                      num_min = 99;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_MODE ){

            set_mode( OSUI_MODE_VIEWCONF ); // exit current mode, changing it to "view settings" which is the next mode
                return true;
         }
         else if( btn == BUTTON_CONFIRM ){
// start manual watering run on selected channel with selected number of minutes to run

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd_print_pgm(PSTR("Starting Manual"));

                lcd.setCursor(0, 1);
                lcd_print_pgm(PSTR("Ch: "));    lcd_print_2digit(sel_manual_ch);
                lcd_print_pgm(PSTR(" Min: "));  lcd_print_2digit(num_min);

                delay(2000);

                if( num_min != 0 ){
                  
                   manual_station_on((byte)sel_manual_ch, num_min);        // start required station in manual mode for num_min only if required time is != 0
                }
                set_mode( OSUI_MODE_HOME ); // Manual watering started, exit current UI mode changing it to HOME
                return true;
     }
// now screen update and blinking cursor
     if( (last_time != curr_time) || (forceRefresh !=0) ) {  // update UI once a second (for blinking), OR if explicit refresh is required

       last_time = curr_time;

           lcd.setCursor(0, 1);
       lcd_print_2digit(num_min);
       if( curr_time%2 ) lcd_print_pgm(PSTR("   "));
       else                      lcd_print_pgm(PSTR("#  "));
     }

  }
  return true;
}

/*
  Local config viewer. Supports multiple pages

*/
byte OSLocalUI::modeHandler_Viewconf(byte forceRefresh)
{
// assert
   if( osUI_Mode != OSUI_MODE_VIEWCONF )  return false;  // Basic protection to ensure current UI mode is correct

// initial setup, start from page 0
   if( forceRefresh == 2 ) osUI_Page = 0;

   char btn = get_button_async(0);

// handle input
   if( btn == BUTTON_MODE ){

       set_mode( OSUI_MODE_HOME); // change mode back to HOME (skip SETUP for now)
       return true;
   }
   else if( btn == BUTTON_UP ){

       if( osUI_Page < 3 ) osUI_Page++;
       else                         osUI_Page = 0;
       
       forceRefresh = 1;
   }
   else if( btn == BUTTON_DOWN ){

       if( osUI_Page > 0 ) osUI_Page--;
       else                         osUI_Page = 3;
       
       forceRefresh = 1;
   }

   if( forceRefresh != 0 ){   // entering VIEWCONF mode, refresh things

          lcd.clear();
          lcd.setCursor(0, 0);
          
          if( osUI_Page == 0 ){
            
             lcd_print_pgm(PSTR("Conf: Version"));
             lcd.setCursor(0,1);
             lcd_print_pgm(PSTR("V 2.00 (AP)"));
          }
          else if( osUI_Page == 1 ){
            
             lcd_print_pgm(PSTR("Conf: IP"));
             lcd.setCursor(0,1);
             lcd_print_ip(ether.myip);

          }
          else if( osUI_Page == 2 ){
            
             lcd_print_pgm(PSTR("Conf: Port"));
             lcd.setCursor(0,1);
             lcd.print(ether.hisport);
          }
          else if( osUI_Page == 3 ){
            
             lcd_print_pgm(PSTR("Conf: Gateway"));
             lcd.setCursor(0,1);
             lcd_print_ip(ether.gwip);

          }
   }

  return true;
}

byte OSLocalUI::modeHandler_Setup(byte forceRefresh)
{
  return true;
}




//
// Code copied from OpenSprinklerGen2.cpp

// =============
// LCD Functions
// =============


 // Print station bits
void OSLocalUI::lcd_print_station(byte line, char c) {
  //lcd_print_line_clear_pgm(PSTR(""), line);
  lcd.setCursor(0, line);
  if (svc.status.display_board == 0) {
    lcd_print_pgm(PSTR("MC:"));  // Master controller is display as 'MC'
  }
  else {
    lcd_print_pgm(PSTR("E"));
    lcd.print((int)svc.status.display_board);
    lcd_print_pgm(PSTR(":"));   // extension boards are displayed as E1, E2...
  }

  if (!svc.status.enabled) {
    lcd_print_line_clear_pgm(PSTR("-Disabled!-"), 1);
  }
  else {
    byte bitvalue = svc.station_bits[svc.status.display_board];
    for (byte s=0; s<8; s++) {
      lcd.print((bitvalue&1) ? (char)c : '_');
      bitvalue >>= 1;
    }
  }
  lcd_print_pgm(PSTR("    "));
  lcd.setCursor(15, 1);
  lcd.write(svc.status.network_fails>0?1:0);
}

 // Print stations string using provided default char, and highlight specific station using provided alt char
void OSLocalUI::lcd_print_station(byte line, char def_c, byte sel_stn, char sel_c) {
  lcd.setCursor(0, line);
  if (svc.status.display_board == 0) {
    lcd_print_pgm(PSTR("MC:"));  // Master controller is display as 'MC'
  }
  else {
    lcd_print_pgm(PSTR("E"));
    lcd.print((int)svc.status.display_board);
    lcd_print_pgm(PSTR(":"));   // extension boards are displayed as E1, E2...
  }

  for (byte s=0; s<8; s++) {
      lcd.print((s == sel_stn) ? sel_c : def_c);
  }
  lcd_print_pgm(PSTR("    "));
  lcd.setCursor(15, 1);
  lcd.write(svc.status.network_fails>0?1:0);
}


// Print a program memory string
void OSLocalUI::lcd_print_pgm(PGM_P PROGMEM str) {
  uint8_t c;
  while((c=pgm_read_byte(str++))!= '\0') {
    OSLocalUI::lcd.print((char)c);
  }
}

// Print a program memory string to a given line with clearing
void OSLocalUI::lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line) {
  lcd.setCursor(0, line);
  uint8_t c;
  int8_t cnt = 0;
  while((c=pgm_read_byte(str++))!= '\0') {
    OSLocalUI::lcd.print((char)c);
    cnt++;
  }
  for(; (16-cnt) >= 0; cnt ++) lcd_print_pgm(PSTR(" "));
}

void OSLocalUI::lcd_print_2digit(int v)
{
  lcd.print((int)(v/10));
  lcd.print((int)(v%10));
}

// Print time to a given line
void OSLocalUI::lcd_print_time(byte line)
{
  time_t t=now();
  lcd.setCursor(0, line);
  lcd_print_2digit(hour(t));

  lcd_print_pgm( t%2 > 0 ? PSTR(":") : PSTR(" ") );                     // flashing ":" in the time display

  lcd_print_2digit(minute(t));
  lcd_print_pgm(PSTR("  "));
  lcd_print_pgm(days_str[weekday_today()]);
  lcd_print_pgm(PSTR(" "));
  lcd_print_2digit(month(t));
  lcd_print_pgm(PSTR("-"));
  lcd_print_2digit(day(t));
}

int freeRam(void)
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Print free memory
void OSLocalUI::lcd_print_memory(byte line)
{
  OSLocalUI::lcd.setCursor(0, line);
  lcd_print_pgm(PSTR("Free RAM:        "));

  lcd.setCursor(9, line);
  lcd.print(freeRam());

  lcd.setCursor(15, line);
  lcd.write(svc.status.network_fails>0?1:0);
}

// print ip address 
void OSLocalUI::lcd_print_ip(const byte *ip) {
  byte i;
  
  for (i=0; i<3; i++) {
    lcd.print((int)ip[i]);
    lcd_print_pgm(PSTR("."));
  }
  lcd.print((int)ip[i]);
}

 // Index of today's weekday (Monday is 0)
byte weekday_today() {
  return ((byte)weekday()+5)%7; // Time::weekday() assumes Sunday is 1
}

