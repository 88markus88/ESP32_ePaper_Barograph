/**************************************************!
   main drawing functions
***************************************************/

#include <Arduino.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

// Screen parameters
#include "screenParameters.h"

// GxEPD2 ePaper library
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>

#ifdef CROW_PANEL
  // limit display buffer, in case of memory problems  
  #define MAX_DISPLAY_BUFFER_SIZE 32768ul 

  // Connections for Crowpanel with 4,2" display
  static const uint8_t EPD_PWR  = 7;   // to EPD BUSY
  static const uint8_t EPD_BUSY = 48;  // to EPD BUSY
  static const uint8_t EPD_CS   = 45;  // to EPD CS
  static const uint8_t EPD_RST  = 47; // to EPD RST
  static const uint8_t EPD_DC   = 46; // to EPD DC
  static const uint8_t EPD_SCK  = 12; // to EPD CLK
  static const uint8_t EPD_MOSI = 11; // to EPD DIN / SDA
  // display class definition  for GxRPD  from class template
  // this works for the WeAct 4.2" ePaper 400x300
  //GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=D8*/ EPD_CS, /*DC=D3*/ EPD_DC, /*RST=D4*/ EPD_RST, /*BUSY=D2*/ EPD_BUSY));

  // this should work for crowpanel, but does not.
  // GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> display(GxEPD2_420_GYE042A87(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

  // copied from tommy64
  #define PWR 7
  #define BUSY 48
  #define RES 47
  #define DC 46
  #define CS 45

  #define I2C_SDA 15
  #define I2C_SCL 19

  // Display-Objekt
  // 1/4 of screen buffer size, requires paged drawing
  // GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT/4> display(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));
  // full buffer size, does not require paged drawing
  GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> display(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));
#endif // CROW_PANEL

#ifdef LOLIN32_LITE 
  // Connections for LOLIN D32 
  static const uint8_t EPD_BUSY = 4;  // to EPD BUSY
  static const uint8_t EPD_CS   = 5;  // to EPD CS
  static const uint8_t EPD_RST  = 16; // to EPD RST
  static const uint8_t EPD_DC   = 17; // to EPD DC
  static const uint8_t EPD_SCK  = 18; // to EPD CLK
  static const uint8_t EPD_MOSI = 23; // to EPD DIN
  // display class definition  for GxRPD  from class template
  // this works for the WeAct 4.2" ePaper 400x300
  GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=D8*/ EPD_CS, /*DC=D3*/ EPD_DC, /*RST=D4*/ EPD_RST, /*BUSY=D2*/ EPD_BUSY)); 
#endif // LOLIN32_LITE 

//***************** general libraries ****************************
// Genutzte Schriften importieren
// https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
// https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html#workflow 
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#include "ePaperGraphics.h"
#include "global.h"

// platformio libdeps: olikraus/U8g2_for_Adafruit_GFX@^1.8.0
#include <U8g2_for_Adafruit_GFX.h>
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

// Using fonts:
// u8g2_font_helvB08_tf
// u8g2_font_helvB10_tf
// u8g2_font_helvB12_tf
// u8g2_font_helvB14_tf
// u8g2_font_helvB18_tf
// u8g2_font_helvB24_tf



//****************** file global variables ********************************/


//*************************** inspect a c-string for debug purposes **********/
void inspectCString(char* str)
{
  int i, cn;
  size_t strl;
  char cc;

  strl = strlen(str);
  // logOut(2,"-----------------------------------------------------------------");
  sprintf(outstring,"Len: %d", strl);
  logOut(1, outstring);
  for (i=0; i<= strl;i++)
  {
    cn = (int)str[i];
    cc = (char)str[i];
    sprintf(outstring,"%d [%3d]%c ",i, cn, cc);
    logOut(1, outstring);
  }
}

//+++++++++++++++++++++++++++ clear screen using partial update ++++++++++++++
void clearScreenPartialUpdate()
{
  display.setPartialWindow(0, 0, display.width(), display.height()); 
  
  do{
    display.fillScreen(bgndColor);
  }while(display.nextPage());
}  

//+++++++++++++++++++++++++++ clear screen using full update ++++++++++++++
void clearScreenFullUpdate()
{
  display.setFullWindow();
  do{
    display.fillScreen(bgndColor);
  }while(display.nextPage());
};  


//************************** Display Data ****************************/

#ifdef showSimpleData
void displayTextData( uint32_t startCounter, uint32_t dischgCnt,
                      float temperature, float humidity, float pressure,
                      float percent, float volt, uint32_t multiplier)
{
  int y;

  //display.setRotation(1);                // Display um 90° drehen
  display.setTextColor(fgndColor);     // Schriftfarbe Schwarz
  display.setFont(&FreeMonoBold18pt7b);  // Schrift definieren

  sprintf(outstring, "Start of displaySimpleData() ");
  logOut(2,outstring);

  display.firstPage();
  do{
      // Rechteck mit weissem Hintergrund erstellen
      //X-Position, Y-Position, Breite, Höhe, Farbe. Here full screen
      //display.fillRect(0, 0, display.width(), display.height(), bgndColor); //Xpos,Ypos,box-w,box-h
      
      y=12;
      logOut(2, "2");
      // Titel schreiben
      display.setCursor(0, y);
      display.setFont(&FreeMonoBold9pt7b);
      #ifdef VERSION
        sprintf(outstring, "WetterBME280 %s",VERSION);
      #else
        sprintf(outstring,"Wetter BME280");
      #endif  
      display.print(outstring);

      y = 48; // 40
      // Temperatur schreiben
      display.setCursor(0, y);
      display.setFont(&FreeMonoBold24pt7b);
      //display.setFont(&FreeMono24pt7b);
      if (temperature >= 10) display.print("+");
      if (temperature < 0) display.print("-");
      if (temperature >= 0 & temperature < 10) display.print(" +");
      display.print(temperature, 1);
      display.setCursor(140, y);
      display.setFont(&FreeMonoBold12pt7b);
      display.print(" C");
      logOut(2,"3");

      // Da bei der Schrift kein Grad Zeichen vorhanden ist selber eins mit Kreisen erstellen
      display.fillCircle(150, 23+5, 4, fgndColor);  //Xpos,Ypos was 23,r,Farbe
      display.fillCircle(150, 23+5, 2, bgndColor);  //Xpos,Ypos,r,Farbe

      // Luftfeuchtigkeit schreiben
      y = 84; // 70
      display.setCursor(0, y);
      display.setFont(&FreeMonoBold24pt7b);
      //display.setFont(&FreeMono24pt7b);
      if (humidity >= 20) display.print(" ");
      if (humidity >= 0 & humidity < 20) display.print("  ");
      display.print(humidity, 1);
      display.setCursor(140, y);
      display.setFont(&FreeMonoBold12pt7b);
      display.print(" %");
      logOut(2,"4");

      // Luftdruck schreiben
      y = 120; // 100
      if(pressure < 1000)
        display.setFont(&FreeMonoBold24pt7b);
      else  
        display.setFont(&FreeMonoBold18pt7b);
      //display.setFont(&FreeMono24pt7b);
      display.setCursor(0, y);
      display.print(pressure, 1);
      if (pressure < 1000) display.print(" ");
      display.setFont(&FreeMonoBold12pt7b);
      display.setCursor(140, y);
      display.print(" hPa");
      logOut(2,"5");

      // Batteriedaten und startCounter schreiben
      int x_offset=15; // X-Offset to take care of damaged display in grey unit
      display.setFont(&FreeMonoBold9pt7b);  // Schrift definieren
      display.setCursor(x_offset, 199);  

      sprintf(outstring, "%4.3f V %2.0 %", volt, percent);
      display.print(outstring);
      //display.print(volt, 3);
      //display.print("V ");
      //display.print(percent, 0);
      //display.print("% ");

      sprintf(outstring, "sl:%dm",multiplier); 
      display.print(outstring);
      // print 1-4 "!" depending on battery status. Corresponds to sleeptime multiplier
      // percent = 0 means: no battery
      display.write(0x08);
     

      display.setCursor(x_offset, 185);  
      display.print(startCounter);
      display.print(" ");
      display.print(dischgCnt);
      logOut(2,"7");
      /*
      display.print((float)lastTime/1000,1);
      display.print(" sec");
      */

  }while (display.nextPage());
  logOut(2," 8");
  // Teil refresh vom  Display
  // display.updateWindow(0, 0, display.width(), display.heigth(), false);
}
#endif // showSimpleData

#undef TEST_CROW_PANEL
#ifdef CROW_PANEL

// power on for crow panel
// https://www.segeln-forum.de/thread/93278-standalone-barograph-diy/?postID=2733566#post2733566
// https://github.com/norbert-walter/esp32-nmea2000-obp60/tree/master
void epdPower(int state) {
  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR, state);
}
#ifdef TEST_CROW_PANEL
void epdInit() {
  display.init(115200, true, 50, false);
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold12pt7b);
  display.setFullWindow();
}

#define TIME_STEP 15 // Minuten
#define NUM_POINTS 96 // 24 Stunden (alle 15 Minuten ein Wert)
#define MIN_PRESSURE 950 // Minimaler Luftdruck in hPa
#define MAX_PRESSURE 1070 // Maximaler Luftdruck in hPa
#define GRAPH_X 20 // X-Position des Graphen
#define GRAPH_Y 50 // Y-Position des Graphen
#define GRAPH_WIDTH 300 // Breite des Graphen
#define GRAPH_HEIGHT 220 // Höhe des Graphen

void epdTest()
{
  display.fillScreen(GxEPD_WHITE);

  // Achsen zeichnen
  display.drawRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, GxEPD_BLACK);

  // Y-Achse Labels und Hilfslinien
  static const int ySteps = 12; // Anzahl der Schritte auf der Y-Achse
  static const int yLabels[] = { 950, 960, 970, 980, 990, 1000, 1010, 1020, 1030, 1040, 1050, 1060, 1070 };

  for (int i = 0; i <= ySteps; i++) {
    int y = GRAPH_Y + GRAPH_HEIGHT - (i * GRAPH_HEIGHT / ySteps);
    // Hilfslinien für alle Werte zeichnen
    display.drawLine(GRAPH_X, y, GRAPH_X + GRAPH_WIDTH, y, GxEPD_BLACK);

    // Nur "ungerade" Labels (950, 970, 990, ...) anzeigen
    if (i % 2 != 0) { // Wenn Index ungerade ist, überspringen
      continue;
    }
  int16_t x1, y1;
  uint16_t w, h;
  String label = String(yLabels[i]);
  display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(GRAPH_X + GRAPH_WIDTH + 60 - w, y + h / 2);
  display.print(label);
  }
}
#endif // TEST_CROW_PANEL
#endif // CROW_PANEL


