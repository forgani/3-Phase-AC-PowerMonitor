/*******************************************************************************
  ESP-12 based  3 phase energy monitoring system
  A system to control the energy usage and monitoring the current of the 3-Phase AC load of your home in real time.

  Forgani, Forghanain
  https://www.forgani.com/electronics-projects/home-energy-monitor

  Initial 18 Apr. 2021 
  Last update 31 Mar. 2021, 
******************************************************************************/

#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
#include <math.h>
#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance

/*
  Virtual Pins - Base
*/
#define vPIN_VOLTAGE  V0
#define vPIN_COST_VAL  V1

// L1
#define vPIN_CURRENT_L1_REAL V4
#define vPIN_CURRENT_L1_AVG V5
#define vPIN_CURRENT_L1_PEAK V6

// L2
#define vPIN_CURRENT_L2_REAL V7
#define vPIN_CURRENT_L2_AVG V8
#define vPIN_CURRENT_L2_PEAK V9

// L3
#define vPIN_CURRENT_L3_REAL V10
#define vPIN_CURRENT_L3_AVG V11
#define vPIN_CURRENT_L3_PEAK V12

// Test,  average of DAILY_AVG_CYCLE measures
#define vPIN_L1_DAILY_AVG V13 
#define vPIN_L2_DAILY_AVG V14
#define vPIN_L3_DAILY_AVG V15

#define vPIN_CALIBRATION_L1 V22
#define vPIN_CALIBRATION_L2 V23
#define vPIN_CALIBRATION_L3 V24

// Test, without individual calibration parameters
#define vPIN_CURRENT_LF1_REAL V55
#define vPIN_CURRENT_LF2_REAL V56
#define vPIN_CURRENT_LF3_REAL V57

#define vPIN_BUTTON_RESET_AVG V32
#define vPIN_BUTTON_RESET_PEAK V30

#define vPIN_MONTOR_AVG_TIME V31
#define vPIN_MONTOR_PEAK_TIME V33

#define vPIN_DAILY_ENERGY_YESTERDAY_DATE V41 // Yesterday
#define vPIN_DAILY_ENERGY_USED V42
#define vPIN_DAILY_ENERGY_COST V43

#define vPIN_CURRENT_FULL_DATE_TIME V50
#define vPIN_CURRENT_DATE V51

#define S0 D5    // D5 on NodeMCU  GPIO14
#define S1 D7    // D7 on NodeMCU  GPIO13
#define ANALOG_INPUT A0

float currentL1, currentL2, currentL3; 
float currentL1_PEAK = 0.00, currentL2_PEAK = 0.00, currentL3_PEAK = 0.00;
float loadVoltage = 230;
float costFactor = 0.25;

long stopAvgWatch, stopPeakWatch;
char Day[8];
char Date[16];
char Time[8];
char TimeAndDate[32];
char FullDate[32];

BlynkTimer timer;
WidgetRTC rtc;

char auth[] = "xxx"; 
char ssid[] = "xxx"; 
char pass[] = "xxx"; 
IPAddress BlynkServerIP(xxx, xxx, xxx, xxx);
int port = xxx;

/*
  Value Average Settings.
  Set the number of samples to take for the average values.
  Value = Seconds (default 5 seconds)
*/
#define AVG_CYCLE 5
#define DAILY_AVG_CYCLE 5

float calibration1=74.51, calibration2=74.51, calibration3=74.51;

int avg_cycle = 0;
float currentL1_AVG[AVG_CYCLE + 1], currentL2_AVG[AVG_CYCLE + 1], currentL3_AVG[AVG_CYCLE + 1];
float currentL1_avg, currentL2_avg, currentL3_avg;

int daily_avg_cycle = 0;
float currentDailyL1_AVG[DAILY_AVG_CYCLE + 1], currentDailyL2_AVG[DAILY_AVG_CYCLE + 1], currentDailyL3_AVG[DAILY_AVG_CYCLE + 1];
float currentL1_Daily_AVG, currentL2_Daily_AVG, currentL3_Daily_AVG;
int daily = false;

/*---------- SYNC ALL SETTINGS ON BOOT UP ----------*/
bool isFirstConnect = true;

