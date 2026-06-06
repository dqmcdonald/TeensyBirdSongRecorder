
/*
  Autonomous bird song recorder. Sleeps most of the time, waking three times per day to
  make 5-minute WAV recordings to SD card:
    - SUNRISE_OFFSET minutes before sunrise
    - a random time between MINUTES_AFTER_SUNRISE after sunrise and MINUTES_BEFORE_SUNSET before sunset
    - SUNSET_OFFSET minutes before sunset

  Audio input: MEMS microphone via I2S (AudioInputI2S + AudioAmplifier).
  Targets Teensy 3.6 (Snooze hibernate) and Teensy 4.1 (SNVS power-off).
  Files are named: <PREFIX>_YYYY_MM_DD_HH_MM.WAV

  D. Q. McDonald  July 2018, updated July 2025

  Non-standard libraries:
    ArduinoLog: https://github.com/thijse/Arduino-Log
    Snooze:     https://github.com/duff2013/Snooze  (Teensy 3.x only)
*/

#ifndef __IMXRT1062__  // Can't use Snooze library on Teensy 4.1
#include <Snooze.h>
#endif

#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Audio.h>
#include <Wire.h>
#include <TimeLib.h>
#include <String.h>
#include <ArduinoLog.h>
#include <EEPROM.h>


#include "RecorderTime.h"


// Uncomment following line for debugging output
#define DEBUG 1

// WAV file header fields (written by writeOutHeader() after recording completes)
unsigned long ChunkSize = 0L;
unsigned long Subchunk1Size = 16;
unsigned int AudioFormat = 1;
unsigned int numChannels = 1;
unsigned long sampleRate = 44100;
unsigned int bitsPerSample = 16;
unsigned long byteRate = sampleRate * numChannels * (bitsPerSample / 8);  // samplerate x channels x (bitspersample / 8)
unsigned int blockAlign = numChannels * bitsPerSample / 8;
unsigned long Subchunk2Size = 0L;
unsigned long recByteSaved = 0L;
unsigned long NumSamples = 0L;
byte byte1, byte2, byte3, byte4;

String filename;



const int SEC_PER_HOUR = 60 * 60;
const int SEC_PER_MINUTE = 60;

const int DAY_REC_FLAG_EEPROM_ADDRESS = 0;  // EEPROM address of the daytime-recording flag (survives T4.1 power-off)


bool do_record;
const char* file_prefix = "";
byte record_during_day = 0;


AudioPlaySdWav audioSD;
AudioInputI2S audioInput;
AudioOutputI2S audioOutput;
AudioRecordQueue queue1;
AudioAmplifier amp1;

const float AMP_GAIN = 100.0;  // MEMS mic output is weak; needs high gain

// Signal path: mic → amplifier → record queue
AudioConnection patchCord4(audioInput, 0, amp1, 0);
AudioConnection patchCord1(amp1, 0, queue1, 0);
// Playback objects wired but unused during recording
AudioConnection patchCord2(audioSD, 0, audioOutput, 0);
AudioConnection patchCord3(audioSD, 0, audioOutput, 1);




enum rec_modes { STOPPED,
                 RECORDING };  // Represent state of recording.
rec_modes mode = STOPPED;

File frec;
File logFile;

const long int RECORDING_MINUTES = 5;                             // Time for each file in minutes
const long int RECORDING_MILLIS = 1000 * 60 * RECORDING_MINUTES;  // Time for each file in milliseconds

#define SDCARD_CS_PIN BUILTIN_SDCARD
#define SDCARD_MOSI_PIN 11  // not actually used
#define SDCARD_SCK_PIN 13   // not actually used

const int SUNRISE_OFFSET = -15;  // minutes relative to sunrise (negative = before)
const int SUNSET_OFFSET = -15;   // minutes relative to sunset  (negative = before)

elapsedMillis etime;


#ifndef __IMXRT1062__
SnoozeAlarm alarm;
SnoozeAudio audio;
#endif

#ifndef __IMXRT1062__
SnoozeBlock snooze_config(alarm, audio);
#endif



