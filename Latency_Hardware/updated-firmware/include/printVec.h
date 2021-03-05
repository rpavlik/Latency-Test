// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik

#include <Eigen/Core>
#include <Arduino.h>
#include <sstream>

static inline void printVec(Eigen::Vector3f const &vec)
{
    static const Eigen::IOFormat format{Eigen::FullPrecision, Eigen::DontAlignCols, ","};
    std::ostringstream os;
    os << vec.transpose().format(format);
    Serial.print(os.str().c_str());
}