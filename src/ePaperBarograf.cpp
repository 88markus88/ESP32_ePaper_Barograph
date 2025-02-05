
/****************************************************************/
// ePaperBarograf by. M. Preidel
/****************************************************************/

#include <Arduino.h>
#include <math.h>

//**** ESP32 sleep and rtc memory (which survives deep sleep)
#include "esp_sleep.h"
#include "driver/rtc_io.h"

//*** libs and defines for BME280
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// *** lib for permanent data storage in EEPROM
// https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
// if these defines are set, preferences are written / saved to eeprom. 
// otherwise just from RTC storage (survives deep sleep)
#define WRITE_PREFERENCES
#define WRITE_PREFS_INTERVAL 250 // store counters into preferences every <x> run
#define READ_PREFERENCES
#include <Preferences.h>
Preferences preferences; // object for preference storage in EEPROM
#define prefIDENT "barograf1"

// header file mit #defines, forward declarations, "extern" declarations von woanders deklarierten globalen var's
// globale variablen und alle anderen #includes sind im .cpp file
#include "ePaperBarograf.h"
#include "global.h" // global stuff from other modules

//************ push button stuff *****************/
struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

#ifdef isPushButtons
  #ifdef LOLIN32_LITE
    #define BUTTON1 GPIO_NUM_12   // upper black button
    #define BUTTON2 GPIO_NUM_14  // middle red button
    #define BUTTON3 GPIO_NUM_13  // lower red  button
  #endif
  #ifdef CROW_PANEL  
    #define BUTTON1 GPIO_NUM_2  // upper button MENU
    #define BUTTON2 GPIO_NUM_1  // lower button EXIT
    #define BUTTON3 GPIO_NUM_5  // button CONFIRM at rotary switch
    #define BUTTON4 GPIO_NUM_4  // button DOWN at rotary switch
    #define BUTTON5 GPIO_NUM_6  // button DOWN at rotary switch
  #endif  
  volatile Button button1 = {BUTTON1, 0, false};
  volatile Button button2 = {BUTTON2, 0, false};
  volatile Button button3 = {BUTTON3, 0, false};

  void ARDUINO_ISR_ATTR button1Handler();
  void ARDUINO_ISR_ATTR button2Handler();
  void ARDUINO_ISR_ATTR button3Handler();
#endif // isPushButtons

// 08.01.25: store these variables in RTC memory, which survives deep sleep. 
RTC_DATA_ATTR uint32_t startCounter = 0;    // total counter for starts of ESP32
RTC_DATA_ATTR uint32_t dischgCnt = 0;    // counter for starts of ESP32 since last charge
RTC_DATA_ATTR uint32_t prevMicrovolt = 0;
RTC_DATA_ATTR float prevVoltage = 0;
float prefs[4] = {0.0, 0.0, 0.0, 0.0};  // storage for preferences

float volt, percent;                    // battery voltage and percent fill degree

uint32_t multiplier = MULTIPLIER_FULL;  // slepp time multiplier (standard is 1 min, this multiplier is applied to that)
//uint32_t targetSleepSec;                // target sleep time in seconds
uint32_t targetSleepUSec;               // target sleep time in microseconds

char outstring[maxLOG_STRING_LEN];      // for serial and other debug output

Adafruit_BME280 bme;                    // BME280 sensor I2C
float temperature = 0;                  // data read by BME280
float humidity = 0;
float pressure = 0;


//----------------------- measurement data variables -----------------------
uint32_t startTimeMillis; 
uint32_t measTimeMillis;

// measurement data

#define createTestData          // flag to create test data at setup, overwriting whatever may be there

// measurement data, stored in RTC memory which survives the deep sleep. ESP32 has 8 K, used for this 5448 Bytes
RTC_DATA_ATTR measurementData wData;

// screen colors
RTC_DATA_ATTR uint32_t fgndColor;
RTC_DATA_ATTR uint32_t bgndColor;

// determine after how many partial updates a full update of the epaper is to be done
// 1: always full update
#ifdef LOLIN32_LITE
  #define FULL_UPDATE_INTERVAL  10
#endif
#ifdef CROW_PANEL
  #define FULL_UPDATE_INTERVAL  1
#endif

/**************************************************!
   @brief    logOut()
   @details  Function create log output
   @return   void
***************************************************/
void logOut(int logLevel, char* str)
{
  // for safety: ensure zero termination
  str[maxLOG_STRING_LEN-1]=0;

  if(logLevel <= logLEVEL){
    Serial.println(str);
  }
}

/**************************************************!
   @brief    Function to read the BME280 data 
   @details  The BME280 data is read in 
   @return   void
***************************************************/
void getBME280SensorData() 
{
  // Only needed in forced mode! Forced mode lets the sensor sleep between measurements.
  // normal mode: sensor warming +1,5°C vs. DS18B20. Forced mode: +0,9°C
  bme.takeForcedMeasurement(); // has no effect in normal mode

  pressure = bme.readPressure()/100;
  sprintf(outstring,"Pressure: %3.1f mBar ", pressure);
  logOut(2, outstring);
  humidity = bme.readHumidity();
  sprintf(outstring,"Humidity: %3.1f %% ", humidity);
  logOut(2, outstring);
  temperature = bme.readTemperature();
  sprintf(outstring,"Temperature: %3.1f °C \n", temperature);
  logOut(2, outstring);
}  // of method "getSensorData()"


/*************************** getBatteryCapacity ************************** */
int readBatteryVoltage(float* percent, float* volt)
{
  uint16_t readval;

  *percent = 100;
  pinMode(35, INPUT);
  readval = analogRead(35);// / 4096.0 * 7.23;      // LOLIN D32 (no voltage divider need already fitted to board.or NODEMCU ESP32 with 100K+100K voltage divider
  *volt= (float)readval / 4096.0 * 7.23;
  //float voltage = analogRead(39) / 4096.0 * 7.23;    // NODEMCU ESP32 with 100K+100K voltage divider added
  //float voltage = analogRead(A0) / 4096.0 * 4.24;    // Wemos / Lolin D1 Mini 100K series resistor added
  //float voltage = analogRead(A0) / 4096.0 * 5.00;    // Ardunio UNO, no voltage divider required
  
  *percent = 2808.3808 * pow(*volt, 4) - 43560.9157 * pow(*volt, 3) + 252848.5888 * pow(*volt, 2) - 650767.4615 * *volt + 626532.5703;
  
  if(*percent<0) *percent =0; // to ensure that the above formula does not produce negative values, as it does slightly above 3.5V

  if (*volt > 4.19) *percent = 100;
  else if (*volt <= 3.50) *percent = 0;

  sprintf(outstring," readval: %d Voltage: %3.2f Percent: %3.1f\n",readval, *volt,*percent);
  logOut(2,outstring);
  return(true);
}

/*************************  fillTestData ******************************************/
// fill the measurement arrays with arbitrary demo data to test graphics routine

