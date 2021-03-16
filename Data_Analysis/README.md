# Latency Data Analysis

<!--
Copyright 2021, Collabora, Ltd.
SPDX-License-Identifier: BSL-1.0
-->

For use with the *updated* "log" firmware.

## Environment setup

You'll likely want to open your log file using Jupyter Notebook: here's how to get it ready.

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
python3 -m pip install jupyter notebook matplotlib scipy pandas numpy
```

(On Windows, use `python` instead of `python3` since that's what the program
gets named in virtual environments.)

You'll only need to do this the first time.

### Launch jupyter notebook

In the same command prompt where you activated the virtual environment, run the
following:

```sh
jupyter notebook
```

It will start a local Jupyter server and open it in your web browser.

You will probably want to start with the sample notebook in this directory.
