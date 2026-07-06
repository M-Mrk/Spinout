# Spinout

A custom ESP32-S3-based persistence of vision (POV) display capable of displaying images in mid-air while spinning.

## Features

### Display
- Full-color Persistence of Vision (POV) image display
- Image upload through a built-in web interface
- Image scaling and positioning
- Start and stop motor directly from the web interface
- Automatic zero-point synchronization using a Hall effect sensor
- Adjustable display settings for different rotation speeds


## Hardware

- Seeed Studio XIAO ESP32-S3
- DC motor with motor driver
- Hall effect sensor for zero-point detection
- Addressable LED strips mounted on both rotor arms
- Li-Po battery
- Buck converters for stable voltage rails
- Built-in Wi-Fi web server for configuration


## Why?

The world's ending in a few days... It's going to be hot outside, so you'll probably want a fan, right?

The power grid will be down, the lights won't work, and everything will be **fall**(**out**)ing apart. So naturally, the only sensible solution is a fan with **RGB LEDs**. YAY!

Not only does it keep you cool, but it also looks awesome. And thanks to POV magic, you can display anything you want on it...

Including **SOUP**.

## Usage

Power on the device and connect to its Wi-Fi network.

Open the web interface in your browser where you can:
- select the image preset.
- start or stop the motor.

## Images


### Assembled Device


### WEB INTERFACE

### DEMO VIDEO


# Build Your Own

## PCB and Parts

| Reference | Qty | Value | Description | Price EUR | Pack Info | Link |
| ---------- | --: | ----- | ----------- | --------: | --------- | ---- |
| U1 | 1 | Seeed Studio XIAO ESP32-S3 | Main microcontroller | 6.90 | 1 pc | Buy |
| U2 | 1 | DRV8833 | Drives the DC motor | 2.50 | 1 pc | Buy |
| M1 | 1 | DC Motor | Main rotating motor | 9.99 | 1 pc | Buy |
| D1 | 2 | WS2812B LED Strip | Addressable LEDs mounted on both rotor arms | 5.00 | 2 pcs | Buy |
| H1 | 1 | Hall Effect Sensor | Rotor zero-point detection and rpm measurement | 0.60 | 5 pcs | Buy |
| Battery | 1 | Li-Po Battery | Powers the system | 8.90 | 1 pc | Buy |
| U3 | 2 | Buck Converter | Generates the required voltages | 2.00 | 2 pcs | Buy |


## Soldering

Recommended order:

1. Hall effect sensor
2. Motor driver
3. XIAO ESP32-S3
4. Connectors
5. Power circuitry

## Case

3D print the provided STL files.

PLA works well for the enclosure, while PETG is recommended for the rotor components because of the mechanical stress during rotation.

## Custom images
You can generate your own image using the python script in the `converter` folder. Just replace on of the existing presets in `firmware/include/frames.h` with the resulting `image_frames` in `frames.h`. Then build the firmware and flash it.

## Programming

**Flashing** is straightforward with PlatformIO:

1. Open the `firmware` folder.
2. Flash the firmware with pio (`pio run -t upload`).

After flashing, connect to the Wi-Fi network created by the device and open the web interface.

**But** you might not want to download pio and the entire toolchain. Then you can grab all the binaries from the [releases](https://github.com/M-Mrk/Spinout/releases/latest). And flash them with the tool of your liking (esptool.py or ESPWebTool). Just follow this table
| Start Address | Name |
| ------------- | ---- |
| 0x0000 | bootloader.bin |
| 0x8000 | partitions.bin |
| 0xe000 | firmware.bin |

## Assembly

1. Mount the DC motor into the enclosure.
4. Connect the LED strips to the rotors.
5. Mount the rotor arms.
6. Install the battery and buck converters.
7. Verify that the Hall sensor correctly detects the magnet.
8. Close the enclosure.
9. Upload an image using the web interface and start the display.

Everything is designed to fit together without requiring any specialized tools, and the firmware, electronics, and mechanical components were designed specifically for this project.
