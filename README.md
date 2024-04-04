# Forex ESP32 MQTT TFT Expert data exchange.

Connect to Metatrader 4 Expert by MQTT, receive data through the python bridge and MQTT, and display on the screen TFT every x seconds. 

![GitHub last commit (branch)](https://img.shields.io/github/last-commit/Den-FST/FX_ESP32_MQTT/main?style=plastic)

## In my case is 2 config, TFT

``` C++

```

Next implementation tasks:
- [x] Touch screen for On/Off backlight.
- [x] Buzzer play then any order is positive.
- [x] Button for On/Off buzzer.
- [x] Screen Off in WeekEnd and in night time.
- [ ] AP WiFi Portal.
- [ ] WebServer View Forex data.
- [ ] Time Sync and scenaries.
- [ ] Send data to Home Assistant.
- [ ] WebSerial control. Send "cmd" to view commands. 

<img src="main.jpg" width="400"/>

for ILI9341

```C++

#define TFT_MOSI 13 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select control pin
#define TFT_DC   2  // Data Command control pin
#define TFT_RST  12  // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL   21  // LED back-light

#define TOUCH_CS 33     // Chip select pin (T_CS) of touch screen



```