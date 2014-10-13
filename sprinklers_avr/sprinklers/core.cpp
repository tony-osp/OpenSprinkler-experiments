// core.cpp
// This file constitutes the core functions that run the scheduling for the Sprinkler system.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "core.h"
#include "settings.h"
#include "Weather.h"
#include "web.h"
#include "Event.h"
#include "port.h"
#include <stdlib.h>
#include "sensors.h"
#ifdef ARDUINO
#include "tftp.h"
static tftp tftpServer;
#else
#include <wiringPi.h>
#include <unistd.h>
#endif

// Core modules

#ifdef LOGGING
Logging sdlog;
#endif
static web webServer;
nntp nntpTimeServer;
runStateClass runState;
Sensors sensorsModule;

// A bitfield that defines which zones are currently on.
int ZoneState = 0;

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


runStateClass::runStateClass() : m_bSchedule(false), m_bManual(false), m_iSchedule(-1), m_zone(-1), m_endTime(0), m_eventTime(0)
{
}

void runStateClass::LogSchedule()
{
#ifdef LOGGING
        if ((m_eventTime > 0) && (m_zone >= 0))
                sdlog.LogZoneEvent(m_eventTime, m_zone, nntpTimeServer.LocalNow() - m_eventTime, m_bSchedule ? m_iSchedule+1:-1, m_adj.seasonal, m_adj.wunderground);
#endif
}

void runStateClass::SetSchedule(bool val, int8_t iSched, const runStateClass::DurationAdjustments * adj)
{
        LogSchedule();
        m_bSchedule = val;
        m_bManual = false;
        m_zone = -1;
        m_endTime = 0;
        m_iSchedule = val?iSched:-1;
        m_eventTime = nntpTimeServer.LocalNow();
        m_adj = adj?*adj:DurationAdjustments();
}

void runStateClass::ContinueSchedule(int8_t zone, short endTime)
{
        LogSchedule();
        m_bSchedule = true;
        m_bManual = false;
        m_zone = zone;
        m_endTime = endTime;
        m_eventTime = nntpTimeServer.LocalNow();
}

void runStateClass::SetManual(bool val, int8_t zone)
{
        LogSchedule();
        m_bSchedule = false;
        m_bManual = val;
        m_zone = zone;
        m_endTime = 0;
        m_iSchedule = -1;
        m_eventTime = nntpTimeServer.LocalNow();
        m_adj=DurationAdjustments();
}

