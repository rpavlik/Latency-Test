// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik
//
// Skeleton "main" file for latency test apps

// Must come before Arduino because of abs
#include <Eigen/Core>
#include "apps.h"
using Eigen::Vector3f;

#include <Arduino.h>
#include "defines.h"

Board board{};

void setup()
{
    Serial.begin(115200);

    // Pause until serial console on devices with native USB.
    while (!Serial)
        delay(10);

    // Try to initialise and warn if we couldn't detect the chip
    if (!board.begin())
    {
        Serial.println("Oops ... unable to initialize the IMU. Check your wiring!");
        while (1)
            ;
    }

#ifndef HAVE_LOOP_METHOD
#if defined(APP_ONSET)
    onsetSetup();
#elif defined(APP_TURNAROUND)
    turnaroundSetup();
#elif defined(APP_LOG)
    logSetup();
#endif
#endif
}

void loop()
{
#ifdef HAVE_LOOP_METHOD
    board.loop();
#else
#if defined(APP_ONSET)
    onsetLoop(board);
#elif defined(APP_CALIBRATE)
    calibrateLoop(board);
#elif defined(APP_IMUTEST)
    imutestLoop(board);
#elif defined(APP_TURNAROUND)
    turnaroundLoop(board);
#elif defined(APP_LOG)
    logLoop(board);
#else
#error "not sure what app you want"
#endif
#endif
}