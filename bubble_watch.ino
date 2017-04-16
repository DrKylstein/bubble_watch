#include <RTCZero.h>
#include <SevSeg.h>
#include <LowPower.h>
#include <time.h>

//offset from UTC in hours, change as appropriate for your local timezone.
#define NORM_OFF -5
#define DST_OFF -4

#define HOURS 3600

#define BUTTON_PIN 16
#define VBAT_PIN A7
#define SEGMENT_OFF LOW
#define DIGIT_OFF HIGH
#define INPUT_MAX 25
const uint8_t digitPins[] = {1, 5, 23, 19};
const uint8_t segmentPins[] = {20, 21, 22, 9, 0, 6, 10, 24};

enum Mode {
  battery,
  time_,
  day_,
  date,
  sleep,
} mode = Mode::time_;
uint32_t timer;
float measuredVBat;
char input[INPUT_MAX + 1];
int inputIndex;

RTCZero rtc;
SevSeg sevseg;

//Custom approximate strings for seven segment
const char* dayStrings = "Sun\0Non\0Tue\0Ued\0Thu\0Fri\0Sat\0";

void setup() {
  Serial.begin(9600);

  sevseg.begin(COMMON_CATHODE, 4, (uint8_t*)digitPins, (uint8_t*)segmentPins, true);
  sevseg.setBrightness(100);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  rtc.begin();

  enterMode(Mode::time_);
}

void buttonPressed() {}

inline int twelveHour(int h) {
  if(h < 0) h += 24;
  return (h % 12) ? (h % 12) : 12;
}

int offset() {
  time_t t = rtc.getEpoch();
  t += NORM_OFF*HOURS;
  
  tm* curr_tm = localtime(&t);

  tm start_tm;
  tm end_tm;

  //US DST rules as of 2017 (last changed in 2007[?])
  start_tm.tm_mday = 14-(1+(1900+curr_tm->tm_year)*5/4)%7; //first Sunday of March
  end_tm.tm_mday =  7-(1+(1900+curr_tm->tm_year)*5/4)%7; //first Sunday of November
  start_tm.tm_year = end_tm.tm_year = curr_tm->tm_year; //current year
  start_tm.tm_hour = end_tm.tm_hour = 2; //2 am AKA 0200 hours
  start_tm.tm_min = end_tm.tm_min = start_tm.tm_sec = end_tm.tm_sec = 0; // on the dot
  start_tm.tm_mon = 3 - 1; //March
  end_tm.tm_mon = 11 - 1; //November

  time_t start_t = mktime(&start_tm);
  time_t end_t = mktime(&end_tm);

  //return appropriate offset in hours
  return (start_t < t && t+(NORM_OFF-DST_OFF)*HOURS < end_t)? DST_OFF : NORM_OFF;
}

const char * weekdayName(time_t t) {
  tm* m = localtime(&t);
  return dayStrings + (m->tm_wday << 2);
}

int monthAndDay(time_t t) {
  tm* m = localtime(&t);
  return (m->tm_mon + 1)*100 + m->tm_mday;
}

void turnOffDisplay() {
  for (int i = 0; i < 8; ++i) {
    pinMode(segmentPins[i], INPUT);
  }
  for (int i = 0; i < 4; ++i) {
    pinMode(digitPins[i], INPUT);
  }
}

void turnOnDisplay() {
  for (int i = 0; i < 8; ++i) {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], SEGMENT_OFF);
  }
  for (int i = 0; i < 4; ++i) {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], DIGIT_OFF);
  }
}

void enterMode(Mode m) {
  mode = m;
  timer = millis() + 1500;
  switch (mode) {
    default:
      mode = Mode::time_;
    case Mode::time_:
      sevseg.setNumber(twelveHour((rtc.getHours()+offset())) * 100 + rtc.getMinutes(), 2);
      break;

    case Mode::day_:
      sevseg.setChars((char*)weekdayName(rtc.getEpoch()+offset()*HOURS));
      break;

    case Mode::date:
      sevseg.setNumber(monthAndDay(rtc.getEpoch()+offset()*HOURS), -1);
      break;

    case Mode::sleep:
      break;

    case Mode::battery:
      break;
  }
}

void loop() {
  if (millis() > timer) {
    enterMode((Mode)(mode + 1));
  }

  sevseg.refreshDisplay();

  if (Serial.available()) {
    timer = 1500 + millis();
    if(Serial.peek() >= '0' && Serial.peek() <= '9') {
      sevseg.blank();
      turnOffDisplay();
      time_t recieved = Serial.parseInt();
      rtc.setEpoch(recieved);
      turnOnDisplay();
      sevseg.setChars("SET");
      enterMode(Mode::battery);
    } else {
      Serial.read();
    }
  }

  if (mode == Mode::sleep) {
    //turn off display
    sevseg.blank();
    turnOffDisplay();


    //turn off USB
    Serial.end();
    USBDevice.detach();
    //sleep until button pressed
    attachInterrupt(BUTTON_PIN, buttonPressed, LOW);
    LowPower.standby();
    detachInterrupt(BUTTON_PIN);
    //turn on USB
    USBDevice.attach();
    Serial.begin(9600);


    //read battery level
    measuredVBat = analogRead(VBAT_PIN) * 2 * 3.3 / 1024;
    pinMode(VBAT_PIN, INPUT);


    turnOnDisplay();
    //Display time or battery state (Max 4.2, med 3.7, min 3.5)
    if (measuredVBat < 3.8) {
      sevseg.setChars(" LO ");
      enterMode(Mode::battery);
    } else {
      enterMode(Mode::time_);
    }
  }
}
