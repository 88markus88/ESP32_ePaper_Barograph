#ifndef _ePaperBarograf_H
#define _ePaperBarograf_H
// From Arduino project 
// Connections for e.g. LOLIN D32 according to WeAct. Works with WeAct 4.2" Display. This is the WeAct Standard
/*
static const uint8_t EPD_BUSY = 15; // to EPD BUSY
static const uint8_t EPD_CS   = 5;  // to EPD CS
static const uint8_t EPD_RST  = 2;  // to EPD RST
static const uint8_t EPD_DC   = 0;  // to EPD DC
static const uint8_t EPD_SCK  = 18; // to EPD CLK
static const uint8_t EPD_MISO = 19; // Master-In Slave-Out not used, as no data from display
static const uint8_t EPD_MOSI = 23; // to EPD DIN
*/ 

// Screen parameters
#include "screenParameters.h"

// global stuff (variables, function prototypes)


//***  defines for BME280
/* THIS IS NEEDED FOR Lolin32 Lite ONLY */
/* for normal ESP32 boards, use pins 21 + 22 */
#define I2C_SDA_PIN 15
#define I2C_SCL_PIN 19

//*** defines for sleep */
#define USESLEEP
#define SECONDS (1000 * 1000)   // 1 second = 1 Mio microseconds
//#define SLEEPTIME 60            // sleep time in seconds

//**** use push buttons */
#define isPushButtons 
#undef showSimpleData // no simple data display, but full graphics

//*** define for voltage threshhold: recognition of chage/discharge if voltage increases/ drops more than this
#define CHARGE_THRESHOLD      0.025
#define DISCHARGE_THRESHOLD   0.025

// sleeptimer multipliers for battery percentages in minutes. 1 == 1 min, 10 = 10 min 
#define MULTIPLIER_FULL 1 // 1
#define MULTIPLIER_50   2 // 2
#define MULTIPLIER_20   10 // 5
#define MULTIPLIER_10   30 // 10
#define MULTIPLIER_5    60 // 30

/************************** forward declarations *************************/
void getBME280SensorData();
int readBatteryVoltage(float* percent, float* volt);
//void clearScreenPartialUpdate();
//void clearScreenFullUpdate();
//void displaySimpleData( uint32_t startCounter, uint32_t dischgCnt,
//                        float temperature, float humidity, float pressure,
//                        float percent, float volt, uint32_t multiplier);
void doWork();
uint32_t print_wakeup_reason();

#endif // _ePaperBarograf_H