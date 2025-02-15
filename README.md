# ESP32 ePaper Barograph / Thermograph / Hydrograph
This project is an ESP32 based device that records and displays atmospheric pressure, temperature and humidity using a BME280 sensor and displays them as curves on an 4.2" B/W ePaper display. It operates from a LiPo battery which should be good for 3-6 months on one charge, and can be recharged via USB.
Basic features:
- 4.2" ePaper Display (I am using the display from WeAct, but others with 400x300 resolution are also possible)
- ESP32 Lolin32 Lite module for battery operation, with 1800 mAh LiPo battery intended for 3-6 months operation (not yet tested), charging via USB
- BME280 high quality sensor for pressure, temperature and humidity
- Curves for pressure, temperature, humidity in all combinations
- automatic adjustment of display with independent scales for all 3 measured paramters 
- Time scale can be switched "on the fly" between 21/36/72 hours, while retaining the data that fit into the newly selected time scale
- Tendency display (symbols and numeric) for all 3 measured parameters
- Buzzer can be added, to enable audible alert for large pressure changes and feedback when changing parameters via bluetooth
- Offset for altitude of observation can be enabled and adjusted
- Settings, after activation with button, via bluetooth
- 3D printed housing

## Pictures
Some pictures of the housing and the screen:

![Display](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/Pictures/Standard%20PT.jpg)
![Inverted display](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/Pictures/Inverted%20PTH.jpg)
![Housing](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/Pictures/Hoursing.jpg)

