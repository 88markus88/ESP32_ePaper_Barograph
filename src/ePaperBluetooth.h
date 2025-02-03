#ifndef _ePaperBluetooth_H
#define _ePaperBluetooth_H

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//************** module defines *************************/
#define MAX_WAIT_FOR_BLUETOOTH  30000   // wait for bluetooth commands for max 30 sec after last entry

//************** module global variables *************************/

BluetoothSerial SerialBT;  // Bluetooth Serial object
// global variable to store the string sent by bluetooth
String btReadStr; 

// Handle received and sent messages
String message = "";
char incomingChar;
String temperatureString = "";

// Timer: auxiliar variables
unsigned long previousMillis = 0;    // Stores last time temperature was published
const long interval = 30000;         // interval at which to publish sensor readings

//************** function prototypes *************************/
int findIntInString(String inputString);
float findFloatInString(String inputString);
bool bluetoothInputHandler(String btReadStr);
void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
void initMessagetoBTClient();

//*************** function prototypes ******************/


#endif // _ePaperBluetooth_H