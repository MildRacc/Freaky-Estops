# FreakyEStops - Raspberry Pi Version

A Raspberry Pi implementation of the emergency stop system originally developed for ESP32.

## Requirements

- Raspberry Pi (any model with GPIO pins)
- WiringPi library
- GCC and Make

## Hardware Setup

1. Connect an emergency stop button to GPIO 17 (WiringPi pin 0) and ground
2. Connect an LED with appropriate resistor to GPIO 18 (WiringPi pin 1) and ground

## Installation

### Install Dependencies

```bash
sudo apt-get update
sudo apt-get install build-essential git
```

For Raspberry Pi OS Bullseye and newer, WiringPi needs to be installed manually:

```bash
git clone https://github.com/WiringPi/WiringPi
cd WiringPi
./build
```

### Build and Install FreakyEStops

```bash
git clone https://github.com/MildRacc/Freaky-Estops
cd Freaky-Estops
make
sudo make install
```

## Running as a Service

To run FreakyEStops as a background service on boot:

```bash
make service
sudo cp freaky-estop.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable freaky-estop.service
sudo systemctl start freaky-estop.service
```

## Usage

The system monitors the emergency stop button state and performs these actions:
- When E-Stop is triggered: LED stays on solid, emergency protocols activate
- During normal operation: LED blinks at 1Hz

## Customization

Edit the `freaky_config.h` file to change pin assignments and other settings.
