#include <Arduino.h>
#include "RecorderTime.h"

#define DEBUG 1

#define DAYLIGHT_SAVINGS_TIME 1

static const int MARGIN = 1; // Allow up to 1 minute difference when comparing times.

RecorderTime::RecorderTime(int hour, int minute) {
  d_hour = hour;
  d_minute = minute;
}

void RecorderTime::applyOffset(int offset) {
  // Apply offset in minutes to the current time.
  // Offset can be posiitve or negative

  d_minute = d_minute + offset;

  if (d_minute > 60) {
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
  // Return True if the current time is "before" the current time. In reality we will use up to one minute after the target
  // time
 

  if (d_hour < target.hour()) {
    return true;  // lower hour, then is before the target
  }

  if (d_hour == target.hour()) {
    // Same hour, check minute
    if (d_minute <= target.minute() + MARGIN) {
      return true;
    }
  }
  // Otherwise it is not before the target event:
  return false;
}

bool RecorderTime::sleepOrRecord(const RecorderTime& target, int* sleep_hour, int* sleep_minute) const {
  // Compare this object against the target time and decide if we need to sleep until that time or begin recording now
  // if we are already at it
  bool do_record = false;

  int hour_diff = target.hour() - d_hour;



  // Normally this will only be used for events that are on the same day and before the target time. But
  // if the hour difference is negative we can assume it's an event the next day so we need to calculate the hour
  // difference to take consideration of that
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

  // Check to see if we can record now:
  if (hour_diff == 0 && minute_diff == 0 ) {
    do_record = true;
#if DEBUG
    Serial.println("      sleepOrRecord(): close to event, recording");
    
#endif
  } else {

    if( hour_diff < 0 )
      hour_diff = -hour_diff;
    *sleep_hour = hour_diff;
    *sleep_minute = minute_diff;
    do_record = false;
   #if DEBUG
    Serial.println("      sleepOrRecord(): sleeping");
    
#endif
  }

  return do_record;
}