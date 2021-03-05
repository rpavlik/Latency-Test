// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik
//
// Class for calibrating zero-rate out of a gyro.

#include "gyroProc.h"
#include "pairwiseReduce.h"
#include "printVec.h"

std::pair<bool, Eigen::Vector3f> GyroProc::process(sensors_event_t &gyroData)
{
    Eigen::Vector3f data = Eigen::Vector3f::Map(gyroData.gyro.v);
    if (samplesAcquired < DiscardSamples)
    {
        printVec(data);
        Serial.println();
        samplesAcquired++;
        return {false, Eigen::Vector3f::Zero()};
    }
    if (samplesAcquired < (DiscardSamples + InitSamples))
    {
        printVec(data);
        Serial.println();
        samples[samplesAcquired - DiscardSamples] = data;
        samplesAcquired++;
        return {false, Eigen::Vector3f::Zero()};
    }
    if (samplesAcquired == DiscardSamples + InitSamples)
    {
        if (shouldEstimateZeroRate_)
        {
            zeroRate = pairwiseReduce(samples.begin(), samples.end());
        }

        samplesAcquired++;
        Serial.print("Zero rate: ");
        printVec(zeroRate);
        Serial.println();
    }
    return {true, data - zeroRate};
}