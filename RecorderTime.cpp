#include <Arduino.h>
#include "RecorderTime.h"

#define DEBUG 1

static const int MARGIN = 1; // Allow up to 1 minute difference when comparing times.

RecorderTime::RecorderTime(int hour, int minute) {
  d_hour = hour;
  d_minute = minute;
}

void RecorderTime::applyOffset(int offset) {
  // Applies an offset in minutes (positive or negative). Only handles offsets in (-60, +60).
  d_minute = d_minute + offset;

  if (d_minute >= 60) {
    d_minute = d_minute - 60;
    d_hour = d_hour + 1;
    if (d_hour > 23) {
      d_hour = 0;
    }
  }

  if (d_minute < 0) {
    d_minute = d_minute + 60;
    d_hour = d_hour - 1;
    if (d_hour < 0) {
      d_hour = 23;
    }
  }
}


bool RecorderTime::before(const RecorderTime& target) {
  // Returns true if this time is before target, including up to MARGIN minutes past target
  // (grace period so a wake that lands just after the scheduled minute still triggers recording).
  if (d_hour < target.hour()) {
    return true;
  }
  if (d_hour == target.hour()) {
    if (d_minute <= target.minute() + MARGIN) {
      return true;
    }
  }
  return false;
}

bool isNZDT(int month, int day, int dow, int hour) {
  // Clear cases: no need to check exact transition day.
  if (month >= 5 && month <= 8) return false;   // May–Aug: always NZST
  if (month <= 3 || month >= 10) return true;   // Oct–Mar: always NZDT

  // Convert TimeLib dow (1=Sun) to 0-indexed (0=Sun).
  int dow0 = dow - 1;

  if (month == 9) {
    // NZDT starts at 02:00 on the last Sunday of September.
    // Find day-of-month of the last Sunday: compute weekday of Sep 30, then step back.
    int dow30 = (dow0 + (30 - day)) % 7;
    int last_sun = 30 - dow30;
    if (day != last_sun) return day > last_sun;
    return hour >= 2;
  }

  // month == 4: NZDT ends at 03:00 on the first Sunday of April.
  // Find day-of-month of the first Sunday: compute weekday of Apr 1, then step forward.
  int dow1 = ((dow0 - (day - 1)) % 7 + 7) % 7;
  int first_sun = 1 + (7 - dow1) % 7;
  if (day != first_sun) return day < first_sun;
  return hour < 3;
}


bool RecorderTime::sleepOrRecord(const RecorderTime& target, int* sleep_hour, int* sleep_minute) const {
  // Returns true (record now) if within MARGIN of target; otherwise sets sleep_hour/sleep_minute and returns false.
  bool do_record = false;

  int hour_diff = target.hour() - d_hour;



  // Negative hour_diff means target is on the next calendar day (e.g. current=23:00, target=06:00).
  if (hour_diff < 0) {
    hour_diff = 24 - d_hour + target.hour();
  }
#if DEBUG
  Serial.print("      sleepOrRecord(): hour diff = ");
  Serial.println(hour_diff);
#endif

  int minute_diff = target.minute() - d_minute;

  if( fabs(minute_diff) == MARGIN) // If we are within the margin then treat the difference as zero
    minute_diff = 0;


  if (minute_diff < 0) {
    minute_diff = minute_diff + 60;
    hour_diff--;
  }

#if DEBUG
  Serial.print("      sleepOrRecord(): minute diff = ");
  Serial.println(minute_diff);
#endif

  if (hour_diff == 0 && minute_diff == 0 ) {
    do_record = true;
#if DEBUG
    Serial.println("      sleepOrRecord(): close to event, recording");
#endif
  } else {
    if (hour_diff < 0)
      hour_diff = -hour_diff;  // defensive: shouldn't happen if before() was checked first
    *sleep_hour = hour_diff;
    *sleep_minute = minute_diff;
    do_record = false;
   #if DEBUG
    Serial.println("      sleepOrRecord(): sleeping");
    
#endif
  }

  return do_record;
}