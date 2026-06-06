# Building Outside the Arduino IDE

## Prerequisites

`arduino-cli` is installed via Homebrew and already configured with the Teensy board
package and all required libraries (ArduinoLog, Snooze, Time).

## Compile

```bash
# Teensy 4.1
arduino-cli compile --fqbn teensy:avr:teensy41 TeensyBirdSongRecorder/

# Teensy 3.6
arduino-cli compile --fqbn teensy:avr:teensy36 TeensyBirdSongRecorder/
```

## Upload

Find the port first:

```bash
arduino-cli board list
```

Then upload (replace `/dev/cu.usbmodemXXX` with the port from the above command):

```bash
# Teensy 4.1
arduino-cli upload --fqbn teensy:avr:teensy41 -p /dev/cu.usbmodemXXX TeensyBirdSongRecorder/

# Teensy 3.6
arduino-cli upload --fqbn teensy:avr:teensy36 -p /dev/cu.usbmodemXXX TeensyBirdSongRecorder/
```

## Compile and upload in one step

```bash
arduino-cli compile --upload --fqbn teensy:avr:teensy41 -p /dev/cu.usbmodemXXX TeensyBirdSongRecorder/
```
