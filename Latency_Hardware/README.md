# Latency Tester Firmware - PlatformIO-based

<!--
Copyright 2021, Collabora, Ltd.
SPDX-License-Identifier: BSL-1.0
-->

This is an evolution of the original OSVR latency tester hardware and firmware:
refactored to build/run with PlatformIO (with Arduino libraries/framework)
instead of the Arduino IDE, and to explicitly support multiple devices.

## Hardware

- [Arduino Nano 33 BLE](https://www.arduino.cc/en/Guide/NANO33BLE)
  - This board is based on the nRF52840, a 32-bit Arm Cortex-M4F microcontroller
    with Bluetooth Low Energy (not used in our application) and native USB.
  - It also contains an ST Micro LSM9DS1 IMU which we use for measuring our
    ground-truth rotation data.
  - You will need a reasonably-long Micro USB cable for this, since it will have
    to stay plugged in during the testing while being attached to the HMD.
- Simple photosensor constructed from a photodiode and a resistor.
  - Most photodiodes are advertised as being IR - this is not a problem, that's
    just their peak sensitivity. This isn't a demanding application, any
    photodiode you can get will probably be fine.
  - Wired in "reverse-bias"
  - Requires a resistor: start with a value around 680K. May adjust value:
    increase value if brightness levels too small
  - Should be on long enough wires so it can be **attached to the lens/screen**.
  - You may wish to mask out the sides to avoid stray light affecting the
    sensor: black hot glue works well.
- Optional but encouraged: A turntable/"lazy susan" (often used for storage and
  spices) to allow easy rotation of the HMD and Arduino together.
  - During testing, the Arduino, the photodiode, and the HMD must remain rigidly
    attached to each other: any relative movement between them will ruin the
    data. Using gaffer tape to tape the Arduino and HMD down to the turntable,
    and the photodiode to the lens/screen, is what worked for me.

![Schematic showing reverse-biased photodiode with 680K load resistor to ground and Arduino pin A0 connected between the photodiode and resistor](schematic.svg)

## Software

Unlike the earlier Arduino IDE firmware, this firmware is in a unified source
tree, sharing a number of files. You'll have the best experience if you open
this directory in an IDE supported by [PlatformIO](https://platformio.org/),
such as VS Code.

Each combination of board and application has its own PlatformIO "environment" -
similar to a build target or config. There are multiple applications, which you
can look in `platformio.ini` to learn about: most correspond to an app in the
previous firmware.

To build and flash a firmware, you will run a command like this, in a PlatformIO
command prompt (or do the equivalent in your IDE):

```sh
platformio run --environment nano33ble.log --target upload
```

where `nano33ble.log` is the main testing application running on the Nano 33
BLE: substitute with other environment names as required.

PlatformIO will automatically download dependencies, build the firmware, and
flash (upload) it to your Arduino board. If you get a build error, try updating
the platform and libraries in platformio. If you get an upload error, you may
need to hit the Reset button on the Arduino and try again.

You will usually then open the USB serial port created by the Arduino in a
terminal program to view and/or log the output. When it comes time to actually
record the data, you'll likely use a Python script to walk you through it and
write out the log.

On the HMD, you will need to run an application that will display a gradient
from black to white based on the rotation of the headset. This should be easy
enough to implement in most any VR/XR environment. A sample using VRPN is
included in [desktop_app](../desktop_app/).

### Photosensor Latency Test

Run this first, to confirm your photodiode is working well. This is environment
`nano33ble.calibrate`:

```sh
platformio run --environment nano33ble.calibrate --target upload
```

Before you open the serial console after flashing, point the photodiode
at the small square light-colored component near the metal "can" on the Arduino:
this is actually an RGB LED, which we will use to measure the latency and
performance of your photosensor. If you can't find it, just open the serial
console anyway, and you'll see it start to blink.

If your "dark" value is small but larger than 0, your "bright" value is at least
about 800, and your delays are less than about 200us, you're ready to go.
Otherwise you may need to adjust your resistor value.

### Log Test

This is a simple but very general test, which offloads all data processing to
you, by simply logging data at about 300Hz.

```sh
platformio run --environment nano33ble.log --target upload
```

For this test you'll want the tester securely mounted to the HMD, as described
above.

The new fancy way of running this test is with the `capture.py` script in the
[Python directory](../Python) . This is highly recommended.

The old way is with a terminal program capable of saving to a log file: Tera
Term Pro is a good one on Windows. (Start logging before connecting.)

Start up the "gradient when rotating" application on the HMD, and make sure you
have the HMD able to move back and forth in a range without hitting all-the-way
white or black, because it's actually the extremes of the motion we care about.

Then, just follow the script's steps (or log the output to a file if you're
using a terminal program). A CSV file is written, with the following columns:

- us (timestamp in microseconds)
- drx, dry, drz (rotation rates about x, y, and z respectively)
- brightness (raw ADC value read from the light sensor)

### Other Tests

While the log test is recommended as it preserves the most data for analysis,
the other tests were also ported from the original Arduino sketches. See the
[PreviousDocs directory](PreviousDocs/) for more details on those other tests.

## Data analysis

See the [Python directory](../Python) for information on how to analyze the log
data.
