// Copyright 2015, Sensics, Inc.
// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: Apache-2.0
//
// latency-testing hardware firmware source code.
// Code for photosensor latency check test.
//
// Original author: Russell Taylor working for Sensics.com through ReliaSolve.com
// Ported to PlatformIO and multi-board, multi-app framework by Ryan Pavlik for Collabora.

#ifdef APP_CALIBRATE
#include <Arduino.h>
#include "defines.h"
#include "apps.h"
#include <cmath>

static bool calibrated = false;
static int on_threshold = -1;
static int off_threshold = -1;

void measureSensorLatency()
{
    // We run a finite-state machine to test the latency between when the LED is
    // turned on and when the photosensor changes its value to pass the threshold.
    // We test both the rising and falling brightness condition.

    Serial.println("Start photosensor latency measurement");
    while (true)
    {
        static enum { S_SET_ON,
                      S_MEASURE_ON,
                      S_SET_OFF,
                      S_MEASURE_OFF } state = S_SET_ON;

        // micros() will wrap around after about 7 hours of running.
        // It has an accuracy of 4 microseconds.
        static unsigned long start = 0;

        switch (state)
        {
        case S_SET_ON:
            // Turn on the LED and record when we did.
            start = micros();
            ledOn();
            state = S_MEASURE_ON;
            break;

        case S_MEASURE_ON:
            // Wait until the LED passes threshold, then report how long it
            // took and wait a bit for the printing to happen.
            if (readBrightness() >= on_threshold)
            {
                unsigned long now = micros();
                unsigned long latency = now - start;
                Serial.print("On delay (microseconds) = ");
                Serial.println(latency);
                delay(500);
                state = S_SET_OFF;
            }
            break;

        case S_SET_OFF:
            // Turn off the LED and record when we did.
            ledOff();
            start = micros();
            state = S_MEASURE_OFF;
            break;

        case S_MEASURE_OFF:
            // Wait until the LED passes threshold, then report how long it
            // took and wait it bit for the printing to happen.
            if (readBrightness() <= off_threshold)
            {
                unsigned long now = micros();
                unsigned long latency = now - start;
                Serial.print("Off delay (microseconds) = ");
                Serial.println(latency);
                delay(500);
                state = S_SET_ON;
            }
            break;
        }
    }
}
void measureLoopDelay()
{

    // Once we have calibrated, measure how long
    // it takes to do an analog read and go around
    // the loop.  This lets us know how much of the
    // latency in the photodiode measurement is due
    // to housekeeping.
    static bool loop_delay_measured = false;
    while (!loop_delay_measured)
    {
        static bool first_time = true;
        static unsigned long start;
        if (first_time)
        {
            start = micros();
            int unused [[maybe_unused]] = readBrightness();

            first_time = false;
        }
        else
        {
            unsigned long now = micros();
            unsigned long latency = now - start;
            Serial.print("Loop delay (microseconds) = ");
            Serial.println(latency);
            Serial.println();
            delay(400);
            loop_delay_measured = true;
        }
    }
}
void calibrate()
{
    // The first time through the loop, we do a calibration to find out
    // what the threshold for turning the LED on and off should be.  We
    // turn it off, wait for it to go off, and then read it.  Then back
    // on and wait, then back off.
    while (true)
    {
        ledOff();

        delay(1000);
        int dark_value = readBrightness();
        Serial.print("dark: ");
        Serial.println(dark_value);

        ledOn();
        delay(1000);
        int bright_value = readBrightness();
        Serial.print("bright: ");
        Serial.println(bright_value);

        ledOff();
        delay(1000);
        if (bright_value < dark_value) {
            Serial.println("please invert the meaning of the readBrightness command.\n");
            delay(1000);

        }
        if ((std::abs)(bright_value - dark_value) < 100)
        {
            Serial.println("Not enough brightness difference, reposition photosensor to see LED\n");
            delay(1000);
        }
        else
        {
            int tenth_gap = (bright_value - dark_value) / 10;
            on_threshold = dark_value + tenth_gap;
            off_threshold = bright_value - tenth_gap;
            Serial.print("Calibrated: on threshold = ");
            Serial.print(on_threshold);
            Serial.print(", off threshold = ");
            Serial.print(off_threshold);
            Serial.print(" (dark = ");
            Serial.print(dark_value);
            Serial.print(", bright = ");
            Serial.print(bright_value);
            Serial.println(")");
            delay(400);
            calibrated = true;
            return;
        }
    }
}
void calibrateSetup()
{
    setupAnalog();
}
//*****************************************************
void calibrateLoop(Board &board)
//*****************************************************
{
    if (!calibrated)
    {
        calibrate();
    }
    measureLoopDelay();
    measureSensorLatency();

} //end loop

#endif // APP_CALIBRATE