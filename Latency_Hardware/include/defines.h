// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik
//
// Config defines and per-board includes

#pragma once

#if defined(TARGET_ARDUINO_NANO33BLE)
#include "nano33ble.h"
#elif defined(ARDUINO_NRF52840_CLUE)
#include "cluenrf52.h"
#else
constexpr auto LED_PIN = BUILTIN_LED;
#endif

#ifdef HAVE_PHOTODIODE
static inline int
readBrightness()
{
    return MAX_ANALOG - analogRead(A0);
}
#else
static inline int
readBrightness()
{
    return analogRead(A0);
}
#endif