int sleep_hour = 0;
int sleep_minute = 0;

// checkNextEvent() is defined in SunData.ino; forward-declared here so loop() can call it
bool checkNextEvent(int sunrise_offset, int sunset_offset, int* sleep_hour, int* sleep_minute, const char** prefix,
                    bool is_sunrise);

void setup() {

  randomSeed(analogRead(0));

  //EEPROM.write(DAY_REC_FLAG_EEPROM_ADDRESS, 0);  // uncomment once to clear a stuck EEPROM flag



#ifdef DEBUG
  Serial.begin(9600);
  Serial.print("\n");
#endif

  setSyncProvider(getTeensy3Time);  // sync TimeLib to the hardware RTC
#ifdef DEBUG
  delay(100);
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }
#endif

  AudioMemory(60);
  amp1.gain(AMP_GAIN);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
#ifdef DEBUG
      Serial.println("Unable to access the SD card");
#endif
      delay(500);
    }
  }


  logFile = SD.open("log.txt", FILE_WRITE);
  if (logFile) {
    Log.begin(LOG_LEVEL_VERBOSE, &logFile, true);
    Log.setPrefix(printPrefix);
    Log.trace(F("\n\n\n******************************\n"));
    Log.setPrefix(printPrefix);
    Log.trace(F("Logging started to SD card.\n"));
#ifdef DEBUG
    Serial.println("Logging started to log.txt");
#endif
  } else {
#ifdef DEBUG
    Serial.println("Error opening log.txt");
#endif
  }

#ifdef DEBUG
  Serial.println("SD Card Setup Complete");
  Log.trace(F("SD Card Setup Complete.\n"));
  Serial.println("RTC setup complete, time and date follows");
  digitalClockDisplay();
  Serial.println("Initialization done\n");
#endif
  Log.trace(F("Initialization done.\n"));

  delay(1000);
}


void loop() {



  if (mode == STOPPED) {
    record_during_day = EEPROM.read(DAY_REC_FLAG_EEPROM_ADDRESS);  // flag survives T4.1 power-off; check each wake

    if (record_during_day) {  // woke specifically for the random daytime recording
      EEPROM.write(DAY_REC_FLAG_EEPROM_ADDRESS, 0);
      do_record = true;
      file_prefix = "DA";
#ifdef DEBUG
      Serial.print("Record during day flag set to true - about to record ");
      Log.trace(F("Record during day flag set to true - about to record \n"));
#endif
    } else {
      do_record = checkNextEvent(SUNRISE_OFFSET, SUNSET_OFFSET, &sleep_hour, &sleep_minute, &file_prefix, false);
    }

    if (do_record) {
      startRecording(file_prefix);
#ifdef DEBUG
      Serial.println("Started Recording");
      Log.trace(F("Started recording.\n"));
#endif
      etime = 0;
      mode = RECORDING;
    } else {
#ifdef DEBUG
      Serial.print("About to sleep for ");
      Serial.print(sleep_hour);
      Serial.print("hours and ");
      Serial.print(sleep_minute);
      Serial.println(" minutes");
      Log.trace(F("About to sleep for %d hours and %d minutes\n"), sleep_hour, sleep_minute);
      logFile.flush();
      logFile.close();
#endif
      delay(1000);  // allow log write to complete before power-off
      setWakeupCallandSleep(sleep_hour * SEC_PER_HOUR + sleep_minute * SEC_PER_MINUTE);
      doReboot();
    }
  }

  if (mode == RECORDING && (etime > RECORDING_MILLIS)) {
    stopRecording();
    mode = STOPPED;

    // After sunrise recording, set EEPROM flag so the next wake triggers a daytime recording.
    String filep = String(file_prefix);
    bool is_sunrise = false;
    if (filep == "SR") {
      is_sunrise = true;
      EEPROM.write(DAY_REC_FLAG_EEPROM_ADDRESS, 1);
#ifdef DEBUG
      Serial.println("Completed sunrise recordings, setting day record flag");
#endif
      Log.trace(F("Completed sunrise recordings, setting day record flag\n"));
    }

    do_record = checkNextEvent(SUNRISE_OFFSET, SUNSET_OFFSET, &sleep_hour, &sleep_minute, &file_prefix, is_sunrise);
#ifdef DEBUG
    Serial.print("About to sleep for ");
    Serial.print(sleep_hour);
    Serial.print("hours and ");
    Serial.print(sleep_minute);
    Serial.println(" minutes");
    Log.trace(F("About to sleep for %d hours and %d minutes\n"), sleep_hour, sleep_minute);
    logFile.flush();
    logFile.close();
#endif
    
    delay(1000);
    setWakeupCallandSleep(sleep_hour * SEC_PER_HOUR + sleep_minute * SEC_PER_MINUTE);
    doReboot();
  }

  if (mode == RECORDING) {
    continueRecording();
  }
}

