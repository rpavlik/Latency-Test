// Copyright 2015, Sensics, Inc.
// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: Apache-2.0
//
// latency-testing hardware firmware source code.
// Code for "motion turn-around" test.
//
// Original author: Russell Taylor working for Sensics.com through ReliaSolve.com
// Ported to PlatformIO and multi-board, multi-app framework by Ryan Pavlik for Collabora.

#ifdef APP_TURNAROUND
// Must come before Arduino because of abs
#include <Eigen/Core>
#include "apps.h"

using Eigen::Array3f;
using Eigen::Vector3f;

#include <Arduino.h>
#include "defines.h"
#include "gyroProc.h"

#include "motionShared.h"

GyroProc gyroProc{};

// Debugging definitions.
#undef VERBOSE
#undef VERBOSE2
#define PRINT_TRACE

#undef abs

// Thresholds
const int ACCEL_CHANGE_THRESHOLD = 500;
const float GYRO_MIN_SPEED_THRESHOLD = 0.75f;
const int BRIGHTNESS_THRESHOLD = 5;
const float GYRO_CALIBRATION_THRESHOLD = 4.0f;
const int BRIGHTNESS_CALIBRATION_THRESHOLD = 7;
const unsigned long TIMEOUT_USEC = 2000000L;
const unsigned long CALIBRATE_USEC = 1000000L;

// Arrays to hold measurements surrounding the reversal for debugging
#ifdef PRINT_TRACE
const int TRACE_SIZE = 250;
const int TRACE_SKIP = 5;
static int trace_count = 0;
static int trace_skip_count = 0;
float trace_gyroY[TRACE_SIZE];
unsigned char trace_bright[TRACE_SIZE];
#endif


// Determine if we are moving in the positive direction above
// threshold (1), in the negative direction below threshold (-1),
// or within threshold of the origin (0);
//*****************************************************
inline int gyro_direction(int value)
//*****************************************************
{
  if (value >= GYRO_THRESHOLD) {
    return 1;
  }
  
  if (value <= -GYRO_THRESHOLD) {
    return -1;
  }
  
  return 0;
}

// Determine if we brightness changed in the positive direction above
// threshold (1), in the negative direction below threshold (-1),
// or not (0);
//*****************************************************
inline int brightness_direction(int value)
//*****************************************************
{
  if (value >= BRIGHTNESS_THRESHOLD) {
    return 1;
  }
  
  if (value <= -BRIGHTNESS_THRESHOLD) {
    return -1;
  }
  
  return 0;
}


//*****************************************************
void turnaroundSetup()
//*****************************************************
{
    Serial.println("latency_hardware_firmware turnaround test v02.00.00");
    Serial.println(" Mount the photosensor rigidly on the eyepiece or screen.");
    Serial.println(" Rotate the inertial sensor along with the tracking hardware.");
    Serial.println(" Make the app vary brightness darker in one direction");
    Serial.println(" and lighter in the other.");
    Serial.println(" Latencies reported in microseconds, 2-second timeout");
    Serial.println(" Hold the device still for 2 seconds.");

    delay(100);
}

