
#ifndef HRECORDER_TIME
#define HRECORDER_TIME

class RecorderTime {

public:
  RecorderTime(int hour, int minute);
  RecorderTime() {
    d_hour = 0;
    d_minute = 0;
  }

  int hour() const {
    return d_hour;
  }

  int minute() const {
    return d_minute;
  }

  int in_minutes() const {
    return (d_hour * 60 + d_minute);
  }
  
  void applyOffset(int offset);

  bool before(const RecorderTime& target);  // true if this time is before target (or within MARGIN minutes past it)

  bool sleepOrRecord(const RecorderTime& target, int* sleep_hour, int* sleep_minute) const;  // true = record now; false = sets sleep duration

private:
  int d_hour;
  int d_minute;
};

#endif