void doReboot() {
  SCB_AIRCR = 0x05FA0004;  // Cortex-M system reset request; only reached on T3.6 after Snooze.hibernate() returns
}

void startRecording(const char* prefix) {
#ifdef DEBUG
  Serial.println("startRecording");
  Log.trace(F("startRecording()\n"));
#endif
  filename = getFileName(prefix);
  if (SD.exists(filename.c_str())) {
    SD.remove(filename.c_str());
  }
#ifdef DEBUG
  Serial.print("Filename = ");
  Serial.println(filename.c_str());
  Log.trace(F("Filename = '%s'\n"), filename.c_str());
#endif
  frec = SD.open(filename.c_str(), FILE_WRITE);
  if (frec) {
    queue1.begin();
    recByteSaved = 0L;
  } else {
    Log.error(F("Failed to open file for recording: '%s'\n"), filename.c_str());
  }
}

void continueRecording() {

  if (queue1.available() >= 2) {
    byte buffer[512];
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer + 256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    frec.write(buffer, 512);
    recByteSaved += 512;
  }
}

void stopRecording() {
#ifdef DEBUG
  Serial.println("stopRecording");
  Log.trace(F("stopRecording()"));
#endif
  queue1.end();
  if (mode == RECORDING) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
      recByteSaved += 256;
    }
    writeOutHeader();

    frec.close();
  }
}

void writeOutHeader() {  // update WAV header with final filesize/datasize

  Subchunk2Size = recByteSaved;
  ChunkSize = Subchunk2Size + 36;  // 36 = WAV header size minus the 8-byte RIFF chunk descriptor
  frec.seek(0);
  frec.write("RIFF");
  byte1 = ChunkSize & 0xff;
  byte2 = (ChunkSize >> 8) & 0xff;
  byte3 = (ChunkSize >> 16) & 0xff;
  byte4 = (ChunkSize >> 24) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  frec.write(byte3);
  frec.write(byte4);
  frec.write("WAVE");
  frec.write("fmt ");
  byte1 = Subchunk1Size & 0xff;
  byte2 = (Subchunk1Size >> 8) & 0xff;
  byte3 = (Subchunk1Size >> 16) & 0xff;
  byte4 = (Subchunk1Size >> 24) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  frec.write(byte3);
  frec.write(byte4);
  byte1 = AudioFormat & 0xff;
  byte2 = (AudioFormat >> 8) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  byte1 = numChannels & 0xff;
  byte2 = (numChannels >> 8) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  byte1 = sampleRate & 0xff;
  byte2 = (sampleRate >> 8) & 0xff;
  byte3 = (sampleRate >> 16) & 0xff;
  byte4 = (sampleRate >> 24) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  frec.write(byte3);
  frec.write(byte4);
  byte1 = byteRate & 0xff;
  byte2 = (byteRate >> 8) & 0xff;
  byte3 = (byteRate >> 16) & 0xff;
  byte4 = (byteRate >> 24) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  frec.write(byte3);
  frec.write(byte4);
  byte1 = blockAlign & 0xff;
  byte2 = (blockAlign >> 8) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  byte1 = bitsPerSample & 0xff;
  byte2 = (bitsPerSample >> 8) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  frec.write("data");
  byte1 = Subchunk2Size & 0xff;
  byte2 = (Subchunk2Size >> 8) & 0xff;
  byte3 = (Subchunk2Size >> 16) & 0xff;
  byte4 = (Subchunk2Size >> 24) & 0xff;
  frec.write(byte1);
  frec.write(byte2);
  frec.write(byte3);
  frec.write(byte4);
#ifdef DEBUG
  Serial.println("header written");
  Serial.print("Subchunk2: ");
  Serial.println(Subchunk2Size);
#endif
}