## Parts
The project presently supports two devices:
### Lolin32 Lite ESP32 module 
With 4.2" WeAct Display, BME 280 sensor module, 1800 mAh LiPo battery and 3d printed housing. A PCB board is also available.
   The following parts are needed:
   - [Lolin32 Lite ESP32 module](https://www.amazon.de/AZDelivery-LOLIN32-Bluetooth-kompatibel-Arduino/dp/B086V8X2RM/ref=sr_1_1_pp?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=2Y27I7AEYM9WX&dib=eyJ2IjoiMSJ9.oNJbRfCZ-fu_vozIfxLCtf3HXLp9I-uPyWlu2nvjfPpeDhkVGGNlZxv5Sz1wx8w9KMfxJls-1ZDVz42O4GHf5jZGgGVwZPdvzvrQdXk0kT_A6RewTc9YMkqjQ3iQTPgM0uivrl79juA1taJghOAjpS3Fwa6kLbrBI_IJpl4o2-8MCH1yzFgi4ckKFVAqtziA.9GUBbFdNLPXHgTDpc1SDFziCLFOkvI_fY1c62FEOx04&dib_tag=se&keywords=Lolin32%2BLite&qid=1738659974&sprefix=lolin32%2Blite%2Caps%2C199&sr=8-1&th=1)
   - [BME 280 Module 3.3V](https://www.amazon.de/s?k=bme280&ref=404_search) Make sure that you get a module with 4 pins (6 pins would also work, but not fit into the housing), and from a reliable supplier. Beware: there are many cheap boards on Ebay that in reality do not contain a BME280 (square chip housing), but a BMP280 (rectangular chip housing)
   - [1800 mAh LiPo Battery 52x34x10 mm with JST-PH 2.0 connector](https://www.ebay.de/itm/255510046348?mkcid=16&mkevt=1&mkrid=707-127634-2357-0&ssspo=yTbngviFSJu&sssrc=4429486&ssuid=nJMJusrNRZS&var=555706977194&widget_ver=artemis&media=COPY) Other LiPo batteries will also work, but likely not fit into the housing. **ATTENTION** for whatever reason the +/- on the Lolin32 Lite board are not identical to the polarity of most available batteries. **Do not use as delivered, you will likely fry the board!** With a needle you can unlatch the pins in the JST connector, pull out the pins and insert them again in reverse order.
   - JST PH 2.0 ( male & female) connectors. Connectors alsready configured with cables can be used.
   - [4.2" ePaper display module by WeAct](https://de.aliexpress.com/w/wholesale-Weact-studio-ePaper-4.2.html?spm=a2g0o.detail.search.0) 
   - [Elecrow CrowPanel](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_4.2-inch_HMI_Display.html?srsltid=AfmBOopLg2lyLTtVUnrafO5OQ8krdLfANDyudeDwoHQay9rsuxcy4Twv) with 4.2" ePaper display and BME280 sensor connected to the exposed GPIO pins.
   - 2 Resistors 200 KOhm
   - 1 Pushbutton for hole diameter 7mm
DS18B20, buzzer, transistor are presently not supported and not needed, but can be attached to the PCB
### Elecrow CrowPanel 4.2" ePaper
  - [Elecrow CrowPanel 2.2" e-paper HMI display](https://m.elecrow.com/pages/shop/product/details?id=207653&srsltid=AfmBOoq9SIdzb9AhMf4R4YTJ9BLrYichk5EBjKeTe6YT30B_DNrccfvo)
  - [BME 280 Module 3.3V](https://www.amazon.de/s?k=bme280&ref=404_search) Make sure that you get a module with 4 pins (6 pins would also work, but not fit into the housing), and from a reliable supplier. Beware: there are many cheap boards on Ebay that in reality do not contain a BME280 (square chip housing), but a BMP280 (rectangular chip housing). Cables and 2.54 mm pinheader for electrical connection.
   - LiPo Battery 3.7V with Micro JST 1.0 connector. Unfortunately Elecrow has chosen a tiny connector which is rarely used with LiPo batteries. I could not find a battery that comes with this connector, you will have to order battery and connector separately.

## Schematics and PCB
### Lolin32 Lite
I have created a PCB for the project, for use with the Lolin32 Lite module. It permits to connect the Lolin32 Lite module, ePaper, BME280 and battery connectors. It is also possible to add up to 2 DS18B20 temperature sensors and a piezo beeper - both not presently supported by the software. GPIO pins that are not directly used are exposed on a separate connector.
Gerber files are included in this project.
![PCB](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/PCB/ePaperBarograf_schematic_V0.1.jpg)
![PCB](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/PCB/ePaperBarograf_PCB_V0.1.jpg)
### Elecrow CrowPanel 4.2" ePaper
This handy device can be used as is, with these addtions:
- BME280 (3.3V version) has to be connected via the connector on top to 3.3V, GND and SDA (GPIO15)/ SCL (GPIO19)
- LiPo battery can be connected via "Battery" connector on the left side of the housing. This is optional, without battery you need to keep the system permanently connected to power via USB - with every power loss the history data, which are kept in "RTC memory" are lost. 

## Software
### Software Structure
The software consists of 3 main components, which are located in three .cpp files
1. Control (ePaperBarograf.cpp): Timing, preferences storage, measurement, data storage & recalculations, hibernation
2. Graphics (ePaperGraphics.cpp): Graphics functions for ePaper display
3. Bluetooth configuration (ePaperBluetooth.cpp): Serial Bluetooth functions for adjustment of settings. Serial Bluetooth can only be used with the Lolin32 Lite - the CrowPanel has an ESP32S3 which only supports Bluetooth Low Energy (BLE).
4. BLE configuation - presently experimental and not yet functional
### Libraries
- OneWire                  : needed for BME280
- Arduino Unified Sensor   : needed for BME280
- Arduino BME280           : needed for BME280
- GxEPD2                   : ePaper Graphics
- Dallas Temperature       : DS18B20 temperature sensor
- u8g2 for Adafruit        : Fonts
### Building the Software
The software is written in C++, platform is Arduino and development environment is Platformio with VSCode. 
platformio.ini contains two development environments:
- env:Lolin32Lite_ePaper - this is the environment used for the Lolin32 Lite with separate 4.2" ePaper and battery. 
- env:CrowPanel_42 - this is the environment used for the Elecrow CrowPanel 4.2" ePaper.
Just connect your ESP32 to the computer via USB, select the env for the system you are building for and start the build. Platformio will automatically load the libraries that are needed and upload the firmware via USB Port. 
Switching of environments is done by clicking on the "env:..." entry in the lower status bar of VSCode, and then selecting the environment in the list that is displayed on top. Switching takes a few seconds.
Note that two versions of the hardware board are used, which have slightly different pinouts. Inparticular, in prototypes with hand mande board GPIO 35 is used for measurement of the battery voltage. In the newer printed circuit boards, GPIO 39 is used. This must be taken into account by setting either -D HANDMADE_BOARD or -D PCB_BOARD in [env] section within platformio.ini. Just comment out the part not needed.
### Starting the Barograph
After final assembly and test of all parts the software needs to be flashed on the ESP32. This is done by connecting the device the deveopment computer via USB cable and using the "Build" feature of platformio.
Once the software has been flashed, it will begin to operate directly:
- Default parameters are written into the ESP32 preferences memory (flash), which survies power down and even flashing of a new sofware version
- Test data are created, stored in RTC memory and displayed. They consist of artificial curves, which are easy to recognize and overwritten by real measured data as they are created. Complete overwrite of all test data takes 21 hours in default settings, up o 84 hours if the maximum time setting is used. 
Note that RTC memory content survives the sleep period between two measurements, but not power down (battery disconnected) or flashing of new software
- The barograf will then start routine operation- In default settings the measuring range is 18 hours, Pressure, Temperature and Humidiy are displayed. If more than 1 curve is shown, each has it's own scale. he further right a scale is on the screen, the thicker the corresponding curve If, e.g., Pm T and H are displayed, the pressure curve is 3 pixels, the temperature curve 2 pixels and the humidity curve one pixel.
### Settings
Settings are adjusted via Bluetooth connection. There is no special app for this, please use a bluetooth terminal on a mobile device with Bluetooth. A good tool for this is the [Serial Bluetooth Terminal by Kai Morich](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal&hl=de)
This is how it is done:
- The Barograf is set into settings mode by presing the button. A corresponding message is shown on the screen
- Connect the mobile device to the barograph using the bluetooth settings. This has to be done only once
- Connect the Bluetooth terminal to the barograph, using the list of available devices in the "Hamburger" menu
- The barograph will then send a list of available commands 
- Enter the desired command. 
Examples: 
ATI to invert the display
ATC,1 to enable pressure correction (add the correction value)
ATX to leave the bluetooth settings and restart measurements
- Exit bluetooth settings 
If no command is given, the barograph will revert to measurement mode after 60 seconds
## Open topics
- BLE functionality to allow setting of parameters for CrowPanel
- Bluetooth setting for Lolin32 Lite is presently only possible via Bluetooth terminal on a mobile device. Create App or BLE web page to allow easier configuration
