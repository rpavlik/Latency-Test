// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik
//
// Header for calibrating zero-rate out of a gyro.
// Defaults to do-nothing because the LSM6DS1 in the Nano 33 BLE is
// very good from the factory and my calibration only made it worse.

#pragma once
// Must come before Arduino because of abs
#include <Eigen/Core>
#include <Adafruit_Sensor.h>
#include <utility>
#include <array>

using Eigen::Vector3f;

class GyroProc
{
public:
    std::pair<bool, Vector3f> process(sensors_event_t &gyroData);

    static constexpr size_t InitSamples = 16;
    static constexpr size_t DiscardSamples = 16;

private:
    bool shouldEstimateZeroRate_ = false;
    Vector3f zeroRate{Vector3f::Zero()};
    std::array<Vector3f, InitSamples> samples;
    size_t samplesAcquired = 0;
};