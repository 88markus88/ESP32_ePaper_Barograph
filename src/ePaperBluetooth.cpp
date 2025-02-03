//************** Bluetooth configuration functions *************************/

// only for Lolin
#ifdef LOLIN32_LITE

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#include "ePaperBluetooth.h"
#include "global.h"

//---- find first integer afer a ',' in String
int findIntInString(String inputString)
{
  int pos, number;
  String subS;

  pos = inputString.indexOf(',');
  subS =inputString.substring(pos+1);
  number = subS.toInt();
  sprintf(outstring,"findIntInString pos: %d subS: %s number: %d", pos, subS, number);
  Serial.println(outstring);
  return(number);
}

//---- find first float afer a ',' in String
float findFloatInString(String inputString)
{
  int pos;
  float number;
  String subS;

  pos = inputString.indexOf(',');
  subS =inputString.substring(pos+1);
  number = subS.toFloat();

  sprintf(outstring,"findFloatInString pos: %d subS: %s number: %f", pos, subS, number);
  Serial.println(outstring);
  return(number);
}

// send start message to bluetooth
void initMessagetoBTClient()
{
  SerialBT.println("-------------------------------------"); delay(10);
  Serial.println("Sending init message via bluetooth");   delay(10);
  SerialBT.println("**** ePaper Barograph connected *****");   delay(10);
  SerialBT.println("-------------------------------------");   delay(10);
  SerialBT.println("Commands:");   delay(10);
  SerialBT.println("ATI     : Invert screen");  delay(10);
  SerialBT.println("ATC,0   : Pressure correction (0|1)");  delay(10);
  SerialBT.println("ATD,15.2: Set pressure corr.val (hPa)");  delay(10);
  SerialBT.println("ATS,84  : Set timescale (21|42|84)");  delay(10);
  SerialBT.println("ATP     : Pressure graphics");  delay(10);
  SerialBT.println("ATT     : Temperature graphics"); delay(10);
  SerialBT.println("ATH     : Humidity graphics");  delay(10);
  SerialBT.println("ATL     : Press/Temp graphics");  delay(10);
  SerialBT.println("ATM     : Press/Humi graphics"); delay(10);
  SerialBT.println("ATN     : Temp/Humi graphics");  delay(10);
  SerialBT.println("ATO     : P/T/H graphics");  delay(10);
  SerialBT.println("ATQ     : Meas Scale 84->21h");  delay(10);
  SerialBT.println("ATR     : Meas Scale 84/42->42/21h");  delay(10);
  SerialBT.println("ATU     : Meas Scale 21/42->42/84");  delay(10);
  SerialBT.println("ATV     : Meas Scale 21->84h");  delay(10);
  SerialBT.println("ATX     : Exit Bluetooth Setup");  delay(10);
  SerialBT.println("AT?     : Help");  delay(10);
  SerialBT.println("-------------------------------------");  delay(10);
}

