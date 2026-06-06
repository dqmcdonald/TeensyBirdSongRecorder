# TeensyBirdSongRecorder — Architecture Reference

## Purpose

A long-running autonomous bird-song recorder that sleeps almost all the time, waking
three times per day to make 5-minute WAV recordings:

1. **Sunrise** — 15 minutes before sunrise (`SUNRISE_OFFSET = -15`)
2. **Random daytime** — a random moment between 90 min after sunrise and 90 min before sunset
3. **Sunset** — 15 minutes before sunset (`SUNSET_OFFSET = -15`)

Target hardware: **Teensy 3.6** and **Teensy 4.1**, selected at compile time with
`#ifdef __IMXRT1062__` (true on T4.1, false on T3.6).

---

## Source Files

| File | Role |
|---|---|
| `TeensyBirdSongRecorder.ino` | Main sketch: `setup()`, `loop()`, recording state machine, WAV writer, sleep/reboot |
| `SunData.ino` | Static sunrise/sunset lookup table (NZST, all 31-slot months) + `checkNextEvent()` |
| `RecorderTime.h` | `RecorderTime` class declaration |
| `RecorderTime.cpp` | `RecorderTime` class implementation |

`SunData.ino` and `TeensyBirdSongRecorder.ino` are concatenated by the Arduino build system
into a single translation unit; `RecorderTime.cpp` is compiled separately.

---

## Hardware

- **MCU**: Teensy 3.6 (MK66FX1M0) or Teensy 4.1 (IMXRT1062)
- **Microphone**: MEMS microphone via I2S (`AudioInputI2S`)
- **Amplifier**: `AudioAmplifier` at gain 100 in software
- **Storage**: Built-in SD card (`BUILTIN_SDCARD`)
- **RTC**: Teensy internal RTC (accessed via `Teensy3Clock.get()` / `setSyncProvider`)
- **Power**: Separate lithium battery supply

### Audio Signal Path

```
AudioInputI2S (channel 0)
    → AudioAmplifier (gain 100)
        → AudioRecordQueue (queue1)
```

Playback objects (`AudioPlaySdWav`, `AudioOutputI2S`) are wired but unused during recording.
Audio memory: 60 blocks (`AudioMemory(60)`).

---

## Recording State Machine

The `loop()` function implements a two-state machine:

```
STOPPED ──── do_record == true ──→ RECORDING
   ↑                                    │
   └────── etime > RECORDING_MILLIS ────┘
```

- **STOPPED**: calls `checkNextEvent()`, then either starts recording or calls
  `setWakeupCallandSleep()` followed by `doReboot()` (never returns).
- **RECORDING**: calls `continueRecording()` each loop iteration; drains
  `AudioRecordQueue` in 512-byte blocks to SD.

---

## Wake/Sleep Strategy

### Teensy 3.6 — Snooze library

```cpp
alarm.setRtcTimer(h, m, s);
SIM_SCGC6 &= ~SIM_SCGC6_I2S;   // disable I2S clock (prevents spurious wake)
Snooze.hibernate(snooze_config);
```

The I2S clock gate is explicitly cleared before hibernate because it would otherwise
cause an immediate wake.

### Teensy 4.1 — SNVS hardware alarm + power-off

```cpp
setWakeupCall(nsec);   // programs SNVS_LPTAR alarm register
powerDown();           // sets SNVS_LPCR bit 6, then spins on `wfi`
```

The device fully powers off and reboots when the alarm fires. State is preserved only
via EEPROM and the RTC.

### Reboot on Teensy 3.6

`doReboot()` triggers a software reset via `SCB_AIRCR = 0x05FA0004`. After waking from
Snooze hibernate the sketch calls `doReboot()` anyway so both platforms always start
fresh after sleep.

---

## Three-Recording-Event Sequence

```
Power on / reboot
       │
       ▼
 Read EEPROM flag ──── set? ──→ record "DA" (daytime)
       │                              │
      not set                 clear EEPROM flag
       │                              │
       ▼                              ▼
 checkNextEvent()              checkNextEvent()  (is_sunrise=false)
       │                              │
  before sunrise?                sleep until next event
       │ yes
  record "SR" (sunrise)
       │
  set EEPROM flag = 1
       │
  checkNextEvent(is_sunrise=true)
       │
  compute random sleep within [sunrise+90, sunset-90]
       │
  sleep → wake → record "DA"
       │
  checkNextEvent(is_sunrise=false)
       │
  before sunset? → record "SS" (sunset)
       │
  sleep until next day sunrise
```

