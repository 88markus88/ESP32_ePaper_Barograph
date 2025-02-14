//************** Bluetooth Low Energy (BLE) configuration functions ******/

// only for CrowPanel
#ifdef CROW_PANEL


// ESP32 Web Bluetooth (BLE)
// https://randomnerdtutorials.com/esp32-web-bluetooth/


#include <Arduino.h>
/*
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
*/
// https://github.com/h2zero/NimBLE-Arduino/blob/master/docs/Migration_guide.md
#include "NimBLEDevice.h"

#include "global.h"
#include "ePaperBLE.h"

#define bleTEST
#ifdef bleTEST

NimBLEServer* pServer = NULL;
NimBLECharacteristic* pSensorCharacteristic = NULL;
NimBLECharacteristic* pLedCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

NimBLEService *pService;

uint32_t actTime, startTime;  

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define SENSOR_CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"
#define LED_CHARACTERISTIC_UUID "19b10002-e8f2-537e-4f6c-d104768a1214"

class MyServerCallbacks: public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pLedCharacteristic) {
    //String value = pLedCharacteristic->getValue();
    std::string value = pLedCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Characteristic event, written: ");
      Serial.println(static_cast<int>(value[0])); // Print the integer value

      int receivedValue = static_cast<int>(value[0]);
      if (receivedValue == 1) {
        //digitalWrite(ledPin, HIGH);
        Serial.println("LED on, timer reset");
        startTime = millis();
      } else {
        //digitalWrite(ledPin, LOW);
        Serial.println("LED off, timer reset");
        startTime = millis();
      }
    }
  }
};

void bleSetup() {
  //Serial.begin(115200);
  // pinMode(ledPin, OUTPUT);

  Serial.printf("bleSetup started\n");
  Serial.printf("Free Heap: %ld Max Alloc: %ld\n",ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  // Create the NimBLE Device
  NimBLEDevice::init("ESP32");

  // Create the NimBLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the NimBLE Service
  //NimBLEService *pService = pServer->createService(SERVICE_UUID);
  pService = pServer->createService(SERVICE_UUID);

  // Create a NimBLE Characteristic
  pSensorCharacteristic = pService->createCharacteristic(
          SENSOR_CHARACTERISTIC_UUID,
          NIMBLE_PROPERTY::READ   |
          NIMBLE_PROPERTY::WRITE  |
          NIMBLE_PROPERTY::NOTIFY |
          NIMBLE_PROPERTY::INDICATE
  );
  /*
  pSensorCharacteristic = pService->createCharacteristic(
                      SENSOR_CHARACTERISTIC_UUID,
                      NimBLECharacteristic::PROPERTY_READ   |
                      NimBLECharacteristic::PROPERTY_WRITE  |
                      NimBLECharacteristic::PROPERTY_NOTIFY |
                      NimBLECharacteristic::PROPERTY_INDICATE
                    );
*/
  // Create the ON button Characteristic
  /*
  pLedCharacteristic = pService->createCharacteristic(
                      LED_CHARACTERISTIC_UUID,
                      NimBLECharacteristic::PROPERTY_WRITE
                    );
  */
  pLedCharacteristic = pService->createCharacteristic(
                      LED_CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::WRITE
);
  // Register the callback for the ON button characteristic
  pLedCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a NimBLE Descriptor
  // left out for nimBLE
  //pSensorCharacteristic->addDescriptor(new BLE2902());
  //pLedCharacteristic->addDescriptor(new BLE2902());
  NimBLE2904* pSensorCharacteristic2904 = pSensorCharacteristic->create2904();
  pSensorCharacteristic2904->setFormat(NimBLE2904::FORMAT_UTF8);
  //pSensorCharacteristic2904->setCallbacks(&dscCallbacks);
  NimBLE2904* pLedCharacteristic2904 = pLedCharacteristic->create2904();
  pLedCharacteristic2904->setFormat(NimBLE2904::FORMAT_UTF8);
  //pSensorCharacteristic2904->setCallbacks(&dscCallbacks);

  // Start the service
  pService->start();

  // Start advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("ESP32");
  pAdvertising->addServiceUUID(SERVICE_UUID);

  // Changed for NimBLE
  pAdvertising->enableScanResponse(true);
  //pAdvertising->setScanResponse(false);

  // disable for NimBLE
  //pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  // new for NimBLE
  pAdvertising->start();

  // removed for NimBLE
  //NimBLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

// experimental function to clean up NimBLE
void bleCleanup() 
{
  pServer->removeService(pService); 
  NimBLEDevice::deinit(true);
  
}

#endif // bleTEST

void bleConfigMain() 
{
  bool stopBLE = false;
  startTime = millis();  
  Serial.printf("bleConfigMain started\n");
  sprintf(outstring,"Heap Size: %ld FreeHp: %ld Max Block Alloc: %ld    ",
      ESP.getHeapSize(),ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  logOut(2, outstring);  
  #ifdef bleTEST
  bleSetup();
  do{
    // notify changed value
    if (deviceConnected) {
        pSensorCharacteristic->setValue(String(value).c_str());
        pSensorCharacteristic->notify();
        value++;
        Serial.print("New value notified: ");
        Serial.println(value);
        delay(3000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        Serial.println("Device disconnected, exiting.");
        stopBLE = true;
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
        Serial.println("Device Connected");      
    }
    actTime = millis();  
  }  
  while((actTime < startTime + 60000) && (!stopBLE));
  Serial.println("Cleaning up in bleConfigMain()");
  sprintf(outstring,"Heap Size: %ld FreeHp: %ld Max Block Alloc: %ld    ",
      ESP.getHeapSize(),ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  logOut(2, outstring);  
  void bleCleanup();
  Serial.println("Leaving bleConfigMain()");
  sprintf(outstring,"Heap Size: %ld FreeHp: %ld Max Block Alloc: %ld    ",
      ESP.getHeapSize(),ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  logOut(2, outstring);  
  #endif // bleTEST
}


#endif // CROW_PANEL