//----- handle the string received from bluetooth
// return value: false if command received, true if Command "X" has been sent
// format is: AT<x>,<parameter>
bool bluetoothInputHandler(String btReadStr)
{
  int i, paramInt;
  float paramFloat;
  char c;
  bool ret=false; 
  //-------------- parse string
  if(btReadStr.startsWith("AT")){ // string starts with "AT"
    c = btReadStr[2];             // get third character, this is the command
    switch(c){
      case 'I': // Invert screen, no parameter
        sprintf(outstring,"Command: %c - Changing screen inverted display",c);
        wData.applyInversion = !wData.applyInversion;
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);
        break;
      case 'C': // Pressure correction. parameter 0=OFF|1=ON. 1 = corection enabled
        paramInt = findIntInString(btReadStr);
        if(paramInt ==0 || paramInt ==1){
          sprintf(outstring,"Command: %c Param: %d - changing pressure correction",c,paramInt);
          wData.applyPressureCorrection = !wData.applyPressureCorrection;
          wData.preferencesChanged = true;
        }  
        else  
          sprintf(outstring,"INVALID command: %c Param: %d",c,paramInt);  
        Serial.println(outstring);      
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);
        break;
      case 'D': // Pressure correction.Parameter is value to be added to pressure if correction is on
        paramFloat = findFloatInString(btReadStr); 
        if(paramFloat >=-300 || paramFloat <=300) {
          sprintf(outstring,"Command: %c Param: %f - changing pressure correction",c,paramFloat);
          wData.pressureCorrValue = paramFloat;
          wData.preferencesChanged = true;
        }  
        else  
          sprintf(outstring,"INVALID command: %c Param: %f",c,paramFloat);
        Serial.println(outstring);
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);
        break;
      case 'S': // Timescale in hours. valid are 84 (default), 42, 21 hours (not yet implemented)
        paramInt = findIntInString(btReadStr);
        if(paramInt ==21 || paramInt ==42 || paramInt == 84)
          sprintf(outstring,"Command: %c Param: %d",c,paramInt);
        else  
          sprintf(outstring,"INVALID command: %c Param: %d",c,paramInt);   
        Serial.println(outstring);  
        SerialBT.println(outstring);    
        drawBluetoothInfo(outstring, 1);
        break;  
      case 'P': // use pressure graphics (default)
        sprintf(outstring,"Command: %c - use pressure graphics",c);
        wData.graphicsType = 0;
        wData.preferencesChanged = true;
        Serial.println(outstring);      
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);
        break;
      case 'T': // use temperature graphics 
        sprintf(outstring,"Command: %c - temperature graphics",c);
        wData.graphicsType = 1;
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);      
        break;
      case 'H': // use humidity graphics 
        sprintf(outstring,"Command: %c - humidity graphics",c);
        wData.graphicsType = 2;
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);  
        break;  
      case 'L': // use Pressure and temperature graphics.
        sprintf(outstring,"Command: %c - Press/Temp graphics",c);
        wData.graphicsType = 4;
        wData.preferencesChanged = true;
        Serial.println(outstring);      
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);
        break;
      case 'M': // use Pressure and Humidity graphics.
        sprintf(outstring,"Command: %c - Press/Humidity graphics",c);
        wData.graphicsType = 5;
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);      
        break;
      case 'N': // use Temperature and Humidity graphics
        sprintf(outstring,"Command: %c - Temp/Humidity graphics",c);
        wData.graphicsType = 6;
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);  
        break;   
      case 'O': // use Pressure, Temperature and Humidity graphics
        sprintf(outstring,"Command: %c - Press/Temp/Humi graphics",c);
        wData.graphicsType = 7;
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);  
        break;     
      case 'Q': // quarter measurement scale
        sprintf(outstring,"Command: %c - quarter meas scale",c);
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);  
        if(wData.targetMeasurementIntervalSec == 900){
          quarterMeasurementScale();
          sprintf(outstring,"quartering of scale done");
        }  
        else  
          sprintf(outstring,"not possible, Interval: %ld", wData.targetMeasurementIntervalSec);  
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1); 
        break;  
      case 'R': // half measurement scale: 84->42 or 42->21 h
        sprintf(outstring,"Command: %c - half meas scale",c);
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);  
        if(wData.targetMeasurementIntervalSec == 450 || wData.targetMeasurementIntervalSec == 900){
          halfMeasurementScale();
          sprintf(outstring,"halfing of scale done");
        }  
        else  
          sprintf(outstring,"not possible, Interval: %ld", wData.targetMeasurementIntervalSec);  
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1); 
        break;        
      case 'U': // double measurement scale 21/42->42/84h
        sprintf(outstring,"Command: %c - double meas scale",c);
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);  
        if(wData.targetMeasurementIntervalSec == 225 || wData.targetMeasurementIntervalSec == 450){
          doubleMeasurementScale();
          sprintf(outstring,"doubling of scale done");
        }  
        else  
          sprintf(outstring,"not possible, Interval: %ld", wData.targetMeasurementIntervalSec);  
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1); 
        break;  
      case 'V': // quadruple measurement scale: 21->84h
        sprintf(outstring,"Command: %c - quadruple meas scale",c);
        wData.preferencesChanged = true;
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);  
        if(wData.targetMeasurementIntervalSec == 225){
          quadrupleMeasurementScale();
          sprintf(outstring,"quadrupling of scale done");
        }  
        else  
          sprintf(outstring,"not possible, Interval: %ld", wData.targetMeasurementIntervalSec);  
        Serial.println(outstring);
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1); 
        break;              
      case 'X': // exit bluetooth setup
        sprintf(outstring,"Command: %c - Exit",c);
        ret = true;
        Serial.println(outstring);  
        SerialBT.println(outstring);  
        drawBluetoothInfo(outstring, 1);
        break;
      case '?': // provide help
        sprintf(outstring,"Command: %c - Sending help message",c);
        Serial.println(outstring);  
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);
        initMessagetoBTClient();
        break;    
      default: // invalid command, provide help via bluetooth anyway
        sprintf(outstring,"Not a valid command: _%c_",c);
        Serial.println(outstring);
        SerialBT.println(outstring);
        drawBluetoothInfo(outstring, 1);
        initMessagetoBTClient();
        break;
    }

  }
  else{
    sprintf(outstring,"Not a valid command: _%s_", btReadStr);
    Serial.println(outstring);
  }  

  return(ret);
}

