// Copyright 2015, Sensics, Inc.
// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: Apache-2.0
//
// IMU-related code shared between a few different apps
// Original author: Russell Taylor working for Sensics.com through ReliaSolve.com
// Ported to PlatformIO and multi-board, multi-app framework by Ryan Pavlik for Collabora.

#pragma once

// Must come before Arduino because of abs
#undef abs
#include <Eigen/Core>
using Eigen::Vector3f;
#include "gyroProc.h"

#include <Arduino.h>

const float GYRO_THRESHOLD = 0.5f;

struct ReadResults
{
    bool dataGood = false;
    unsigned long timestamp;
    Vector3f gyro;
};

static inline ReadResults doRead(Board& board, GyroProc& gyroProc)
{
    unsigned long timestamp;
    sensors_event_t g;
    if (!board.getGyroData(&timestamp, &g))
    {
        Serial.println("Read failed!?");
        delay(10);
        return {};
    }
    bool dataGood = false;
    Vector3f processed;
    std::tie(dataGood, processed) = gyroProc.process(g);
    if (!dataGood)
    {
        return {};
    }
    return {true, timestamp, processed};
}

static inline void startupImu(Board& board, GyroProc& gyroProc)
{
    static bool initialized = false;
    if (!initialized) {
        Serial.println("zeroing out IMU");

    }
    while (!initialized)
    {
        auto results = doRead(board, gyroProc);
        initialized = results.dataGood;
    }
}

static inline bool moving(Vector3f const &gyro)
{
    return (gyro.array().abs() > GYRO_THRESHOLD).any();
}