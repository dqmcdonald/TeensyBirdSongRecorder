
/*
  A bird song recorder. Record environmental sounds via microphone attached to LineIn of Teensy Audio Shield
  Files are recorded in 5 minute periods, with a date stamp making up the name of the file.
  Tested on Teensy 3.6 with Audio shield. MEMS mincrophone with separate Lithirm battery powersupply.
  This is based strongly on the recorder example file associated with the Audio Shield Examples

  D. Q. McDonald
  July 2018

Updated July 2025 for direct MEMS microphone and not using AudioShield

*/

#include <Snooze.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Audio.h>
#include <Wire.h>
#include <TimeLib.h>
#include <String.h>


//Variables used to write Wave file
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

int led_val = 0;
int led_inc = 2;
elapsedMillis led_timer;
const int LED_UPDATE_PERIOD = 10;  // Update LED 50x second



void flashLED(int numflash, int on_time, int off_time);



// Uncomment following line for debugging output
#define DEBUG 1


// Definitions of Audio Library objects
const int myInput = AUDIO_INPUT_LINEIN;
AudioPlaySdWav audioSD;
AudioInputI2S audioInput;
AudioOutputI2S audioOutput;
AudioRecordQueue queue1;



//record from mic to buffer:
AudioConnection patchCord1(audioInput, 0, queue1, 0);
AudioConnection patchCord2(audioSD, 0, audioOutput, 0);
AudioConnection patchCord3(audioSD, 0, audioOutput, 1);




enum rec_modes { STOPPED,
                 RECORDING };  // Represent state of recording.
rec_modes mode = STOPPED;

File frec;


const long int SUNRISE_OFFSET = 5;                                // Number of minutes less than a full day we will sleep for
const long int RECORDING_MINUTES = 5;                             // Time for each file in minutes
const long int RECORDING_MILLIS = 1000 * 60 * RECORDING_MINUTES;  // Time for each file in milliseconds

const long int DAY_SLEEP_SECONDS = (60 * 60 * 24) - 240;  // 24 hours - 4 minutes in case sunrise is earlier

#define SDCARD_CS_PIN BUILTIN_SDCARD
#define SDCARD_MOSI_PIN 11  // not actually used
#define SDCARD_SCK_PIN 13   // not actually used

#define LED_PIN A16

const int OFFSET = 0;  // Time +/- from sunrise data to begin recording

elapsedMillis etime;

SnoozeAlarm alarm;
SnoozeAudio audio;

SnoozeBlock snooze_config(alarm, audio);


void setup() {

  pinMode(LED_PIN, OUTPUT);

#ifdef DEBUG
  Serial.begin(9600);
  Serial.print("\n");
#endif

  AudioMemory(60);
  

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
#ifdef DEBUG
      Serial.println("Unable to access the SD card");
#endif
      delay(500);
    }
  }

  
#ifdef DEBUG
  Serial.println("SD Card Setup Complete");
#endif



  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);
#ifdef DEBUG
  delay(100);
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }

  Serial.println("RTC setup complete, time and date follows");
  digitalClockDisplay();
  Serial.println("Initialization done\n");
#endif


  alarm.setRtcTimer(0, 1, 0);  // hour, min, sec

  flashLED(3, 100, 50);
  delay(100);
}


void loop() {


  if (mode == RECORDING) {

    if (led_timer > LED_UPDATE_PERIOD) {
      led_val += led_inc;
      if (led_val > 255) {
        led_val = 255;
        led_inc = -led_inc;
      }

      if (led_val < 0) {
        led_val = 0;
        led_inc = -led_inc;
      }
      analogWrite(LED_PIN, led_val);
      led_timer = 0;
    }

    analogWrite(LED_PIN, led_val);
  }

  if (mode == STOPPED) {


    // Check if it's time to start recording:
    if (checkForSunrise(OFFSET)) {
      startRecording();
#ifdef DEBUG
      Serial.println("Started Recording");
#endif
      etime = 0;
      mode = RECORDING;
    } else {
      // Sleep for another minute and check again:
#ifdef DEBUG
      Serial.println("About to sleep for 1 min");
      delay(1000);
#endif
      alarm.setRtcTimer(0, 1, 0);       // hour, min, sec
      SIM_SCGC6 &= ~SIM_SCGC6_I2S;      // Turn off i2s otherwise it wakes immediately
      Snooze.hibernate(snooze_config);  // return module that woke processor
      doReboot();
    }
  }

  if (mode == RECORDING && (etime > RECORDING_MILLIS)) {
    // Recording time has finished:
    stopRecording();
    mode = STOPPED;

    // Sleep for another nearly a day
    long int minutes = 60 - SUNRISE_OFFSET - RECORDING_MINUTES;

#ifdef DEBUG
    Serial.print("About to sleep for 23 hours and ");
    Serial.print(minutes);
    Serial.print(" minutes");
    delay(1000);
#endif

    alarm.setRtcTimer(23, minutes, 0);  // hour, min, sec
    SIM_SCGC6 &= ~SIM_SCGC6_I2S;        // Turn off i2s otherwise it wakes immediately
    Snooze.hibernate(snooze_config);    // return module that woke processor
    doReboot();
  }

  if (mode == RECORDING) {
    continueRecording();
  }
}

void doReboot() {
  SCB_AIRCR = 0x05FA0004;
}

void startRecording() {
#ifdef DEBUG
  Serial.println("startRecording");
#endif
  filename = getFileName();
  if (SD.exists(filename.c_str())) {
    SD.remove(filename.c_str());
  }
#ifdef DEBUG
  Serial.print("Filename = ");
  Serial.println(filename.c_str());
#endif
  frec = SD.open(filename.c_str(), FILE_WRITE);
  if (frec) {
    queue1.begin();
    recByteSaved = 0L;
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
    //    elapsedMicros usec = 0;
    //    Serial.print("SD write, us=");
    //    Serial.println(usec);
  }
}

void stopRecording() {
#ifdef DEBUG
  Serial.println("stopRecording");
#endif
  queue1.end();
  if (mode == RECORDING) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
      recByteSaved += 256;
    }
    writeOutHeader();
    flashLED(5, 500, 500);
    frec.close();
  }
}

void writeOutHeader() {  // update WAV header with final filesize/datasize

  Subchunk2Size = recByteSaved;
  ChunkSize = Subchunk2Size + 36;
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
  frec.close();
#ifdef DEBUG
  Serial.println("header written");
  Serial.print("Subchunk2: ");
  Serial.println(Subchunk2Size);
#endif
}

void digitalClockDisplay() {
  // digital clock display of the time
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

String getFileName() {
  // Returns a filename based on the current date and time: MMDDHHMM
  String ret = "";
  if (month() < 10) {
    ret += "0";
  }
  ret += String(month());
  if (day() < 10) {
    ret += "0";
  }
  ret += String(day());
  if (hour() < 10) {
    ret += "0";
  }
  ret += String(hour());
  if (minute() < 10) {
    ret += "0";
  }
  ret += String(minute());
  ret += ".WAV";
  return ret;
}

void flashLED(int numflash, int on_time, int off_time) {
  // Flash the builtin LED numflash times with on_time and off_time between each one
  int i;
  for (i = 0; i < numflash; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(on_time);
    digitalWrite(LED_PIN, LOW);
    delay(off_time);
  }
}