/*
typedef enum {
    ESP_SPP_INIT_EVT                    = 0,                //!< When SPP is inited, the event comes  
    ESP_SPP_UNINIT_EVT                  = 1,                //!< When SPP is uninited, the event comes  
    ESP_SPP_DISCOVERY_COMP_EVT          = 8,                //!< When SDP discovery complete, the event comes  
    ESP_SPP_OPEN_EVT                    = 26,               //!< When SPP Client connection open, the event comes  
    ESP_SPP_CLOSE_EVT                   = 27,               //!< When SPP connection closed, the event comes  
    ESP_SPP_START_EVT                   = 28,               //!< When SPP server started, the event comes  
    ESP_SPP_CL_INIT_EVT                 = 29,               //!< When SPP client initiated a connection, the event comes  
    ESP_SPP_DATA_IND_EVT                = 30,               //!< When SPP connection received data, the event comes, only for ESP_SPP_MODE_CB  
    ESP_SPP_CONG_EVT                    = 31,               //!< When SPP connection congestion status changed, the event comes, only for ESP_SPP_MODE_CB  
    ESP_SPP_WRITE_EVT                   = 33,               //!< When SPP write operation completes, the event comes, only for ESP_SPP_MODE_CB  
    ESP_SPP_SRV_OPEN_EVT                = 34,               //!< When SPP Server connection open, the event comes  
    ESP_SPP_SRV_STOP_EVT                = 35,               //!< When SPP server stopped, the event comes  
    ESP_SPP_VFS_REGISTER_EVT            = 36,               //!< When SPP VFS register, the event comes  
    ESP_SPP_VFS_UNREGISTER_EVT          = 37,               //!< When SPP VFS unregister, the event comes  
} esp_spp_cb_event_t;
*/

void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  int i;
  String readStr;

  switch(event)
  {
    case ESP_SPP_SRV_OPEN_EVT: // server opened, connection established
      Serial.println("BT Event: ESP_SPP_SRV_OPEN_EVT"); 
      initMessagetoBTClient(); // send init message via bluetooth
      break;  
    case ESP_SPP_DATA_IND_EVT: // individual data received
      Serial.println("BT Event: ESP_SPP_DATA_IND_EVT");  // individual data received
      readStr = SerialBT.readString();
      for(i=0;i<readStr.length();i++)
        readStr[i]=toupper(readStr[i]);
      btReadStr = readStr;  
      Serial.println("Raw command: " + btReadStr);
      break;
    case ESP_SPP_INIT_EVT: Serial.println("BT Event: ESP_SPP_INIT_EVT"); break;          // Serial bluetooth parallel initiated
    case ESP_SPP_START_EVT: Serial.println("BT Event: ESP_SPP_START_EVT"); break;        // server started
    case ESP_SPP_WRITE_EVT: Serial.println("BT Event: ESP_SPP_WRITE_EVT"); break;        // write operation completed
    case ESP_SPP_UNINIT_EVT: Serial.println("BT Event: ESP_SPP_UNINIT_EVT"); break;      // un-initiation of SPP
    default:
      sprintf(outstring," BT Event: %d", event);
      Serial.println(outstring);
    break;
  }
}


// disconnect from bluetooth and end server
void disconnectBluetooth()
{
  delay(100);
  Serial.println("BT stopping");
  SerialBT.println("Bluetooth disconnecting...");
  delay(100);
  SerialBT.flush();
  SerialBT.disconnect();
  SerialBT.end();
  Serial.println("BT stopped");
  delay(1000);
}

// setup for bluetooth
void bluetoothSetup() 
{
  Serial.begin(115200);
  SerialBT.register_callback(bluetoothCallback); // register callback function
  SerialBT.begin("ESP32Barograph"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

// main function to configure the device via bluetooth
void bluetoothConfigMain() 
{
  unsigned long startMillis = millis();
  unsigned long actMillis;
  bool exitSelected;

  Serial.println("Entering bluetoothConfigMain()");
  sprintf(outstring,"Ready for Bluetooth Configuration");

  bluetoothSetup();       // start BT, register callback

  delay(20);
  drawBluetoothInfo(outstring, 0);
  delay(100);

  do
  {
    // string is filled by bluetoothCallback function
    if(btReadStr.length() > 0){
        exitSelected = bluetoothInputHandler(btReadStr);
        startMillis = millis(); // restart counter if something received
        btReadStr = "";
    }
    delay(50);
    actMillis = millis();
  } while ((actMillis - startMillis < MAX_WAIT_FOR_BLUETOOTH) && !exitSelected);

  if(wData.preferencesChanged){
    writePreferences();
    wData.preferencesChanged = false;
  }  
  
  sprintf(outstring,"Bluetooth Configuration ended.");  // \n exitSelected: %d (Millis: start: %d end: %d)",
                                                        //exitSelected, startMillis, actMillis);
  Serial.println(outstring);
  drawBluetoothInfo(outstring, 1);
  delay(1000);
  //unnecessary
  //drawBluetoothInfo(outstring, 2); // mode2: clear display
}

#endif //LOLIN32_LITE