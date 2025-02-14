#ifndef _ePaperGraphics_H
#define _ePaperGraphics_H

//*************** defines ******************************/
// graph display range selection limits
#define drLimitUpper4  2.5
#define drLimitUpper8  5
#define drLimitUpper12  7
#define drLimitUpper20  10
#define drLimitUpper40  20
#define drLimitUpper100 55
#define drLimitUpper200 110
#define drLimitUpper400 240

// tendency graphics limits
#define pressureTendencyLimit1 1.0
#define pressureTendencyLimit2 2.0
#define pressureTendencyLimit3 3.0

#define humidityTendencyLimit1 2.0
#define humidityTendencyLimit2 5.0
#define humidityTendencyLimit3 10.0

#define temperatureTendencyLimit1 0.5
#define temperatureTendencyLimit2 1.0
#define temperatureTendencyLimit3 2.0


// define how many of the data are printed while drawing
#define noPRINTLINESLOW 2// noDataPoints/2 //2
#define noPRINTLINESHIGH 2 // noDataPoints/2 //2

//*************** function prototypes ******************/
void drawPressureGraphics();
void displayTextData( uint32_t startCounter, uint32_t  dischgCnt,
                      float temperature, float humidity, float pressure,
                      float percent, float volt, uint32_t multiplier);

#endif // _ePaperGraphics_H