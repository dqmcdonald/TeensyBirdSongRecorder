# TeensyBirdSongRecorder

An autonomous bird song recorder that sleeps most of the time, waking three times per day to make 5-minute WAV recordings to an SD card.

## Recording schedule

Each day the device wakes and records at:

1. **Sunrise** — 15 minutes before sunrise
2. **Random daytime** — a randomly chosen time between 90 minutes after sunrise and 90 minutes before sunset
3. **Sunset** — 15 minutes before sunset

Sunrise and sunset times are looked up from a pre-computed NZST table covering the full year. Daylight saving time is handled automatically at runtime (no recompilation needed).

## Hardware

| Component | Details |
|---|---|
| MCU | Teensy 3.6 or Teensy 4.1 |
| Microphone | MEMS microphone via I2S |
| Storage | Built-in SD card (FAT32) |
| Power | Lithium battery |
| RTC | Teensy internal RTC |

The Teensy 4.1 uses SNVS hardware power-off between recordings; the Teensy 3.6 uses the [Snooze](https://github.com/duff2013/Snooze) library hibernate mode.

## Output files

Files are written to the root of the SD card and named:

```
<PREFIX>_YYYY_MM_DD_HH_MM.WAV
```

| Prefix | Recording type |
|---|---|
| `SR` | Sunrise |
| `DA` | Random daytime |
| `SS` | Sunset |

Format: 44,100 Hz, 16-bit, mono PCM WAV (~26 MB per 5-minute file).

A log of all activity is appended to `log.txt` on the SD card.

## Configuration

All tuneable constants are at the top of `TeensyBirdSongRecorder.ino`:

| Constant | Default | Description |
|---|---|---|
| `SUNRISE_OFFSET` | `-15` | Minutes relative to sunrise (negative = before) |
| `SUNSET_OFFSET` | `-15` | Minutes relative to sunset (negative = before) |
| `RECORDING_MINUTES` | `5` | Length of each recording |
| `AMP_GAIN` | `100.0` | Software amplifier gain for the MEMS microphone |

The daytime recording window is set in `SunData.ino`:

| Constant | Default | Description |
|---|---|---|
| `MINUTES_AFTER_SUNRISE` | `90` | Earliest the daytime recording can occur |
| `MINUTES_BEFORE_SUNSET` | `90` | Latest the daytime recording can occur |

## Daylight saving time

The sunrise/sunset table is stored in NZST (UTC+12). The sketch automatically determines at runtime whether the current date falls within New Zealand Daylight Time (last Sunday of September 02:00 → first Sunday of April 03:00) and adjusts the RTC reading accordingly. No recompilation is needed when clocks change.

For best results, set the RTC to the correct local time whenever the device is serviced.

## Building and uploading

See [BUILDING.md](BUILDING.md) for `arduino-cli` commands.

### Dependencies

Install via the Arduino Library Manager or the paths shown:

| Library | Source |
|---|---|
| ArduinoLog | https://github.com/thijse/Arduino-Log |
| Snooze | https://github.com/duff2013/Snooze |
| Time | Arduino Library Manager |

The Teensy core (Teensyduino) must also be installed. See [BUILDING.md](BUILDING.md).

## Documentation

| File | Contents |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | System design, state machine, class reference |
| [BUGS.md](BUGS.md) | Bug history and resolution notes |
| [BUILDING.md](BUILDING.md) | CLI build and upload commands |

## License

MIT — see [LICENSE](LICENSE).
