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
1. Lolin32 Lite ESP32 module with 4.2" WeAct Display, BME 280 sensor module, 1800 mAh LiPo battery and 3d printed housing
2. [Elecrow CrowPanel](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_4.2-inch_HMI_Display.html?srsltid=AfmBOopLg2lyLTtVUnrafO5OQ8krdLfANDyudeDwoHQay9rsuxcy4Twv) with 4.2" ePaper display and BME280 sensor connected to the exposed GPIO pins
