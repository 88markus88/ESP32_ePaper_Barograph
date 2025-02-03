//---------------------------- Screen parameters -----------------------------
#define SCREEN_WIDTH  400.0    // Set for landscape mode, don't remove the decimal place!
#define SCREEN_HEIGHT 300.0

//--------------------------- graphics parameters ----------------------------
// margin
#define extBW             2       // margin between screen border and outer box
#define numWXL            3       // left margin between graph and outer box
#define numWXR            44      // right marging between graph and o.box. Right number area
#define intBW             4       // width of border of the graph itself, with ticks
#define lowerBW           5       // margin between outer box and lower graph border
#define upperBW           6       // margin between upper text box and upper graph border
#define offsetPix72hGraph    48      // offset from left for 72 instead of 84 hour graph

// width of text on top
#define textLWidth        110     // left area: sea level, alt, cal. etc.
#define textMWidth        176     // middle area: pressure, temp, humidity
//#define textR1Width       50      // right area 1: pressure tendency graph and text
//#define textR2Width       60      // right area 2: battery symbol and %
#define textRWidth        109

// height  of text on top
#define infoHeight        75
#define pressureHeight    38
#define temperatureHeight 37
#define prognameHeight    35
#define levelHeight       20
#define corrHeight        20
#define batHeight         45
#define intervalHeight    30

// drawing canvas size, not including the borders. Actually available for drawing
#define canvasHeight      200
#define canvasWidth       337

#define canvasLeft      extBW + numWXL + intBW
#define canvasTop       extBW + infoHeight + upperBW + intBW