// initialize display, taken from setup()
void initDisplay(int startCounter, int fullInterval)
{
  #ifdef CROW_PANEL
    epdPower(HIGH);

    #ifdef TEST_CROW_PANEL
      epdInit();
      logOut(2,"Testing panel");
      epdTest();
      delay(3000);
    #endif // TEST_CROW_PANEL  
  #endif // CROW_PANEL  
  // full refresh of epaper every fullInterval's time. 1: every time
  if (startCounter % fullInterval == 0)
  {
    logOut(2,(char*)"+++++++ Full window clearing");
    display.init(115200, true, 2, false); // initial = true  for first start
    display.setFullWindow();
  }
  else{
    logOut(2,(char*)"------- Partial window clearing");
    display.init(115200, false, 2, false); // initial = false for subsequent starts
    display.setPartialWindow(0, 0, display.width(), display.height());
  }
  // set colors, if not yet initialiazed (first run of program)
  /*
  if((fgndColor != GxEPD_BLACK)&&(fgndColor!=GxEPD_WHITE))
    {fgndColor = GxEPD_BLACK;bgndColor = GxEPD_WHITE;}
  if((bgndColor != GxEPD_BLACK)&&(bgndColor!=GxEPD_WHITE))
    {fgndColor = GxEPD_BLACK;bgndColor = GxEPD_WHITE;}
  if(bgndColor == fgndColor)
    {fgndColor = GxEPD_BLACK;bgndColor = GxEPD_WHITE;}  
  */
  if(wData.applyInversion){
    fgndColor = GxEPD_WHITE;
    bgndColor = GxEPD_BLACK;
  }
  else{
    fgndColor = GxEPD_BLACK;
    bgndColor = GxEPD_WHITE;    
  }
  // prepare display colors (adafruit)
  display.setTextColor(fgndColor, bgndColor);  

  // prepare u8g2 fonts
  u8g2Fonts.begin(display); // connect u8g2 procedures to Adafruit GFX
  u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);             // left to right (this is default)
  u8g2Fonts.setForegroundColor(fgndColor); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(bgndColor); // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

}

// power off display
void endDisplay(int mode)
{
    if(mode ==0)  
      display.powerOff(); // danach wird beim wieder aufwachen ein voller Reset des Screens gemacht
    else
      display.hibernate();  // danach wird beim wieder aufwachen kein Reset des Screens gemacht.
}

