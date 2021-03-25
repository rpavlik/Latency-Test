// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik
//
// IMU test application for the latency tester hardware,
// based on the original design for the OSVR latency tester.


#ifdef APP_IMUTEST
// Must come before Arduino because of abs
#include <Eigen/Core>
#include "apps.h"

using Eigen::Vector3f;

#include <Arduino.h>
#include "defines.h"

#include "gyroProc.h"


bool ledState = false;
unsigned long lastTimestamp = 0;

GyroProc gyroProc;

void imutestLoop(Board& board)
{
    unsigned long timestamp;
    sensors_event_t g;
    if (!board.getGyroData(&timestamp, &g))
    {
        Serial.println("Read failed!?");
        delay(10);
        return;
    }
    bool dataGood = false;
    Vector3f processed;
    std::tie(dataGood, processed) = gyroProc.process(g);
    if (dataGood)
    {

        Serial.print("Gyro X: ");
        Serial.print(processed.x());
        Serial.print(" rad/s");
        Serial.print("\tY: ");
        Serial.print(processed.y());
        Serial.print(" rad/s");
        Serial.print("\tZ: ");
        Serial.print(processed.z());
        Serial.println(" rad/s");

        Serial.println();
        Serial.print("Elapsed time since last reading: ");
        auto elapsed = timestamp - lastTimestamp;
        Serial.print(elapsed);
        Serial.print("us (");
        Serial.print(std::floor(1000000.0f / elapsed));
        Serial.println("Hz)");
    }
    lastTimestamp = timestamp;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
}
#endif