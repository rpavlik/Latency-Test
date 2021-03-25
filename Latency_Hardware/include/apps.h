// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik
//
// Application entry points for the various latency test apps.
// This allows easier use with Platform IO, as eell as including
// multiple apps on devices with more user interface abilities.

#pragma once

class Board;
void imutestLoop(Board &board);

void onsetSetup();
void onsetLoop(Board &board);

void calibrateSetup();
void calibrateLoop(Board &board);

void turnaroundSetup();
void turnaroundLoop(Board &board);

void logSetup();
void logLoop(Board &board);