BLYNK_CONNECTED() {
  if (isFirstConnect) {
    Blynk.syncAll();
    isFirstConnect = false;
  }
  // Synchronize time on connection
  rtc.begin();
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.print("Connecting to ");  Serial.println(ssid);
  
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("WiFi connected."); Serial.print("IP address:"); Serial.println(WiFi.localIP());
  timer.setInterval(2000L, CheckConnection); // check if still connected every 11s
  Blynk.config(auth, BlynkServerIP, port);
  Blynk.connect();
  Serial.println("Connected to Blynk server.");

  //Define output pins for Mux
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  // Ratio/BurdenResistor. 2000/25=79 25ohm  by 28A
  // Burden Resistor (ohms) = (AREF * CT TURNS) / (2√2 * max primary current)
  // 28A => 25, 20A => 35.35
  emon1.current(ANALOG_INPUT, 79); // Same analog pin
  Blynk.virtualWrite(vPIN_CALIBRATION_L1, calibration1);
  Blynk.virtualWrite(vPIN_CALIBRATION_L2, calibration2);
  Blynk.virtualWrite(vPIN_CALIBRATION_L3, calibration3);
  Blynk.virtualWrite(vPIN_VOLTAGE, loadVoltage);
  Blynk.virtualWrite(vPIN_COST_VAL, costFactor);

  // Display digital clock every 10 seconds
  timer.setInterval(10000L, clockDisplay);
  timer.setInterval(5000L, stopAvgWatchCounter);
  timer.setInterval(5000L, stopPeakWatchCounter);
  timer.setInterval(1000L, getValues);
  timer.setInterval(1000L, sendValuesAVG);
  timer.setInterval(5000L, sendValuesDailyAVG);
  timer.setInterval(5000L, sendValuesMAX);
}

void loop() {
  if (Blynk.connected())
    Blynk.run();
  timer.run();
}


void getValues() {
  changeMuliplexer(0, 1);
  currentL1 = emon1.calcIrms(1480)-calibration1;
  Blynk.virtualWrite(vPIN_CURRENT_LF1_REAL, currentL1);
  if (currentL1 < 0) {currentL1 = 0.00;}
  Blynk.virtualWrite(vPIN_CURRENT_L1_REAL, currentL1);
  Serial.print( "currentL1 ="); Serial.println(currentL1);
  changeMuliplexer(1, 0);
  currentL2 = emon1.calcIrms(1480)-calibration2;
  Blynk.virtualWrite(vPIN_CURRENT_LF2_REAL, currentL2);
  if (currentL2 < 0) {currentL2 = 0.00;}
  Blynk.virtualWrite(vPIN_CURRENT_L2_REAL, currentL2);
  Serial.print( "currentL2 ="); Serial.println(currentL2);
  changeMuliplexer(1, 1);
  currentL3 = emon1.calcIrms(1480)-calibration3;
  Blynk.virtualWrite(vPIN_CURRENT_LF3_REAL, currentL3);
  if (currentL3 < 0) {currentL3 = 0.00;}
  Blynk.virtualWrite(vPIN_CURRENT_L3_REAL, currentL3);
  Serial.print( "currentL3 ="); Serial.println(currentL3);
  changeMuliplexer(0, 0);
}

// setting average value on timer
void sendValuesAVG() {
  currentL1_AVG[avg_cycle] = currentL1;
  currentL2_AVG[avg_cycle] = currentL2;
  currentL3_AVG[avg_cycle] = currentL3;
  avg_cycle++;
  if (avg_cycle == AVG_CYCLE) {
	float L1_total=0, L2_total=0, L3_total=0;
    for (int i = 0; i < (AVG_CYCLE - 1); i++) {
      L1_total += currentL1_AVG[i];
      L2_total += currentL2_AVG[i];
      L3_total += currentL3_AVG[i];
    }
    currentL1_avg = L1_total / AVG_CYCLE;
    currentL2_avg = L2_total / AVG_CYCLE;
    currentL3_avg = L3_total / AVG_CYCLE;
    currentL1_AVG[0] = currentL1_avg;
    currentL2_AVG[0] = currentL2_avg;
    currentL3_AVG[0] = currentL3_avg;
    avg_cycle = 1;
    Blynk.virtualWrite(vPIN_CURRENT_L1_AVG, currentL1_avg);
    Blynk.virtualWrite(vPIN_CURRENT_L2_AVG, currentL2_avg);
    Blynk.virtualWrite(vPIN_CURRENT_L3_AVG, currentL3_avg);
  }
}

