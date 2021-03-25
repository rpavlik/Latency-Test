// Copyright 2015, Sensics, Inc.
// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: Apache-2.0
//
// latency-testing hardware firmware source code.
// Code for "motion onset" test.
// Original author: Russell Taylor working for Sensics.com through ReliaSolve.com
// Ported to PlatformIO and multi-board, multi-app framework by Ryan Pavlik for Collabora.

#ifdef APP_ONSET
// Must come before Arduino because of abs
#include <Eigen/Core>
#include "apps.h"

using Eigen::Vector3f;

#include <Arduino.h>
#include "defines.h"
#include "gyroProc.h"

#include "motionShared.h"

GyroProc gyroProc{};

#define VERBOSE
#undef abs
// Thresholds
const int BRIGHTNESS_CHANGE_THRESHOLD = 3;
const unsigned long TIMEOUT_USEC = 1000000L;

// Keeps track of delays so we can do an average.
const int NUM_DELAYS = 16;
unsigned long delays[NUM_DELAYS];
static int count = 0, odd_count = 0, even_count = 0;

void onsetSetup()
{

    Serial.println("latency_hardware_firmware onset test v04.00.00");
    Serial.println(" Mount the photosensor rigidly on the screen.");
    Serial.println(" Move the inertial sensor along with the tracking hardware.");
    Serial.println(" Make the app change the brightness in front of the photosensor.");
    Serial.println(" Latencies reported in microseconds, 1-second timeout");
}

//*****************************************************
void onsetLoop(Board &board)
//*****************************************************
{
    // We run a finite-state machine that cycles through cases of waiting for
    // a period of non motion, detecting a sudden motion, waiting until there
    // is a change in brightness, and reporting the time passed in microseconds
    // between the motion and the brightness change.
    static enum { S_CALM,
                  S_MOTION,
                  S_BRIGHTNESS } state = S_CALM;
    static unsigned long start;
    static int initial_brightness;
    if (state == S_CALM)
    {
        startupImu(board, gyroProc);
    }

    switch (state)
    {
    case S_CALM:
    {
        // Wait for a period of at least 100 cycles where there is no motion above
        // the motion threshold.
        static int calm_cycles = 0;
        auto data = doRead(board, gyroProc);
        if (moving(data.gyro))
        {
            calm_cycles = 0;
        }
        else if (++calm_cycles >= 50)
        {
            Serial.println("waiting for calm...");
            state = S_MOTION;
            calm_cycles = 0;
        }
        else
        {
            Serial.println("Make sure we stay calm for a while");
            // Make sure we stay calm for a while (half a second)
            delay(10);
        }
    }
    break;

    case S_MOTION:
    {
        // Wait for a sudden motion.  When we find it, record the time in microseconds
        // so we can compare it to when the brightness changes.  Also record the brightness
        // so we can look for changes.
        auto data = doRead(board, gyroProc);
        if (moving(data.gyro))
        {
            start = data.timestamp;
            initial_brightness = readBrightness();
            state = S_BRIGHTNESS;
#ifdef VERBOSE
            Serial.println("Moving");
#endif
        }
    }
    break;

    case S_BRIGHTNESS:
    {
        // Wait for a change in brightness compared to the original value that
        // passes a threshold.  When we get it, report the latency.
        // If it takes too long, then we time out and start over.
        int brightness = readBrightness();
        unsigned long now = micros();

        // Keep track of how many values we got for odd and even rows and
        // compute a running average when we get a full complement for each.
        // Ignore timeout values
        if (abs(brightness - initial_brightness) > BRIGHTNESS_CHANGE_THRESHOLD)
        {
            // Print the result for this time
            Serial.println(now - start);
            if (count % 2 == 0)
            {
                odd_count++; // This is the first one (zero indexed) or off by twos
            }
            else
            {
                even_count++;
            }
            delays[count++] = now - start;
            state = S_CALM;
        }
        else if (now - start > TIMEOUT_USEC)
        {
            Serial.println("Timeout: no brightness change after motion, restarting");
            // We don't increment the counter and we set the reading to 0 so we ignore it.
            delays[count++] = 0;
            state = S_CALM;
        }

        // See if it is time to print the average result.
        if (count == NUM_DELAYS)
        {
            unsigned long even_average = 0;
            unsigned long odd_average = 0;
            for (int i = 0; i < NUM_DELAYS / 2; i++)
            {
                odd_average += delays[2 * i];
                even_average += delays[2 * i + 1];
            }

            odd_average /= odd_count;
            Serial.print("Average of last ");
            Serial.print(NUM_DELAYS / 2);
            Serial.print(" odd counts (ignoring timeouts) = ");
            Serial.println(odd_average);
            even_average /= even_count;
            Serial.print("Average of last ");
            Serial.print(NUM_DELAYS / 2);
            Serial.print(" even counts (ignoring timeouts) = ");
            Serial.println(even_average);

            count = 0;
            odd_count = 0;
            even_count = 0;
        }
    }
    break;

    default:
        Serial.println("Error: Unrecognized state; restarting");
        state = S_CALM;
        break;
    }
}
#endif