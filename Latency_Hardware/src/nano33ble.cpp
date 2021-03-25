// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: Apache-2.0
// Original Author: Ryan Pavlik
//
// Board implementation for Arduino Nano 33 BLE - with built in IMU

#if defined(TARGET_ARDUINO_NANO33BLE)
#include "nano33ble.h"

#include <Arduino.h>

bool Board::begin()
{
#if defined(WANT_IMU)
    if (!lsm.begin())
    {
        Serial.println("Oops ... unable to initialize the LSM9DS1. Check your wiring!");
        return false;
    }
    Serial.println("Found LSM9DS1 9DOF");

    // forcibly turn off the mag
    static_cast<Adafruit_LIS3MDL &>(lsm.getMag()).setOperationMode(LIS3MDL_POWERDOWNMODE);

    lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);

    lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
#endif
    pinMode(LED_RED, OUTPUT);
    ledOff();
    return true;
}
bool Board::getGyroData(unsigned long *microseconds, sensors_event_t *gyroEvent)
{
#if defined(WANT_IMU)
    auto ret = gyro.getEvent(gyroEvent);
    if (ret)
    {
        *microseconds = micros();
    }
    return ret;
#else
    return false;
#endif
}
#endif // defined(TARGET_ARDUINO_NANO33BLE)