### EEPROM State

Address 0 (`DAY_REC_FLAG_EEPROM_ADDRESS`) holds a single byte:

| Value | Meaning |
|---|---|
| `0` | Normal operation — check sunrise/sunset schedule |
| `1` | Wake for daytime recording — record immediately with "DA" prefix |

The flag is set after sunrise recording and cleared at the start of the daytime recording.

---

## `checkNextEvent()` Logic (SunData.ino)

### Inputs
- `sunrise_offset`, `sunset_offset` — minutes to add to table times (negative = earlier)
- `sleep_hour`, `sleep_minute` — out params for sleep duration
- `prefix` — out param: `"SR"`, `"DA"`, or `"SS"`
- `is_sunrise` — if true, compute random daytime sleep instead of event check

### Non-sunrise path
1. Look up today's sunrise/sunset from `sun_data[]` using index `(month-1)*31 + day-1`
2. Apply offsets via `RecorderTime::applyOffset()`
3. Compute `next_sunrise` from `sun_data[idx+1]` (wraps to index 0 at year end)
4. Test in order: before sunrise → before sunset → after sunset (sleep until next sunrise)
5. For each case call `current_time.sleepOrRecord(target, ...)` which returns true if within
   MARGIN (1 minute) of the target, false + sleep duration otherwise.

### Sunrise path (`is_sunrise == true`)
Computes a random sleep duration within the daytime window:
```
start = sunrise_minutes + MINUTES_AFTER_SUNRISE (90)
end   = sunset_minutes  - MINUTES_BEFORE_SUNSET  (90)
random_minutes = random(MINUTES_AFTER_SUNRISE, end - start)
```
Returns false with sleep duration = `random_minutes`.

---

## `RecorderTime` Class

Stores a time-of-day as (hour, minute).

| Method | Purpose |
|---|---|
| `applyOffset(int)` | Add minutes (positive or negative); wraps hour |
| `before(target)` | True if `this` is before `target`, including up to MARGIN=1 min past |
| `sleepOrRecord(target, &h, &m)` | Returns true if within margin of target; else sets sleep duration |
| `in_minutes()` | Returns `hour*60 + minute` |

---

## File Naming

`getFileName(prefix)` → `<PREFIX>_YYYY_MM_DD_HH_MM.WAV`

Prefixes: `SR` (sunrise), `DA` (daytime), `SS` (sunset).

---

## WAV File Format

The recording writes raw PCM audio first, then `writeOutHeader()` seeks back to offset 0
and writes a standard RIFF/WAV header:

- PCM format (AudioFormat = 1)
- Mono (numChannels = 1)
- 44 100 Hz sample rate
- 16-bit depth
- ~26 MB per 5-minute file

The `frec` file handle is closed inside `writeOutHeader()`; `stopRecording()` also calls
`frec.close()` (redundant but harmless).

---

## Logging

ArduinoLog (`Log`) writes to `log.txt` on the SD card at VERBOSE level. Log entries are
timestamped with `YYYY-MM-DD HH:MM` via `printPrefix`/`printTimestamp`. The log file is
flushed and closed before each sleep.

---

## DST Handling

The `SunData.ino` table is in NZST (UTC+12). A compile-time define controls DST:

```cpp
// In SunData.ino (controls event scheduling):
//#define DAYLIGHT_SAVINGS_TIME   ← currently commented out (NZST mode)

// In RecorderTime.cpp (affects debug output label only):
#define DAYLIGHT_SAVINGS_TIME     ← always defined
```

When `DAYLIGHT_SAVINGS_TIME` is active in `SunData.ino`, `current_hour` is decremented
by 1 (with midnight wrap) before comparing against the table. This is a binary toggle;
there is no automatic DST transition.

---

## Compile-time Guards Summary

| Guard | Effect when defined |
|---|---|
| `DEBUG` | Enables Serial output, AudioMemory init, SD init, all hardware setup |
| `__IMXRT1062__` | Teensy 4.1 path: SNVS hibernate + no Snooze library |
| `DAYLIGHT_SAVINGS_TIME` | Subtract 1 hr from RTC time before event lookup |

---

## Known Bugs and Suspicious Constructs

See the **Bugs** section in `BUGS.md`.