//*****************************************************
void turnaroundLoop(Board &board)
//*****************************************************
{
    startupImu(board, gyroProc);

    // Read the values from the inertial sensors and photosensor.
    // Record the time we read these values.
    auto data = doRead(board, gyroProc);
    if (!data.dataGood)
    {
        return;
    }
    int brightness = readBrightness();
    unsigned long now = data.timestamp;

#ifdef VERBOSE2
    /* Debugging info to help figure out what the thresholds should be */
    Serial.print(data.gyro.x());
    Serial.print(" ");
    Serial.print(data.gyro.y());
    Serial.print(" ");
    Serial.print(data.gyro.z());
    Serial.print(", ");
    Serial.println(brightness);
    delay(1000);
#endif

    // Keep track of the reversals in brightness and the time at
    // which the brightness reached within threshold of the value
    // it held before the inverted jump.  We keep track of the
    // previous change away from threshold and check it against
    // a current change to see if the polarities are in the opposite
    // direction.  If so, then we set last_brightness_reach_end_time to
    // now.  Brightness change direction and last brightness to
    // reach the end should both be set to 0 when entering the
    // S_REVERSE_BRIGHTNESS state so that they will have to be
    // filled in by actual changes.
    static int last_brightness_change_direction = 0;
    static unsigned long int last_brightness_reach_end_time = 0;

    // Keep track of when the last time the gyroscope's motion
    // dropped below threshold.  Used by the brightness state to
    // determine latency.  Set to 0 when entering the S_REVERSE_DIRECTION
    // state.
    static unsigned long last_gyroscope_settling_time = 0;

    // We run a finite-state machine that cycles through cases of waiting for
    // a period of non motion, determining the axis and brightness direction
    // associated with motion, looking for extrema of motion, looking for
    // extrema of brightness, and reporting statistics.
    static enum { S_CALM,
                  S_CALIBRATE,
                  S_REVERSE_DIRECTION,
                  S_REVERSE_BRIGHTNESS } state = S_CALM;
    static unsigned long calm_start = now;
    static unsigned long calibration_start = now;

    // Which axis to use for tracking the motion?  Initially set to
    // -1 and then set in the calibration code to the axis with the
    // largest motion.
    static int tracking_axis_index = -1;

    // Statistics of latencies.
    static int latency_count = 0;
    static unsigned long latency_sum = 0;
    static unsigned long latency_max = 0;
    static unsigned long latency_min = TIMEOUT_USEC;

    // Whether or not we're in calm mode, if we hold still for the
    // timeout duration, we reset statistics and go into calibrate
    // mode.
    if (moving(data.gyro))
    {
        calm_start = now;
    }
    else if (now - calm_start >= TIMEOUT_USEC)
    {
        if (latency_count > 0)
        {
            Serial.print("Min latency: ");
            Serial.println(latency_min);
            Serial.print("Max latency: ");
            Serial.println(latency_max);
            Serial.print("Mean latency: ");
            Serial.println(latency_sum / latency_count);
        }
        Serial.println("Statistics reset.  Initiate periodic motion to calibrate.");

        latency_count = 0;
        latency_sum = 0;
        latency_max = 0;
        latency_min = TIMEOUT_USEC;

        calm_start = now;
        calibration_start = now;
        tracking_axis_index = -1;
        state = S_CALIBRATE;
    }

#ifdef PRINT_TRACE
    if ((state != S_CALIBRATE) && (state != S_CALM))
    {
        if (++trace_skip_count >= TRACE_SKIP)
        {
            trace_skip_count = 0;
            if (trace_count < TRACE_SIZE)
            {
                trace_gyroY[trace_count] = data.gyro.y();
                trace_bright[trace_count] = brightness / 4;
                trace_count++;
            }
        }
    }
#endif

    switch (state)
    {
    // Wait for a period where there is no motion above
    // the motion threshold.
    case S_CALM:
    {
        // Nothing extra to do; we're always checking for calm above.
    }
    break;

    // Determine which of the rotational axes is moving the most and
    // then wait for the start of a move in the direction that makes
    // the photosensor brighter.
    case S_CALIBRATE:
    {
        // The first loop iteration, we reset our variables.
        static Array3f minGyro, maxGyro;
        static int minBright, maxBright;
        if (calibration_start == now)
        {
            minGyro = data.gyro.array();
            maxGyro = data.gyro.array();
            minBright = maxBright = brightness;
        }

        // Keep track of the maximum and minimum in all 3 axes;
        minGyro = minGyro.min(data.gyro.array());
        maxGyro = maxGyro.max(data.gyro.array());
        if (brightness < minBright)
        {
            minBright = brightness;
        }
        if (brightness > maxBright)
        {
            maxBright = brightness;
        }

        // If it has been long enough, and the tracking axis has not yet been
        // set, set it to the one that has the largest range.
        // First, check to make sure we've seen sufficient change in the
        // maximum axis and in the brightness.
        if ((tracking_axis_index < 0) && (now - calibration_start >= CALIBRATE_USEC))
        {
            Array3f range = maxGyro - minGyro;
            size_t index = 0;
            float maxRange = range.maxCoeff(&index);
            tracking_axis_index = index;
            if ((maxRange < GYRO_CALIBRATION_THRESHOLD) ||
                (maxBright - minBright < BRIGHTNESS_CALIBRATION_THRESHOLD))
            {
                Serial.println("Insufficient change for calibrating, retrying");
                Serial.print("  Gyroscope difference = ");
                Serial.print(maxRange);
                Serial.println("");
                Serial.print("  Brightness difference = ");
                Serial.println(maxBright - minBright);
                tracking_axis_index = -1;
                calibration_start = now;
            }
            else
            {
                // We have not yet dropped below motion threshold
                // for this iteration.
                last_gyroscope_settling_time = 0;
                state = S_REVERSE_DIRECTION;
                Serial.print("Calibration complete, measuring latencies on ");
                constexpr const char * axis[] = {"X axis", "Y axis", "Z axis"};
                Serial.print(axis[tracking_axis_index]);
                Serial.print(" (brightness difference ");
                Serial.print(maxBright - minBright);
                Serial.println(")");
                Serial.println("Continue periodic motion to test latency.");
            }
        }
    }
    break;

    // Wait until the motion settles down (so that we were moving
    // but now have stopped.
    case S_REVERSE_DIRECTION:
    {
        // Don't say that we were moving until we get to a speed that
        // is above a larger threshold to keep us from getting lots
        // of spurious false stopping estimates.
        static bool have_moved_fast = false;

        // Keep track of when the gyroscope value most recently
        // dropped below threshold (was above threshold in the previous
        // time step and dropped below threshold this time).
        static float last_gyroscope_value = 0;
        if (tracking_axis_index >= 0)
        {
            float tracking_axis = data.gyro[tracking_axis_index];
            if ((tracking_axis > GYRO_MIN_SPEED_THRESHOLD) ||
                (tracking_axis < -GYRO_MIN_SPEED_THRESHOLD))
            {
                have_moved_fast = true;
            }
            if (have_moved_fast &&
                (gyro_direction(tracking_axis) == 0) &&
                (gyro_direction(last_gyroscope_value) != 0))
            {
                last_gyroscope_settling_time = now;
            }
            last_gyroscope_value = tracking_axis;
        }
        else
        {
            last_gyroscope_value = 0;
        }

        if (last_gyroscope_settling_time != 0)
        {
            //Serial.println("Stopped");

            // We have dropped to a stop, so we need to set the trigger for
            // moving fast again and make sure we actually move before the
            // last gyro value registers a move.
            have_moved_fast = false;
            last_gyroscope_value = 0;

            // Flush the change status so that we have to get a brightness change
            // in one direction followed by a change in the other starting now.
            last_brightness_reach_end_time = 0;
            last_brightness_change_direction = 0;
            state = S_REVERSE_BRIGHTNESS;
        }
    }
    break;

    // Wait until the brightness changes direction and then report
    // the time difference between when motion dropped below
    // threshold and when brightness came within threshold of its
    // extreme value before reversing direction.
    case S_REVERSE_BRIGHTNESS:
    {
        static int last_unchanged_brightness_value = 0;
        static unsigned long last_brightness_change_time = 0;
        int this_change = brightness_direction(brightness - last_unchanged_brightness_value);
        if (this_change != 0)
        {
            if (this_change * last_brightness_change_direction == -1)
            {
                last_brightness_reach_end_time = last_brightness_change_time;
                /*
            if (this_change == -1) {
              Serial.print(" +");
            } else {
              Serial.print(" -");
            }
            Serial.print(last_unchanged_brightness_value);
            Serial.print("->");
            Serial.println(brightness);
            */
            }
            last_unchanged_brightness_value = brightness;
            last_brightness_change_direction = this_change;
            last_brightness_change_time = now;
        }

        if (last_brightness_reach_end_time != 0)
        {
            if (last_brightness_reach_end_time <= last_gyroscope_settling_time)
            {
                Serial.print("Error: Inverted settling times: gyro settling time ");
                Serial.print(last_gyroscope_settling_time);
                Serial.println(" (resetting)");
                state = S_CALM;
            }

#ifdef PRINT_TRACE
            Serial.println("GyroY/100*100,brightness/4*4");
            for (int i = 0; i < trace_count; i++)
            {
                Serial.print(static_cast<int>(trace_gyroY[i]) * 10);
                Serial.print(",");
                Serial.println(static_cast<int>(trace_bright[i]) * 4);
                Serial.println("");
            }
            Serial.println();
            trace_count = 0;
#endif
            unsigned long value = last_brightness_reach_end_time - last_gyroscope_settling_time;
            Serial.println(value);

            // Track statistics;
            latency_count++;
            if (value < latency_min)
            {
                latency_min = value;
            }
            if (value > latency_max)
            {
                latency_max = value;
            }
            latency_sum += value;

            last_gyroscope_settling_time = 0;
            state = S_REVERSE_DIRECTION;
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
