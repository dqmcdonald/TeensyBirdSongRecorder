
#ifndef HRECORDER_TIME
#define HRECORDER_TIME

class RecorderTime {

public:
  RecorderTime(int hour, int minute);
  RecorderTime() { d_hour = 0; d_minute=0; }

  int hour() const {
    return d_hour;
  }

  int minute() const {
    return d_minute;
  }

  
  void applyOffset(int offset);

  bool before(const RecorderTime& target);  // returns true if the current time is before the target time

  bool sleepOrRecord(const RecorderTime& target, int* sleep_hour, int* sleep_minute) const;  // Return true if to record and sleep time otherwise
                                      
private:
  int d_hour;
  int d_minute;
  
};

#endif
