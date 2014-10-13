/*
  Sensors handling module for the Sprinklers control program.

  This module is intended to be used with my modified version of the OpenSprinkler code, as well as
  with modified Sprinklers control program sprinklers_pi.


This module handles various sensors connected to the microcontroller - Temperature, Pressure, Humidity, Waterflow etc.
Multiple sensor types are supported - BMP180 (pressure), DS18B20 (soil temperature), DHT21 (humidity and temperature) etc. 
Each sensor type is handled by its own piece of code, all sensors are wrapped into the common Sensors class.

Operation is waitless (to a possible extent). The common Sensors class is expected to be called for initialization (from setup()), and from the polling loop.

The Sensors class will handle sensors configuration and sensors reading at configured frequency. Readings are logged using external Logging class (sd-log) 
using time-series pattern. Errors are reported using common Trace infrastructure.



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

#include "sensors.h"
#include "port.h"
#include <SFE_BMP180.h>
#include <Wire.h>

// external reference
extern Logging sdlog;


// For BMP180 sensor we need SFE_BMP180 object, here called "bmp180":

SFE_BMP180 bmp180;

// maximum ulong value
#define MAX_ULONG       4294967295

// Helper inline function
//
// Subtract two millis values and return delta
// Takes into account counter rollover
//
inline unsigned long subt_millis(unsigned long new_millis, unsigned long old_millis)
{
  if( new_millis > old_millis ) return (new_millis-old_millis); // main case - new is bigger than old

  unsigned long delta = MAX_ULONG - old_millis; delta += new_millis;    // do math in two steps to ensure no overflow
  return delta;
}

// local forward declarations

char pressure_MinTimer(void);
char bmp180_Read(int *pressure, int *temperature);

// initialization. Intended to be called from setup()
//
// returns TRUE on success and FALSE otherwise
//
byte Sensors::begin(void)
{
  // Initialize the sensor (it is important to get calibration values stored on the device).

#ifdef SENSOR_ENABLE_BMP180

     if( !bmp180.begin() ){
           trace(F("BMP180 init failure.\n"));
           
           return false;
     }
#endif  //   SENSOR_ENABLE_BMP180

     return true;
}

// -- Operation --

// Main loop. Intended to be called regularly  to handle sensors readings. Usually this will be called from Arduino loop()
byte Sensors::loop(void)
{
       static unsigned long  old_millis = 0;
       unsigned long  new_millis = millis();    // Note: we are using built-in Arduino millis() function instead of now() or time-zone adjusted LocalNow(), because it is a lot faster
                                                                  // and for detecting minutes changes it does not make any difference.

//       if( subt_millis(new_millis, old_millis) >= 10000 ){   // for now let's make it 10 sec
       if( subt_millis(new_millis, old_millis) >= 60000 ){   // one minute detection

             old_millis = new_millis;
             
             pressure_MinTimer();             
       }
}


// timer worker for pressure sensors.
// This funciton will be called once a minute, allowing pressure sensors code to read sensors if required.

char pressure_MinTimer(void)
{
#ifdef SENSOR_ENABLE_BMP180

     static char minutesCounter = SENSORS_PRESSURE_DEFAULT_REPEAT;
     static char pressure_defaultRepeat = SENSORS_PRESSURE_DEFAULT_REPEAT;
     
     int  pressure, temperature;
    
     minutesCounter--;
     if( minutesCounter == 0 ){  // required repeat interval check
    
           minutesCounter = pressure_defaultRepeat;  // reload counter
           // now read BMP180 sensor
           
           if( bmp180_Read(&pressure, &temperature) == false ){
            
                trace(F("Failure reading pressure from BMP180.\n"));
                return false; 
           }
           
           sdlog.LogSensorReading(SENSOR_TYPE_TEMPERATURE, 1, temperature);    // BMP180 temperature sensor has ID=1
           sdlog.LogSensorReading(SENSOR_TYPE_PRESSURE, 1, pressure);                 // BMP180 pressure sensor ID=1
     }
#endif  //   SENSOR_ENABLE_BMP180

     return true;       
}


// Worker function to read bmp180 pressure sensor.
// Returns true if successful, false if failed.
char bmp180_Read(int *pressure, int *temperature)
{
           double T,P;

           char  status = bmp180.startTemperature();
           if (status != 0){

               delay(status);  // Wait for the measurement to complete

               // Retrieve the completed temperature measurement:
               // Note that the measurement is stored in the variable T.
               // Function returns 1 if successful, 0 if failure.

               status = bmp180.getTemperature(T);
               if (status != 0){
    
                    *temperature = (int) ((9.0/5.0)*T+32.5);    // Sensor reads temp in C, but we want to return temperature in F, also converting it to integer (note 0.5 correction for rounding).
      
                    // Start pressure measurement:
                    // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
                    // If request is successful, the number of ms to wait is returned.
                    // If request is unsuccessful, 0 is returned.

                    status = bmp180.startPressure(1);    // accuracy 1 which should be relatively fast.
                    if (status != 0){
     
                         delay(status); // Wait for the measurement to complete

                         // Note that the function requires the previous temperature measurement (T).

                         status = bmp180.getPressure(P,T);      // Function returns 1 if successful, 0 if failure.
                         if (status != 0)   *pressure = (int) (P + 0.5);  // convert it to int. Note 0.5 correction for rounding
                         else                   return false;  // error retrieving pressure measurement
                    }
                    else return false;   // error starting pressure measurement
               }
               else return false; // error retrieving temperature measurement
           }
           else return false;  // error starting temperature measurement

           return  true;
}
