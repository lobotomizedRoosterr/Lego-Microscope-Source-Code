# LEGO Computational Microscope (ESP32-S3)

> **Status:** This repository is currently incomplete and is intended for the LEGO Computational Microscope outreach project based on the ESP32-S3.

## Overview

This repository contains the source code for the LEGO Computational Microscope outreach project. It is designed to be used with **ESP-IDF v5.3.5** and the **VS Code ESP-IDF Extension**.

> **Note**
> Development has only been tested on **Windows**. macOS and Linux instructions are based on the official Espressif documentation.

---

# Prerequisites

Before getting started, install the prerequisites listed in the official Espressif documentation:

https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/prerequisites.html

## Windows

Install:

- Git
- Python 3.10, 3.11, 3.12, 3.13, or 3.14

> **Note:** The ESP-IDF installer automatically installs any missing Windows dependencies.

---

## macOS
Instructions can also be found here https://docs.espressif.com/projects/esp-idf/en/v5.3.5/esp32s3/get-started/linux-macos-setup.html

Install:

- git
- Homebrew
- Ninja
- CMake
- dfu-util
- Python (with pip, virtual environment support, and SSL support)

---

## Linux

Install:

- git
- CMake
- wget
- flex
- bison
- gperf
- ccache
- libffi-dev
- libssl-dev
- dfu-util
- libusb-1.0-0
- Python (with pip, virtual environment support, and SSL support)

---

# Installation

## Step 1 — Install the ESP-IDF Extension

1. Open **Visual Studio Code**.
2. Install the **ESP-IDF** extension from the Extensions Marketplace.
3. Verify that the extension is enabled.

---

## Step 2 — Install ESP-IDF

Open the Command Palette:

```
View → Command Palette
```

Run:

```
ESP-IDF: Open ESP-IDF Installation Manager
```

Alternatively, download the ESP-IDF Installation Manager (EIM):

https://dl.espressif.com/dl/eim/index.html

Install:

- **ESP-IDF v5.3.5**

---

## Step 3 — Configure VS Code

The ESP-IDF extension normally detects installations automatically using the EIM configuration file.

Default locations:

### Windows

```
C:\Espressif\tools\eim_idf.json
```

### macOS / Linux

```
$HOME/.espressif/tools/eim_idf.json
```

If VS Code does not detect your installation:

1. Open **Preferences → Settings**.
2. Search for:

```
idf.eimIdfJsonPath
```

3. Set the path to your `eim_idf.json` file.

---

## Step 4 — Open the git repo on VS-Code

1. Open the command palette on VS-Code
2. Type the command "Git: Clone" and run it
4. Enter the URL for the repo https://github.com/Zangle-Lab/Lego-Microscope-Source-Code
5. Select "Clone from URL"
6. Select a directory to clone the project to

---

## Step 5 - Configure ESP-IDF Version

1. Go to command palette
2. Type "ESP-IDF: Select Current ESP-IDF Version"
3. Select v5.3.5

---

## Step 6 — Reconfigure the Project

Open the Command Palette and run:

```
ESP-IDF: Run idf.py reconfigure Task
```

Wait for the process to complete.

---

## Step 7 — Build the Project

Build the project using either method:

### Option 1

Click the **Build** (wrench) icon in the VS Code status bar.

### Option 2

Run:

```
ESP-IDF: Build Your Project
```

The initial build may take several minutes.

A successful build should produce the project binaries without errors.

---

## Step 8 — Configure the Serial Port

Connect the ESP32-S3 to your computer.

> **Important:** Use a **USB data cable**. Many USB charging cables do **not** support data transfer.

Open the Command Palette:

```
ESP-IDF: Select Port to Use
```

The extension should detect the correct port automatically.

If it does not:

### Windows

1. Open **Device Manager**.
2. Expand **Ports (COM & LPT)**.
3. Locate your ESP32 device.
4. Select the corresponding COM port in VS Code.

If no device appears:

- Verify the USB cable supports data.
- Ensure the board is connected to the USB port used for programming.
- Try another USB cable or USB port.

---

## Step 10 — Flash and Monitor

Building the project **does not** automatically flash it.

Likewise, flashing **does not** rebuild any code changes.

Use UART as a flash method.

A typical development workflow is:

1. Build
2. Flash
3. Monitor

The monitor displays serial output from the ESP32-S3, making it useful for debugging.

---

# Notes

## Reconfiguring

Run:

```
ESP-IDF: Run idf.py reconfigure Task
```

whenever:

- files are added
- files are removed
- `sdkconfig` is modified

---

## Importance of sdkconfig

sdkconfig is vital for the project to run properly.

In general, the following must be configured (note that this is done in the repo's sdkconfig file)

- External PSRAM supported
- Octal SPI mode PSRAM
- External Flash Support
- Quad SPI Flash
- 16 mb Flash

---

# Repository Structure

```
.
├── components/
├── computation/
├── images/
├── main/
├── sdkconfig
├── CMakeLists.txt
└── README.md
```

---

# Troubleshooting

### VS Code cannot find ESP-IDF

- Verify ESP-IDF v5.3.5 is installed.
- Verify `eim_idf.json` exists.
- Check the `idf.eimIdfJsonPath` setting.

### ESP32 is not detected

- Use a USB **data** cable.
- Try another USB port.
- Check Device Manager for the COM port.

### Build errors after adding files

Run:

```
ESP-IDF: Run idf.py reconfigure Task
```

before rebuilding.

---
