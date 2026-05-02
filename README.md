# GREE AC Remote

[中文](README_CN.md) | [English](README.md)

GREE Electric Air Conditioner remote control for Flipper Zero.

## Description

This application allows you to control GREE brand air conditioners using your Flipper Zero device via infrared signals. It supports common AC functions including power on/off, mode selection (cool/dry/fan/heat/auto), temperature adjustment, fan speed control, and special features like swing, turbo, and clean modes.

## Features

- **Mode Selection**: Cool, Dry, Fan, Heat, Auto
- **Temperature Control**: 16°C - 30°C
- **Fan Speed**: Auto, 1, 2, 3
- **Swing**: Toggle vertical swing
- **LED**: Toggle display between desired and ambient temperature
- **Turbo**: Enable turbo cooling/heating
- **Clean**: Enable self-cleaning mode

## Button Actions

Black top-right corner of the button indicates long-press availability. Actions:
* "Swing" button: sends command to toggle vertical swing instead of horizontal one (if supported)
* "LED" button: sends command to change between desired and ambient temperature indication (if supported)
* "Fan" button: toggle silent mode if supported, reset on power off, if mode or fan power changed or if "Turbo"/"Clean" pressed.

## Building

```shell
# Clone repository
git clone https://github.com/CreeperAWA/FlipperZero-GREE-AC-Remote_YAPOF20.git
cd FlipperZero-GREE-AC-Remote_YAPOF20

# Build the application
ufbt build

# Build and launch the application
ufbt launch
```

## Acknowledgments

This project is based on and references the following projects:

- [Midea AC Remote](https://github.com/xakep666/flipperzero-midea-ac-remote) by [@xakep666](https://github.com/xakep666) - Original AC remote implementation for Flipper Zero
- [Mitsubishi AC Remote](https://github.com/achistyakov/flipperzero-mitsubishi-ac-remote) by [@achistyakov](https://github.com/achistyakov) - Base code structure

Thanks to all contributors from these projects for laying the groundwork.