/**************************************************!
   @brief    prepareGraphicsParameters
   @details  sets the graphics related parameters within global struct wData
   @details  takes into account if 72 or 84 hour graph
   @details  recalculates the min and max values according to the time scale of the graph
   @param    hours : 72 or 84, determines which graph to be prepared
   @return   void
***************************************************/
void prepareGraphicsParameters(uint16_t hours)
{
  int i;
  
  if((hours != 72) && (hours != 84))
  {
    sprintf(outstring,"prepareGraphicsParameters: wrong time range %d", hours);
    logOut(2,outstring);
    return;
  }
  #ifdef extendedDEBUG_OUTPUT
  logOut(2,(char*)"prepareGraphicsParameters started");
  sprintf(outstring,"Heap Size: %ld FreeHp: %ld Max Alloc: %ld",
      ESP.getHeapSize(),ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  logOut(2,outstring);    
  #endif    

  switch(hours){
    case 72:
      wData.indexFirstPointToDraw = offsetData72hGraph;
    break;
    case 84:
      wData.indexFirstPointToDraw = 0;
    break;
  }

  wData.pressHistoryMax= - 1000000;
  wData.pressHistoryMin=   1000000;
  wData.tempHistoryMax= - 1000000;
  wData.tempHistoryMin=   1000000;
  wData.humiHistoryMax=   0;      // unsigned integer
  wData.humiHistoryMin=   10000;

  // check for min / max in data window for the graph in use
  for(i=wData.indexFirstPointToDraw;i<noDataPoints;i++)
  {
    if(wData.pressHistory[i] < (nanDATA/4)){
    if(wData.pressHistory[i] > wData.pressHistoryMax)  wData.pressHistoryMax = wData.pressHistory[i];
    if(wData.pressHistory[i] < wData.pressHistoryMin)  wData.pressHistoryMin = wData.pressHistory[i];
    }
    if(wData.tempHistory[i] < (nanDATA/4)){
    if(wData.tempHistory[i] > wData.tempHistoryMax)  wData.tempHistoryMax = wData.tempHistory[i];
    if(wData.tempHistory[i] < wData.tempHistoryMin)  wData.tempHistoryMin = wData.tempHistory[i];
    }
    if(wData.humiHistory[i] < (nanDATA/4)){
      if(wData.humiHistory[i] > wData.humiHistoryMax)  wData.humiHistoryMax = wData.humiHistory[i];
      if(wData.humiHistory[i] < wData.humiHistoryMin)  wData.humiHistoryMin = wData.humiHistory[i];
    }
  }  
  #ifdef extendedDEBUG_OUTPUT
  sprintf(outstring,"Heap Size: %ld FreeHp: %ld Max Alloc: %ld",
      ESP.getHeapSize(),ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  logOut(2,outstring);    

  sprintf(outstring,"PrepGraphParam: i: %d FirstPoint: %ld", 
        i, wData.indexFirstPointToDraw);
  logOut(2,outstring);    

  sprintf(outstring,"Heap Size: %ld FreeHp: %ld Max Alloc: %ld",
      ESP.getHeapSize(),ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  logOut(2,outstring);    

  sprintf(outstring,"PrepGraphParam: Min/Max: P: %3.1f-%3.1f", 
        wData.pressHistoryMin, wData.pressHistoryMax);
  logOut(2,outstring);    

  sprintf(outstring,"PrepGraphParam: T:  %3.1f-%3.1f H: %d-%d", 
        wData.tempHistoryMin,  wData.tempHistoryMax,
        wData.humiHistoryMin, wData.humiHistoryMax);
  logOut(2,outstring);    
  #endif 
}

/**************************************************!
   @brief    draw a y coordinate bar for the graphics
   @details  starts at xpos, ypos (upper left); from there length down and width right
   @details  draws tickmarks: number is noTickmarks, distance is distanceTickmarks
   @return   void
***************************************************/
void drawYCoordinateBar(int xpos, int ypos,  
                        int noTickmarks)
{
  unsigned int i, x0, y0, x1, y1, width, height, distanceTickmarks;
  x0 = xpos; y0=ypos; width = intBW+1; height = canvasHeight+1;
  distanceTickmarks = (canvasHeight / (noTickmarks+1));
  display.drawRect(x0,y0,width,height, fgndColor);
  //sprintf(outstring,"drawYCoordinateBar x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
  //logOut(2,outstring);

  for(i=0; i<noTickmarks; i++)
  {
    x0=xpos; x1=xpos+intBW-1;
    y0=ypos + (i+1)*distanceTickmarks;
    y1=y0;
    display.drawLine(x0, y0, x1, y1, fgndColor);
  }
}

/**************************************************!
   @brief    draw a X coordinate bar for the graphics
   @details  starts at xpos, ypos (upper left); from there length down and width right
   @details  draws tickmarks: number is noTickmarks, distance is distanceTickmarks
   @return   void
***************************************************/
void drawXCoordinateBar(int xpos, int ypos, int noTickmarks, uint16_t width)
{
  unsigned int i, x0, y0, x1, y1, height, distanceTickmarks;
  x0 = xpos; y0=ypos; height = intBW+1;
  //distanceTickmarks = (canvasWidth / (noTickmarks+1));
  distanceTickmarks = (width / (noTickmarks+1));
  display.drawRect(x0,y0,width,height, fgndColor);
  //sprintf(outstring,"drawXCoordinateBar x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
  //logOut(2,outstring);    

  for(i=0; i<noTickmarks; i++)
  {
    x0=xpos + (i+1)*distanceTickmarks; 
    x1=x0;
    y0=ypos;
    y1=ypos+intBW-1;
    display.drawLine(x0, y0, x1, y1, fgndColor);
  }
}

/**************************************************!
   @brief    draw the x and y main indicator lines in the graph
   @details  
   @details  
   @return   void
***************************************************/
void drawIndicatorLines (uint16_t xpos, uint16_t ypos, uint16_t canvasW, uint16_t canvasH ,uint16_t noXLines, uint16_t noYLines)
{
  uint16_t i, x0, y0, x1, y1, width, height, distanceLines;
  x0 = xpos; y0=ypos; width = canvasW+1; height = intBW+1;

  // vertical lines along the X-Axis
  distanceLines = (canvasW / (noXLines+1));
  for(i=0; i<noXLines; i++)
  {
    x0=xpos + (i+1)*distanceLines; 
    x1=x0; 
    y0=ypos;
    y1=ypos+canvasH-1;
    display.drawLine(x0, y0, x1, y1, fgndColor);
  }

  // horizontal lines along the X-Axis
  distanceLines = (canvasH / (noYLines+1));
  for(i=0; i<noYLines; i++)
  {
    x0=xpos ; 
    x1=xpos + canvasW -1; 
    y0=ypos +(i+1)*distanceLines;
    y1=y0;
    display.drawLine(x0, y0, x1, y1, fgndColor);
  }
}

/**************************************************!
   @brief    draw the generic frame of the graphics. not the numbers.
   @details  uses the #defines for screen size 
   @param    hours: 72 or 84 are valid. Draws frame for either
   @return   void
***************************************************/
void drawGraphFrame(uint16_t hours)
{
  unsigned int x, y, x0, y0, x1, y1, width, height, tickmarks;

  if(hours != 84 && hours !=72){
    sprintf(outstring,"wrong graph time hours: %d", hours);
    logOut(2,outstring);
    return;
  }

  // frames
  //x,y, width, height, color
  x0=0; y0=0; width=SCREEN_WIDTH; height=SCREEN_HEIGHT;
  display.drawRect(x0,y0,width,height, fgndColor);
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"drawRect1 x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
    logOut(2,outstring);    
  #endif

  x0=0+extBW; y0=0+extBW; width=SCREEN_WIDTH-2*extBW; height=SCREEN_HEIGHT-2*extBW; 
  display.drawRect(x0,y0,width,height, fgndColor);
    display.drawRect(x0,y0,width,height, fgndColor);
  #ifdef extendedDEBUG_OUTPUT  
    sprintf(outstring,"drawRect2 x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
    logOut(2,outstring);    
  #endif

  //display.drawRect(0+extBW, 0+extBW, SCREEN_WIDTH-2*extBW, SCREEN_HEIGHT-2*extBW, extBW+infoHeight, fgndColor);
  
  // lines in top section
  // x0, y0, x1, y1, color
  // lower line of top box area
  display.drawLine(0+extBW, extBW+infoHeight, extBW+textLWidth+textMWidth+textRWidth, extBW+infoHeight, fgndColor);
  // the top boxes left
  //display.drawRect(extBW, extBW+prognameHeight, textLWidth, levelHeight, fgndColor);  // ALT box
  x0=extBW; y0=extBW+prognameHeight; width= textLWidth+1; height= levelHeight+1;
  display.drawRect(x0,y0,width,height, fgndColor);
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"drawRect3 x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
    logOut(2,outstring);    
  #endif  

  // pressure, temperature, humidity boxes
  //display.drawRect(extBW+textLWidth, extBW, textMWidth, pressureHeight, fgndColor); // pressure box
  x0=extBW+textLWidth; y0= extBW; width= textMWidth+1; height= pressureHeight+1;
  display.drawRect(x0,y0,width,height, fgndColor);
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"drawRect4 x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
    logOut(2,outstring);    
  #endif  

  //display.drawRect(extBW+textLWidth, extBW+pressureHeight, textMWidth/2, temperatureHeight, fgndColor); // temperature box
  x0=extBW+textLWidth; y0=extBW+pressureHeight; width= textMWidth/2+1; height= temperatureHeight+1;
  display.drawRect(x0,y0,width,height, fgndColor);
  //sprintf(outstring,"drawRect5 x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
  //logOut(2,outstring);    

  // battery box
  //display.drawRect(extBW+textLWidth+textMWidth, extBW, textR1Width, infoHeight, fgndColor); // tendency box
  x0=extBW+textLWidth+textMWidth; y0= extBW; width=textRWidth+1; height= batHeight+1;
  display.drawRect(x0,y0,width,height, fgndColor);
  //sprintf(outstring,"drawRect battery box x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
  //logOut(2,outstring);    

  // interval box
  //display.drawRect(extBW+textLWidth+textMWidth, extBW, textR1Width, infoHeight, fgndColor); // tendency box
  x0=extBW+textLWidth+textMWidth; y0= extBW+batHeight; width=textRWidth+1; height= intervalHeight+1;
  display.drawRect(x0,y0,width,height, fgndColor);
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"drawRect interval box x0: %d, y0: %d, width: %d, height: %d", x0,y0,width,height);
    logOut(2,outstring);    
  #endif  

  // coordinate bars, bordering the drawing canvas
  if(hours == 84){
    x=extBW+numWXL; y= extBW+infoHeight+upperBW+intBW;  tickmarks = 19;// left bar
    drawYCoordinateBar(x,  y, tickmarks);
    x= extBW+numWXL+canvasWidth+intBW; y= extBW+infoHeight+upperBW+intBW;  tickmarks = 19; // right bar
    drawYCoordinateBar(x, y , tickmarks);
    width = canvasWidth+1;
    x= extBW+numWXL+intBW; y= extBW+infoHeight+upperBW;  tickmarks = 27;//13;
    drawXCoordinateBar(x, y, tickmarks, width);
    x= extBW+numWXL+intBW; y= extBW+infoHeight+upperBW+canvasHeight+intBW;  tickmarks = 27;//13;
    drawXCoordinateBar(x, y, tickmarks, width);
    // draw the indicator lines parallel and vertical to the axis. number is the lines between the axis frames
    //drawIndicatorLines (int xpos, int ypos, int canvasW, int noXLines, int noYLines)
    drawIndicatorLines (canvasLeft, canvasTop, canvasWidth, canvasHeight, 6, 3);
  } 
  else if (hours ==72){
    uint16_t xoff = offsetPix72hGraph;
    width = canvasWidth - offsetPix72hGraph +1;
    x=extBW+numWXL+ xoff; y= extBW+infoHeight+upperBW+intBW;  tickmarks = 19;// left bar
    drawYCoordinateBar(x,  y, tickmarks );
    x= extBW+numWXL+canvasWidth+intBW; y= extBW+infoHeight+upperBW+intBW;  tickmarks = 19; // right bar
    drawYCoordinateBar(x, y , tickmarks );
    x= extBW+numWXL+intBW + xoff; y= extBW+infoHeight+upperBW;  tickmarks = 23;// 13-2; // upper bar
    drawXCoordinateBar(x, y, tickmarks, width);
    x= extBW+numWXL+intBW + xoff; y= extBW+infoHeight+upperBW+canvasHeight+intBW;  tickmarks = 23; //13-2; // lower bar
    drawXCoordinateBar(x, y, tickmarks, width);

    //drawIndicatorLines (int xpos, int ypos, int canvasW, int noXLines, int noYLines)
    drawIndicatorLines (canvasLeft+offsetPix72hGraph, canvasTop, canvasWidth-offsetPix72hGraph, canvasHeight, 5, 3);
  }
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"end of drawGraphFrame()");
  #endif
}

/**************************************************!
   @brief    function to draw an arrow
   @details  used to indicate pressure changes
   @param    x, y: position of center
   @param    asize: angular position offset
   @param    angle: angle of arrow. 0°: down, 180°: up, 270°: right
   @param    pwidth, plength: base width and length of arrow
   @return   void
***************************************************/

void arrow_old(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float dx = (asize + 28) * cos((aangle - 90) * PI / 180) + x; // calculate X position
  float dy = (asize + 28) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
  float x1 = 0;           float y1 = plength;
  float x2 = pwidth / 2;  float y2 = pwidth / 2;
  float x3 = -pwidth / 2; float y3 = pwidth / 2;
  float angle = aangle * PI / 180;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  display.fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, fgndColor);
}

void arrow(int x, int y, float aangle, int pwidth, int plength) {
  float dx =  x; // calculate X position
  float dy =  y; // calculate Y position
  float x1 = 0;           float y1 = -plength/2;
  float x2 = pwidth / 2;  float y2 = plength / 2;
  float x3 = -pwidth / 2; float y3 = plength / 2;
  float angle = aangle * PI / 180;
  int16_t xx1 = (int)(0.5+ x1 * cos(angle) - y1 * sin(angle) + dx);
  int16_t yy1 = (int)(0.5+ y1 * cos(angle) + x1 * sin(angle) + dy);
  int16_t xx2 = (int)(0.5+ x2 * cos(angle) - y2 * sin(angle) + dx);
  int16_t yy2 = (int)(0.5+ y2 * cos(angle) + x2 * sin(angle) + dy);
  int16_t xx3 = (int)(0.5+ x3 * cos(angle) - y3 * sin(angle) + dx);
  int16_t yy3 = (int)(0.5+ y3 * cos(angle) + x3 * sin(angle) + dy);
  display.fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, fgndColor);
}


/**************************************************!
   @brief    function to draw the tendency graphics 
   @details  using "arrow" as a basic building block
   @param    x, y: base position in pixel
   @param    al, aw: length and width of arrows
   @param    tendencyValue: value of the tendency to be drawn
   @param    limit1, limit2, limit3: limit values, from lowest to highest
   @return   void
***************************************************/
void drawTendency(int x, int y, int aw, int al, 
              float tendencyValue, float limit1, float limit2, float limit3)
{
  if(tendencyValue > limit3) // 2 arrows up
  //  arrow(x, y-al/2,  0, aw, al);
    {arrow(x, y-al/2-1,  0, aw, al);arrow(x, 1+y+al/2,  0, aw, al);}
  if((tendencyValue) > limit2 && (tendencyValue <= limit3)) // 1 arrow up
    arrow(x, y,  0, aw, al); 
  if((tendencyValue) > limit1 && (tendencyValue <= limit2)) // 1 arrow 45° up
    arrow(x, y,  45, aw, al); 
  if((tendencyValue) > -limit1 && (tendencyValue <= limit1)) // 1 arrow right
    arrow(x, y, 90, aw, al); 
  if((tendencyValue) > -limit2 && (tendencyValue <= -limit1)) // 1 arrow 45° down
    arrow(x, y, 135, aw, al); 
  if((tendencyValue) > -limit3 && (tendencyValue <= -limit2)) // 1 arrow down
    arrow(x, y, 180,  aw, al);  
  if(tendencyValue <= -limit3) // 2 arrows down
  //  arrow(x, y+3*al/2,  0, aw, al);
    {arrow(x, y-al/2-1, 180, aw, al);   arrow(x, 1+y+al/2, 180, aw, al); }  
}

/**************************************************!
   @brief    function to fill the text fields on top 
   @details  takes data from global structure wData
   @return   void
***************************************************/

