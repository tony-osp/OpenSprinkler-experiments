/*
	Local UI (for local LCD and buttons) on OpenSprinkler
	***AP***

*/
#ifndef _OSLocalUI_h
#define _OSLocalUI_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "defines.h"
#include <Time.h>
#include <LiquidCrystal.h>

// global states definitions

// Modes
// Modes numbering starts from zero, and we could have up to 255 Modes, with the mode value of 255 reserved as "undefined"
#define OSUI_MODE_UNDEFINED		255		// undefined
#define OSUI_MODE_HOME			0		// Home screen
#define OSUI_MODE_MANUAL		        1		// Manual mode
#define OSUI_MODE_VIEWCONF		2		// View Config mode
#define OSUI_MODE_SETUP			3		// Setup mode

// Pages
// Pages numbering starts from zero, and each Mode has its own set of pages
#define OSUI_PAGE_UNDEFINED		255


// Overall UI state
#define OSUI_STATE_UNDEFINED	255
#define OSUI_STATE_SUSPENDED	0
#define OSUI_STATE_ENABLED		1

// Button meaning

// nothing pressed
#define BUTTON_NONE   0

#define BUTTON_MODE		BUTTON_1
#define BUTTON_UP		        BUTTON_2
// note: button 3 is the optional button
#define BUTTON_DOWN		BUTTON_3
#define BUTTON_CONFIRM	        BUTTON_4

class OSLocalUI {
public:

  // ====== Member Functions ======
  // -- Setup --
  static byte begin(void);              		// initialization. Intended to be called from setup()

    // -- Operation --
  static byte loop(void);               		// Main loop. Intended to be called regularly and frequently to handle input and UI. Normally this will be called from Arduino loop()
  static byte refresh(void);            		// Force UI refresh.
  static byte suspend(void);            		// Stop UI updates and input handling (useful when taking over the screen for custom output)
  static byte resume(void);						// Resume UI operation

  static byte set_mode(char mode);   // Switch UI to desired Mode
  static void lcd_print_pgm(PGM_P PROGMEM str);
  static void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line);    // Print a program memory string to a given line with clearing
  static void lcd_print_2digit(int v);

// Data
  static LiquidCrystal lcd;		// Main LCD object. We have to expose this object to allow custom code access to LCD bypassing UI

  static byte osUI_State;
  static byte osUI_Mode;
  static byte osUI_Page;

private:

// internal stuff

  static void lcd_print_time(byte line);                                 // Print time to a given line
  static void lcd_print_memory(byte line);                          // Print free memory
  static void lcd_print_ip(const byte *ip);                            // print ip address and port
  static void lcd_print_station(byte line, char c);
  static void lcd_print_station(byte line, char def_c, byte sel_stn, char sel_c);

  
// Mode handlers.
// Normally called from loop(), parameter:   0 - regular loop call, no special handling 
//                                                                  1 - force UI refresh     (i.e. screen may be corrupted)
//                                                                  2 - initial mode entry, setup things and paint the screen
//
  static byte modeHandler_Home(byte forceRefresh);
  static byte modeHandler_Manual(byte forceRefresh);
  static byte modeHandler_Viewconf(byte forceRefresh);
  static byte modeHandler_Setup(byte forceRefresh);
  
  static byte callHandler(byte needs_refresh);

};

#endif