// setting daily average values  TODO
void sendValuesDailyAVG() {
  // daily current averages
  currentDailyL1_AVG[daily_avg_cycle] = currentL1_avg;
  currentDailyL2_AVG[daily_avg_cycle] = currentL2_avg;
  currentDailyL3_AVG[daily_avg_cycle] = currentL3_avg;
  daily_avg_cycle++;
  if (daily_avg_cycle == DAILY_AVG_CYCLE) {
	float L1_total=0, L2_total=0, L3_total=0;
    for (int i = 0; i < (DAILY_AVG_CYCLE - 1); i++) {
      L1_total += currentDailyL1_AVG[i];
      L2_total += currentDailyL2_AVG[i];
      L3_total += currentDailyL3_AVG[i];
    }
    currentL1_Daily_AVG = L1_total / DAILY_AVG_CYCLE;
    currentL2_Daily_AVG = L2_total / DAILY_AVG_CYCLE;
    currentL3_Daily_AVG = L3_total / DAILY_AVG_CYCLE;
    currentDailyL1_AVG[0] = currentL1_Daily_AVG;
    currentDailyL2_AVG[0] = currentL2_Daily_AVG;
    currentDailyL3_AVG[0] = currentL3_Daily_AVG;
    daily_avg_cycle = 1;
    Blynk.virtualWrite(vPIN_L1_DAILY_AVG, currentL1_Daily_AVG);
    Blynk.virtualWrite(vPIN_L2_DAILY_AVG, currentL2_Daily_AVG);
    Blynk.virtualWrite(vPIN_L3_DAILY_AVG, currentL3_Daily_AVG);
  }
}

// setting the MAX values
void sendValuesMAX() {
  if (currentL1_avg > currentL1_PEAK) {
    currentL1_PEAK = currentL1_avg;
    Blynk.virtualWrite(vPIN_CURRENT_L1_PEAK, currentL1_PEAK);
  }
  if (currentL2_avg > currentL2_PEAK) {
    currentL2_PEAK = currentL2_avg;
    Blynk.virtualWrite(vPIN_CURRENT_L2_PEAK, currentL2_PEAK);
  }
  if (currentL3_avg > currentL3_PEAK) {
    currentL3_PEAK = currentL3_avg;
    Blynk.virtualWrite(vPIN_CURRENT_L3_PEAK, currentL3_PEAK);
  }
}

// stop average's timer
BLYNK_WRITE(vPIN_BUTTON_RESET_AVG) {
  if (param.asInt()) {
    Blynk.virtualWrite(vPIN_CURRENT_L1_AVG, 0);
    Blynk.virtualWrite(vPIN_CURRENT_L2_AVG, 0);
    Blynk.virtualWrite(vPIN_CURRENT_L3_AVG, 0);
    for (int i = 0; i < (AVG_CYCLE - 1); i++) {
      currentL1_AVG[i] = currentL1_avg;
      currentL2_AVG[i] = currentL2_avg;
      currentL3_AVG[i] = currentL3_avg;
    }
    avg_cycle = 0;
    Blynk.virtualWrite(vPIN_MONTOR_AVG_TIME, "00:00:00");
    stopAvgWatch = 0;
  }
}

// stop the timer of maximum values
BLYNK_WRITE(vPIN_BUTTON_RESET_PEAK) {
  if (param.asInt()) {
    Blynk.virtualWrite(vPIN_CURRENT_L1_PEAK, 0);
    Blynk.virtualWrite(vPIN_CURRENT_L2_PEAK, 0);
    Blynk.virtualWrite(vPIN_CURRENT_L3_PEAK, 0);
    currentL1_PEAK = 0;
    currentL2_PEAK = 0;
    currentL3_PEAK = 0;
    Blynk.virtualWrite(vPIN_MONTOR_PEAK_TIME, "00:00:00");
    stopPeakWatch = 0;
  }
}