void drawTextFields()
{
  int x, y, aw, al;
  float tendencyValue, limit1, limit2, limit3;

  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"Start of drawTextFields");
  #endif  

  // Pressure, pressure unit and tendency 
  u8g2Fonts.setFont(u8g2_font_helvB24_tf);
  x = extBW + textLWidth + textMWidth/20;
  y = extBW + pressureHeight-pressureHeight/5;
  u8g2Fonts.setCursor(x,y);
  if(wData.applyPressureCorrection)
    sprintf(outstring, "%3.1f", wData.actPressureCorr);
  else
    sprintf(outstring, "%3.1f", wData.actPressureRaw);
  u8g2Fonts.print(outstring);
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  x= u8g2Fonts.getCursorX();
  y= u8g2Fonts.getCursorY();
  sprintf(outstring, "  %s", wData.pressureUnit);
  u8g2Fonts.print(outstring);
  y=y - u8g2Fonts.getFontAscent() + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "  %+3.1f", wData.pressure3hChange);
  u8g2Fonts.print(outstring);
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF1 ");
  #endif  

  // pressure tendency graphics
  tendencyValue = wData.pressure3hChange;
  limit1 = pressureTendencyLimit1;
  limit2 = pressureTendencyLimit2;
  limit3 = pressureTendencyLimit3;
  //x = extBW + textLWidth + textMWidth + textR1Width/2; 
  //y = extBW + infoHeight-infoHeight/2;
  x = extBW + textLWidth + 90*textMWidth/100; 
  y = extBW + pressureHeight/2;
  aw=10; al=14;
  drawTendency(x, y, aw, al, tendencyValue, limit1, limit2, limit3);

  // crude alarm feature
  /*
  if(abs(tendencyValue) >= pressureTendencyLimit3)
    buzzer(5, 150, 75);
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF2 ");
  #endif  
  */
  // better alarm feature, with acknowledge via button
  bool alertCondition = false;
  if(abs(tendencyValue) >= pressureTendencyLimit3){
    alertCondition = true;
    logOut(2,(char*)"alertCondition true");
  }  
  if(alertCondition){         // is condition for alert met?
    if(!wData.buttonPressed){ // and button not pressed?
      buzzer(5, 150, 75);     // then audible alert
      wData.alertON = true;   // remember that alert has been given
      logOut(2,(char*)"alert beep sounded");
      delay(50);
    } else{                     // button has been pressed
                                // no buzzer
                                // do not reset the button pressed since alert condition still active
      wData.alertON = false;    // remember that alert has NOT been given
      logOut(2,(char*)"NO alert beep sounded since buttonPressed active");
      delay(50);
    }
  } else {                      // else: no alertCondition
                                // no buzzer
    wData.buttonPressed = false;// reset the alert by resetting the button pressed condition
    wData.alertON = false;      // and remember that no alert has been given
    logOut(2,(char*)"no alert. reset buttonPressed, reset alertON");
    delay(50);
  }  

  // Temperature
  //display.setFont(&FreeMonoBold12pt7b);  // Schrift definieren
  u8g2Fonts.setFont(u8g2_font_helvB18_tf);
  x = extBW + textLWidth + 3*textMWidth/100;
  y = extBW + infoHeight-infoHeight/8;
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "%3.1f", wData.actTemperature);
  u8g2Fonts.print(outstring); 
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  sprintf(outstring, "%s",wData.temperatureUnit);
  x= u8g2Fonts.getCursorX();      // get new position before font change and before print
  y= u8g2Fonts.getCursorY();
  //x-=5;
  y= y - u8g2Fonts.getFontAscent() + u8g2Fonts.getFontDescent();
  u8g2Fonts.print(outstring);
  // 3h temperature tendency value
  u8g2Fonts.setFont(u8g2_font_helvR08_tf);
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "%+3.1f", wData.temperature3hChange);
  u8g2Fonts.print(outstring);
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF3 ");
  #endif  

  // temperaure tendency graphics
  tendencyValue = wData.temperature3hChange;
  limit1 = temperatureTendencyLimit1;
  limit2 = temperatureTendencyLimit2;
  limit3 = temperatureTendencyLimit3;
  x = extBW + textLWidth + 90*(textMWidth/2)/100; 
  y = extBW + pressureHeight + temperatureHeight/2;
  aw=9; al=13;
  drawTendency(x, y, aw, al, tendencyValue, limit1, limit2, limit3);
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF4 ");
  #endif  

  // Humidity
  u8g2Fonts.setFont(u8g2_font_helvB18_tf);
  x = extBW + textLWidth + textMWidth/2+3*textMWidth/100;
  y = extBW + infoHeight-infoHeight/8;
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "%3.1f", (float)wData.actHumidity/(float)10.0);
  u8g2Fonts.print(outstring);  
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  sprintf(outstring, " %s",wData.humidityUnit);
  x= u8g2Fonts.getCursorX();      // get new position before font change and before print
  y= u8g2Fonts.getCursorY();
  //x-=5;
  y= y - u8g2Fonts.getFontAscent() + u8g2Fonts.getFontDescent();
  u8g2Fonts.print(outstring);
  // 3h humidity tendency value
  u8g2Fonts.setFont(u8g2_font_helvR08_tf);
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "%+3.1f", wData.humidity3hChange);
  u8g2Fonts.print(outstring);
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF4 ");
  #endif  
  // humidity tendency graphics
  tendencyValue = wData.humidity3hChange;
  limit1 = humidityTendencyLimit1;
  limit2 = humidityTendencyLimit2;
  limit3 = humidityTendencyLimit3;
  x = extBW + textLWidth + textMWidth/2 + 90*(textMWidth/2)/100; 
  y = extBW + pressureHeight + temperatureHeight/2;
  aw=9; al=13;
  drawTendency(x, y, aw, al, tendencyValue, limit1, limit2, limit3);
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF5 ");
  #endif  

  // Battery voltage and percent
  u8g2Fonts.setFont(u8g2_font_helvR12_tf);
  x = extBW + textLWidth + textMWidth + 5*textRWidth/100; 
  y = extBW + 1*batHeight/10 + u8g2Fonts.getFontAscent();
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring,"Batterie");
  u8g2Fonts.print(outstring); 
  u8g2Fonts.setFont(u8g2_font_helvR10_tf);
  x = extBW + textLWidth + textMWidth + 60*textRWidth/100;
  y = extBW + 1*batHeight/10 + u8g2Fonts.getFontAscent();
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "%3.0f%%", wData.batteryPercent);
  u8g2Fonts.print(outstring); 
  x = extBW + textLWidth + textMWidth + 50*textRWidth/100;
  y = extBW + 5*batHeight/10 + u8g2Fonts.getFontAscent();
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "%3.3f V", wData.batteryVoltage);
  u8g2Fonts.print(outstring);  
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF6 ");
  #endif

  // counter (for debugging)
  u8g2Fonts.setFont(u8g2_font_helvR08_tf);
  x = extBW + textLWidth + textMWidth + 5*textRWidth/100; 
  y = extBW + batHeight + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "%d", wData.startCounter);
  u8g2Fonts.print(outstring);  
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF7 ");
  #endif  
  // pressure correction mode: on: SEA LEVEL, off: STATION
  u8g2Fonts.setFont(u8g2_font_helvR10_tf);
  x = extBW + 5*textLWidth/100;
  y = extBW + prognameHeight +4*levelHeight/5;
  u8g2Fonts.setCursor(x,y);
  if(wData.applyPressureCorrection)
    sprintf(outstring,    "Meereshöhe");
  else
    sprintf(outstring,    "Unkorrigiert");
  u8g2Fonts.print(outstring);    
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF8 ");
  #endif  
  // data interval in sec
  u8g2Fonts.setFont(u8g2_font_helvR10_tf);
  x = extBW + textLWidth + textMWidth + 5 * textRWidth/100; 
  y = extBW + batHeight + 3* intervalHeight/4;
  u8g2Fonts.setCursor(x,y);
  // this is the target interval
  // sprintf(outstring, "Interval: %d s", wData.targetMeasurementIntervalSec);
  // this is the actually elapsed interval
  sprintf(outstring, "Interval: %3.1f s", wData.actSecondsSinceLastMeasurement);
  u8g2Fonts.print(outstring);  
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF9 ");
  #endif  
  // correction value
  u8g2Fonts.setFont(u8g2_font_helvR10_tf);
  x = extBW + 5*textLWidth/100;
  y = extBW + prognameHeight + levelHeight + corrHeight - corrHeight/4;
  u8g2Fonts.setCursor(x,y);
  sprintf(outstring, "Offset:%+3.1f", wData.pressureCorrValue);
  u8g2Fonts.print(outstring); 
  sprintf(outstring," %s", wData.pressureUnit);
  u8g2Fonts.setFont(u8g2_font_helvR08_tf);
  u8g2Fonts.print(outstring);  
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"dtTF10 ");
  #endif  
  // version info
  u8g2Fonts.setFont(u8g2_font_helvR10_tf);
  x= extBW + 5*textLWidth/100;
  y= extBW + 15*prognameHeight/100 + u8g2Fonts.getFontAscent() + 2;  
  u8g2Fonts.setCursor(x, y);
  sprintf(outstring, "%s", PROGNAME);
  u8g2Fonts.print(outstring); 
  u8g2Fonts.setFont(u8g2_font_helvR08_tf);
  y= extBW + 6*prognameHeight/10 + u8g2Fonts.getFontAscent() + 2;  
  u8g2Fonts.setCursor(x, y);
  sprintf(outstring, "%s %s", VERSION, BUILD_DATE);
  u8g2Fonts.print(outstring);  
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"End of drawTextFields");
  #endif  
}