#ifdef ARDUINO
uint8_t ZoneToIOMap[] = {31, 41, 40, 42, 43, 44, 45, 46, 47, 38, 37, 36, 35, 34, 33, 32};
#define SR_CLK_PIN  30
#define SR_NOE_PIN  29
#define SR_DAT_PIN  28
#define SR_LAT_PIN  27
#else
uint8_t ZoneToIOMap[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
#define SR_CLK_PIN  7
#define SR_NOE_PIN  0
#define SR_DAT_PIN  2
#define SR_LAT_PIN  3
#endif

static uint16_t outState;
static uint16_t prevOutState;

static void io_latch()
{
        // check if things have changed
        if (outState == prevOutState)
                return;

        const EOT eot = GetOT();
        switch (eot)
        {
        case OT_NONE:
                break;
        case OT_DIRECT_POS:
        case OT_DIRECT_NEG:
                for (int i = 0; i <= NUM_ZONES; i++)
                {
                        if (eot == OT_DIRECT_POS)
                                digitalWrite(ZoneToIOMap[i], (outState&(0x01<<i))?1:0);
                        else
                                digitalWrite(ZoneToIOMap[i], (outState&(0x01<<i))?0:1);
                }
                break;

        case OT_OPEN_SPRINKLER:
#ifndef ARDUINO
                // turn off the latch pin
                digitalWrite(SR_LAT_PIN, 0);
                digitalWrite(SR_CLK_PIN, 0);

                for (uint8_t i = 0; i < 16; i++)
                {
                        digitalWrite(SR_CLK_PIN, 0);
                        digitalWrite(SR_DAT_PIN, outState&(0x01<<(15-i)));
                        digitalWrite(SR_CLK_PIN, 1);
                }
                // latch the outputs
                digitalWrite(SR_LAT_PIN, 1);

                // Turn off the NOT enable pin (turns on outputs)
                digitalWrite(SR_NOE_PIN, 0);
#endif
                break;
        }

        // Now store the new output state so we know if things have changed
        prevOutState = outState;
}

void io_setup()
{
        const EOT eot = GetOT();
        if ((eot != OT_NONE))
        {

#ifndef ARDUINO
                if (geteuid() != 0)
                {
                        trace("You need to be root to run this.  Setting output mode to NONE\n");
                        SetOT(OT_NONE);
                        return;
                }
                else if (wiringPiSetup() == -1)
                {
                        trace("Failed to Setup Outputs\n");
                }
#endif
                if (eot == OT_OPEN_SPRINKLER)
                {
                        pinMode(SR_CLK_PIN, OUTPUT);
                        digitalWrite(SR_CLK_PIN, 0);
                        pinMode(SR_NOE_PIN, OUTPUT);
                        digitalWrite(SR_NOE_PIN, 0);
                        pinMode(SR_DAT_PIN, OUTPUT);
                        digitalWrite(SR_DAT_PIN, 0);
                        pinMode(SR_LAT_PIN, OUTPUT);
                        digitalWrite(SR_LAT_PIN, 0);
                }
                else
                {
                        for (uint8_t i=0; i<sizeof(ZoneToIOMap); i++)
                        {
                                pinMode(ZoneToIOMap[i], OUTPUT);
                                digitalWrite(ZoneToIOMap[i], (eot==OT_DIRECT_NEG)?1:0);
                        }
                }
        }
        outState = 0;
        prevOutState = 1;
        io_latch();
}


void TurnOffZones()
{
        trace(F("Turning Off All Zones\n"));
        outState = 0;
}

bool isZoneOn(int iNum)
{
        if ((iNum <= 0) || (iNum > NUM_ZONES))
                return false;
        return outState & (0x01 << iNum);
}

int ActiveZoneNum(void)
{
        if( outState == 0 ) return -1;      // nothing is running

        for( byte n=1; n<=NUM_ZONES; n++){          // note: zones are numbered from 1. Slot 0 is used for the common pump.

             if(  outState & (0x01 << n) ) return n;   // we found active zone, return its index
        }

        return -1;  // strange, we really should not get there - it could happen only when pump is On, but all zones are off. Treat this condition as Off.
}

static void pumpControl(bool val)
{
        if (val)
                outState |= 0x01;
        else
                outState &= ~0x01;
}

void TurnOnZone(int iValve)
{
        trace(F("Turning on Zone %d\n"), iValve);
        if ((iValve <= 0) || (iValve > NUM_ZONES))
                return;

        ShortZone zone;
        LoadShortZone(iValve - 1, &zone);
        outState = 0x01 << iValve;
        // Turn on the pump if necessary
        pumpControl(zone.bPump);
}

// Adjust the durations based on atmospheric conditions
static runStateClass::DurationAdjustments AdjustDurations(Schedule * sched)
{
        runStateClass::DurationAdjustments adj(100);
        if (sched->IsWAdj())
        {
                Weather w;
                char key[17];
                GetApiKey(key);
                char pws[12] = {0};
                GetPWS(pws);
                adj.wunderground = w.GetScale(GetWUIP(), key, GetZip(), pws, GetUsePWS());   // factor to adjust times by.  100 = 100% (i.e. no adjustment)
        }
        adj.seasonal = GetSeasonalAdjust();
        long scale = ((long)adj.seasonal * (long)adj.wunderground) / 100;
        for (uint8_t k = 0; k < NUM_ZONES; k++)
                sched->zone_duration[k] = min(((long)sched->zone_duration[k] * scale + 50) / 100, 254);
        return adj;
}

// return true if the schedule is enabled and runs today.
static inline bool IsRunToday(const Schedule & sched, time_t time_now)
{
        if ((sched.IsEnabled())
                        && (((sched.IsInterval()) && ((elapsedDays(time_now) % sched.interval) == 0))
                                        || (!(sched.IsInterval()) && (sched.day & (0x01 << (weekday(time_now) - 1))))))
                return true;
        return false;
}

// Load the on/off events for a specific schedule/time or the quick schedule
void LoadSchedTimeEvents(int8_t sched_num, bool bQuickSchedule)
{
        Schedule sched;
        runStateClass::DurationAdjustments adj;
        if (!bQuickSchedule)
        {
                const uint8_t iNumSchedules = GetNumSchedules();
                if ((sched_num < 0) || (sched_num >= iNumSchedules))
                        return;
                LoadSchedule(sched_num, &sched);
                adj=AdjustDurations(&sched);
        }
        else
                sched = quickSchedule;

        const time_t local_now = nntpTimeServer.LocalNow();
        short start_time = (local_now - previousMidnight(local_now)) / 60;

        for (uint8_t k = 0; k < NUM_ZONES; k++)
        {
                ShortZone zone;
                LoadShortZone(k, &zone);
                if (zone.bEnabled && (sched.zone_duration[k] > 0))
                {
                        if (iNumEvents >= MAX_EVENTS - 1)
                        {  // make sure we have room for the on && the off events.. hence the -1
                                trace(F("ERROR: Too Many Events!\n"));
                        }
                        else
                        {
                                events[iNumEvents].time = start_time;
                                events[iNumEvents].command = 0x01; // Turn on a zone
                                events[iNumEvents].data[0] = k + 1; // Zone to turn on
                                events[iNumEvents].data[1] = (start_time + sched.zone_duration[k]) >> 8;
                                events[iNumEvents].data[2] = (start_time + sched.zone_duration[k]) & 0x00FF;
                                iNumEvents++;
                                start_time += sched.zone_duration[k];
                        }
                }
        }
        // Load up the last turn off event.
        events[iNumEvents].time = start_time;
        events[iNumEvents].command = 0x02; // Turn off all zones
        events[iNumEvents].data[0] = 0;
        events[iNumEvents].data[1] = 0;
        events[iNumEvents].data[2] = 0;
        iNumEvents++;
        runState.SetSchedule(true, bQuickSchedule?99:sched_num, &adj);
}

void ClearEvents()
{
        iNumEvents = 0;
        runState.SetSchedule(false);
}

// TODO:  Schedules that go past midnight!
//  Pretty simple.  When we one-shot at midnight, check to see if any outstanding events are at time >1400.  If so, move them
//  to the top of the event stack and subtract 1440 (24*60) from their times.

// Loads the events for the current day
void ReloadEvents(bool bAllEvents)
{
        ClearEvents();
        TurnOffZones();

        // Make sure we're running now
        if (!GetRunSchedules())
                return;

        const time_t time_now = nntpTimeServer.LocalNow();
        const uint8_t iNumSchedules = GetNumSchedules();
        for (uint8_t i = 0; i < iNumSchedules; i++)
        {
                Schedule sched;
                LoadSchedule(i, &sched);
                if (IsRunToday(sched, time_now))
                {
                        // now load up events for each of the start times.
                        for (uint8_t j = 0; j <= 3; j++)
                        {
                                const short start_time = sched.time[j];
                                if (start_time != -1)
                                {
                                        if (!bAllEvents && (start_time <= (long)(time_now - previousMidnight(time_now))/60 ))
                                                continue;
                                        if (iNumEvents >= MAX_EVENTS)
                                        {
                                                trace(F("ERROR: Too Many Events!\n"));
                                        }
                                        else
                                        {
                                                events[iNumEvents].time = start_time;
                                                events[iNumEvents].command = 0x03;  // load events for schedule i, time j
                                                events[iNumEvents].data[0] = i;
                                                events[iNumEvents].data[1] = j;
                                                events[iNumEvents].data[2] = 0;
                                                iNumEvents++;
                                        }
                                }
                        }
                }
        }
}

// Check to see if there are any events that need to be processed.
static void ProcessEvents()
{
        const time_t local_now = nntpTimeServer.LocalNow();
        const short time_check = (local_now - previousMidnight(local_now)) / 60;
        for (uint8_t i = 0; i < iNumEvents; i++)
        {
                if (events[i].time == -1)
                        continue;
                if (time_check >= events[i].time)
                {
                        switch (events[i].command)
                        {
                        case 0x01:  // turn on valves in data[0]
                                TurnOnZone(events[i].data[0]);
                                runState.ContinueSchedule(events[i].data[0], events[i].data[1] << 8 | events[i].data[2]);
                                events[i].time = -1;
                                break;
                        case 0x02:  // turn off all valves
                                TurnOffZones();
                                runState.SetSchedule(false);
                                events[i].time = -1;
                                break;
                        case 0x03:  // load events for schedule(data[0]) time(data[1])
                                if (runState.isSchedule())  // If we're already running a schedule, push this off 1 minute
                                        events[i].time++;
                                else
                                {
                                        // Load all the individual events for the individual zones on/off
                                        LoadSchedTimeEvents(events[i].data[0]);
                                        events[i].time = -1;
                                }
                                break;
                        };
                }
        }
}

void mainLoop()
{
        static bool firstLoop = true;
        static bool bDoneMidnightReset = false;
        if (firstLoop)
        {
                firstLoop = false;
                freeMemory();

                if (IsFirstBoot())
                        ResetEEPROM();
                io_setup();

                sensorsModule.begin();  // start sensors module
                
                TurnOffZones();
                ClearEvents();

                //Init the web server
                if (!webServer.Init())
                        exit(EXIT_FAILURE);

#ifdef ARDUINO
                //Init the TFTP server
                tftpServer.Init();
#endif

                // Set the clock.
                nntpTimeServer.checkTime();

                ReloadEvents();
                //ShowSockStatus();
#ifdef LOGGING
                sdlog.begin(PSTR("System started."));
#endif

        }

// Optimization - following code needs to be executed regularly but high frequency is not required.
// To preserve system resources let's make it execute once per second.

       static unsigned long  old_millis = 0;
       unsigned long  new_millis = millis();    // Note: we are using built-in Arduino millis() function instead of now() or time-zone adjusted LocalNow(), because it is a lot faster
                                                               // and for detecting second change it does not make any difference.

       if( subt_millis(new_millis, old_millis) >= 1000 ){   

             old_millis = new_millis;

             // Check to see if we need to set the clock and do so if necessary.
             nntpTimeServer.checkTime();

             const time_t timeNow = nntpTimeServer.LocalNow();
             // One shot at midnight
             if ((hour(timeNow) == 0) && !bDoneMidnightReset)
             {
                     trace(F("Reloading Midnight\n"));
                     bDoneMidnightReset = true;
                     // TODO:  outstanding midnight events.  See other TODO for how.
                     ReloadEvents(true);
             }
             else if (hour(timeNow) != 0)
                     bDoneMidnightReset = false;

             sensorsModule.loop();  // read and process sensors. Note: sensors module has its own scheduler.
                     
        }  // one-second block
        
        //  See if any web clients have connected
        webServer.ProcessWebClients();

        // Process any pending events.
        ProcessEvents();

#ifdef ARDUINO
        // Process the TFTP Server
        tftpServer.Poll();
#else
        // if we've changed the settings, store them to disk
        EEPROM.Store();
#endif

        // latch any output modifications
        io_latch();
}

