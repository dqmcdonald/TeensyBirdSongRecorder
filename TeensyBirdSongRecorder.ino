
/*
  A bird song recorder. Record environmental sounds via microphone attached to LineIn of Teensy Audio Shield
  Files are recorded in 30 minute periods, with a date stamp making up the name of the file.
  Tested on Teensy 3.6 with Audio shield. MEMS mincrophone with separate 2xAA powersupply.
  This is based strongly on the recorder example file associated with the Audio Shield Examples

  D. Q. McDonald
  July 2018
*/


#include <SPI.h>
#include <SD.h>
#include <SD_t3.h>
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
unsigned long byteRate = sampleRate * numChannels * (bitsPerSample / 8); // samplerate x channels x (bitspersample / 8)
unsigned int blockAlign = numChannels * bitsPerSample / 8;
unsigned long Subchunk2Size = 0L;
unsigned long recByteSaved = 0L;
unsigned long NumSamples = 0L;
byte byte1, byte2, byte3, byte4;

String filename;

#define DEBUG 1

const int myInput = AUDIO_INPUT_LINEIN;


AudioPlaySdWav           audioSD;
AudioInputI2S            audioInput;
AudioOutputI2S           audioOutput;
AudioRecordQueue         queue1;

//recod from mic
AudioConnection          patchCord1(audioInput, 0, queue1, 0);
AudioConnection          patchCord2(audioSD, 0, audioOutput, 0);
AudioConnection          patchCord3(audioSD, 0, audioOutput, 1);

AudioControlSGTL5000     audioShield;

int mode = 0;  // 0=stopped, 1=recording
File frec;
elapsedMillis  msecs;

const long int RECORDING_MINUTES = 30;   // Time for each file in minutes
const long int RECORDING_MILLIS = 1000 * 60 *  RECORDING_MINUTES; // Time for each file in milliseconds

#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

elapsedMillis etime;

void setup() {
  Serial.begin(9600);
  AudioMemory(60);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.micGain(40);  //0-63
  audioShield.volume(0.5);  //0-1

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }



#ifdef DEBUG
  Serial.println("SD Card Setup Complete");
#endif
  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);
#ifdef DEBUG
  while (!Serial);  // Wait for Arduino Serial Monitor to open
  delay(100);
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }

  Serial.println("RTC setup complete, time and date follows");
  digitalClockDisplay();
  Serial.println("Initialization done");
#endif

  flashLED(5, 250, 100);
}


void loop() {
  if ( mode == 0 ) {
    startRecording();
#ifdef DEBUG
    Serial.println("Started Recording");
#endif
    mode = 1;
  }

  if ( mode == 1 && (etime > RECORDING_MILLIS)) {
    stopRecording();
    mode = 0;
    etime = 0;
    startRecording();
  }

  if (mode == 1) {
    continueRecording();
  }

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
  if (mode == 1) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
      recByteSaved += 256;
    }
    writeOutHeader();
    frec.close();
  }
  flashLED(5, 100, 250);
}

void writeOutHeader() { // update WAV header with final filesize/datasize

  Subchunk2Size = recByteSaved;
  ChunkSize = Subchunk2Size + 36;
  frec.seek(0);
  frec.write("RIFF");
  byte1 = ChunkSize & 0xff;
  byte2 = (ChunkSize >> 8) & 0xff;
  byte3 = (ChunkSize >> 16) & 0xff;
  byte4 = (ChunkSize >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  frec.write("WAVE");
  frec.write("fmt ");
  byte1 = Subchunk1Size & 0xff;
  byte2 = (Subchunk1Size >> 8) & 0xff;
  byte3 = (Subchunk1Size >> 16) & 0xff;
  byte4 = (Subchunk1Size >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = AudioFormat & 0xff;
  byte2 = (AudioFormat >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  byte1 = numChannels & 0xff;
  byte2 = (numChannels >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  byte1 = sampleRate & 0xff;
  byte2 = (sampleRate >> 8) & 0xff;
  byte3 = (sampleRate >> 16) & 0xff;
  byte4 = (sampleRate >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = byteRate & 0xff;
  byte2 = (byteRate >> 8) & 0xff;
  byte3 = (byteRate >> 16) & 0xff;
  byte4 = (byteRate >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = blockAlign & 0xff;
  byte2 = (blockAlign >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  byte1 = bitsPerSample & 0xff;
  byte2 = (bitsPerSample >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  frec.write("data");
  byte1 = Subchunk2Size & 0xff;
  byte2 = (Subchunk2Size >> 8) & 0xff;
  byte3 = (Subchunk2Size >> 16) & 0xff;
  byte4 = (Subchunk2Size >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
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

time_t getTeensy3Time()
{
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
  if ( month() < 10 ) {
    ret += "0";
  }
  ret += String(month());
  if ( day() < 10) {
    ret += "0";
  }
  ret += String(day());
  if ( hour() < 10) {
    ret += "0";
  }
  ret += String(hour());
  if ( minute() < 10) {
    ret += "0";
  }
  ret += String(minute());
  ret += ".WAV";
  return ret;
}

void flashLED( int numflash, int on_time, int off_time ) {
  // Flash the builtin LED numflash times with on_time and off_time between each one
  int i;
  for ( i = 0; i < numflash; i++) {
    digitalWrite(13, HIGH);
    delay(on_time);
    digitalWrite(13, LOW);
    delay(off_time);
  }

}