/**************************************************!
   @brief    print X axis numbers
   @details  
   @param    int hours: indicator if the graph type is 72 or 84 hours
   @return   void
***************************************************/
void drawXAxisNumbers(int hours)
{
  uint32_t oldest, youngest, timerange_sec, timerange_hours;
  int i, x, y, lowestTimeHours, highestTimeHours, timeRangeValues[7];
  // determine time range. time is in seconds, age of data points, [0] guaranteed oldest
  if(hours == 84){
    oldest = wData.ageOfDatapoint[0];
  }  
  else if (hours ==72){
    oldest = wData.ageOfDatapoint[wData.indexFirstPointToDraw];  
  }  
  youngest=wData.ageOfDatapoint[noDataPoints-1];
  timerange_sec = oldest - youngest;
  // calculate y axis numbers
  timerange_hours = (int)(0.5 + (float)timerange_sec / 3600); // default 
  if(hours==84){
    if((timerange_sec < 84*3600+1000) && (timerange_sec > 84*3600-1000))
      timerange_hours = 84;
    if((timerange_sec < 42*3600+1000) && (timerange_sec > 42*3600-1000))
      timerange_hours = 42;
    if((timerange_sec < 24*3600+1000) && (timerange_sec > 24*3600-1000))
      timerange_hours = 21;
    wData.graphTimeRangeHours = timerange_hours; // store the value found  in wData
    //{-72, -60, -48, -36, -24, -12} for 84 h, factors 2 and 4 smaller for 42 and 21 h
    for(i=0;i<6;i++)
      timeRangeValues[i] = - timerange_hours +(i+1)*timerange_hours/7; 
    lowestTimeHours = timeRangeValues[0]-timerange_hours/7;
    highestTimeHours = timeRangeValues[5]+timerange_hours/7;  
  }
  else{
    if((timerange_sec < 72*3600+1000) && (timerange_sec > 72*3600-1000))
      timerange_hours = 72;
    if((timerange_sec < 36*3600+1000) && (timerange_sec > 36*3600-1000))
      timerange_hours = 36;
    if((timerange_sec < 18*3600+1000) && (timerange_sec > 18*3600-1000))
      timerange_hours = 18;
    wData.graphTimeRangeHours = timerange_hours; // store the value found  in wData
    //{-60, -48, -36, -24, -12} for 84 h, factors 2 and 4 smaller for 36 and 18 h
    for(i=0;i<5;i++)
        timeRangeValues[i] = - timerange_hours +(i+1)*timerange_hours/6; 
    lowestTimeHours = timeRangeValues[0]-timerange_hours/6;
    highestTimeHours = timeRangeValues[4]+timerange_hours/6;  
  }    

  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"oldest: %ld youngest: %ld timerange_sec: %ld timerange_hours: %ld",
        oldest, youngest, timerange_sec, timerange_hours);
    logOut(2,outstring);
    sprintf(outstring,"lowestTimeHours: %d highestTimeHours: %d",
        lowestTimeHours, highestTimeHours);
    logOut(2,outstring);
  #endif  

  // draw x-axis numbers
  display.setFont(&FreeMonoBold9pt7b);
  y = canvasTop + canvasHeight -5;
  if(hours==84){
    for(i=0;i<6;i++)
    {
      x = canvasLeft + (i+1)*canvasWidth/7 - canvasWidth/15;
      display.setCursor(x, y);
      display.print(timeRangeValues[i]);
      #ifdef extendedDEBUG_OUTPUT
        sprintf(outstring, "time[%d] %d: ", i, timeRangeValues[i]);
        logOut(2,outstring);
      #endif  
    }
  }
  else{
    for(i=0;i<5;i++)
    {
      x = canvasLeft + offsetPix72hGraph + (i+1)*(canvasWidth-offsetPix72hGraph)/6 - canvasWidth/15;
      display.setCursor(x, y);
      display.print(timeRangeValues[i]);
      #ifdef extendedDEBUG_OUTPUT
        sprintf(outstring, "time[%d] %d: ", i, timeRangeValues[i]);
        logOut(2,outstring);
      #endif  
    }    
  }
}

/**************************************************!
   @brief    draw pressure Y axis numbers
   @details  
   @param    char* unit: unit to be shown along the Y axis
   @param    char* graphName : Name of graph
   @return   void
***************************************************/
void drawPressureYAxisNumbers(char* unit, char* graphName)
{
  int i, x, y, pressureRangeValues[6];
  
  // determine y range
  float pressureRange, displayRange;
  int lowestPressureMbar, highestPressureMbar;
  pressureRange = wData.pressHistoryMax - wData.pressHistoryMin;
  if(pressureRange <= drLimitUpper20) 
    displayRange = 20;
  if(pressureRange > drLimitUpper20  && pressureRange <= drLimitUpper40) 
    displayRange = 40;
  if(pressureRange > drLimitUpper40  && pressureRange <= drLimitUpper100) 
    displayRange = 100;
  if(pressureRange > drLimitUpper100 && pressureRange <= drLimitUpper200) 
    displayRange = 200;  
  if(pressureRange > drLimitUpper200 && pressureRange <= drLimitUpper400) 
    displayRange = 400;    
  if(pressureRange > drLimitUpper400) 
    displayRange = 1000;     

  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"PressHistoryMax: %3.1f pressHistoryMin: %3.1f displayRange:%f",
      wData.pressHistoryMax, wData.pressHistoryMin, displayRange);
    logOut(2,outstring);
  #endif  

  // calculate y axis numbers
  float pressHistsMinCorr = wData.pressHistoryMin;
  if(wData.applyPressureCorrection)
    pressHistsMinCorr += wData.pressureCorrValue;

  lowestPressureMbar = (displayRange/4) * (int)(pressHistsMinCorr / (displayRange/4));
  if (pressureRange < 0.4 * displayRange) 
      lowestPressureMbar -= displayRange/4; // lower the lower tickmark by 1/4 to avoid hugging of the x axis
  //else if((wData.pressHistoryMin-lowestPressureMbar) < displayRange/4)  
  else if((wData.pressHistoryMin-lowestPressureMbar) < displayRange/8)    
      lowestPressureMbar -= displayRange/4; // lower the lower tickmark by 1/4 to avoid hugging of the x axis    
  highestPressureMbar = lowestPressureMbar + displayRange;

  wData.graphYDisplayRange = displayRange;             // store the value found  in wData  
  wData.graphHighestPressureMbarCorr = highestPressureMbar; // store the value found  in wData  
  wData.graphLowestPressureMbarCorr = lowestPressureMbar;   // store the value found  in wData  

  int strW;
  // draw y-axis numbers
  //display.setFont(&FreeMonoBold9pt7b);
  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  x = canvasLeft + canvasWidth + intBW + 4;
  for(i=0;i<5;i++)
  { 
    pressureRangeValues[i] = lowestPressureMbar + i*displayRange/4;
    sprintf(outstring, "%d",pressureRangeValues[i]);
    strW = u8g2Fonts.getUTF8Width(outstring);
    y = canvasTop + canvasHeight - i*canvasHeight/4 + 8; 
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.print(outstring);
  }
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"cvTop: %d cvHeight:%d lowestP: %d displayRange:%f",canvasTop, canvasHeight, lowestPressureMbar, displayRange);
    logOut(2,outstring);
  #endif  

  // draw unit and graph name
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  sprintf(outstring,"Druck");
  x = canvasLeft + canvasWidth + numWXR/3;
  y = canvasTop + 9*canvasHeight/10;
  u8g2Fonts.setCursor(x, y); 
  u8g2Fonts.print(unit);

  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  //x = canvasLeft + 85*canvasWidth/100 +2;
  //y = canvasTop + intBW + u8g2Fonts.getFontAscent() + 2;  
  x = canvasLeft + canvasWidth + intBW + 2;
  y= canvasTop + intBW + canvasHeight/8;  
  u8g2Fonts.setCursor(x, y); 
  //u8g2Fonts.print(graphName);
  u8g2Fonts.print(outstring);
}

/**************************************************!
   @brief    draw temperature Y axis numbers
   @details  
   @param    char* unit: unit to be shown along the Y axis
   @param    char* graphName : Name of graph   
   @param    int position: 0: right, 1: left
   @return   void
***************************************************/
void drawTemperatureYAxisNumbers(char* unit,  char* graphName, int position)
{
  int i, x, y, temperatureRangeValues[6];
  
  // determine y range
  float temperatureRange, displayRange;
  int lowestTemperatureC, highestTemperatureC;
  temperatureRange = wData.tempHistoryMax - wData.tempHistoryMin;

  if(temperatureRange <= drLimitUpper4) 
    displayRange = 4;
  if(temperatureRange > drLimitUpper4  && temperatureRange <= drLimitUpper8) 
    displayRange = 8;  
  if(temperatureRange > drLimitUpper8  && temperatureRange <= drLimitUpper12) 
    displayRange = 12;
  if(temperatureRange > drLimitUpper12  && temperatureRange <= drLimitUpper20) 
    displayRange = 20;
  if(temperatureRange > drLimitUpper20  && temperatureRange <= drLimitUpper40) 
    displayRange = 40;
  if(temperatureRange > drLimitUpper40  && temperatureRange <= drLimitUpper100) 
    displayRange = 100;
  if(temperatureRange > drLimitUpper100 && temperatureRange <= drLimitUpper200) 
    displayRange = 200;  
  if(temperatureRange > drLimitUpper200 && temperatureRange <= drLimitUpper400) 
    displayRange = 400;    
  if(temperatureRange > drLimitUpper400) 
    displayRange = 1000;    

  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"tempHistoryMax: %f tempHistoryMin:%f", wData.tempHistoryMax, wData.tempHistoryMin);
    logOut(2,outstring);
  #endif
  // calculate y axis numbers
  float tempHistMin = wData.tempHistoryMin;

  lowestTemperatureC = (displayRange/4) * (int)(tempHistMin / (displayRange/4));
  if (temperatureRange < 0.4 * displayRange) 
      lowestTemperatureC -= displayRange/4; // lower the lower tickmark by 1/4 to avoid hugging of the x axis
  //else if((wData.tempHistoryMin-lowestTemperatureC) < displayRange/4)    
  else if((wData.tempHistoryMin-lowestTemperatureC) < displayRange/8)  // /8 better than /4, otherwise data too high
      lowestTemperatureC -= displayRange/4; // lower the lower tickmark by 1/4 to avoid hugging of the x axis
  highestTemperatureC = lowestTemperatureC + displayRange;

  wData.graphYDisplayRange = displayRange;             // store the value found  in wData  
  wData.graphLowestTemperatureCelsius = lowestTemperatureC; // store the value found  in wData  
  wData.graphHighestTemperatureCelsius = highestTemperatureC;   // store the value found  in wData  

  // draw y-axis numbers
  int strW, xpos;
  // display.setFont(&FreeMonoBold9pt7b);
  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  if(position == 0)
    x = canvasLeft + canvasWidth + intBW + 4;
  else if(position == 1)
    //x = extBW + (numWXL+offsetPix72hGraph)/2 -2;  
    xpos = extBW + numWXL+offsetPix72hGraph - 8;  
  else if(position == 2) 
    xpos = extBW + numWXL+offsetPix72hGraph - 4;  
  for(i=0;i<5;i++)
  { 
    temperatureRangeValues[i] = lowestTemperatureC + i*displayRange/4;
    sprintf(outstring,"%d",temperatureRangeValues[i]);
    strW = u8g2Fonts.getUTF8Width(outstring);
    if((position == 1)||(position==2)) 
      x = xpos - strW;
    y = canvasTop + canvasHeight - i*canvasHeight/4 + 8; 
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.print(outstring);
  }
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"cvTop: %d cvHeight:%d lowestT: %d highestT: %d displayRange:%f",canvasTop, canvasHeight, 
      lowestTemperatureC, highestTemperatureC, displayRange);
    logOut(2,outstring);
  #endif

  // draw unit and graph name
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  y = canvasTop + 9*canvasHeight/10;
  if(position == 0)
    x = canvasLeft + canvasWidth + numWXR/2;
  else if (position ==1)   
    x = extBW + (numWXL+offsetPix72hGraph)/2;
  else if (position == 2) {
    x = extBW + (numWXL+offsetPix72hGraph)/2+4;
    y = canvasTop + intBW + 18*canvasHeight/100;
  }
  u8g2Fonts.setCursor(x, y); 
  u8g2Fonts.print(unit);

  //u8g2Fonts.setFont(u8g2_font_helvB_tf);
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  sprintf(outstring,"Temp");
  strW = u8g2Fonts.getUTF8Width(outstring);
  y= canvasTop + intBW + canvasHeight/8;  
  if(position == 0) // right
    x = canvasLeft + canvasWidth + intBW + 2;
  else if (position ==1)             // left
    x = extBW + numWXL + offsetPix72hGraph - 4 - strW;
  else if (position == 2){
    x = extBW + numWXL + offsetPix72hGraph - 4 - strW;
    //y= canvasTop + intBW + canvasHeight/8;  
    y = canvasTop +intBW + 10*canvasHeight/100;
  }
  u8g2Fonts.setCursor(x, y); 
  //u8g2Fonts.print(graphName);
  u8g2Fonts.print(outstring);
}

