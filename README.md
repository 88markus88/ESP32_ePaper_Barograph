# ESP32 ePaper Barograph
This project is an ESP32 based device that records and displays atmospheric pressure, temperature and humidity using a BME280 sensor and displays them as curves on an 4.2" B/W ePaper display. It operates from a LiPo battery which should be good for 3-6 month on one charge, and can be recharged via USB.
Basic features:
- 4.2" ePaper Display (I am using the display from WeAct, but others with 400x300 resolution are also possible)
- ESP32 Lolin32 Lite module for battery operation, with 1800 mAh LiPo battery intended for 3-6 months operation (not yet tested), chraging via USB
- BME280 high quality sensor for pressure, temperature and humidity
- Curves for pressure, temperature, humidity in all combinations
- automatic adjustement of display with independent scales for all 3 measured paramters 
- Time scale can be switched "on the fly" between 21/36/72 hours, while retaining the data that fit into the newly selected time scale
- Tendency display (symbols and numeric) for all 3 measured parameters
- Offset for hight of observation can be enabled and adjusted
- Settings, after activation with button, via bluetooth
- 3D printed housing

## Pictures
Some pictures of the housing and the screen:

![Display](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/Pictures/Standard%20PT.jpg)
![Inverted display](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/Pictures/Inverted%20PTH.jpg)
![Housing](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/Pictures/Hoursing.jpg)

## Parts
The project presently supports two devices:
1. Lolin32 Lite ESP32 module with 4.2" WeAct Display, BME 280 sensor module, 1800 mAh LiPo battery and 3d printed housing. A PCB board is also available.
   The following parts are needed:
   - [Lolin32 Lite RSP32 module](https://www.amazon.de/AZDelivery-LOLIN32-Bluetooth-kompatibel-Arduino/dp/B086V8X2RM/ref=sr_1_1_pp?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=2Y27I7AEYM9WX&dib=eyJ2IjoiMSJ9.oNJbRfCZ-fu_vozIfxLCtf3HXLp9I-uPyWlu2nvjfPpeDhkVGGNlZxv5Sz1wx8w9KMfxJls-1ZDVz42O4GHf5jZGgGVwZPdvzvrQdXk0kT_A6RewTc9YMkqjQ3iQTPgM0uivrl79juA1taJghOAjpS3Fwa6kLbrBI_IJpl4o2-8MCH1yzFgi4ckKFVAqtziA.9GUBbFdNLPXHgTDpc1SDFziCLFOkvI_fY1c62FEOx04&dib_tag=se&keywords=Lolin32%2BLite&qid=1738659974&sprefix=lolin32%2Blite%2Caps%2C199&sr=8-1&th=1)
   - [BME 280 Module 3.3V](https://www.amazon.de/s?k=bme280&ref=404_search) Make sure that you get a module with 4 pins (6 pins would also work, but not fit into the housing), and from a reliable supplier. Beware: there are many cheap boards on Ebay that in reality do not contain a BME280 (square chip housing), but a BMP280 (rectangular chip housing)
   - 1800 mAh LiPo Battery 
   - 4.2" ePaper display module by WeAct 
3. [Elecrow CrowPanel](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_4.2-inch_HMI_Display.html?srsltid=AfmBOopLg2lyLTtVUnrafO5OQ8krdLfANDyudeDwoHQay9rsuxcy4Twv) with 4.2" ePaper display and BME280 sensor connected to the exposed GPIO pins

## Schematics and PCB
### Lolin32 Lite
I have created a PCB for the project, when using the Lolin32 Lite module. It permits to connect the Lolin32 Lite module, ePaper, BME280 and battery connectors. It is also possible to add up to 2 DS18B20 temperature sensors and a piezo beeper - both not presently supported by the software. GPIO pins that are not directly used are exposed on a separate connector.
Gerber files are included in this project.
![PCB](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/PCB/ePaperBarograf_schematic_V0.1.jpg)
![PCB](https://github.com/88markus88/ESP32_ePaper_Barograph/blob/main/PCB/ePaperBarograf_PCB_V0.1.jpg)
### Elecrow CrowPanel 4.2" ePaper
This handy device can be used as is, with these addtions:
- BME280 (3.3V version) has to be connected via the connector on top to 3.3V, GND and SDA (GPIO15)/ SCL (GPIO19)
- LiPo battery can be connected via "Battery" connector on the left side of the housing. This is optional, without battery you need to keep the system permanently connected to power via USB - with every power loss the history data, which are kept in "RTC memory" are lost. 

## Building the Software
The software is written in C++, platform is Arduino and development environment is Platformio with VSCode. 
platformio.ini contains two development environments:
- env:Lolin32Lite_ePaper - this is the environment used for the Lolin32 Lite with separate 4.2" ePaper and battery. 
- env:CrowPanel_42 - this is the environment used for the Elecrow CrowPanel 4.2" ePaper.
Just connect your ESP32 to the computer via USB, select the env for the system you are building for and start the build. Platformio will automatically load the libraries that are needed and upload the firmware via USB Port. 
