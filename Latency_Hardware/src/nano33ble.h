// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: Apache-2.0
// Original Author: Ryan Pavlik
//
// Board header for Arduino Nano 33 BLE

#pragma once

#ifdef TARGET_ARDUINO_NANO33BLE
#include <Arduino.h>
#include <SPI.h>
constexpr int MAX_ANALOG = 65535;

static inline void setupAnalog() {
    analogReadResolution(16);
    analogReference(AnalogReferenceMode::AR_VDD);
    // analogAcquisitionTime(AT_40_US);
}

static inline void ledOn() {
    digitalWrite(LED_RED, LOW);
}

static inline void ledOff() {
    digitalWrite(LED_RED, HIGH);
}
#include <Wire.h>
#include <Adafruit_Sensor.h>

#ifdef WANT_IMU
#include <Adafruit_LSM9DS1.h>
#endif // WANT_IMU

class Board
{
public:
    bool begin();
    bool getGyroData(unsigned long *microseconds, sensors_event_t *gyroEvent);
    void loop();
private:
#ifdef WANT_IMU
    Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1(&Wire1);
    Adafruit_Sensor &gyro = lsm.getGyro();
#endif // WANT_IMU
};

#endif // TARGET_ARDUINO_NANO33BLE