/**************************************************!
   @brief    draw humiity Y axis numbers
   @details  
   @param    char* unit: unit to be shown along the Y axis
   @param    char* graphName : Name of graph
   @param    int position: 0: right, 1: left, 2: very left
   @return   void
***************************************************/
void drawHumidityYAxisNumbers(char* unit,  char* graphName, int position)
{
  int i, x, y, humidityRangeValues[6];
  
  // determine y range
  float humidityRangePercent, displayRange;
  int lowestHumidityPM, highestHumidityPM;
  humidityRangePercent = (wData.humiHistoryMax - wData.humiHistoryMin) / 10.0; // converted promille to %
  if(humidityRangePercent <= drLimitUpper20) 
    displayRange = 20;
  if(humidityRangePercent > drLimitUpper20  && humidityRangePercent <= drLimitUpper40) 
    displayRange = 40;
  if(humidityRangePercent > drLimitUpper40  && humidityRangePercent <= drLimitUpper100) 
    displayRange = 100;
  if(humidityRangePercent > drLimitUpper100 && humidityRangePercent <= drLimitUpper200) 
    displayRange = 200;  
  if(humidityRangePercent > drLimitUpper200 && humidityRangePercent <= drLimitUpper400) 
    displayRange = 400;    
  if(humidityRangePercent > drLimitUpper400) 
    displayRange = 1000;     

  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"humiHMax: %d humiHMin:%d (‰) humidityRangePercent: %f", 
      wData.humiHistoryMax, wData.humiHistoryMin, humidityRangePercent);
    logOut(2,outstring);
  #endif  

  // calculate y axis numbers
  float humiHistMinPM = wData.humiHistoryMin; // promille

  lowestHumidityPM = 10*((displayRange/4) * (int)(humiHistMinPM/10 / (displayRange/4)));
  if (humidityRangePercent < 0.4 * displayRange) 
      lowestHumidityPM -= 10*displayRange/4; // lower the lower tickmark by 1/4 to avoid hugging of the x axis
  // else if((wData.humiHistoryMin-lowestHumidityPM) < 10*displayRange/4)    
  else if((wData.humiHistoryMin-lowestHumidityPM) < 10*displayRange/8)  
      lowestHumidityPM -= 10*displayRange/4; // lower the lower tickmark by 1/4 to avoid hugging of the x axis    
  highestHumidityPM = lowestHumidityPM + 10*displayRange;

  wData.graphYDisplayRange = displayRange;             // store the value found  in wData  
  wData.graphLowestHumidityPromille =  lowestHumidityPM; // store the value found  in wData  
  wData.graphHighestHumidityPromille = highestHumidityPM;   // store the value found  in wData  

  // draw y-axis numbers
  int strW, xpos;
  // display.setFont(&FreeMonoBold9pt7b);
  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  if(position == 0)
    x = canvasLeft + canvasWidth + intBW + 8;
  else if (position == 1)
    xpos = extBW + numWXL+offsetPix72hGraph - 8;  
  else if (position ==2)
    x = extBW + 2;    
  for(i=0;i<5;i++)
  { 
    humidityRangeValues[i] = lowestHumidityPM/10 + i*displayRange/4;
    sprintf(outstring,"%d",humidityRangeValues[i]);
    strW = u8g2Fonts.getUTF8Width(outstring);
    if(position == 1) 
      x = xpos - strW;
    y = canvasTop + canvasHeight - i*canvasHeight/4 + 8; 
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.print(outstring);
  }
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"cvTop: %d cvHeight:%d lowestH[‰]: %d highestH[‰]: %d displayRange:%f",canvasTop, canvasHeight, 
      lowestHumidityPM, highestHumidityPM, displayRange);
    logOut(2,outstring);
  #endif  

  // draw unit and graphName
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  y = canvasTop + 9*canvasHeight/10;
  if(position == 0)
    x = canvasLeft + canvasWidth + numWXR/2;
  else if(position ==1 )
    x = extBW + (numWXL+offsetPix72hGraph)/2;  
  else if(position == 2){
    x = extBW + 2;  
    y = canvasTop + intBW + 43*canvasHeight/100;
  }
  u8g2Fonts.setCursor(x, y); 
  u8g2Fonts.print(unit);

  /*
  u8g2Fonts.setFont(u8g2_font_helvB08_tf);
  if(position == 0)
    x = canvasLeft + 85*canvasWidth/100 + 2;
  else  
    x = canvasLeft + offsetPix72hGraph + 2;   
  y = canvasTop + intBW + u8g2Fonts.getFontAscent() + 2;  
  u8g2Fonts.setCursor(x, y); 
  u8g2Fonts.print(graphName);
  */

  u8g2Fonts.setFont(u8g2_font_helvB10_tf);
  sprintf(outstring,"Humi");
  strW = u8g2Fonts.getUTF8Width(outstring);
  y= canvasTop + intBW + canvasHeight/8;  
  if(position == 0) // right
    x = canvasLeft + canvasWidth + intBW + 2;
  else if(position ==1 )       // left
    x = extBW + numWXL + offsetPix72hGraph - 4 - strW;
  else if(position == 2){      // very left  
    x = extBW + 2; 
    y = canvasTop + intBW + 35*canvasHeight/100;  
  }
  u8g2Fonts.setCursor(x, y); 
  u8g2Fonts.print(outstring);
}

/**************************************************!
   @brief    paint pressure graphics within canvas
   @details  
   @param    int hours: indicator if the graph type is 72 or 84 hours
   @return   void
***************************************************/
void drawPressureGraphics(int hours)
{
  // prepare graphics parameters incl. start index, min/max
  prepareGraphicsParameters(hours);
  // draw numbers for the axis
  char unit[10], name[20];
  strcpy(unit, wData.pressureUnit); // makes the compiler happy
  strcpy(name, wData.pressureName);
  drawXAxisNumbers(hours);
  drawPressureYAxisNumbers(unit, name);

   // draw pressure graph (simplified for fixed time distances)
  int i, x0, y0, x1, y1;
  long age0, age1;
  float p0, p1;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];
    if(wData.applyPressureCorrection){
      p0 = wData.pressHistory[i] + wData.pressureCorrValue; 
      p1 = wData.pressHistory[i+1] + wData.pressureCorrValue; 
    }
    else{
      p0 = wData.pressHistory[i];
      p1 = wData.pressHistory[i+1];      
    }
    if(p0 <(nanDATA/4) && p1 <(nanDATA/4)){
      y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }
}

/**************************************************!
   @brief    paint temperature graphics within canvas
   @details  
   @param    int hours: indicator if the graph type is 72 or 84 hours   
   @return   void
***************************************************/
void drawTemperatureGraphics(int hours)
{
  // prepare graphics parameters incl. start index, min/max
  prepareGraphicsParameters(hours);
  // draw numbers for the axis
  char unit[10], name[20];
  strcpy(unit, wData.temperatureUnit); // makes the compiler happy
  strcpy(name, wData.temperatureName);
  drawXAxisNumbers(hours);
  drawTemperatureYAxisNumbers(unit, name, 0);

  // draw temperature graph (simplified for fixed time distances)
  float wDLTC = wData.graphLowestTemperatureCelsius;
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"WData.lTC: %d wData.hTC %d wData.dR: %f",
        wData.graphLowestTemperatureCelsius,wData.graphHighestTemperatureCelsius, wData.graphYDisplayRange);
    logOut(2,outstring); 
  #endif  
  int i, x0, y0, x1, y1;
  float t0, t1, d0, d1, e0, e1;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;

    t0 = wData.tempHistory[i];
    t1 = wData.tempHistory[i+1];      
    if(t0 <(nanDATA/4) && t1<(nanDATA/4)){
      //float d0 =  t0-wData.graphLowestTemperatureCelsius;
      //float d1 =  t1-wData.graphLowestTemperatureCelsius;
      d0 =  t0-wDLTC;
      d1 =  t1-wDLTC;
      e0 = d0 / wData.graphYDisplayRange;
      e1 = d1 / wData.graphYDisplayRange;
      y0=(canvasTop+canvasHeight)-canvasHeight*e0;
      y1=(canvasTop+canvasHeight)-canvasHeight*e1;
      // y0=(canvasTop+canvasHeight)-canvasHeight*((t0-(float)wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      // y1=(canvasTop+canvasHeight)-canvasHeight*((t1-(float)wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d t: %f %f d: %f %f e: %f %f wData.lTC: %d %f",
        i, x0, y0, x1, y1, t0, t1, d0, d1, e0, e1, wData.graphLowestTemperatureCelsius, wDLTC);
      logOut(3,outstring);
    }
  }
}