void fillTestData()
{
  int i, j;
  float startval = 1017.0;
  float randomspread = 2.0;
  float cosrange = 5.0;

  logOut(2,(char*)"Filling test data");

  wData.pressHistoryMax= - 1000000;
  wData.pressHistoryMin=   1000000;
  wData.tempHistoryMax= - 1000000;
  wData.tempHistoryMin=   1000000;
  wData.humiHistoryMax=   0;      // unsigned integer
  wData.humiHistoryMin=   10000;

  // TEST - remove next 3 lines for working version. Overwrites preferences every time when recompiled!
  wData.targetMeasurementIntervalSec = d_measIntervalSec;
  wData.graphTimeRangeHours = d_timeRangeHours;
  wData.preferencesChanged = true;
  wData.indexFirstPointToDraw = offsetData72hGraph;
  wData.graphicsType = d_graphicsType;
  //if(wData.applyPressureCorrection) 
  //  wData.applyPressureCorrection = true;
  //  wData.pressureCorrValue = 15.0;
  gettimeofday(&wData.last2MeasurementTimestamp, NULL);    // get&set previous measurement time
  delay(2000);
  gettimeofday(&wData.lastMeasurementTimestamp, NULL);     // get&set act measurement time
  wData.lastTargetSleeptime = wData.targetMeasurementIntervalSec;
  wData.lastActualSleeptimeAfterMeasUsec = 0; // we have not slept yet. causes measurement ot be started immediately in doWork()
  delay(500);                                // this brings us beyond the 5 sec threshold in doWork()

  sprintf(outstring,"************** fill test data %d ********************\n", wData.dataPresent);
  logOut(2,outstring);
  for(i=0;i<noDataPoints;i++)
  {
    wData.ageOfDatapoint[i] = (noDataPoints-i) * wData.targetMeasurementIntervalSec; // default spacing: 15 min = 900 sec
    wData.pressHistory[i]= 994 + 40/(i+1);
    //wData.pressHistory[i]= 1030-(float)i/20;
    //wData.pressHistory[i]= startval+(float)cos((float)i/50)*cosrange + i/70 + randomspread*((float)rand()/ RAND_MAX - 1.5);
    if(wData.pressHistory[i] > wData.pressHistoryMax) 
        wData.pressHistoryMax = wData.pressHistory[i];
    if(wData.pressHistory[i] < wData.pressHistoryMin) 
        wData.pressHistoryMin = wData.pressHistory[i];    

    wData.tempHistory[i] = 20 + 2*sin((float)i/20);
    if(wData.tempHistory[i] > wData.tempHistoryMax) 
        wData.tempHistoryMax = wData.tempHistory[i];
    if(wData.tempHistory[i] < wData.tempHistoryMin) 
        wData.tempHistoryMin = wData.tempHistory[i]; 

    wData.humiHistory[i] = int(10.0*(35.5 + 20 * i / noDataPoints+ randomspread*((float)rand()/ RAND_MAX - 1.5))); 
    if(wData.humiHistory[i] > wData.humiHistoryMax) 
        wData.humiHistoryMax = wData.humiHistory[i];
    if(wData.humiHistory[i] < wData.humiHistoryMin) 
        wData.humiHistoryMin = wData.humiHistory[i];      
  }

  sprintf(outstring,"Min/Max Tstdata: P: %3.1f-%3.1f T:  %3.1f-%3.1f H: %d-%d", 
        wData.pressHistoryMin, wData.pressHistoryMax,  wData.tempHistoryMin,  wData.tempHistoryMax,
        wData.humiHistoryMin, wData.humiHistoryMax);
  logOut(2,outstring);  
  sprintf(outstring,"Tstdata: Time: %ld.%06ld sec", 
        wData.lastMeasurementTimestamp.tv_sec, wData.lastMeasurementTimestamp.tv_usec);
  logOut(2,outstring);  

  // 3 hour pressure change value: minus 3h = 12 data points at 15 min per point
  wData.pressure3hChange = wData.pressHistory[noDataPoints-1]-wData.pressHistory[noDataPoints-1-12];
  wData.humidity3hChange = wData.humiHistory[noDataPoints-1]-wData.humiHistory[noDataPoints-1-12];
  wData.temperature3hChange=wData.tempHistory[noDataPoints-1]-wData.tempHistory[noDataPoints-1-12];

  // actual data for initial display
  wData.actPressureRaw = wData.pressHistory[noDataPoints-1];
  wData.actPressureCorr= wData.actPressureRaw + wData.pressureCorrValue;
  wData.actHumidity    = wData.humiHistory[noDataPoints-1];
  wData.actTemperature = wData.tempHistory[noDataPoints-1];

  // set marker for "data present" to avoid calling this function again.
  wData.dataPresent = true;

  //set marker for "just initialized"
  wData.justInitialized = true;
}

/**************************************************!
   @brief    Function to calculate the pressure, temperature,humdity change during the last 3 hours 
   @details  using the time information in wData.ageOfDatapoint[i]
   @return   void
***************************************************/
void calc3HourChanges()
{
  int i, marker;
  uint32_t threeHourMark = 3*3600;
  float newP, oldP, oldT, newT, oldH, newH;

  for (i=noDataPoints-1; i>0; i--)
  {
    if ((wData.ageOfDatapoint[i]-threeHourMark) < wData.targetMeasurementIntervalSec/2)
    {
      marker=i;
      break;
    }
  }

  //calculate average over three data points for now and three hours ago, changed to only one last point
  oldP = (wData.pressHistory[marker-1] + wData.pressHistory[marker] + wData.pressHistory[marker+1])/3;
  //newP = (wData.pressHistory[noDataPoints-1]+wData.pressHistory[noDataPoints-2]+wData.pressHistory[noDataPoints-3])/3;
  newP = wData.pressHistory[noDataPoints-1];
  wData.pressure3hChange =  newP - oldP;

  //same for humidity
  oldH = (wData.humiHistory[marker-1] + wData.humiHistory[marker] + wData.humiHistory[marker+1])/3;
  // newH = (wData.humiHistory[noDataPoints-1]+wData.humiHistory[noDataPoints-2]+wData.humiHistory[noDataPoints-3])/3;
  newH = wData.humiHistory[noDataPoints-1];
  wData.humidity3hChange =  (float)(newH - oldH)/10.0;

  //and temperature
  oldT = (wData.tempHistory[marker-1] + wData.tempHistory[marker] + wData.tempHistory[marker+1])/3;
  //newT = (wData.tempHistory[noDataPoints-1]+wData.tempHistory[noDataPoints-2]+wData.tempHistory[noDataPoints-3])/3;
  newT = wData.tempHistory[noDataPoints-1];
  wData.temperature3hChange =  newT - oldT;

  //previous simplified method, on point and assuming 900 sec per data point
  //wData.pressure3hChange = wData.pressHistory[noDataPoints-1]-wData.pressHistory[noDataPoints-1-12];
  sprintf(outstring, "3h:%d t:%ld Pold:%3.2f Pnew:%3.2f 3hC:%3.3f Told:%3.2f, Tnew:%3.2f 3hC:%3.3f Hold:%3.2f Hnew:%3.2f 3hC:%3.3f\n",
      marker, wData.ageOfDatapoint[marker], 
      oldP, newP, wData.pressure3hChange, 
      oldT, newT, wData.temperature3hChange,
      oldH, newH, wData.humidity3hChange);
  logOut(2,outstring);    
}

/**************************************************!
   @brief    output the stored data
   @details  intended to check the work of the data timescale change functions
   @param    l_idx, u_idx: control start and end ranges
   @return   void
***************************************************/
void outputStoredData(uint16_t l_idx, uint16_t u_idx)
{
  int i; 

  for(i=0;i<noDataPoints;i++){
    if(i <= l_idx || i >= u_idx){
      //sprintf(outstring,"i: %d, age %ld time_sec: %ld P: %3.1f T: %3.1f H: %3.1f",
      sprintf(outstring,"i: %d, age %ld P: %3.1f H: %d H: %3.1f",
        i, wData.ageOfDatapoint[i], //wData.timestampSecondsOfDataPoint[i],
        wData.pressHistory[i], wData.humiHistory[i], wData.tempHistory[i]);
      logOut(2,outstring)  ;
    }  
  }

}

