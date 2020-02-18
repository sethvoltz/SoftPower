# Soft Power Supply

A soft (push button) power supply with signalling to a Raspberry Pi and detection with power can be
cleanly cut. The behavior is modeled loosely off ATX power supplies.

## Hardware

A relatively simple MOSFET-centered circuit which detects a button press to power on a main circuit
and passes control over to a microcontroller, which then controls when to cut the power.

Details will be forthcoming in the `hardware/` directory.

## Firmware

Used to report the current state of the system back to the user through lighting patterns on the
power button, detect button presses to trigger safe power down and hard halt, signal the power down
to the Raspberry Pi, and detect when the Pi has safely shut down.

The firmware handles all modes through a single pin using press-length detection, with a 3s hold
sending a clean power down signal, and a 10s hold triggering a hard power off, in the event the Pi
stops responding or some other user-determined reason.

Firmware is located in the `firmware/` directory, and is powered by PlatformIO, which expects to be
run from within that directory.