/**************************************************!
   @brief    paint humidity graphics within canvas
   @details  humdity is stored in promille as integer!
   @param    int hours: indicator if the graph type is 72 or 84 hours   
   @return   void
***************************************************/
void drawHumidityGraphics(int hours)
{
  // prepare graphics parameters incl. start index, min/max
  prepareGraphicsParameters(hours);
  // draw numbers for the axis
  char unit[10], name[20];
  strcpy(unit, wData.humidityUnit); // makes the compiler happy
  strcpy(name, wData.humidityName);
  drawXAxisNumbers(hours);
  drawHumidityYAxisNumbers(unit, name, 0);

  // draw humidity graph (simplified for fixed time distances)
  int wDLHP = wData.graphLowestHumidityPromille;
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"WData.lHP: %d wData.hHP %d wData.ydR: %f",
        wData.graphLowestHumidityPromille,wData.graphHighestHumidityPromille, wData.graphYDisplayRange);
    logOut(2,outstring); 
  #endif  
  int i, x0, y0, x1, y1; 
  float h0, h1, lHPercent;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    if(wData.humiHistory[i] <(nanDATA/4) && wData.humiHistory[i+1]<(nanDATA/4)){
      x0=canvasLeft+i+1;
      x1=canvasLeft+i+2;
      h0 = (float)wData.humiHistory[i]/10;                          // humidity0 in percent
      h1 = (float)wData.humiHistory[i+1]/10;                        // humidity1 in percent
      lHPercent = (float)wDLHP / 10.0;       // lowest humidity within canvas in percent
      
      y0=(canvasTop+canvasHeight)-canvasHeight*((h0-lHPercent)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((h1-lHPercent)/wData.graphYDisplayRange);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d h: %f %f wData.lH%%: %f promille: %d",
        i, x0, y0, x1, y1, h0, h1, lHPercent, wDLHP);
      logOut(3,outstring);
    }
  }
}

/**************************************************!
   @brief    paint pressure and temperature graphics within canvas
   @details  
   @param    int hours: indicator if the graph type is 72 or 84 hours   
   @return   void
***************************************************/
void drawPressTempGraphics(int hours)
{
  // prepare graphics parameters incl. start index, min/max
  prepareGraphicsParameters(hours);
  // draw numbers for the axis
  drawXAxisNumbers(hours);
  char unit[10], name[20];
  strcpy(unit, wData.pressureUnit); // makes the compiler happy
  strcpy(name, wData.pressureName);
  drawPressureYAxisNumbers(unit, name);

  // draw pressure graph (simplified for fixed time distances)
  int i, x0, y0, x1, y1;
  long age0, age1;
  float p0, p1;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];
    if(wData.applyPressureCorrection){
      p0 = wData.pressHistory[i] + wData.pressureCorrValue; 
      p1 = wData.pressHistory[i+1] + wData.pressureCorrValue; 
    }
    else{
      p0 = wData.pressHistory[i];
      p1 = wData.pressHistory[i+1];      
    }
    if((nanDATA/4) && p1<(nanDATA/4)){
      y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      display.drawLine(x0, y0-1, x1, y1-1, fgndColor);
      display.drawLine(x0, y0, x1, y1, fgndColor);
      //display.drawLine(x0, y0, x1+1, y1+1, fgndColor);
    }
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }

  //--- draw the temperature graph, first the axis then the graph
  strcpy(unit, wData.temperatureUnit); // makes the compiler happy
  strcpy(name, wData.temperatureName);
  drawTemperatureYAxisNumbers(unit, name, 1); // calculates wDatagraphYDisplayRange, therefore directly before drawing
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];

    p0 = wData.tempHistory[i];
    p1 = wData.tempHistory[i+1];      
    if((p0<nanDATA/4) && p1<(nanDATA/4)){
      y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld l%d p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }
}

/**************************************************!
   @brief    paint pressure and humidity graphics within canvas
   @details  
   @param    int hours: indicator if the graph type is 72 or 84 hours
   @return   void
***************************************************/
void drawPressHumiGraphics(int hours)
{
  // prepare graphics parameters incl. start index, min/max
  prepareGraphicsParameters(hours);
  // draw numbers for the axis
  char unit[10], name[20];
  strcpy(unit, wData.pressureUnit); // makes the compiler happy
  strcpy(name, wData.pressureName);
  drawXAxisNumbers(hours);
  drawPressureYAxisNumbers(unit, name);

   // draw pressure graph (simplified for fixed time distances)
  int i, x0, y0, x1, y1;
  long age0, age1;
  float p0, p1;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];
    if(wData.applyPressureCorrection){
      p0 = wData.pressHistory[i] + wData.pressureCorrValue; 
      p1 = wData.pressHistory[i+1] + wData.pressureCorrValue; 
    }
    else{
      p0 = wData.pressHistory[i];
      p1 = wData.pressHistory[i+1];      
    }
    if(p0<(nanDATA/4) && p1<(nanDATA/4)){
      y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      display.drawLine(x0, y0-1, x1, y1-1, fgndColor);
      display.drawLine(x0, y0, x1, y1, fgndColor);
      //display.drawLine(x0, y0, x1+1, y1+1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }

  //--- draw the humidity graph, first the axis then the graph
  strcpy(unit, wData.humidityUnit); // makes the compiler happy
  strcpy(name, wData.humidityName);
  drawHumidityYAxisNumbers(unit, name, 1); // calculates wDatagraphYDisplayRange, therefore directly before drawing
  
  int wDLHP = wData.graphLowestHumidityPromille;
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"WData.lHP: %d wData.hHP %d wData.ydR: %f",
        wData.graphLowestHumidityPromille,wData.graphHighestHumidityPromille, wData.graphYDisplayRange);
    logOut(2,outstring); 
  #endif  

  float h0, h1, lHPercent;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;

    if(wData.humiHistory[i] <(nanDATA/4) && wData.humiHistory[i+1]<(nanDATA/4)){
      h0 = (float)wData.humiHistory[i]/10;                          // humidity0 in percent
      h1 = (float)wData.humiHistory[i+1]/10;                        // humidity1 in percent
    
      lHPercent = (float)wDLHP / 10.0;       // lowest humidity within canvas in percent
    
      y0=(canvasTop+canvasHeight)-canvasHeight*((h0-lHPercent)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((h1-lHPercent)/wData.graphYDisplayRange);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d h: %f %f wData.lH%%: %f promille: %d",
        i, x0, y0, x1, y1, h0, h1, lHPercent, wDLHP);
      logOut(3,outstring);
    }
  }
  /*
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];

    p0 = (float)wData.humiHistory[i]/10;    // humidity0 in percent, stored as promille
    p1 = (float)wData.humiHistory[i+1]/10;  // humidity1 in percent
  
    y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
    y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
    display.drawLine(x0, y0, x1, y1, fgndColor);
    if((i<10+wData.indexFirstPointToDraw) || (i>325))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }
  */
}

/**************************************************!
   @brief    paint temperature and humidity graphics within canvas
   @details  
   @param    int hours: indicator if the graph type is 72 or 84 hours
   @return   void
***************************************************/
void drawTempHumiGraphics(int hours)
{
  // prepare graphics parameters incl. start index, min/max
  prepareGraphicsParameters(hours);
  // draw numbers for the axis
  drawXAxisNumbers(hours);
  char unit[10], name[20];
  strcpy(unit, wData.temperatureUnit); // makes the compiler happy
  strcpy(name, wData.temperatureName);
  drawTemperatureYAxisNumbers(unit, name, 0); // calculates wData.graphYDisplayRange, therefore directly before drawing

   // draw temperature graph (simplified for fixed time distances)
  int i, x0, y0, x1, y1;
  long age0, age1;
  float p0, p1;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];
    p0 = wData.tempHistory[i];
    p1 = wData.tempHistory[i+1];      
    if(p0 <(nanDATA/4) && p1<(nanDATA/4)){
      y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      //display.drawLine(x0, y0-1, x1, y1-1, fgndColor);
      display.drawLine(x0, y0, x1, y1, fgndColor);
      display.drawLine(x0, y0, x1+1, y1+1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }


  //--- draw the humidity graph, first the axis then the graph
  strcpy(unit, wData.humidityUnit); // makes the compiler happy
  strcpy(name, wData.humidityName);
  drawHumidityYAxisNumbers(unit, name, 1); // calculates wData.graphYDisplayRange, therefore directly before drawing
  
  int wDLHP = wData.graphLowestHumidityPromille;
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"WData.lHP: %d wData.hHP %d wData.ydR: %f",
        wData.graphLowestHumidityPromille,wData.graphHighestHumidityPromille, wData.graphYDisplayRange);
    logOut(2,outstring); 
  #endif  

  float h0, h1, lHPercent;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    if(wData.humiHistory[i] <(nanDATA/4) && wData.humiHistory[i+1]<(nanDATA/4)){
      h0 = (float)wData.humiHistory[i]/10;                          // humidity0 in percent
      h1 = (float)wData.humiHistory[i+1]/10;                        // humidity1 in percent
      lHPercent = (float)wDLHP / 10.0;       // lowest humidity within canvas in percent
      
      y0=(canvasTop+canvasHeight)-canvasHeight*((h0-lHPercent)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((h1-lHPercent)/wData.graphYDisplayRange);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d h: %f %f wData.lH%%: %f promille: %d",
        i, x0, y0, x1, y1, h0, h1, lHPercent, wDLHP);
      logOut(3,outstring);
    }
  }
  /*
  //--- draw the humidity graph, first the axis then the graph
  strcpy(unit, wData.humidityUnit); // makes the compiler happy
  strcpy(name, wData.humidityName);
  drawHumidityYAxisNumbers(unit, name, 1); // calculates wData.graphYDisplayRange, therefore directly before drawing
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];

    p0 = (float)wData.humiHistory[i]/10;
    p1 = (float)wData.humiHistory[i+1]/10;      
  
    y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
    y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
    display.drawLine(x0, y0, x1, y1, fgndColor);
    if((i<10+wData.indexFirstPointToDraw) || (i>325))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }
  */
}

