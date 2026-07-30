1. Mode Selection & Setup
UI touch event selects a mode via mode_registry (DF, BF, QDF, or one of DPC LR/RL/TB/BT)
mode_registry lookup returns the mode's illumination pattern table (1 pattern for DF/BF, 4 for QDF, 2 for DPC variants)
Frame pool buffers (PSRAM) allocated/reset for the number of patterns the mode requires

3. Acquisition Sequencer (per pattern in the mode's table)

State machine loop, one pass per illumination pattern:

SET_PATTERN — write the current pattern's LED bitmask to the MAX7219 over SPI3 (asymmetric top/bottom/left/right for DPC, ring/quadrant patterns for DF/QDF)
SETTLE — brief delay to let LED intensity and camera exposure stabilize (avoids ghosting/partial-illumination frames)
CAPTURE — trigger OV2640 raw/RGB565 frame capture
STORE — write the captured frame into its slot in the PSRAM frame pool
Loop advances to the next pattern until all required frames for the mode are collected

There are two frame pools - one active and one inactive.
The active frame pool is finished, and being used for image processing.
The inavtive frame pool is not finished, and is currently being created. 
When the inactive frame pool is full of captured frames, it will swap its contents with the active frame pool (if image processing with that pool is completed) 
It will then be wiped, and ready for new images to be pushed onto it

3. Mode-Specific Processing

Frames combined per mode logic (e.g., QDF quadrant combination) directly into an 8-bit greyscale result

Operations are done with floats for QDF, DPC, and then converted into 8-bit greyscale result

4. Render Preparation
The computation frame is converted to RGB565 and pushed to the frame buffer.
UI is then rendered onto the frame buffer.

6. Display Output
push_frame() sends the completed frame_buffer to the ILI9341 over SPI2 via esp_lcd_panel_draw_bitmap
Touch controller (XPT2046, same SPI2 bus, separate CS/clock config) polled via get_touch() for the next UI interaction, feeding back into step 1

7. Loop
Sequencer returns to SET_PATTERN
