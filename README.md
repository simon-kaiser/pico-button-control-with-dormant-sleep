# pico-button-control-with-dormant-sleep
## The Problem
A Raspberry Pi that is supplied with energy via a battery should be shut down and booted cleanly via a button.
To do this, a very energy-efficient microcontroller circuit must monitor the button, switch on the power supply to the Pi or send a shutdown command to the Pi and then switch off the power supply.

## Solution
A Raspberry Pi Pico microcontroller is used to control the power supply and monitor the power button.
The Pico is connected to the Pi with two GPIO lines (Pi power status and shutdown command).
The power button is connected to the Pico.
The C code in this repository is executed on the Pico.

## How it works
The Pico is set to DORMANT sleep mode. This greatly limits the Pico's energy consumption. The Pico is woken up from sleep mode by pressing the power button. The Pico switches on the power supply to the Raspberry Pi by switching a relay circuit. The Pi sends a high signal via the power status line as soon as it is booted.
When the power button is pressed again the Pico sends a high signal via the shutdown command line to signal the Pi to shutdown.
After a successful shutdown, the Pico goes back to DORMANT sleep mode.

## Dependencies
The pico-extra-sdk is required for the sleep function.
https://github.com/raspberrypi/pico-extras