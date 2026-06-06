# TeensyBirdSongRecorder — Bug & Issue Report

## CRITICAL

### 1. Essential hardware init is inside `#ifdef DEBUG` block
**File**: `TeensyBirdSongRecorder.ino` lines 152–202

`AudioMemory(60)`, `amp1.gain(AMP_GAIN)`, `SPI.setMOSI/setSCK`, `SD.begin()`, and
`logFile = SD.open()` are all inside the `#ifdef DEBUG` block. If `#define DEBUG` is
ever removed (e.g. for a production build), the audio system and SD card are never
initialised and all recording silently fails.

**Fix**: Move hardware init outside the DEBUG block; keep only Serial prints gated on DEBUG.

---

### 2. DST define is inconsistent across compilation units
**File**: `RecorderTime.cpp` line 6, `SunData.ino` line 12

`RecorderTime.cpp` unconditionally defines `DAYLIGHT_SAVINGS_TIME 1`.
`SunData.ino` has it commented out. The actual DST correction (subtracting 1 hour from
`current_hour`) lives in `SunData.ino`, so DST is **currently disabled** regardless of
what `RecorderTime.cpp` says. The define in `RecorderTime.cpp` only affects a debug
print label, creating a false impression that DST is active.

**Fix**: Pick one location (a shared header or a project-level define) for this flag and
remove the duplicate.

---

## BUGS

### 3. `applyOffset()` off-by-one: `> 60` should be `>= 60`
**File**: `RecorderTime.cpp` line 21

```cpp
if (d_minute > 60) {   // BUG: d_minute == 60 is not wrapped
```

If `d_minute` ends up exactly 60 the time becomes `HH:60` (invalid). Example: a time of
`HH:45` with offset `+15` → `d_minute = 60`, which is not caught by `> 60`.

**Fix**: Change to `if (d_minute >= 60)`.

---

### 4. Random daytime sleep range is computed incorrectly
**File**: `SunData.ino` lines 552–568

```cpp
int start = sunrise.in_minutes() + MINUTES_AFTER_SUNRISE;
int end   = sunset.in_minutes()  - MINUTES_BEFORE_SUNSET;
int day_length = end - start;

int random_minutes = random(MINUTES_AFTER_SUNRISE, day_length);  // sleep duration
```

The intent is to wake at a random moment between `start` and `end`. But `random_minutes`
is used directly as a **sleep duration** from the current time without subtracting the
current time-of-day. The result is that the actual wakeup time depends on both the
current time and `day_length`, and the upper bound is roughly `sunset - 3 hours` rather
than `sunset - 1.5 hours`.

The correct approach:
```cpp
int current_minutes = current_hour * 60 + current_minute;
int random_time = random(start, end + 1);          // an absolute time in the window
int sleep_minutes = random_time - current_minutes; // duration to sleep
secondsToHMS((uint32_t)sleep_minutes * SEC_PER_MINUTE, h, m, s);
```

---

### 5. `secondsToHMS()` called twice in the `is_sunrise` block (dead code)
**File**: `SunData.ino` lines 557 and 565

`secondsToHMS` is called at line 557 and again at line 565 with identical arguments.
The first call's results are immediately overwritten. The first call is dead code.

**Fix**: Remove the call at line 557.

---

## SUSPICIOUS / MINOR

### 6. `*prefix = SUNSET_PREFIX` set for daytime random recording
**File**: `SunData.ino` line 567

When `is_sunrise == true`, the function returns `false` (go to sleep) and sets
`*prefix = SUNSET_PREFIX` ("SS"). The prefix is only used when the function returns
`true`, so this never creates a wrong filename — but the assignment is misleading since
a daytime recording should use `DAY_PREFIX` ("DA"). (The actual "DA" prefix is set via
a separate EEPROM path in `loop()`.)

---

### 7. Unconditional `Serial.println(" ")` in `continueRecording()`
**File**: `TeensyBirdSongRecorder.ino` line 332

```cpp
Serial.println(" ");
```

This is outside any `#ifdef DEBUG` guard and executes on every iteration of
`continueRecording()` — thousands of times during a 5-minute recording. Serial I/O
during SD writes can cause audio buffer underruns.

**Fix**: Wrap in `#ifdef DEBUG` or delete it.

---

### 8. `frec.close()` called twice
**File**: `TeensyBirdSongRecorder.ino`

`writeOutHeader()` closes `frec` at line 428, then `stopRecording()` calls `frec.close()`
again at line 359 after returning from `writeOutHeader()`. SD.h tolerates double-close
but it is misleading. Either remove the close from `writeOutHeader()` or from
`stopRecording()`.

---

### 9. Typo: "ompleted sunrise recordings"
**File**: `TeensyBirdSongRecorder.ino` line 270

```cpp
Serial.println("ompleted sunrise recordings, setting day record flag ");
```

Missing leading 'C'. Also this `Serial.println` is outside a `#ifdef DEBUG` guard.

---

### 10. No validity check on `frec` after `SD.open()`
**File**: `TeensyBirdSongRecorder.ino` `startRecording()` function

If `SD.open()` fails, `frec` is invalid. Subsequent `frec.write()` calls will silently
discard data. A check like:

```cpp
if (!frec) {
    // log error and skip recording
    return;
}
```

would make failures visible.

---

### 11. `before()` MARGIN is asymmetric
**File**: `RecorderTime.cpp` line 50

```cpp
if (d_minute <= target.minute() + MARGIN) {
```

This allows the current time to be up to `MARGIN` minutes **past** the target and still
be considered "before" it. The intent (grace period for recording) is correct, but the
method is named `before()` which implies strictly before. The companion
`sleepOrRecord()` handles the margin case correctly via `fabs(minute_diff) == MARGIN`.

---

### 12. `alarm.setRtcTimer(0, 1, 0)` in `setup()` is never used
**File**: `TeensyBirdSongRecorder.ino` line 205

```cpp
alarm.setRtcTimer(0, 1, 0);  // hour, min, sec
```

This sets a 1-minute Snooze timer at startup but the device never calls
`Snooze.hibernate()` from `setup()`. The timer is overwritten in
`setWakeupCallandSleep()` before use. This line has no effect.