/**************************************************!
   @brief    paint pressure and temperature graphics within canvas
   @details  
   @param    int hours: indicator if the graph type is 72 or 84 hours   
   @return   void
***************************************************/
void drawPressTempHumiGraphics(int hours)
{
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"Start of drawPressTempHumiGraphics()");
  #endif  
  // prepare graphics parameters incl. start index, min/max
  prepareGraphicsParameters(hours);
  // draw numbers for the axis
  drawXAxisNumbers(hours);
  char unit[10], name[20];
  strcpy(unit, wData.pressureUnit); // makes the compiler happy
  strcpy(name, wData.pressureName);
  drawPressureYAxisNumbers(unit, name);

  // draw pressure graph (simplified for fixed time distances)
  int i, x0, y0, x1, y1;
  long age0, age1;
  float p0, p1;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];
    if(wData.applyPressureCorrection){
      p0 = wData.pressHistory[i] + wData.pressureCorrValue; 
      p1 = wData.pressHistory[i+1] + wData.pressureCorrValue; 
    }
    else{
      p0 = wData.pressHistory[i];
      p1 = wData.pressHistory[i+1];      
    }
    if(p0 <(nanDATA/4) && p1<(nanDATA/4)){
      y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestPressureMbarCorr)/wData.graphYDisplayRange);
      display.drawLine(x0, y0-1, x1, y1-1, fgndColor);
      display.drawLine(x0, y0, x1, y1, fgndColor);
      display.drawLine(x0, y0, x1+1, y1+1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      //inspectCString(outstring);  
      logOut(3,outstring);
    }
  }

  //--- draw the temperature graph, first the axis then the graph
  strcpy(unit, wData.temperatureUnit); // makes the compiler happy
  strcpy(name, wData.temperatureName);
  drawTemperatureYAxisNumbers(unit, name, 2); // calculates wDatagraphYDisplayRange, therefore directly before drawing
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    age0 = wData.ageOfDatapoint[i];
    age1 = wData.ageOfDatapoint[i+1];

    p0 = wData.tempHistory[i];
    p1 = wData.tempHistory[i+1];      
    if(p0 <(nanDATA/4) && p1<(nanDATA/4)){
      y0=(canvasTop+canvasHeight)-canvasHeight*((p0-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((p1-wData.graphLowestTemperatureCelsius)/wData.graphYDisplayRange);
      display.drawLine(x0, y0-1, x1, y1-1, fgndColor);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d age: %ld %ld p: %f %f",
        i, x0, y0, x1, y1, age0, age1, p0, p1);
      logOut(3,outstring);
    }
  }

  //--- draw the humidity graph, first the axis then the graph
  strcpy(unit, wData.humidityUnit); // makes the compiler happy
  strcpy(name, wData.humidityName);
  drawHumidityYAxisNumbers(unit, name, 2); // calculates wDatagraphYDisplayRange, therefore directly before drawing
  
  int wDLHP = wData.graphLowestHumidityPromille;
  #ifdef extendedDEBUG_OUTPUT
    sprintf(outstring,"WData.lHP: %d wData.hHP %d wData.ydR: %f",
        wData.graphLowestHumidityPromille,wData.graphHighestHumidityPromille, wData.graphYDisplayRange);
    logOut(2,outstring); 
  #endif  

  float h0, h1, lHPercent;
  for(i=wData.indexFirstPointToDraw;i<noDataPoints-1; i++)
  {
    x0=canvasLeft+i+1;
    x1=canvasLeft+i+2;
    if(wData.humiHistory[i] <(nanDATA/4) && wData.humiHistory[i+1]<(nanDATA/4)){
      h0 = (float)wData.humiHistory[i]/10;                          // humidity0 in percent
      h1 = (float)wData.humiHistory[i+1]/10;                        // humidity1 in percent
      lHPercent = (float)wDLHP / 10.0;       // lowest humidity within canvas in percent
      
      y0=(canvasTop+canvasHeight)-canvasHeight*((h0-lHPercent)/wData.graphYDisplayRange);
      y1=(canvasTop+canvasHeight)-canvasHeight*((h1-lHPercent)/wData.graphYDisplayRange);
      display.drawLine(x0, y0, x1, y1, fgndColor);
    }  
    if((i<noPRINTLINESLOW+wData.indexFirstPointToDraw) || (i>=noDataPoints-noPRINTLINESHIGH-1))
    {
      sprintf(outstring,"i: %d x0: %d y0: %d x1: %d y1: %d h: %f %f wData.lH%%: %f promille: %d",
        i, x0, y0, x1, y1, h0, h1, lHPercent, wDLHP);
      logOut(3,outstring);
    }
  }
  #ifdef extendedDEBUG_OUTPUT
    logOut(2,(char*)"End of drawPressTempHumiGraphics()");
  #endif  
}


/**************************************************!
   @brief    main Function to draw the graphics 
   @details  
   @return   void
***************************************************/
void drawMainGraphics(uint32_t graphicsType)
{
  if(wData.applyInversion){
    fgndColor = GxEPD_WHITE;
    bgndColor = GxEPD_BLACK;
  }
  else{
    fgndColor = GxEPD_BLACK;
    bgndColor = GxEPD_WHITE;
  }

  u8g2Fonts.setForegroundColor(fgndColor); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(bgndColor); // apply Adafruit GFX color

  display.setTextColor(fgndColor,bgndColor);     // Schriftfarbe Schwarz
  display.setFont(&FreeMonoBold12pt7b);  // Schrift definieren

  display.firstPage();
  do{
    display.fillScreen(bgndColor);// clear screen
    switch (graphicsType)
    {
      case 0:
      default:
        sprintf(outstring,"Pressure graphics. graphicsType: %ld",graphicsType);
        logOut(2,outstring);
        drawGraphFrame(84);           // main line frame for 84 h graphics
        drawTextFields();             // text section
        drawPressureGraphics(84);  
        break;
      case 1:
        sprintf(outstring,"Temperature graphics. graphicsType: %ld",graphicsType);
        logOut(2,outstring);
        drawGraphFrame(84);           // main line frame
        drawTextFields();             // text section
        drawTemperatureGraphics(84); 
        break;
      case 2:
        sprintf(outstring,"Humidity graphics. graphicsType: %ld",graphicsType);
        logOut(2,outstring);
        drawGraphFrame(84);           // main line frame
        drawTextFields();             // text section
        drawHumidityGraphics(84); 
        break;
      case 4:
        sprintf(outstring,"Pressure and temperature graphics. graphicsType: %ld",graphicsType);
        logOut(2,outstring);
        drawGraphFrame(72);           // main line frame for 72 h graphics
        drawTextFields();             // text section
        drawPressTempGraphics(72); 
        break; 
      case 5:
        sprintf(outstring,"Pressure and humidity graphics. graphicsType: %ld",graphicsType);
        logOut(2,outstring);
        drawGraphFrame(72);           // main line frame for 72 h graphics
        drawTextFields();             // text section
        drawPressHumiGraphics(72); 
        break;    
      case 6:
        sprintf(outstring,"Temperature and humidity graphics. graphicsType: %ld",graphicsType);
        logOut(2,outstring);
        drawGraphFrame(72);           // main line frame for 72 h graphics
        drawTextFields();             // text section
        drawTempHumiGraphics(72); 
        break;      
      case 7:
        sprintf(outstring,"Pressure, Temperature and Humidity graphics. graphicsType: %ld    ",graphicsType);
        //inspectCString(outstring);
        logOut(2,outstring);
        drawGraphFrame(72);           // main line frame for 72 h graphics
        drawTextFields();             // text section
        drawPressTempHumiGraphics(72); 
        break;     
    }
    
  }while (display.nextPage());
  //display.display(false); // Full screen update mode from weather display
}

/**************************************************!
   @brief    main Function to draw the graphics 
   @details  
   @return   void
***************************************************/
void drawBluetoothInfo(char* text, int mode)
{
  static int x, y=20;
  static bool lastInversion = false;

  if(wData.applyInversion){
    fgndColor = GxEPD_WHITE;
    bgndColor = GxEPD_BLACK;
  }
  else{
    fgndColor = GxEPD_BLACK;
    bgndColor = GxEPD_WHITE;
  }
  
  /*
  if(mode == 0){
    display.init(115200, true, 2, false); // initial = true  for first start
    display.setFullWindow();
  }  
  else
  {
    display.init(115200, false, 2, false); // initial = false for subsequent starts
    display.setPartialWindow(0, 0, display.width(), display.height());
  }
  */

  u8g2Fonts.setForegroundColor(fgndColor); // u8g2 apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(bgndColor); // u8g2 apply Adafruit GFX color
  display.setTextColor(fgndColor,bgndColor); // test color for u8gw functions 
  display.setFont(&FreeMonoBold12pt7b);      // set u8g2 font 

  display.firstPage();
  do{
    switch(mode){
      case 0: 
        //display.fillRect(canvasTop, canvasLeft, canvasWidth, canvasHeight, bgndColor);// clear canvas
        display.fillScreen(bgndColor);// clear screen
        x= 10;
        y= 20;
        u8g2Fonts.setCursor(x, y);
        u8g2Fonts.print(text);        
        break;
      case 1:
        //if (lastInversion != wData.applyInversion)
        //display.fillScreen(bgndColor);// clear screen
        //uint16_t charheight = u8g2Fonts.getFontAscent()-u8g2Fonts.getFontDescent();
        display.fillRect(0,y,SCREEN_WIDTH, u8g2Fonts.getFontAscent()-u8g2Fonts.getFontDescent()+2, bgndColor); // clear next writing rectangle
        y+=14;
        u8g2Fonts.setCursor(x, y);
        u8g2Fonts.print(text); 
        break;
      case 2:
        display.fillScreen(bgndColor);// clear screen
        break;  
      default:
        break;  
    }
  } while (display.nextPage());
  
  if(mode==2) display.display(false); // Full screen update mode from weather display

  lastInversion = wData.applyInversion;
}


