#define FUNCTIONS

#ifdef FUNCTIONS

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <sys/time.h>
#include <Timezone.h>


#include <HTTPClient.h>
#include <Update.h>

#include <esp_task_wdt.h>

#include <TFT_eSPI.h>
#include <TFT_Touch.h>

HTTPClient client2;
#define FWURL "http://www.fst.pt/upload/firmware_v"
#define FWversion 1

#define GFXFF 1
#define FF18 &FreeMono9pt7b

TFT_eSPI tft = TFT_eSPI(); // Initialize TFT display object

int currentVersion = FWversion;
int mins_check = 0;
const char* ntpServer = "pool.ntp.org"; //Time server
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

TimeChangeRule myDST = {"DST", Last, Sun, Mar, 2, 60}; // Daylight Saving Time
TimeChangeRule mySTD = {"STD", Last, Sun, Oct, 3, 0};  // Standard Time

Timezone myTZ(myDST, mySTD);

const char* dayWeek[]= 
      {
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
      };


unsigned int currentFileSize = 0;
uint8_t* clientContent = nullptr;

void time_sync() {
    while (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500);
  }
}

void set_time() {

      time_sync();
    // Get the current UNIX timestamp from the time client
    unsigned long epochTime = timeClient.getEpochTime();

    // Set the system time using the UNIX timestamp
    struct timeval tv;
    tv.tv_sec = epochTime;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
    Serial.println("TimeSet Ok");
}

String sendTime() {
  String timeString = "";
  // Get the current datetime
  time_t t = time(nullptr);

    // Convert to local time
  TimeChangeRule* tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  int currentSecond = timeinfo->tm_sec;

  // Format the time string
  if (currentHour < 10) {
    timeString += "0";
  }
  timeString += String(currentHour);
  
  timeString += ":";
  
  if (currentMinute < 10) {
    timeString += "0";
  }
  timeString += String(currentMinute);
  
  timeString += ":";
  
  if (currentSecond < 10) {
    timeString += "0";
  }
  timeString += String(currentSecond);
  
  return timeString;
}

void printDateTime(int tftparam) {
  // Get the current datetime
  time_t t = time(nullptr);

    // Convert to local time
  TimeChangeRule* tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  // Print the datetime
  Serial.print("Current datetime: ");
  Serial.print(timeinfo->tm_year + 1900);
  Serial.print("-");
  Serial.print(timeinfo->tm_mon + 1);
  Serial.print("-");
  Serial.print(timeinfo->tm_mday);
  Serial.print(" ");
  Serial.print(timeinfo->tm_hour);
  Serial.print(":");
  Serial.print(timeinfo->tm_min);
  Serial.print(":");
  Serial.print(timeinfo->tm_sec);

  int dayOfWeek = timeinfo->tm_wday;
  mins_check = timeinfo->tm_min;
  
  Serial.println(dayWeek[dayOfWeek]);

  if (tftparam == 1){
        tft.setTextColor(TFT_GREEN);
        tft.print("Time: ");
        tft.print(timeinfo->tm_mday);
        tft.print("-");
        tft.print(timeinfo->tm_mon);
        tft.print("-");
        tft.print(timeinfo->tm_year + 1900);
        tft.print(" ");
        tft.print(timeinfo->tm_hour);
        tft.print(":");
        tft.print(timeinfo->tm_min);
        tft.print(":");
        tft.print(timeinfo->tm_sec);
        tft.print(" ");
        tft.println(dayWeek[dayOfWeek]);
        tft.setTextColor(TFT_WHITE);
}

}


void ledctrl(int r, int g, int b) {
  digitalWrite(4, r);
  digitalWrite(16, g);
  digitalWrite(17, b);
}


int CheckDayWeek() {
    // Get the current datetime
  time_t t = time(nullptr);

    // Convert to local time
  TimeChangeRule* tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  int dayOfWeek = timeinfo->tm_wday;

  return dayOfWeek;
}


bool checkFileSize(int fileSize) {
  // Get the current firmware size
  int currentFileSize = ESP.getSketchSize();
  Serial.println(currentFileSize);
  // Compare file sizes
  if (fileSize > currentFileSize) {
    Serial.println("New firmware available");
    Serial.println("New firmware available");
    return true;
  } else {
    Serial.println("Firmware is up to date");
    Serial.println("Firmware is up to date");
    return false;
  }
}

void updateFirmware() {
  esp_task_wdt_init(30, false);

  // Connect to the web server
  String fwVersionURL = FWURL + String(currentVersion + 1) + ".bin";
  Serial.print("Checking for firmware at URL: ");
  Serial.println(fwVersionURL);

  client2.begin(fwVersionURL);
  int httpCode = client2.GET();

  // Check if the server returned a valid response
  if (httpCode == HTTP_CODE_OK) {
    int contentLength = client2.getSize();
    Serial.println(contentLength);
    if (checkFileSize(contentLength)) {
      Serial.print("Updating firmware to version ");
      Serial.print("Updating firmware to version ");
      Serial.println(currentVersion + 1);
      delay(500);
      // Start the firmware update process
      if (Update.begin(contentLength, U_FLASH)) {
        // Serial.println("Firmware update started!");
        // Receive and write firmware data to the ESP32 flash memory
        WiFiClient* stream = client2.getStreamPtr();
        uint8_t buff[1024];
        int bytesRead = 0;

        while (client2.connected() && (bytesRead < contentLength)) {
          size_t size = stream->available();
          if (size) {
            int len = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            Update.write(buff, len);
            bytesRead += len;
          }
          delay(1);
        }

        // Finish the firmware update
        if (Update.end()) {
          if (Update.isFinished()) {
            Serial.println("Firmware update successful");
            Serial.println("Firmware update successful");
            currentVersion++;
            ESP.restart();
          } else {
            Serial.println("Firmware update failed");
            Serial.println("Firmware update failed");
          }
        } else {
          Serial.println("Firmware update end failed");
          Serial.println("Firmware update end failed");
        }
      } else {
        Serial.println("Firmware update begin failed");
        Serial.println("Firmware update begin failed");
      }
    }
  } else {
    Serial.println("Firmware download failed");
    Serial.println("Firmware download failed");
  }
  
  client2.end();
}

#endif