// execute every 10 second
void clockDisplay(){  
  sprintf(Day, "%s,", dayShortStr(weekday()));
  sprintf(Date, "%02u %s %04d", day(), monthShortStr(month()), year());
  sprintf(Time, "%02d:%02d", hour(), minute());
  sprintf(TimeAndDate,"%s %s %s", Time, Day, Date);
  Blynk.virtualWrite(vPIN_CURRENT_FULL_DATE_TIME, TimeAndDate);
  if (hour() == 23 && minute() == 57 && second() >= 40 && daily == false){ //
    sprintf(FullDate, "%s %s", Day, Date);
    Blynk.virtualWrite(vPIN_DAILY_ENERGY_YESTERDAY_DATE, FullDate);
    float dailyEnergy = ( (currentL1_Daily_AVG + currentL2_Daily_AVG + currentL3_Daily_AVG)*loadVoltage ) * (24/1000);  //kWh
    float energyCostDaily = dailyEnergy * costFactor;
    Blynk.virtualWrite(vPIN_DAILY_ENERGY_USED, dailyEnergy);
    Blynk.virtualWrite(vPIN_DAILY_ENERGY_COST, energyCostDaily);
    daily = true;
  } else if (minute() >= 58){ //
    daily_avg_cycle = 0;
	daily = false;
  }
}

// the stopwatch counter which is run on a timer
void stopAvgWatchCounter() {
  stopAvgWatch += 5;
  char WatchStr[16];
  int days=0, hours=0, mins=0, secs=0;
  secs = stopAvgWatch; //convect milliseconds to seconds
  mins = secs / 60; //convert seconds to minutes
  hours = mins / 60; //convert minutes to hours
  secs = secs - (mins * 60); //subtract the coverted seconds to minutes in order to display 59 secs max
  mins = mins - (hours * 60); //subtract the coverted minutes to hours in order to display 59 minutes max
  sprintf(WatchStr, "%02d:%02d:%02d", hours, mins, secs);
  Blynk.virtualWrite(vPIN_MONTOR_AVG_TIME, WatchStr);
}

// the stopwatch counter which is run on a timer
void stopPeakWatchCounter() {
  stopPeakWatch += 5;
  char WatchStr[16];
  int days=0, hours=0, mins=0, secs=0;
  secs = stopPeakWatch; 
  mins = secs / 60; 
  hours = mins / 60; 
  secs = secs - (mins * 60); 
  mins = mins - (hours * 60); 
  sprintf(WatchStr, "%02d:%02d:%02d", hours, mins, secs);
  Blynk.virtualWrite(vPIN_MONTOR_PEAK_TIME, WatchStr);
}

// check every 11s if connected to Blynk server
void CheckConnection() { 
  if (!Blynk.connected()) {
    Serial.println("Not connected to Blynk server");
    Blynk.connect(); // try to connect to server with default timeout
  } else {
    Serial.println("Connected to Blynk server");
  }
}

// multiplexer switches inputs 
void changeMuliplexer(int a, int b) {
  digitalWrite(S0, a);
  digitalWrite(S1, b);
}

BLYNK_WRITE(vPIN_CALIBRATION_L1) {// calibration slider 50 to 70  
  calibration1 = param.asFloat();
}
BLYNK_WRITE(vPIN_CALIBRATION_L2) {// calibration slider 50 to 70  
  calibration2 = param.asFloat();
}
BLYNK_WRITE(vPIN_CALIBRATION_L3) {// calibration slider 50 to 70  
  calibration3 = param.asFloat();
}

// SUPPLY VOLTAGE
BLYNK_WRITE(vPIN_VOLTAGE) { // set supply voltage slider 210 to 260
  loadVoltage = param.asFloat();
}

//COST FACTOR
BLYNK_WRITE(vPIN_COST_VAL) { // PF slider 60 to 100 i.e 0.60 to 1.00, default 95
  costFactor = param.asFloat();
}
