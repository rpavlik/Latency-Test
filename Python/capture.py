#!/usr/bin/env python3
# Copyright 2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
"""Handle the process of recording data from the "log" firmware."""

import asyncio
import logging

import aioconsole
import aioserial
import attr
from serial.tools import list_ports

logging.basicConfig(level=logging.DEBUG)

FILENAME = "measurements.csv"


def _get_known_devices():
    return {_vid_pid("2341", "805A"): "Arduino Nano 33 BLE"}


def _vid_pid(vid_string, pid_string):
    return (int(vid_string, 16), int(pid_string, 16))


def _get_known_ports():
    known_devices = _get_known_devices()
    for info in list_ports.comports():
        our_vid_pid = (info.vid, info.pid)
        if our_vid_pid in known_devices:
            print(
                info.device,
                info.description,
                hex(info.vid),
                hex(info.pid),
                known_devices[our_vid_pid],
            )
            return info.device


@attr.s
class Measurement:
    us = attr.ib(type=int)
    drx = attr.ib(type=float)
    dry = attr.ib(type=float)
    drz = attr.ib(type=float)
    brightness = attr.ib(type=int)

    @classmethod
    def from_csv_line(cls, line: bytes):
        try:
            us_str, drx_str, dry_str, drz_str, brightness_str = line.split(b",")
            return Measurement(
                us=int(us_str),
                drx=float(drx_str),
                dry=float(dry_str),
                drz=float(drz_str),
                brightness=int(brightness_str),
            )
        except:
            return None

    def get_csv_line(self):
        return f"{self.us},{self.drx},{self.dry},{self.drz},{self.brightness}\n"


async def get_measurement(serial_port: aioserial.AioSerial, retries=1):
    for _ in range(retries + 1):
        line = await serial_port.readline_async()
        if not line:
            # all done
            return
        if b"\n" not in line:
            # partial line: all done
            return
        meas = Measurement.from_csv_line(line)
        if meas:
            return meas


@attr.s
class RunningExtrema:
    min_val = attr.ib(default=None)
    max_val = attr.ib(default=None)

    def get_range(self):
        return self.max_val - self.min_val

    def process(self, val) -> bool:
        changed = False
        if self.min_val is None:
            self.min_val = val
            changed = True
        if self.max_val is None:
            self.max_val = val
            changed = True
        if val < self.min_val:
            self.min_val = val
            changed = True
        if val > self.max_val:
            self.max_val = val
            changed = True
        return changed


async def get_measurement_or_enter(
    input_task: asyncio.Task, serial_port: aioserial.AioSerial, retries=1
):
    """Return a measurement, or None if there was a problem or the user hit enter."""
    meas_task = asyncio.create_task(get_measurement(serial_port, retries=retries))
    done, _ = await asyncio.wait(
        (input_task, meas_task), return_when=asyncio.FIRST_COMPLETED
    )
    if input_task in done:
        # They hit enter
        meas_task.cancel()
        return None
    meas = meas_task.result()
    if not meas:
        return None
    return meas


async def main(device: str):
    serial_port = aioserial.AioSerial(port=device, baudrate=115200)

    print("Talking with your device")
    meas = await get_measurement(serial_port)
    print(meas)
    if not meas:
        return
    print(
        "OK - Please rotate thru the range of brightnesses to determine the limits. Press enter when done"
    )
    brightness_extrema = RunningExtrema()
    input_task = asyncio.create_task(aioconsole.ainput())
    while True:
        meas = await get_measurement_or_enter(input_task, serial_port)
        if not meas:
            # they hit enter
            break
        if brightness_extrema.process(meas.brightness):
            print(brightness_extrema.min_val, brightness_extrema.max_val)
            if brightness_extrema.min_val == 0:
                print("Got a brightness of zero, not OK. Is your sensor connected right?")
                return

    print("OK, limits found.")
    if brightness_extrema.get_range() < 200:
        print(
            "Range not large enough: improve sensitivity of sensor or connection to display"
        )
        return

    print("OK, recording to disk, enter to stop.")
    with open(FILENAME, "w") as fp:
        # header row
        fp.write("us,drx,dry,drz,brightness\n")
        base_meas = await get_measurement(serial_port)
        if not base_meas:
            raise RuntimeError("Could not get our baseline timestamp")
        zero_time = base_meas.us
        # the task watching for the enter press
        input_task = asyncio.create_task(aioconsole.ainput())
        while True:
            meas = await get_measurement_or_enter(input_task, serial_port)
            if not meas:
                # they hit enter
                break

            # Offset the timestamp for ease of use.
            meas.us -= zero_time
            fp.write(meas.get_csv_line())
    print(f"All done! Go move/rename {FILENAME} and analyze it!")


if __name__ == "__main__":
    device = _get_known_ports()
    print(f"Opening {device}")
    # app = Capture()
    asyncio.run(main(device))
