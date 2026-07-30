# Troubleshooting Guide

This document contains common issues encountered while developing and running the project, along with their likely causes and solutions.

---

# Console & Debugging

## No Console Logs

If you cannot see any console output:

- Check the **Monitor Port** configuration in the ESP-IDF extension.
- Ensure it is configured to the serial port that your ESP32 is connected to.

---

## Screen Constantly Flashing / Restarting

If the display repeatedly flashes or continuously resets, the firmware is likely crashing during initialization.

This usually means:

1. Initialization failed.
2. The ESP32 restarted.
3. Initialization failed again.

Check the serial console to determine which component failed.

---

# Memory Allocation Errors

If the console prints something similar to:

```text
MAIN: Initializing component render engine
RENDER_ENGINE: Failed to allocate framebuffer (153600 bytes)
MAIN: Failed to initialize component render engine | ESP_ERR_NO_MEM
```

The most common cause is an incorrectly configured **SDKCONFIG**.

## Cause

The project expects an **ESP32-S3-WROOM-1**, which uses **Octal SPI PSRAM**.

If PSRAM is not enabled correctly, framebuffer allocation will fail.

## Solution

Verify that **SDKCONFIG** is configured to use:

- ESP32-S3
- Octal SPI PSRAM

---

# Incorrect Display Colors

If switching between **RGB** and **BGR** color order does not fix the colors:

Many inexpensive ILI9341 clones actually use unusual color orders (such as **RBG**).

## Recommendation

Instead of defining colors with hexadecimal values:

```c
0xF800
```

Create colors using an RGB helper function.

For example:

```c
Color color = RGB(r, g, b);
```

This makes experimenting with different channel orders much easier.

---

# Displaying Images

Images are converted using the LVGL Image Converter:

https://lvgl.io/tools/imageconverter

Only the generated:

```c
const uint8_t[]
```

array is used.

## Important

Although the array is `uint8_t`, the image data is still **16-bit RGB565**.

Each pixel occupies **two bytes**.

Recover each pixel with:

```c
uint16_t pixel =
    image[pixel_index] << 8 |
    image[pixel_index + 1];
```

---

# White Screen

If the display is completely white:

This is often a wiring issue.

Common causes include:

- Loose jumper wires
- Jumper-to-jumper extensions
- Poor signal integrity
- SPI synchronization issues

Verify every connection carefully.

---

# Nothing Appears on the Display

Sometimes the display itself is functioning correctly, but the **backlight is disabled**.

Many clone ILI9341 modules use different backlight logic.

Some require:

- HIGH to enable
- LOW to enable

If absolutely nothing appears on the display, verify the backlight control pin.

---

# Random Display Artifacts

Sometimes completely incorrect graphics are rendered.

## Cause

This is often caused by confusing:

- a value
- the memory address of that value

Example:

```c
int value = 5;

int *value_pos = &value;
```

Here:

```c
value
```

contains:

```
5
```

while

```c
value_pos
```

contains the address where `value` is stored.

Mixing these up can result in invalid image data.

---

## Buffer Overruns

If a framebuffer is allocated too small:

```text
Allocated buffer
↓

████████
```

but code continues reading beyond it:

```text
████████?????????
```

the display will interpret whatever random memory follows as image data.

This often produces seemingly random artifacts.

Always ensure buffers are correctly sized.

---

# Incorrect Display Configuration

Artifacts may also occur if the display configuration does not match the hardware.

Verify:

- Display resolution
- Orientation
- Width
- Height

Many clone displays use different orientations, for example:

- Landscape: 320×240
- Portrait: 320×240

Using the wrong orientation can corrupt rendering.

---

# Memory Being Modified During Rendering

If memory is changed while the display is reading from it, rendering corruption may occur.

Possible solutions include:

- `volatile`
- Mutexes
- Synchronization primitives
- Double buffering

---

# Distorted Images

Distorted or torn images can have several causes.

Common issues include:

- Display refresh rate too high
- Incorrect SPI clock speed
- Incorrect RGB565 byte ordering
- Incorrect byte conversion when importing LVGL images

---

# Frame Pool Synchronization

If the frame pool becomes full, frame swapping will stop until image processing finishes.

Avoid continuing to modify the LED array while rendering is still in progress, otherwise rendering and LED updates may become desynchronized.

---

# Image Identification

Currently, image identification (such as top-left, bottom-left, etc.) is determined by array indices.

A safer alternative would be using a structure.

Example:

```c
typedef struct
{
    uint8_t *raw;
    int pattern;
} image;
```

This associates metadata directly with each image rather than relying on index ordering.

---

# Numeric Precision

Performing calculations directly with 8-bit integers can introduce rounding errors.

Instead:

1. Perform calculations using floating-point values.
2. Convert to 8-bit integers only when storing the final result.

This minimizes accumulated precision loss.

---

# Grayscale Image Storage

Because grayscale images only require one intensity value per pixel, they can be stored efficiently as:

```c
uint8_t image[];
```

where each element represents the brightness of a single pixel.

This significantly reduces memory usage compared to RGB565 images.

---

# Quick Troubleshooting Checklist

| Problem | Likely Cause |
|----------|--------------|
| No console output | Incorrect monitor port |
| Continuous restarting | Initialization failure |
| `ESP_ERR_NO_MEM` | PSRAM/SDKCONFIG not configured |
| Wrong colors | Incorrect color ordering (RGB/BGR/RBG) |
| White screen | Wiring or SPI issues |
| Blank display | Backlight not enabled |
| Random artifacts | Pointer mistakes or buffer overruns |
| Distorted image | SPI speed, refresh rate, byte ordering |
| Corrupted rendering | Incorrect resolution or orientation |
| Flickering artifacts | Memory modified while rendering |
| LED/frame mismatch | Frame pool synchronization issue |
| Rounding errors | Calculating with 8-bit integers |
