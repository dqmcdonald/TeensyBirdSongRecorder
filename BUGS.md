# TeensyBirdSongRecorder — Bug & Issue Report

## CRITICAL

### 1. Essential hardware init is inside `#ifdef DEBUG` block
**File**: `TeensyBirdSongRecorder.ino`
**Status**: RESOLVED — hardware init moved outside `#ifdef DEBUG` block.

`AudioMemory(60)`, `amp1.gain(AMP_GAIN)`, `SPI.setMOSI/setSCK`, `SD.begin()`, and
`logFile = SD.open()` were all inside the `#ifdef DEBUG` block. If `#define DEBUG` is
ever removed (e.g. for a production build), the audio system and SD card would not be
initialised and all recording would silently fail.

---

### 2. DST define is inconsistent across compilation units
**File**: `RecorderTime.cpp`, `SunData.ino`
**Status**: RESOLVED — spurious `#define DAYLIGHT_SAVINGS_TIME 1` removed from
`RecorderTime.cpp`. DST is now controlled solely by the define in `SunData.ino`.

---

## BUGS

### 3. `applyOffset()` off-by-one: `> 60` should be `>= 60`
**File**: `RecorderTime.cpp`
**Status**: RESOLVED — changed to `>= 60`.

If `d_minute` ended up exactly 60 the time would become `HH:60` (invalid). Example: a
time of `HH:45` with offset `+15` → `d_minute = 60`, not caught by `> 60`.

---

### 4. Random daytime sleep range is computed incorrectly
**File**: `SunData.ino`
**Status**: RESOLVED — sleep duration is now computed as `random(start, end+1) - current_minutes`,
giving a wakeup time uniformly distributed within `[sunrise+90, sunset-90]`.

Previously `random(MINUTES_AFTER_SUNRISE, day_length)` was used as a sleep duration
directly without subtracting the current time-of-day, producing a skewed window with
the upper bound roughly 3 hours before sunset instead of 1.5 hours.

---

### 5. `secondsToHMS()` called twice in the `is_sunrise` block (dead code)
**File**: `SunData.ino`
**Status**: RESOLVED — duplicate call removed.

---

## SUSPICIOUS / MINOR

### 6. `*prefix = SUNSET_PREFIX` set for daytime random recording
**File**: `SunData.ino`
**Status**: RESOLVED — changed to `DAY_PREFIX`.

---

### 7. Unconditional `Serial.println(" ")` in `continueRecording()`
**File**: `TeensyBirdSongRecorder.ino`
**Status**: RESOLVED — removed.

This was outside any `#ifdef DEBUG` guard and fired on every iteration of
`continueRecording()` — thousands of times during a 5-minute recording, risking audio
buffer underruns from serial I/O during SD writes.

---

### 8. `frec.close()` called twice
**File**: `TeensyBirdSongRecorder.ino`
**Status**: RESOLVED — removed from `writeOutHeader()`; `stopRecording()` closes it.

---

### 9. Typo: "ompleted sunrise recordings"
**File**: `TeensyBirdSongRecorder.ino`
**Status**: RESOLVED — fixed to "Completed"; wrapped in `#ifdef DEBUG`.

---

### 10. No validity check on `frec` after `SD.open()`
**File**: `TeensyBirdSongRecorder.ino`
**Status**: RESOLVED — `Log.error()` is now called if `SD.open()` returns an invalid
file handle in `startRecording()`.

---

### 11. MARGIN in `before()` is asymmetric
**File**: `RecorderTime.cpp`
**Status**: BY DESIGN — `before()` intentionally accepts times up to MARGIN minutes
past the target to provide a grace period for recording. The companion `sleepOrRecord()`
handles this correctly.

---

### 12. `alarm.setRtcTimer(0, 1, 0)` in `setup()` has no effect
**File**: `TeensyBirdSongRecorder.ino`
**Status**: RESOLVED — removed. The timer was overwritten by `setWakeupCallandSleep()`
before it was ever used.
