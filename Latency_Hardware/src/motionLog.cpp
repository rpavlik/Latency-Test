// Copyright 2015, Sensics, Inc.
// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: Apache-2.0
//
// Code for "motion log" test app.
// Original author of the motion_onset code this was based on: Russell Taylor working for Sensics.com through ReliaSolve.com
// Ported to PlatformIO and multi-board, multi-app framework by Ryan Pavlik for Collabora.
#ifdef APP_LOG
// Must come before Arduino because of abs
#include <Eigen/Core>
#include "apps.h"

using Eigen::Vector3f;

#include <Arduino.h>
#include "defines.h"
#include "gyroProc.h"

#include "motionShared.h"

GyroProc gyroProc{};


//*****************************************************
void logSetup()
//*****************************************************
{
    Serial.println("latency_hardware_firmware motion log test v01.00.00");
    Serial.println(" Mount the photosensor rigidly on the eyepiece or screen.");
    Serial.println(" Rotate the inertial sensor along with the tracking hardware.");
    Serial.println(" Make the app vary brightness darker in one direction");
    Serial.println(" and lighter in the other.");
    Serial.println(" Hold the device still for 2 seconds.");

    delay(100);
    Serial.println("us,drx,dry,drz,brightness");
}

//*****************************************************
void logLoop(Board &board)
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
    Serial.print(now);
    Serial.print(",");
    Serial.print(data.gyro.x());
    Serial.print(",");
    Serial.print(data.gyro.y());
    Serial.print(",");
    Serial.print(data.gyro.z());
    Serial.print(",");
    Serial.println(brightness);
}

#endif
