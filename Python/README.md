# Python Capture Script and Latency Data Analysis

<!--
Copyright 2021, Collabora, Ltd.
SPDX-License-Identifier: BSL-1.0
-->

For use with the "log" firmware.

## Environment setup

These steps will walk you through installing the dependencies of the Jupyter
Notebook and utilities for data analysis, as well as for the automatic
log/capture script, in a local directory without polluting your system-wide
Python install.

### Create virtual environment (first time only)

You must already have Python 3 installed. Run the following:

```sh
python3 -m venv venv
```

You only need to do this the first time.

### Activate virtual environment

*nix shells:

```sh
. venv/bin/activate
```

or similar.

On Windows,

```powershell
. .\venv\Scripts\activate.ps1
```

in Powershell, similar for cmd (change `ps1` to `bat`)

You'll need to do this every time.

### Install packages (first time only)

In the same command prompt where you activated the virtual environment, run the
following:

```sh
python3 -m pip install -r requirements.txt
```

(On Windows, use `python` instead of `python3` since that's what the program
gets named in virtual environments.)

You'll only need to do this the first time.

### Capture data script

Hook up the device and install the "log" firmware on it.

In the same command prompt where you activated the virtual environment, run the
following:

```sh
python3 capture.py
```

and follow the steps. (Windows, use `python` instead of `python3`)

**Be sure to rename the output file when you're done!**

### Launch Jupyter Notebook to perform data analysis

In the same command prompt where you activated the virtual environment, run the
following:

```sh
jupyter notebook
```

It will start a local Jupyter server and open it in your web browser.

You will probably want to start with the sample notebook in this directory.