/**************************************************!
   @brief    quarterMeasurementScale() : 84->21
   @details  Adapting the stored measurement data and the measurement interval
   @param
   @return   void
***************************************************/
void quarterMeasurementScale()
{
  int16_t i, idx;
  uint32_t interv;

  sprintf(outstring,"Before quarterMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);

  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 

  // adapt measurement interval
  wData.targetMeasurementIntervalSec = wData.targetMeasurementIntervalSec/4; // controls proper time planning
  wData.lastTargetSleeptime = wData.targetMeasurementIntervalSec;            // to ensure proper time planning for next measurement 
  wData.actSecondsSinceLastMeasurement = wData.actSecondsSinceLastMeasurement/4; // only for display
  wData.lastActualSleeptimeAfterMeasUsec = wData.lastActualSleeptimeAfterMeasUsec/4; // needed for sleep time calculation
  wData.lastActualSleeptimeNotMeasUsec   = wData.lastActualSleeptimeNotMeasUsec /4;  // needed for sleep time calculation
  interv =  wData.targetMeasurementIntervalSec;
  idx=0;
  for(i=3*noDataPoints/4; i<noDataPoints; i++){
    if(idx<30 || idx > (noDataPoints-10))
    {
      sprintf(outstring, "idx: %d i: %d age[idx, i] %ld %ld: P[idx, i]: %3.1f %3.1f", 
        idx, i, wData.ageOfDatapoint[idx], wData.ageOfDatapoint[i],
        wData.pressHistory[idx], wData.pressHistory[i]);
      logOut(2,outstring);
    }  
    wData.ageOfDatapoint[idx]             = wData.ageOfDatapoint[i];
    //wData.timestampSecondsOfDataPoint[idx]= wData.timestampSecondsOfDataPoint[i];
    wData.pressHistory[idx]               = wData.pressHistory[i];
    wData.humiHistory[idx]                = wData.humiHistory[i];
    wData.tempHistory[idx]                = wData.tempHistory[i];
    idx++;
    wData.ageOfDatapoint[idx]             = wData.ageOfDatapoint[i] + interv;
    //wData.timestampSecondsOfDataPoint[idx]= wData.timestampSecondsOfDataPoint[i];
    wData.pressHistory[idx]               = wData.pressHistory[i];
    wData.humiHistory[idx]                = wData.humiHistory[i];
    wData.tempHistory[idx]                = wData.tempHistory[i];
    idx++;
    wData.ageOfDatapoint[idx]             = wData.ageOfDatapoint[i] + 2 * interv; 
    //wData.timestampSecondsOfDataPoint[idx]= wData.timestampSecondsOfDataPoint[i];
    wData.pressHistory[idx]               = wData.pressHistory[i];
    wData.humiHistory[idx]                = wData.humiHistory[i];
    wData.tempHistory[idx]                = wData.tempHistory[i];
    idx++;
    wData.ageOfDatapoint[idx]           = wData.ageOfDatapoint[i] + 3 * interv;
    //wData.timestampSecondsOfDataPoint[idx]= wData.timestampSecondsOfDataPoint[i];
    wData.pressHistory[idx]               = wData.pressHistory[i];
    wData.humiHistory[idx]                = wData.humiHistory[i];
    wData.tempHistory[idx]                = wData.tempHistory[i];
    idx++;    
  }

  // correct nominal ages of data points, required for graphics display - correct x axis
  idx = 0;
  for(i=(noDataPoints-1); i>=0 ; i--){
     wData.ageOfDatapoint[i] = idx * wData.targetMeasurementIntervalSec;
     idx++;
     //sprintf(outstring,"i: %d idx: %d", i, idx);
     //logOut(2,outstring);
  } 

  sprintf(outstring,"After quarterMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);
  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 
}

/**************************************************!
   @brief    halfMeasurementScale() : 42->21 or 84->42h
   @details  Adapting the stored measurement data and the measurement interval
   @param
   @return   void
***************************************************/
void halfMeasurementScale()
{
  int16_t i, idx;
  uint32_t interv;

  sprintf(outstring,"Before halfMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);
  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 

  // adapt measurement interval
  wData.targetMeasurementIntervalSec = wData.targetMeasurementIntervalSec/2; // controls proper time planning
  wData.lastTargetSleeptime = wData.targetMeasurementIntervalSec;            // to ensure proper time planning for next measurement 
  wData.actSecondsSinceLastMeasurement = wData.actSecondsSinceLastMeasurement/2; // only for display
  wData.lastActualSleeptimeAfterMeasUsec = wData.lastActualSleeptimeAfterMeasUsec/2; // needed for sleep time calculation
  wData.lastActualSleeptimeNotMeasUsec   = wData.lastActualSleeptimeNotMeasUsec /2;  // needed for sleep time calculation
  interv =  wData.targetMeasurementIntervalSec;
  idx=0;
  for(i=noDataPoints/2; i<noDataPoints; i++){
    if(idx<15 || idx > (noDataPoints-10))
    {
      sprintf(outstring, "idx: %d i: %d age[idx, i] %ld %ld: P[idx, i]: %3.1f %3.1f", 
        idx, i, wData.ageOfDatapoint[idx], wData.ageOfDatapoint[i],
        wData.pressHistory[idx], wData.pressHistory[i]);
      logOut(2,outstring);
    }  
    wData.ageOfDatapoint[idx]             = wData.ageOfDatapoint[i];
    //wData.timestampSecondsOfDataPoint[idx]= wData.timestampSecondsOfDataPoint[i];
    wData.pressHistory[idx]               = wData.pressHistory[i];
    wData.humiHistory[idx]                = wData.humiHistory[i];
    wData.tempHistory[idx]                = wData.tempHistory[i];
    idx++;
    wData.ageOfDatapoint[idx]             = wData.ageOfDatapoint[i] + interv;
    //wData.timestampSecondsOfDataPoint[idx]= wData.timestampSecondsOfDataPoint[i];
    wData.pressHistory[idx]               = wData.pressHistory[i];
    wData.humiHistory[idx]                = wData.humiHistory[i];
    wData.tempHistory[idx]                = wData.tempHistory[i];
    idx++;
  }

  // correct nominal ages of data points, required for graphics display - correct x axis
  idx = 0;
  for(i=noDataPoints-1; i>=0 ; i--){
     wData.ageOfDatapoint[i] = idx * wData.targetMeasurementIntervalSec;
     idx++;
  } 

  sprintf(outstring,"After halfMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);
  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 
}

/**************************************************!
   @brief    quadrupleMeasurementScale() : 21->84
   @details  Adapting the stored measurement data and the measurement interval
   @param
   @return   void
***************************************************/
void quadrupleMeasurementScale()
{
  int16_t i, idx;
  uint32_t interv;

  sprintf(outstring,"Before quadrupleMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);
  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 

  // adapt measurement interval
  wData.targetMeasurementIntervalSec = wData.targetMeasurementIntervalSec*4; // controls proper time planning
  wData.lastTargetSleeptime = wData.targetMeasurementIntervalSec;            // to ensure proper time planning for next measurement 
  wData.actSecondsSinceLastMeasurement = wData.actSecondsSinceLastMeasurement*4; // only for display
  wData.lastActualSleeptimeAfterMeasUsec = wData.lastActualSleeptimeAfterMeasUsec*4; // needed for sleep time calculation
  wData.lastActualSleeptimeNotMeasUsec   = wData.lastActualSleeptimeNotMeasUsec *4;  // needed for sleep time calculation  
  interv =  wData.targetMeasurementIntervalSec;
  idx=noDataPoints-1;
  for(i=(noDataPoints-1); i>0; i-=4){
    if(idx<15 || idx > (noDataPoints-10))
    {
      sprintf(outstring, "idx: %d i: %d age[idx, i] %ld %ld: P[idx, i]: %3.1f %3.1f", 
        idx, i, wData.ageOfDatapoint[idx], wData.ageOfDatapoint[i],
        wData.pressHistory[idx], wData.pressHistory[i]);
      logOut(2,outstring);
    }  
    wData.ageOfDatapoint[idx]             = (wData.ageOfDatapoint[i]              +wData.ageOfDatapoint[i-1]              +wData.ageOfDatapoint[i-2]              +wData.ageOfDatapoint[i-3])/4;
    //wData.timestampSecondsOfDataPoint[idx]= (wData.timestampSecondsOfDataPoint[i] +wData.timestampSecondsOfDataPoint[i-1] +wData.timestampSecondsOfDataPoint[i-2] +wData.timestampSecondsOfDataPoint[i-3])/4;
    wData.pressHistory[idx]               = (wData.pressHistory[i]                +wData.pressHistory[i-1]                +wData.pressHistory[i-2]                +wData.pressHistory[i-3])/4;
    wData.humiHistory[idx]                = (wData.humiHistory[i]                 +wData.humiHistory[i-1]                 +wData.humiHistory[i-2]                 +wData.humiHistory[i-3])/4;
    wData.tempHistory[idx]                = (wData.tempHistory[i]                 +wData.tempHistory[i-1]                 +wData.tempHistory[i-2]                 +wData.tempHistory[i-3])/4;
    idx--;
  }

  // set first 3/4 of values to a default valu, otherwise still on previous data. can be average or marker "nanDATA"
  // should later be changed to "no data" value
  float avgP, avgT;
  uint32_t avgH;
  /*
  // average
  avgP = 0.0; avgH = 0; avgT = 0.0;
  for(i=3*noDataPoints/4-1; i< noDataPoints; i++){
    avgP += wData.pressHistory[i]; 
    avgT += wData.tempHistory[i];
    avgH += wData.humiHistory[i];
  }
  avgP /= (noDataPoints/4);
  avgT /= (noDataPoints/4);
  avgH /= (noDataPoints/4);
  for(i=0; i<(3*noDataPoints/4); i++){
    wData.pressHistory[i] = avgP; 
    wData.tempHistory[i]  = avgT;
    wData.humiHistory[i]  = avgH;
  }
  */
  // marker nanDATA
  for(i=0; i<(3*noDataPoints/4); i++){
    wData.pressHistory[i] = nanDATA; 
    wData.tempHistory[i]  = nanDATA;
    wData.humiHistory[i]  = nanDATA;
  }

  // correct nominal ages of data points, required for graphics display - correct x axis
  idx = 0;
  for(i=noDataPoints-1; i>=0 ; i--){
     wData.ageOfDatapoint[i] = idx * wData.targetMeasurementIntervalSec;
     idx++;
  } 

  sprintf(outstring,"After quadrupleMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);
  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 
}

/**************************************************!
   @brief    doubleMeasurementScale() : 42->21 or 84->42h
   @details  Adapting the stored measurement data and the measurement interval
   @param
   @return   void
***************************************************/
void doubleMeasurementScale()
{
  int16_t i, idx;
  uint32_t interv;

  sprintf(outstring,"Before doubleMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);
  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 

  // adapt measurement interval
  wData.targetMeasurementIntervalSec = wData.targetMeasurementIntervalSec*2; // controls proper time planning
  wData.lastTargetSleeptime = wData.targetMeasurementIntervalSec;            // to ensure proper time planning for next measurement 
  wData.actSecondsSinceLastMeasurement = wData.actSecondsSinceLastMeasurement*2; // only for display
  wData.lastActualSleeptimeAfterMeasUsec = wData.lastActualSleeptimeAfterMeasUsec*2; // needed for sleep time calculation
  wData.lastActualSleeptimeNotMeasUsec   = wData.lastActualSleeptimeNotMeasUsec *2;  // needed for sleep time calculation  
  interv =  wData.targetMeasurementIntervalSec;
  idx=noDataPoints-1;
  for(i=(noDataPoints-1); i>0; i-=2){
    if(idx<15 || idx > (noDataPoints-10))
    {
      sprintf(outstring, "idx: %d i: %d age[idx, i] %ld %ld: P[idx, i]: %3.1f %3.1f", 
        idx, i, wData.ageOfDatapoint[idx], wData.ageOfDatapoint[i],
        wData.pressHistory[idx], wData.pressHistory[i]);
      logOut(2,outstring);
    }  
    wData.ageOfDatapoint[idx]             = (wData.ageOfDatapoint[i]              +wData.ageOfDatapoint[i-1]              )/2;
    //wData.timestampSecondsOfDataPoint[idx]= (wData.timestampSecondsOfDataPoint[i] +wData.timestampSecondsOfDataPoint[i-1] )/2;
    wData.pressHistory[idx]               = (wData.pressHistory[i]                +wData.pressHistory[i-1]                )/2;
    wData.humiHistory[idx]                = (wData.humiHistory[i]                 +wData.humiHistory[i-1]                 )/2;
    wData.tempHistory[idx]                = (wData.tempHistory[i]                 +wData.tempHistory[i-1]                 )/2;
    idx--;
  }
  // set first half of values to default value, otherwise still on previous data. can be average or marker "nanDATA"
  // should later be changed to "no data" value
  /*
  // average
  float avgP, avgT;
  uint32_t avgH;
  avgP = 0.0; avgH = 0; avgT = 0.0;
  for(i=noDataPoints/2-1; i< noDataPoints; i++){
    avgP += wData.pressHistory[i]; 
    avgT += wData.tempHistory[i];
    avgH += wData.humiHistory[i];
  }
  avgP /= (noDataPoints/2);
  avgT /= (noDataPoints/2);
  avgH /= (noDataPoints/2);
  for(i=0; i<(noDataPoints/2); i++){
    wData.pressHistory[i] = avgP; 
    wData.tempHistory[i]  = avgT;
    wData.humiHistory[i]  = avgH;
  }
  */
  for(i=0; i<(noDataPoints/2); i++){
    wData.pressHistory[i] = nanDATA; 
    wData.tempHistory[i]  = nanDATA;
    wData.humiHistory[i]  = nanDATA;
  }

  // correct nominal ages of data points, required for graphics display - correct x axis
  idx = 0;
  for(i=noDataPoints-1; i>=0 ; i--){
     wData.ageOfDatapoint[i] = idx * wData.targetMeasurementIntervalSec;
     idx++;
  }  

  sprintf(outstring,"After doubleMScale: wData.targetMeasurementIntervalSec: %ld",  wData.targetMeasurementIntervalSec);
  logOut(2,outstring);
  //outputStoredData(5, noDataPoints-5); // limited ouput
  outputStoredData(noDataPoints, 0);   // output all 
}

/**************************************************!
   @brief    Function to change the time scale of the measurement
   @details  This is done by adapting the stored measurement data and the measurement interval
   @param
   @return   void
***************************************************/
/*
void changeMeasurementScale()
{
  int i, mode;
  uint32_t oldTimeScale, newTimeScale; 
  
  // determine the scale
  oldTimeScale = wData.graphTimeRangeHours;

  // set the new scale accordingly
  if(oldTimeScale > 21){
    newTimeScale = oldTimeScale / 2;
    mode = 1; // timescale halfed
  }  
  else {
    newTimeScale = 84;  
    mode = 2; // timescale quadrupled
  }  
  
  // sort data for re-use
}
*/

/**************************************************!
   @brief    Function to store new measurement data in wData struct 
   @details  while shifting previous measurement data.
   @return   void
***************************************************/
void storeMeasurementData()
{
  int i;

  // set flag: we have measurement data
  wData.dataPresent = 1;

  // update counters fom local variables
  wData.startCounter   = startCounter;
  wData.dischgCnt      = dischgCnt;

  // Battery data
  wData.batteryVoltage = volt;
  wData.batteryPercent = percent;

  // sensor data BME280
  wData.actPressureRaw = pressure;
  wData.actPressureCorr= pressure + wData.pressureCorrValue;
  wData.actTemperature = temperature;
  wData.actHumidity    = (int)(10*humidity + 0.5);  // humidity stored in promille as integer

  // shift older data, check min / max
  wData.pressHistoryMax = -1;
  wData.pressHistoryMin = 100000;
  wData.tempHistoryMax = -300;
  wData.tempHistoryMin = 100000;
  wData.humiHistoryMax = 0;  // unsigned!
  wData.humiHistoryMin = 10000;

  for(i=0;i<noDataPoints-1;i++)
  {
    wData.pressHistory[i]  = wData.pressHistory[i+1];
    wData.tempHistory[i]   = wData.tempHistory[i+1];
    wData.humiHistory[i]   = wData.humiHistory[i+1];
    wData.ageOfDatapoint[i]= wData.ageOfDatapoint[i+1] + wData.lastTargetSleeptime; // shift and increment age of data point

    // also check for min / max in remaining data. Last point is checked in next step below
    if(wData.pressHistory[i] > wData.pressHistoryMax)  wData.pressHistoryMax = wData.pressHistory[i];
    if(wData.pressHistory[i] < wData.pressHistoryMin)  wData.pressHistoryMin = wData.pressHistory[i];
    if(wData.tempHistory[i] > wData.tempHistoryMax)  wData.tempHistoryMax = wData.tempHistory[i];
    if(wData.tempHistory[i] < wData.tempHistoryMin)  wData.tempHistoryMin = wData.tempHistory[i];
    if(wData.humiHistory[i] > wData.humiHistoryMax)  wData.humiHistoryMax = wData.humiHistory[i];
    if(wData.humiHistory[i] < wData.humiHistoryMin)  wData.humiHistoryMin = wData.humiHistory[i];
  }

  // add newest data
  wData.pressHistory[noDataPoints-1]  = wData.actPressureRaw;
  wData.tempHistory[noDataPoints-1]   = wData.actTemperature;
  wData.humiHistory[noDataPoints-1]   = wData.actHumidity;
  wData.ageOfDatapoint[noDataPoints-1]= 0; // latest data point is at 0 seconds age

  // min/max handling for new data
  if(wData.pressHistory[noDataPoints-1] > wData.pressHistoryMax)  wData.pressHistoryMax = wData.pressHistory[noDataPoints-1];
  if(wData.pressHistory[noDataPoints-1] < wData.pressHistoryMin)  wData.pressHistoryMin = wData.pressHistory[noDataPoints-1];
  if(wData.tempHistory[noDataPoints-1] > wData.tempHistoryMax)  wData.tempHistoryMax = wData.tempHistory[noDataPoints-1];
  if(wData.tempHistory[noDataPoints-1] < wData.tempHistoryMin)  wData.tempHistoryMin = wData.tempHistory[noDataPoints-1];
  if(wData.humiHistory[noDataPoints-1] > wData.humiHistoryMax)  wData.humiHistoryMax = wData.humiHistory[noDataPoints-1];
  if(wData.humiHistory[noDataPoints-1] < wData.humiHistoryMin)  wData.humiHistoryMin = wData.humiHistory[noDataPoints-1];

  sprintf(outstring,"Min/Max StorMeasedata: P: %3.1f-%3.1f T:  %3.1f-%3.1f H:  %d-%d", 
        wData.pressHistoryMin, wData.pressHistoryMax,  wData.tempHistoryMin,  wData.tempHistoryMax,
        wData.humiHistoryMin, wData.humiHistoryMax);
  logOut(2,outstring);      

  // don't forget to write the target sleeptime in sec last - needed next time!
  wData.lastTargetSleeptime = wData.targetMeasurementIntervalSec;
  //wData.targetMeasurementIntervalSec = targetSleepSec;
  // for test purpose only - fix to 15 min = 900 sec
  // wData.lastTargetSleeptime = 900; // !!! test
  // test purpose: correction value: minus 3h = 12 data points at 15 min per point
  // wData.pressure3hChange = wData.pressHistory[noDataPoints-1]-wData.pressHistory[noDataPoints-1-12];
  // now based on actual stored times:
  
  calc3HourChanges(); // calculate the changes of p,t,h since the time 3 hours before
}


/******************  determine wakeup reason *********************/
// https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/
uint32_t print_wakeup_reason() 
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     logOut(2,(char*)"Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     logOut(2,(char*)"Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    logOut(2,(char*)"Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: logOut(2,(char*)"Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      logOut(2,(char*)"Wakeup caused by ULP program"); break;
    default:                        sprintf(outstring,"Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
                                    logOut(2, outstring);
  }
  return(wakeup_reason);
}

#ifdef isPushButtons
  #ifdef isButtonInterrupts
  /*****************************************************************************! 
    @brief  push button interrupt routines
    @details 
    @return void
  *****************************************************************************/
  void ARDUINO_ISR_ATTR button1Handler()
  {
    if(!button1.pressed)  // avoid repeated pressed to be counted, entprellen
      button1.numberKeyPresses += 1;
    button1.pressed = true;
    //sprintf(outstring, "Button 1 pressed %d\n", button1.numberKeyPresses);
    //logOut(2,outstring);
  }

  void ARDUINO_ISR_ATTR button2Handler()
  {
    if(!button2.pressed)  // avoid repeated pressed to be counted, entprellen
    {
      button2.numberKeyPresses += 1;
      // change scale
      //changeMeasurementScale();
    }  
    button2.pressed = true;
    //printf(outstring, "Button 2 pressed %d\n", button2.numberKeyPresses);
    //logOut(2,outstring);
  }

  void ARDUINO_ISR_ATTR button3Handler()
  {
    if(!button3.pressed)  // avoid repeated pressed to be counted, entprellen
    {
      button3.numberKeyPresses += 1;
      // toggle pressure correction
      wData.applyPressureCorrection = !wData.applyPressureCorrection;

      //sprintf(outstring, "Button 3 pressed %d PressurecCorrection toggled to %d\n", 
      //        button3.numberKeyPresses, wData.applyPressureCorrection);
      //logOut(2,outstring);
    }  
    button3.pressed = true;
   }
   #endif // isButtonInterrupts
#endif // isPushButtons

/*****************************************************************************! 
  @brief  read all preferences values
  @details remember: short names (max 15 char), otherwise not stored
  @return void
*****************************************************************************/
void readPreferences()
{
    preferences.begin(prefIDENT, true);
    //----- counters etc.
    int bytes=preferences.getBytes("prefs", prefs, sizeof(prefs));
    //----- pressure correction
    if(preferences.isKey("applyPCorr"))
      wData.applyPressureCorrection = preferences.getBool("applyPCorr", d_applyPressureCorrection);
    else {  // set default    
      wData.applyPressureCorrection = d_applyPressureCorrection;
      preferences.putBool("applyPCorr", wData.applyPressureCorrection);
    }  
    //----- pressure correction value
    if(preferences.isKey("pCorrValue"))
      wData.pressureCorrValue = preferences.getFloat("pCorrValue", d_pressureCorrValue);
    else {  // set default    
      wData.pressureCorrValue = d_pressureCorrValue;
      preferences.putFloat("pCorrValue", wData.pressureCorrValue);
    }  
    //----- screen inversion
    if(preferences.isKey("applyInversion"))
      wData.applyInversion = preferences.getBool("applyInversion", d_applyInversion);
    else {  // set default    
      wData.applyInversion = d_applyInversion;
      preferences.putBool("applyInversion", wData.applyInversion);
    }  
    //----- measurement interval in sec
    if(preferences.isKey("measIntervalSec"))
      wData.targetMeasurementIntervalSec = preferences.getULong("measIntervalSec", d_measIntervalSec);
    else {  // set default    
      wData.targetMeasurementIntervalSec = d_measIntervalSec;
      preferences.putULong("measIntervalSec", wData.targetMeasurementIntervalSec);
    }  
    //+++ test: for initialization if it was wrong. Needs the changed-marker too to initiate writing
    //wData.targetMeasurementIntervalSec = d_measIntervalSec;
    //wData.preferencesChanged = true;

    //----- time range to display in hours
    if(preferences.isKey("timeRangeHours"))
      wData.graphTimeRangeHours = preferences.getULong("timeRangeHours", d_timeRangeHours);
    else {  // set default    
      wData.graphTimeRangeHours = d_timeRangeHours;
      preferences.putULong("timeRangeHours", wData.graphTimeRangeHours);
    }  

    //----- graphicsType
    if(preferences.isKey("graphicsType"))
      wData.graphicsType = preferences.getULong("graphicsType", 0);
    else {  // set default    
      wData.graphicsType = d_graphicsType;
      preferences.putULong("graphicsType", wData.graphicsType);
    }  

    //int bytes2= preferences.getBytes("teststring2", teststring2, 80); // test
    //preferences.remove("teststring1"); // remove single key
    //preferences.clear();  // clear the namespace completely
    preferences.end(); // close the namespace
  
    // if preference contain larger counter value, use these (likely power outage or firmware update)
    if(startCounter < prefs[0])
    {
      startCounter = prefs[0];
      dischgCnt    = prefs[1];
      prevMicrovolt= prefs[2];
      prevVoltage = (float)prevMicrovolt / 1000000;
    }
    //if(startCounter > 9999999) startCounter = 0; // rollover
    //if(dischgCnt > 9999999) dischgCnt = 0; // rollover

    sprintf(outstring,"Read Preferences: pcorr: %d pval: %3.1f Inv: %d measInt:%ld TRange:%ld Graph: %ld", 
                wData.applyPressureCorrection, wData.pressureCorrValue, wData.applyInversion,
                wData.targetMeasurementIntervalSec, wData.graphTimeRangeHours, wData.graphicsType);
    logOut(2,outstring);  
    sprintf(outstring,"Read Preferences: bytes: %d startCounter: %ld dischgCnt %ld prevVoltage %3.3f, prevMicrovolt %ld ", 
                bytes, startCounter, dischgCnt, prevVoltage, prevMicrovolt);
    logOut(2,outstring); 
}

/*****************************************************************************! 
  @brief  write all preference values
  @details 
  @return void
*****************************************************************************/
void writePreferences()
{
    size_t ret1, ret2, ret3, ret4, ret5, ret6, ret7;

    preferences.begin(prefIDENT, false);
    //----- counters etc.
    prefs[0] = startCounter;
    prefs[1] = dischgCnt;
    prefs[2] = prevMicrovolt;
    prefs[3] = 0;
    ret1 = preferences.putBytes(prefIDENT, prefs, sizeof(prefs));
    sprintf(outstring,"Wrote Counter Preferences: ret: %d ctrM:%ld ctrD:%ld prevMV:%ld [3]:%3.1f",
      ret1, startCounter, dischgCnt, prevMicrovolt, prefs[3]);
    logOut(3,outstring); 

    //----- pressure correction
    ret2 = preferences.putBool("applyPCorr", wData.applyPressureCorrection);

    //----- pressure correction value
    ret3 = preferences.putFloat("pCorrValue", wData.pressureCorrValue);

    //----- screen inversion
    ret4 = preferences.putBool("applyInversion", wData.applyInversion);

    //----- measurement interval in sec
    ret5 =   preferences.putULong("measIntervalSec", wData.targetMeasurementIntervalSec);

    //----- time range to display in hours
    ret6 =  preferences.putULong("timeRangeHours", wData.graphTimeRangeHours);

    //----- graphics type (0: pressure, 1: temperature, 2: humidity)
    ret7 =  preferences.putULong("graphicsType", wData.graphicsType);

    //int bytes2= preferences.getBytes("teststring2", teststring2, 80); // test
    //preferences.remove("teststring1"); // remove single key
    //preferences.clear();  // clear the namespace completely
    preferences.end(); // close the namespace

    sprintf(outstring,"Wrote Preferences: pcorr: %d pval: %3.1f Inv: %d measInt:%ld TRange:%ld", 
                wData.applyPressureCorrection, wData.pressureCorrValue, wData.applyInversion,
                wData.targetMeasurementIntervalSec, wData.graphTimeRangeHours);
    logOut(3,outstring);  
    sprintf(outstring,"Wrote Preferences: ret values: %d %d %d %d %d %d\n", 
                ret1, ret2, ret3, ret4, ret5, ret6);
    logOut(3,outstring); 
}


/*****************************************************************************! 
  @brief    gotoDeepSleep: routine to enter deep sleep
  @details  
  @param  deepSleepTime  time to to into deep sleep
  @return void
*****************************************************************************/
void gotoDeepSleep(gpio_num_t button, uint64_t deepSleepTime)
{
  //**********  TEST override
  // sleeptime = 60 * SECONDS - 1000*(millis()-startTimeMillis);
  uint32_t am = millis();
  sprintf(outstring,"Hibernating for target %ld sec: %lld usec (act millis: %ld start millis: %ld)    ", 
          wData.targetMeasurementIntervalSec, deepSleepTime, am, startTimeMillis);  
  logOut(2,outstring);
  if(deepSleepTime > maxSleeptimeSafetyLimit){
    sprintf(outstring,"Hibernating time %lld usec above safety limit. Reducing to %ld usec    ", 
          deepSleepTime, maxSleeptimeSafetyLimit);  
    logOut(2,outstring);
  }

  // shut down display
  endDisplay(1); // mode 0: power off, mode 1: hibernate
  //1: display.hibernate();  // danach wird beim wieder aufwachen kein Reset des Screens gemacht.
  //0: display.powerOff(); // danach wird beim wieder aufwachen ein voller Reset des Screens gemacht

  // Initiate sleep
  esp_sleep_enable_timer_wakeup(deepSleepTime);   // define sleeptime for timer wakeup
  // and via external pin. only one seems possible.
  // https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/
  #ifdef LOLIN32_LITE
    esp_sleep_enable_ext0_wakeup(button, HIGH); // enable wakeup via button1
  #endif  
  #ifdef CROW_PANEL
    esp_sleep_enable_ext0_wakeup(button, LOW); // enable wakeup via button1
  #endif  
  rtc_gpio_pullup_dis(button);  //Configure pullup/downs via RTCIO to LOW during deepsleep
  rtc_gpio_pulldown_en(button); // EXT0 resides in the same power domain (RTC_PERIPH) as the RTC IO pullup/downs.
    
  esp_deep_sleep_start();                               // go to sleep
}


/*****************************************************************************! 
  @brief  writeCounterPreferences()
  @details writes the counter related preference data
  @return void
*****************************************************************************/
void writeCounterPreferences()
{
  int ret;
  bool writing_counter_prefs = false;
  writing_counter_prefs = true;
  sprintf(outstring,"Store to preferences: startCounter %ld dischgCnt %ld prevVoltage %f prevMicrovolt %ld\n", 
     startCounter, dischgCnt, prevVoltage, prevMicrovolt),
  logOut(2,outstring);
  // open preferences namespace in rw mode mode and write preferences infos.
  preferences.begin(prefIDENT, false);
  prefs[0] = startCounter;
  prefs[1] = dischgCnt;
  prefs[2] = prevMicrovolt;
  prefs[3] = 0;
  ret = preferences.putBytes(prefIDENT, prefs, sizeof(prefs));
  preferences.end(); // close the namespace
  sprintf(outstring,"Write Counter preferenes ret: %d \n", ret);
  logOut(2,outstring);        
}

/*****************************************************************************! 
  @brief  handleExt0Wakeup()
  @details handle wakeup by EXT0 wakeup source = button via GPIO 
  @return void
*****************************************************************************/
void handleExt0Wakeup()
{
    // if woken up by button 1 short press: toggle display
    // not yet implemented

    // if woken up by button1 long press: goto bluetooth configuration routine
    #ifdef LOLIN32_LITE
      bluetoothConfigMain(); 
    #endif
    // if CrowPanel: try Web Bluetooth BLE 
    #ifdef CROW_PANEL
      bleConfigMain();
    #endif
}


/*****************************************************************************! 
  @brief  setup routine
  @details 
  @return void
*****************************************************************************/
void setup()
{
  startTimeMillis = millis(); // remember time when woken up

  char* cp;
  Serial.begin(115200);   // set speed for serial monitor

  logOut(2,(char*)"**********************************************************");
  sprintf(outstring,"* %s %s - %s ",PROGNAME, VERSION, BUILD_DATE);
  logOut(2,outstring);
  logOut(2,(char*)"**********************************************************");

  #ifdef READ_PREFERENCES
    // get data from EEPROM using preferences library in readonly mode
    readPreferences();
  #else
    sprintf(outstring,"Values from RTC memory: startCounter: %ld dischgCnt %ld prevVoltage %3.3f, prevMicrovolt %ld ", 
                startCounter, dischgCnt, prevVoltage, prevMicrovolt);
    logOut(2,outstring);   
  #endif // READ_PREFERENCES        

  // create test data if required
  #ifdef createTestData
    if(wData.dataPresent == 0) {
      fillTestData();
    }  
    if(wData.preferencesChanged)    // if new preferences set during test data creatin: write preferences.
      writePreferences();  // otherwise, the followint readPreferences() would directly overwrite the changed start data
  #endif

  // initialize the display
  logOut(2,(char*)"before initDisplay()");
  initDisplay(startCounter, FULL_UPDATE_INTERVAL); 

  // BME280 initialization. First set the pins I2C, not available in standard for Lolin32 Lite.
  // Default object is : TwoWire Wire;
  Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
  // Init BME280 I2C address depends on sensor 0x76 or 0x77.
  if (!bme.begin(0x76, &Wire)){
    logOut(2,(char*)"Could not find a valid BME280 sensor, check wiring!");
    //delay(2000);
  }  
 
  // recommended settings for weather monitoring
  logOut(2,(char*)"-- Weather Station Scenario --");
  logOut(2,(char*)"forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
  logOut(2,(char*)"filter off");
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );

  // PushButton setup
  #ifdef isPushButtons
    // pinMode(PushButton, RISING);
    
    pinMode(button1.PIN, INPUT_PULLDOWN);
    pinMode(button2.PIN, INPUT_PULLDOWN);
    pinMode(button3.PIN, INPUT_PULLDOWN);    
    #ifdef isButtonInterrupts
      attachInterrupt(digitalPinToInterrupt(button1.PIN),button1Handler, FALLING); 
      attachInterrupt(digitalPinToInterrupt(button2.PIN),button2Handler, FALLING); 
      attachInterrupt(digitalPinToInterrupt(button3.PIN),button3Handler, FALLING); 
    #endif
  #endif // isPushButtons    

  sprintf(outstring,"Heap Size: %ld FreeHp: %ld Max Block Alloc: %ld    ",
      ESP.getHeapSize(),ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  logOut(2, outstring);    
  sprintf(outstring,"Total PSRAM: %ld Free PSRAM: %ld    \n",
      ESP.getPsramSize(),ESP.getFreePsram());    
  logOut(2, outstring);    
}  

/************************** doWork - main worker routine ****************************/
void doWork()
{
  struct timeval nowTime;
  uint64_t sleeptime;
  uint32_t ret, stSec, stUsec; 
  long elapsedSec, elapsedUsec;
  int i;
  time_t      nowSec, measSec;    // seconds since 00:00:00 on January 1, 1970, Coordinated Universal Time. 
  suseconds_t nowUsec, measUsec;   // additional microseconds, never more than a million. Add both to get precise time
  bool readyToMeasure = false;

  // determine reason for wakeup. if timer wakeup: continue with measurements. 
  // if EXT0 wakeup (button pressed): handleExt0Wakeup()
  ret=print_wakeup_reason(); // determine reason for wakeup
  // switch colors if woken up by GPIO = key pressed
  if(ret == ESP_SLEEP_WAKEUP_EXT0)
    handleExt0Wakeup();

  // check if time for measurement or continue to sleep
  // get time. nowTime: present time, just determined. wData.lastMeasurementTimestamp: timeval at last measurement
  gettimeofday(&nowTime, NULL);                         // get time struct
  nowSec= nowTime.tv_sec; nowUsec = nowTime.tv_usec;
  measSec = wData.lastMeasurementTimestamp.tv_sec; measUsec = wData.lastMeasurementTimestamp.tv_usec;
  sprintf(outstring,"doWork: nowSec: %ld nowUsec: %ld measSec: %ld measUsec: %ld    ",
    nowSec,  nowUsec, measSec, measUsec);
  elapsedSec = (long)nowTime.tv_sec - (long)wData.lastMeasurementTimestamp.tv_sec;
  elapsedUsec= (long)nowTime.tv_usec - (long)wData.lastMeasurementTimestamp.tv_usec; // can be negative, therefore singed type!
  elapsedUsec+= 1000000*elapsedSec;  
  // compare time with target sleep time
  // if(nowSec-measSec > wData.lastTargetSleeptime - 5)    // enough time elapsed? we need 5 sec for measurement and display
  if((elapsedUsec + 500000> wData.lastActualSleeptimeAfterMeasUsec)||wData.justInitialized) // unsigned, therefore addition on left side (would overflow at start, 0)
    readyToMeasure = true;
  else  
    readyToMeasure = false;  

  sprintf(outstring,"DoWork. nowSec: %ld nowUsec: %ld measSec: %ld measUsec:%ld lastSleepAftM: %lld    ",
          nowSec, nowUsec, measSec, measUsec, wData.lastActualSleeptimeAfterMeasUsec);
  logOut(2,outstring);  

  sprintf(outstring,"DoWork. now: %ld.%06ld lastMeas: %ld.%06ld elapsed s:%ld usec:%ld ReadytoMeas: %d    ",
          nowTime.tv_sec,nowTime.tv_usec, 
          wData.lastMeasurementTimestamp.tv_sec, wData.lastMeasurementTimestamp.tv_usec,
          elapsedSec, elapsedUsec,
          readyToMeasure);
    logOut(2,outstring);  

  if(!readyToMeasure){  // if time not reached: calculate new sleeptime and go to sleep
    drawMainGraphics(wData.graphicsType);
    // write all preferences, incl. counter, if changed in bluetooth setup
    if(wData.preferencesChanged){
      writePreferences();
      wData.preferencesChanged = false;
    }
    gettimeofday(&nowTime, NULL);                         // get time struct
    elapsedSec = nowTime.tv_sec - wData.lastMeasurementTimestamp.tv_sec;
    elapsedUsec= nowTime.tv_usec - wData.lastMeasurementTimestamp.tv_usec; // can be negative, therefore singed type!
    elapsedUsec+= 1000000*elapsedSec;                     // now we have the actually elapsed usec since last measurement
    targetSleepUSec= wData.targetMeasurementIntervalSec * SECONDS;
    // catch negative numbers, problematic since unsigned long integers!
    if(1000000*wData.lastTargetSleeptime > elapsedUsec)
      sleeptime = 1000000*wData.lastTargetSleeptime - elapsedUsec;
    else  
      sleeptime = 1000000;    
    //wData.lastActualSleeptimeAfterMeasUsec = sleeptime;
    sprintf(outstring,"Before gotoToSleep notReadyTo Measure. now: %ld.%06ld lastMeas: %ld.%06ld elapsedUsec %ld lastTargetS:%ld sleeptime: %lld",
          nowTime.tv_sec,nowTime.tv_usec, 
          wData.lastMeasurementTimestamp.tv_sec, wData.lastMeasurementTimestamp.tv_usec,
          elapsedUsec,
          wData.lastTargetSleeptime, sleeptime);
    logOut(2,outstring);
    // one more safety: when we have a measurement due to initialization, the sleep time becomes larger than the 
    // target sleep time. We have to subtract the targetSleeptimeUsec again. 3 sec tolerance added in comparison.
    if(sleeptime > targetSleepUSec + 10000000)
        sleeptime = sleeptime - targetSleepUSec;
    // this is the number in usec actually used to set the sleep timer when no measurement taken
    wData.lastActualSleeptimeNotMeasUsec = sleeptime;
    gotoDeepSleep(BUTTON1, sleeptime); // go to deep sleep. parameters: sleeptime in us, button to wakeup from  
  }
  else{   // if time reached: continue with measurement
    measTimeMillis = millis();                            // remember millis at measurement
    getBME280SensorData();                                // Lesen der Messwerte vom BME280

    // remember time and store it after checking how long since last measurement
    // https://github.com/espressif/esp-idf/blob/master/examples/system/deep_sleep/main/deep_sleep_example_main.c
    gettimeofday(&nowTime, NULL);                         // get time struct
    int sleep_time_sec = (nowTime.tv_sec - wData.lastMeasurementTimestamp.tv_sec);
    wData.actSecondsSinceLastMeasurement = sleep_time_sec // store seconds passed
      + (float)(nowTime.tv_usec - wData.lastMeasurementTimestamp.tv_usec)/1000000;  // including microseconds, which reset to 0 every second
    
    elapsedSec = nowTime.tv_sec - wData.lastMeasurementTimestamp.tv_sec;
    elapsedUsec= nowTime.tv_usec - wData.lastMeasurementTimestamp.tv_usec; // can be negative, therefore singed type!
    elapsedUsec+= 1000000*elapsedSec;            
    sprintf(outstring,"After Measurement. now: %ld.%06ld lastMeas: %ld.%06ld elapsedUsec %ld    ",
          nowTime.tv_sec,nowTime.tv_usec, 
          wData.lastMeasurementTimestamp.tv_sec, wData.lastMeasurementTimestamp.tv_usec,
          elapsedUsec);
    logOut(2,outstring);

    sprintf(outstring,"Timestamps before assignment.last: %ld.%06ld last2: %lD.%06ld    ",
        wData.lastMeasurementTimestamp.tv_sec, wData.lastMeasurementTimestamp.tv_usec,
        wData.last2MeasurementTimestamp.tv_sec, wData.last2MeasurementTimestamp.tv_usec);
    logOut(2,outstring);
    // remember the previous measurement timestamp
    wData.last2MeasurementTimestamp = wData.lastMeasurementTimestamp; 
    // store actual time as measurement time.
    gettimeofday(&wData.lastMeasurementTimestamp, NULL);         
    sprintf(outstring,"Timestamps after assignment. last: %ld.%06ld last2: %lD.%06ld    ",
        wData.lastMeasurementTimestamp.tv_sec, wData.lastMeasurementTimestamp.tv_usec,
        wData.last2MeasurementTimestamp.tv_sec, wData.last2MeasurementTimestamp.tv_usec);
    logOut(2,outstring);

    #ifdef LOLIN32_LITE
      readBatteryVoltage(&percent, &volt);                  // Auslesen der Batteriespannung
    #endif  
    sprintf(outstring,"Voltage: %4.3f prevVoltage: %4.3f Percent: %3.1f    \n", volt, prevVoltage, percent);
    logOut(2,outstring);

    // recognize start of a discharge: voltage has decreased significantly, of a charge: voltage has increased significantly
    if((volt - prevVoltage > CHARGE_THRESHOLD)||(volt - prevVoltage < -DISCHARGE_THRESHOLD)){
      dischgCnt = 0;
      sprintf(outstring,"Reset dischgCnt to: %ld volt: %3.2f prevVoltage: %f Percent: %3.1f    \n", 
            dischgCnt, volt, prevVoltage, percent);
      logOut(2,outstring);
    }  
    prevVoltage = volt;  

    // set sleep time. 
    targetSleepUSec= wData.targetMeasurementIntervalSec * SECONDS;
    sprintf(outstring,"target sleep time: %ld WData.mIS: %ld, SEC: %ld    ", targetSleepUSec, wData.targetMeasurementIntervalSec, SECONDS);
    logOut(2,outstring);

    // store data to main measurement data structure
    storeMeasurementData();

    #ifdef showSimpleData
      displayTextData(startCounter, dischgCnt, temperature, humidity, pressure, 
                        percent,volt, multiplier);
    #else
      drawMainGraphics(wData.graphicsType);
    #endif    

    logOut(2,(char*)"measurement and display done");
    // increment counter and write it to permanent storage
    startCounter++;
    dischgCnt++;
    prevVoltage = volt;
    prevMicrovolt= (int)(0.5+1000000*prevVoltage);

    #ifdef WRITE_PREFERENCES
      // write counter preferences
      if(startCounter % WRITE_PREFS_INTERVAL == 0){
        writeCounterPreferences();
      }
      // write all preferences, incl. counter
      if(wData.preferencesChanged){
        writePreferences();
        wData.preferencesChanged = false;
      }
    #else
      sprintf(outstring,"Values in RTC memory: startCounter %d dischgCnt %d prevVoltage %f prevMicrovolt %d\n", 
            startCounter, dischgCnt, prevVoltage, prevMicrovolt),
    #endif //WRITE_PREFERENCES

    #ifdef USESLEEP
      if(button1.pressed || button2.pressed || button3.pressed){
          sprintf(outstring,"Button pressed: %ld %ld %ld",
            button1.numberKeyPresses,button2.numberKeyPresses,button3.numberKeyPresses);
          logOut(2,outstring);
          button1.pressed = false;
          button2.pressed = false;
          button3.pressed = false;
      }
      
      /* trial 1
      delay(100); // to give time to properly store before hibernating
      uint32_t am = millis();
      sleeptime = targetSleepUSec - 1000*(am-startTimeMillis);
      */
      /* trial 2 - correct with respect to window over 2 cycles
      gettimeofday(&nowTime, NULL);         // get time struct
      sprintf(outstring,"Timestamps before sleep. now: %ld.%06ld last2: %lD.%06ld",
        nowTime.tv_sec, nowTime.tv_usec,
        wData.last2MeasurementTimestamp.tv_sec, wData.last2MeasurementTimestamp.tv_usec);
      logOut(2,outstring);
      elapsedSec = nowTime.tv_sec - wData.last2MeasurementTimestamp.tv_sec; // compare with the remembered meas. time before
      elapsedUsec= nowTime.tv_usec- wData.last2MeasurementTimestamp.tv_usec; // can be negative, therefore singed type!
      elapsedUsec+= 1000000*elapsedSec;     // now we have the actually elapsed usec since last measurement

      long corr1 = targetSleepUSec - elapsedUsec;
      float dampingFactor = 0.8;
      long corr = corr1 * dampingFactor;
      sprintf(outstring,"sleeptime factors: elsapsedUsec:%ld targetMeasIntv: %ld %ld corr1: %ld corr: %ld",
         elapsedUsec, wData.targetMeasurementIntervalSec, targetSleepUSec, corr1, corr);
      logOut(2,outstring);
      
      if(elapsedSec < 2 * wData.targetMeasurementIntervalSec) // correct sleeptime, with damping to allow drift towards center 
        sleeptime = targetSleepUSec + corr; // instead of 230/220/230/220 sec now: 230/221/228/222/227/223/226/224...
      else
        sleeptime = 2 * targetSleepUSec;  // if abnormally large elapsed time: safety measure - limit sleeptime to 2* targetSleeptime
      //lastMeas  -|              -|
      //           |               | elapsed (e.g. 235 sec)
      //now        |2* target     -|
      //           | (e.g. 2*225)  | sleeptime = 2*target - elapsed (e.g. = 450 - 235 = 215 sec )
      //next meas -|              -|
      //
      
      // one more safety: when we have a measurement due to initialization, the sleep time becomes larger than the 
      // target sleep time. We have to subtract the targetSleeptimeUsec again. 3 sec tolerance added in comparison.
      // removed, since this tends to shorten the intervals permanently if initial short meas. times
      //if(sleeptime > targetSleepUSec + 10000000); // 10000000 takes into account the runtime of measurements etc. Needed, or every 2nd meas very short times...
      //  sleeptime = sleeptime - targetSleepUSec;

      // set correct sleeptime if software just initialized
      if(wData.justInitialized){
        uint32_t am = millis();
        sleeptime = targetSleepUSec - 1000*(am-measTimeMillis);
        sprintf(outstring,">>>>> JUST INITIALIZED. sleeptime set to %ld", sleeptime);
        wData.justInitialized = false;
      }

      wData.lastActualSleeptimeAfterMeasUsec = sleeptime;
      sprintf(outstring,"Before sleep elapsedUs: %ld sleeptime: %lld, now: %ld.%06ld last: %lD.%06ld    ",
        elapsedUsec, sleeptime, 
        nowTime.tv_sec, nowTime.tv_usec,
        wData.last2MeasurementTimestamp.tv_sec, wData.last2MeasurementTimestamp.tv_usec);
      logOut(2,outstring);  
      end of trial 2*/ 

      /* trial 3 - simplified */
      uint32_t am = millis();
      sleeptime = targetSleepUSec - 1000*(am-startTimeMillis);
      //sleeptime = targetSleepUSec - 1000*(am-measTimeMillis);
      wData.lastActualSleeptimeAfterMeasUsec = sleeptime;
      wData.justInitialized = false;
      sprintf(outstring, "simplified sleep calc. targetSlUSec: %ld startTM:%ld measTM: %ld actTM:%ld sleeptime: %lld      ",
          targetSleepUSec, startTimeMillis, measTimeMillis, am, sleeptime);
      logOut(2,outstring);    
      gotoDeepSleep(BUTTON1, sleeptime); // go to deep sleep. parameters: sleeptime in us, button to wakeup from
    #else
      logOut(2,(char*)"Sleep has been disabled");

      // test for buttons only in loop
      for(i=0; i<50; i++){
        if(button1.pressed || button2.pressed || button3.pressed){
          sprintf(outstring,"Button presed: %d %d %d",
            button1.numberKeyPresses,button2.numberKeyPresses,button3.numberKeyPresses);
          logOut(2,outstring);
          button1.pressed = false;
          button2.pressed = false;
          button3.pressed = false;
        }
        delay(250);
      }  
    #endif 
  } // else  if time reached
}


/*****************************************************************************! 
  @brief  main loop
  @details 
  @return void
*****************************************************************************/
void loop() 
{
  int ret, ret1, ret2;
  char* cp;
  bool writing_counter_prefs = false;

  // clearScreenPartialUpdate();
  // clearScreenFullUpdate();
  doWork();

};
