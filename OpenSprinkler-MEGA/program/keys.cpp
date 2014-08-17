#include "localUI.h"

// local forward declarations

unsigned long subt_millis(unsigned long new_millis, unsigned long old_millis);

#ifdef ANALOG_KEY_INPUT

// Analogue buttons version

// Analogue input does not require buttons_loop
void buttons_loop(void)
{
  ;
}

byte get_keys_now(void)
{
  int val = analogRead(KEY_ANALOG_CHANNEL);

  if( val < 0 )   return BUTTON_NONE; // strange input value, treat it as NONE

  if( val < 30 )  return BUTTON_CONFIRM;  // this is Right Key
//  if( val < 150 ) return BUTTON_UP;
  if( val < 360 ) return BUTTON_DOWN;
  if( val < 535 ) return BUTTON_UP;       // Left key is used as Up because the native Up key is broken
  if( val < 760 ) return BUTTON_MODE;

  return BUTTON_NONE; // strange input value, treat it as NONE
}
#else //ANALOG_KEY_INPUT

// Digital input does not require buttons_loop for now
void buttons_loop(void)
{
  ;
}

byte get_keys_now(void)
{
   byte  new_buttons = 0;

#ifdef PIN_INVERTED_BUTTONS
   if( digitalRead(PIN_BUTTON_1) != 0 ) new_buttons |= BUTTON_1;
   if( digitalRead(PIN_BUTTON_2) != 0 ) new_buttons |= BUTTON_2;
   if( digitalRead(PIN_BUTTON_3) != 0 ) new_buttons |= BUTTON_3;
   if( digitalRead(PIN_BUTTON_4) != 0) new_buttons |= BUTTON_4;

#else //PIN_INVERTED_BUTTONS
   if( digitalRead(PIN_BUTTON_1) == 0 ) new_buttons |= BUTTON_1;
   if( digitalRead(PIN_BUTTON_2) == 0 ) new_buttons |= BUTTON_2;
   if( digitalRead(PIN_BUTTON_3) == 0 ) new_buttons |= BUTTON_3;
   if( digitalRead(PIN_BUTTON_4) == 0 ) new_buttons |= BUTTON_4;
#endif

   return new_buttons;
}

#endif //ANALOG_KEY_INPUT

/*
   Main key input function. It is used to poll and read input buttons, behavior depends on mode:

   0    -       Basic input mode. In this mode function returns only key_down events, no hold or auto-repeat
   1    -       In this mode function returns key_down events and handles auto-repeat.


*/

byte get_button_async(byte mode)
{
  byte          key;
  static byte   oldkey=BUTTON_NONE;
  static byte   autorepeat_sent = 0;    // internal flag to track auto-repeat

  static byte   state = 0;                              // internal state
                                                                                // 0 - initial default, nothing pressed
                                                                                //
                                                                                // 1 - key pressed, waiting for debounce timeout.
                                                                                //     old_millis will hold timestamp, oldkey will hold the old key value
                                                                                //
                                                                                // 2 - key pressed, debounce check OK, return key value, oldkey will hold the old key value and
                                                                                //     old_millis will have the timestamp (used for HOLD detection)

   static unsigned long old_millis = 0;
   static unsigned long autorepeat_millis = 0;


  if( state == 0 )              // initial state, nothing was pressed before
  {
    key = get_keys_now();       // read key and convert it into standardized key values. Note: get_keys() returns immediate key state

    if( key == BUTTON_NONE ) return key;        // nothing
// new key detected
        state = 1;
        oldkey = key;
        old_millis = millis();

        return BUTTON_NONE;             // no keypress for now, but internal state changed
  }
  else if( state == 1 )         // we already had prior key press and are waiting for debounce
  {
    if( subt_millis(millis(), old_millis) < KEY_DEBOUNCE_DELAY ) return BUTTON_NONE;    // we are waiting for KEY_DEBOUNCE_DELAY (which is normally 50ms)
// delay expired, check new value

    key = get_keys_now(); // read key and convert it into standardized key values. Note: get_keys() returns immediate key state
    if( key != oldkey ){  // different key value, clear up things

          state = 0;    // reset state
          oldkey = BUTTON_NONE;
          return BUTTON_NONE;           // debounce check failed, return NONE
        }
// key == oldkey, so we can accept it

        state = 2;  // key accepted, but now we will be watching for HOLD time
        return key;
  }
  else if( state == 2 )         // key was previously accepted and we are watching for HOLD
  {
    key = get_keys_now(); // read key and convert it into standardized key values. Note: get_keys() returns immediate key state

    if( key != oldkey )   // something changed, release old key. NOTE: any changes in keys will be treated as key release
    {
          state = 0;
          oldkey = BUTTON_NONE;

          return BUTTON_NONE;
    }
// key == oldkey, so we continue to hold

    if( mode == 0 ){    // basic mode, no auto-repeat. Simply return NONE

        return BUTTON_NONE;
    }
    else if( mode == 1 ){    // autorepeat mode

       if( subt_millis(millis(), old_millis) < KEY_HOLD_DELAY ) return BUTTON_NONE;     // no auto-repeat until HOLD timeout expires

// Auto-repeat
       if( autorepeat_sent == 0 ){      // auto-repeat not sent yet, setup things


          autorepeat_sent = 1;
          autorepeat_millis = millis();

          return key;
       }
       else {   // auto-repeat in progress

          if( subt_millis(millis(), autorepeat_millis) > KEY_REPEAT_INTERVAL ) autorepeat_sent = 0;  // trigger new autorepeat key on the next poll

          return BUTTON_NONE;
       }

    }


    return BUTTON_NONE;  // wrong mode, treat it as if it was basic mode
  }
// we should not really get here - it could happen only if state is invalid. Reset the state

  state = 0; oldkey = BUTTON_NONE;

  return BUTTON_NONE;
}

// maximum ulong value
#define MAX_ULONG       4294967295

// Subtract two millis values and return delta
// Takes into account counter rollover
//
unsigned long subt_millis(unsigned long new_millis, unsigned long old_millis)
{
  if( new_millis > old_millis ) return (new_millis-old_millis); // main case - new is bigger than old

// new millis is smaller than old millis which means - overflow. Calculate correct value

  unsigned long delta = MAX_ULONG - old_millis; delta += new_millis;    // do math in two steps to ensure no overflow
  return delta;
}