void digitalClockDisplay() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

String getFileName(const char* prefix) {
  // Returns a filename based on the current date and time: YYYY_MM_DD_HH_MM
  String ret = "";
  ret += prefix;
  ret += "_";
  ret += String(year());
  ret += "_";
  if (month() < 10) {
    ret += "0";
  }
  ret += String(month());
  ret += "_";
  if (day() < 10) {
    ret += "0";
  }
  ret += String(day());
  ret += "_";
  if (hour() < 10) {
    ret += "0";
  }
  ret += String(hour());
  ret += "_";
  if (minute() < 10) {
    ret += "0";
  }
  ret += String(minute());
  ret += ".WAV";
  return ret;
}


void printPrefix(Print* _logOutput, int logLevel) {
  printTimestamp(_logOutput);
}

void printTimestamp(Print* _logOutput) {
  char timestamp[20];
  sprintf(timestamp, "%04d-%02d-%02d %02d:%02d", year(), month(), day(), hour(), minute());
  _logOutput->print(timestamp);
}

#if defined(__IMXRT1062__)
#define SNVS_LPCR_LPTA_EN_MASK (0x2U)  ///< mask to put MCU to hibernate

// Programs the SNVS hardware alarm to fire nsec seconds from now.
// Reference: https://forum.pjrc.com/threads/58484-issue-to-reporogram-T4-0?highlight=hibernate
void setWakeupCall(uint32_t nsec) {
  uint32_t tmp = SNVS_LPCR;  // save control register

  SNVS_LPSR |= 1;

  // disable alarm
  SNVS_LPCR &= ~SNVS_LPCR_LPTA_EN_MASK;
  while (SNVS_LPCR & SNVS_LPCR_LPTA_EN_MASK)
    ;

  __disable_irq();

  // Read SNVS RTC with double-read to guard against aliasing between the two registers
  uint32_t lsb, msb;
  do {
    msb = SNVS_LPSRTCMR;
    lsb = SNVS_LPSRTCLR;
  } while ((SNVS_LPSRTCLR != lsb) | (SNVS_LPSRTCMR != msb));  // bitwise | avoids short-circuit so both are always checked
  uint32_t secs = (msb << 17) | (lsb >> 15);

  //set alarm
  secs += nsec;
  SNVS_LPTAR = secs;
  while (SNVS_LPTAR != secs)
    ;

  // restore control register and set alarm
  SNVS_LPCR = tmp | SNVS_LPCR_LPTA_EN_MASK;
  while (!(SNVS_LPCR & SNVS_LPCR_LPTA_EN_MASK))
    ;

  __enable_irq();
}

// Power off the Teensy 4.1; execution never returns from here.
void powerDown(void) {
  SNVS_LPCR |= (1 << 6);  // turn off power
  while (1) asm("wfi");
}

void setWakeupCallandSleep(uint32_t nsec) {
  setWakeupCall(nsec);
  powerDown();
}

#else

void setWakeupCallandSleep(uint32_t nsec) {
  uint16_t h;
  uint16_t m;
  uint16_t s;
  secondsToHMS(nsec, h, m, s);

  alarm.setRtcTimer(h, m, s);       // hour, min, sec
  SIM_SCGC6 &= ~SIM_SCGC6_I2S;      // Turn off i2s otherwise it wakes immediately
  Snooze.hibernate(snooze_config);  // return module that woke processor
}

#endif


void secondsToHMS(const uint32_t seconds, uint16_t& h, uint16_t& m, uint16_t& s) {
  uint32_t t = seconds;

  s = t % 60;

  t = (t - s) / 60;
  m = t % 60;

  t = (t - m) / 60;
  h = t;
}


