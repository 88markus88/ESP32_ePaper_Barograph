// this file contains function prototypes and external definitions of variables needed by other modules

#ifndef _global_H
#define _global_H

/***************** global defines */
#define noDataPoints 336        // number of measurement data points stored. 84 h every 15 min
#define offsetData72hGraph 48   // number of points to be ignored at the beginning of arrays if 72 hour graph
#define nanDATA 11111           // this value marks a data point as invalid and not to be shown
#undef showSimpleData           // no simple data display, but full graphics
#undef extendedDEBUG_OUTPUT     // print more stuff
#define logLEVEL 2              // any output with log level <= this number is logged
#define maxLOG_STRING_LEN 240   // max len of logstring

extern RTC_DATA_ATTR uint32_t fgndColor;
extern RTC_DATA_ATTR uint32_t bgndColor;

// defaults for measurement parameters
#define d_dataPresent  false
#define d_applyPressureCorrection  false
#define d_pressureCorrValue 15.0
#define d_measIntervalSec 225 // 900
#define maxSleeptimeSafetyLimit 2000000000 // safety limit: no sleep above 2000 sec
#define d_timeRangeHours 21 //84
#define d_applyInversion false
#define d_graphicsType 7;  // 0: pressure, 1: temperature, 2: humidity

//*************** global global variables ******************/
extern char outstring[maxLOG_STRING_LEN];

struct measurementData
{
  // admin stuff
  bool justInitialized;   // indicator for the fact that software has just been initialized (test data)
  bool dataPresent;       // for simulation. do not create simulation data if this is true
  int32_t graphicsType; // determine which graph is shown  0: pressure, 1: temperature, 2: humidity
  int32_t startCounter;    // total counter for starts of ESP32
  int32_t dischgCnt;    // counter for starts of ESP32 since last charge
  struct timeval lastMeasurementTimestamp;  // time value when last measurement has been taken
  struct timeval last2MeasurementTimestamp;  // time value when measurement before last has been taken
  float actSecondsSinceLastMeasurement;     // seconds elapsed since last measurement, before last deep sleep
  bool preferencesChanged; // determines if preference values have been changed and must be saved
  bool applyPressureCorrection; // false: station mode, true: corrected to sea level
  bool applyInversion;   // if true, white on black. otherwise black on white

  bool buttonPressed;    // remembers if button has been pressed to acknowledge an alert
  bool alertON;          // remembers if an alert has been triggered

  int32_t targetMeasurementIntervalSec;    // sleep time target in seconds, controls the measurement
  int32_t lastTargetSleeptime;             // last target standard sleep time in seconds
  int64_t lastActualSleeptimeAfterMeasUsec;         // this is the number in usec actually used to set the sleep timer after last measurement
  int64_t lastActualSleeptimeNotMeasUsec;         // this is the number in usec actually used to set the sleep timer when no measurement

  // data for the graph that is presently used
  int32_t graphTimeRangeHours; // complete time range of graph (84, 42, 21, 72, 36, 18 hours)
  float graphYDisplayRange;    // display range of last axis drawn in y direction (20, 40, 100, 200, 400 hPa)
  int32_t indexFirstPointToDraw; // the index within the data arrays of the first point to draw
  int graphLowestPressureMbarCorr;  // corrected value for lowest pressure in mbar that fits graph canvas (not within data)
  int graphHighestPressureMbarCorr; // corrected value for lowest pressure in mbar  that fits graph canvas (not within data) 
  int graphLowestTemperatureCelsius;// value for lowest temperature in °C  that fits  graph canvas (not within data)
  int graphHighestTemperatureCelsius;// value for highest temperature in °C  that fits  graph canvas (not within data)
  int graphLowestHumidityPromille;// value for lowest humidity in promille  that fits  graph canvas (not within data)
  int graphHighestHumidityPromille;// value for highest humidity in promille   that fits  graph canvas (not within data)

  // min/ max values in arrays for the time range selected (72 or 84 hour range)
  float pressHistoryMax; 
  float pressHistoryMin;
  int16_t humiHistoryMax; 
  int16_t humiHistoryMin;
  float tempHistoryMax; 
  float tempHistoryMin;

  // data
  float actPressureRaw;   // pressure value as read by the sensor
  float actPressureCorr;  // corrected pressure, pressureCorrValue applied
  float actTemperature;   // temperature as read by the sensor
  int16_t actHumidity=0;      // humidity as read by the sensor. Stored in promille as integer!
  const char* pressureUnit = "hPa";
  const char* humidityUnit = "%";
  const char* temperatureUnit ="°C";
  const char* pressureName    = " Pressure";
  const char* humidityName    = " Humidity";
  const char* temperatureName = "Temperature";
  float pressureCorrValue; // pressure correction value in hPa. applied for display and graph, not storage
  float pressure3hChange;  // change since 3 hours
  float humidity3hChange;  // change since 3 hours
  float temperature3hChange;  // change since 3 hours

  float batteryVoltage;
  float batteryPercent;

  // data points. oldest data with index [0], last data point with index [noDataPoints-1]
  int32_t ageOfDatapoint[noDataPoints];  // age of data point in seconds. more positive value = older
  //uint32_t timestampSecondsOfDataPoint[noDataPoints];  // timestamp in seconds when data point was created
  float pressHistory[noDataPoints];   // pressure data points in the past in hPa, not corrected
  int16_t humiHistory[noDataPoints];  // humidity data points in the past  in %
  float tempHistory[noDataPoints];    // temperature data points in the past in °C

};
extern RTC_DATA_ATTR measurementData wData;

//*************** function prototypes ******************/
void drawMainGraphics(uint32_t graphicsType);   // draw the graphics. graphicsType: 0=pressure, 1=temp, 2=humi
void initDisplay(int startCounter, int fullInterval); // start display
void endDisplay(int mode);                  // power off display if mode =0 else hibernate
void displayTextData(uint32_t startCounter, uint32_t dischgCnt, 
            float temperature, float humidity, float pressure,
            float percent, float volt, uint32_t multiplier);
void writePreferences();         
#ifdef LOLIN32_LITE   
  void bluetoothConfigMain() ;
#endif 
#ifdef CROW_PANEL   
  void bleConfigMain() ;
#endif 
void drawBluetoothInfo(char* text, int mode);
void quarterMeasurementScale();
void halfMeasurementScale();
void quadrupleMeasurementScale();
void doubleMeasurementScale();
void logOut(int logLevel, char* str);
void buzzer(uint16_t number, uint16_t duration, uint16_t interval);

#endif // _global_H