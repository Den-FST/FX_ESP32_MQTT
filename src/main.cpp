#include <Arduino.h>
#include <ArduinoOTA.h>
// #include <esp_task_wdt.h>
// 3 seconds WDT
#define WDT_TIMEOUT 10

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <TFT_eSPI.h>
#include <TFT_Touch.h>

#include <secret.h>

// These are the pins used to interface between the 2046 touch controller and Arduino Pro
#define DOUT 39 /* Data out pin (T_DO) of touch screen */
#define DIN 32  /* Data in pin (T_DIN) of touch screen */
#define DCS 33  /* Chip select pin (T_CS) of touch screen */
#define DCLK 25 /* Clock pin (T_CLK) of touch screen */

/* Create an instance of the touch screen library */
TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);

#define devboard

#define GFXFF 1
#define FF18 &FreeMono9pt7b

TFT_eSPI tft = TFT_eSPI(); // Initialize TFT display object

int y = 0;

const char *symbol;
const char *type;
const char *lots;
const char *profit;
const char *currprof;
const char *todaytrades;
const char *timeopen;
const char *hr_mins;
const char *dayofweek;
const char *trend;

unsigned long previousMillis = 0;      // Stores the previous time
const unsigned long delayTime = 15000; // Delay time in milliseconds

// Adjust these values in secret.h

// const char *ssid = "";
// const char *password = "";
// const char *mqtt_server = "";
// const char *mqtt_user = "";
// const char *mqtt_password = "";

const int mqtt_port = 1883;

const char *topic1 = "mt4";
const char *topic2 = "mt4_2";

WiFiClient espClient;
PubSubClient client(espClient);
// #define MQTT_MAX_PACKET_SIZE 4096 in PubSubClient.h

unsigned long currentMillis = 0;

float lastSub = 0.0;
int srv = 0;
int scr = 1;

#define TRIGGER_PIN 22
#define CHANGE_SRV_PIN 0
#define BUZZOFF_PIN 17

const int buzzerPin = 27; // Use the actual pin number where you connected the buzzer

int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
int noteDuration = 300;
bool buzzMode = false;

// TFT Printing function
void printTFT(int x, int y, String text, const GFXfont *font, uint16_t color, int size, int format)
{
  tft.setFreeFont(font);
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.setCursor(x, y);
  if (format != 1)
  {
    tft.print(text);
  }
  else
  {
    tft.println(text);
  }
}

void buzz(int note, int Duration)
{
  tone(buzzerPin, note);
  delay(Duration);
  noTone(buzzerPin);
}

void ledctrl(int r, int g, int b)
{
  digitalWrite(4, r);
  digitalWrite(16, g);
  // digitalWrite(17, b);
}

void deserializeJson(String jsonData)
{

  DynamicJsonDocument doc(4096);
  // StaticJsonDocument<800> doc; // Adjust the size if necessary
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error)
  {
    Serial.print("deserializeJson() failed with code: ");
    Serial.println(error.c_str());
    return;
  }
  symbol = doc["s"];
  type = doc["t"];
  lots = doc["l"];
  profit = doc["pr"];
  currprof = doc["cpr"];
  todaytrades = doc["ttr"];
  timeopen = doc["td"];
  hr_mins = doc["hm"];
  dayofweek = doc["dw"];
  trend = doc["tr"];

  // Serial.print("Symbol: ");
  // Serial.println(_TYPEINFO(symbol));
  // Serial.println(symbol);

  if (type == NULL)
  {
    Serial.println("No trades");
    tft.fillScreen(TFT_BLACK);
    printTFT(55, 120, "No Trades", FF18, TFT_WHITE, 2, 1);
    printTFT(100, 150, "On server: ", FF18, TFT_YELLOW, 1, 1);
    printTFT(215, 150, String(srv), FF18, TFT_WHITE, 1, 1);
    printTFT(55, 200, "Server Time: ", FF18, TFT_WHITE, 1, 1);
    printTFT(195, 200, hr_mins, FF18, TFT_GREEN, 1, 1);
  }
  else
  {

     tft.fillScreen(TFT_BLACK);

    y += 18;
    tft.fillRect(0, y - 15, TFT_HEIGHT, 50, TFT_BLACK);
    printTFT(TFT_WIDTH / 16, y, symbol, FF18, TFT_WHITE, 1, 0);
    delay(5);

    if (type == "Sell")
    {
      printTFT(TFT_WIDTH / 2.45, y, "SL", FF18, TFT_BLUE, 1, 0);
      delay(5);
    }
    else
    {
      printTFT(TFT_WIDTH / 2.45, y, "BY", FF18, TFT_MAGENTA, 1, 0);
      delay(5);
    }

    printTFT(TFT_WIDTH / 1.8, y, lots, FF18, TFT_WHITE, 1, 0);
    delay(5);

    float col = atoi(profit);
    if (col < 0.01)
    {
      printTFT(TFT_WIDTH / 1.3, y, profit, FF18, TFT_RED, 1, 0); // Negative values
      delay(5);
    }
    else if (col >= 0.01)
    {
      printTFT(TFT_WIDTH / 1.234, y, profit, FF18, TFT_GREEN, 1, 0); // Positive values
      delay(5);

      if (buzzMode == true)
      { // Buzz melody then pozitive value

        if (col >= 1.0 && (col - lastSub) > 2.0)
        {
          buzz((col * 50), 200);
          lastSub = col;
        }
      }
    }
    printTFT(TFT_WIDTH + 20, y, timeopen, FF18, TFT_LIGHTGREY, 1, 0);
    delay(5);

    if (atoi(dayofweek) == 1)
    {
      tft.drawTriangle(2, y - 2, 8, y - 2, 5, y - 7, TFT_GREEN);
    }
    else
    {
      tft.drawTriangle(2, y - 7, 8, y - 7, 5, y - 2, TFT_RED);
    }

    delay(5);
    tft.drawLine(0, y + 5, TFT_HEIGHT, y + 5, TFT_YELLOW); // DRAW YELLOW LINE AND SHOW ON TFT PAIR TEXT
    delay(5);
    tft.fillRect(0, y + 25, TFT_HEIGHT, TFT_WIDTH - (TFT_WIDTH - y), TFT_BLACK);
    delay(5);
    printTFT(20, y + 20, hr_mins, FF18, TFT_GREEN, 1, 1);
    delay(5);
    printTFT(TFT_WIDTH / 1.8, y + 20, todaytrades, FF18, TFT_CYAN, 1, 1);
    delay(5);

    tft.setCursor(TFT_WIDTH / 1.3, y + 20);
    int col1 = atoi(currprof);
    if (col1 < 0)
      tft.setTextColor(TFT_GOLD);
    else
    {
      tft.setTextColor(TFT_GREEN);
    }
    delay(5);
    tft.setCursor(TFT_WIDTH / 1.3, y + 20);
    tft.print(currprof);
    delay(5);

    printTFT(TFT_WIDTH + 15, y + 20, "S:", FF18, TFT_WHITE, 1, 1);
    printTFT(TFT_WIDTH + 35, y + 20, String(srv), FF18, TFT_WHITE, 1, 1);


  } // END IF

  // y = 0;

  // todaytrades = doc["ttr"];
  // hr_mins = doc["hm"];
  // dayofweek = doc["dw"];

  int srv_h = String(hr_mins).substring(0, 2).toInt();
  int srv_hm = String(hr_mins).substring(3, 5).toInt();

  // Serial.print("H:");
  // Serial.print(srv_h);
  // Serial.print(" - M:");
  // Serial.print(srv_hm);
  // Serial.print(" - DoW: ");
  // Serial.println(dayofweek);

  if (srv_h > 6 && srv_h < 23 && atoi(dayofweek) != 6 && atoi(dayofweek) != 0 && scr != 0) // If time is betwen 6 a.m. and 11 p.m. and not weekend day, switch monitor on
  {
    digitalWrite(TFT_BL, HIGH);
    ledctrl(1, 0, 1);
    buzzMode = true;
  }

  else // Monitor off
  {
    digitalWrite(TFT_BL, LOW);
    ledctrl(0, 1, 1);
    buzzMode = false;
  }

  doc.clear(); // Clear memory

  // Serial.println(); // Add an empty line for readability
}

void callback(char *topic, byte *payload, unsigned int length)
{
  char receivedData[2048]; // Adjust the size if necessary
  for (unsigned int i = 0; i < length; i++)
  {
    receivedData[i] = (char)payload[i];
  }
  receivedData[length] = '\0'; // Null-terminate

  // currentMillis = millis();
  // Serial.print("T: ");
  // Serial.print(currentMillis);
  // Serial.println(" ms");
  // //Serial.println(receivedData);

  int dataStart = 0;
  int dataEnd = -1;
  String dataRecv = receivedData;

  // Serial.println(dataRecv);

  while (dataStart < dataRecv.length())
  {
    dataStart = dataRecv.indexOf("{", dataStart);
    dataEnd = dataRecv.indexOf("}", dataEnd + 1);

    if (dataStart == -1 || dataEnd == -1)
    {
      break;
    }

    // Serial.print("Data start: ");
    // Serial.print(dataStart);
    // Serial.print(" Data end: ");
    // Serial.println(dataEnd);

    String singleJsonData = dataRecv.substring(dataStart, dataEnd + 1);

    Serial.println(singleJsonData);

    delay(5);

    deserializeJson(singleJsonData);

    dataStart = dataEnd + 1; // Move to the position after the closing brace of the previous JSON object
  }
  y = 0;
}

void setup()
{
  Serial.begin(115200);

  // esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  // esp_task_wdt_add(NULL); //add current thread to WDT watch

  pinMode(buzzerPin, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(CHANGE_SRV_PIN, INPUT_PULLUP);
  pinMode(BUZZOFF_PIN, INPUT_PULLUP);
  // pinMode(17, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  // digitalWrite(17, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(16, HIGH);

  tft.begin();
  tft.setRotation(1); // Set display rotation (adjust to match your TFT display)
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 20);
  tft.println("Started FX Monitor");

  // Connect to Wi-Fi
  Serial.println("Connecting to WiFi...");
  tft.print("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  tft.setTextColor(TFT_GREEN);
  tft.print(" OK - ");
  tft.setTextColor(TFT_WHITE);
  tft.println(WiFi.localIP());
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Connect to MQTT Broker
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
    tft.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password))
    {
      Serial.println("Connected to MQTT");
      tft.setTextColor(TFT_GREEN);
      tft.print(" OK - ");
      tft.setTextColor(TFT_WHITE);
      tft.println(mqtt_server);
      tft.setTextColor(TFT_YELLOW);
      tft.print("Waiting for data from server: ");
      tft.println(srv);
      client.subscribe(topic2);
    }
    else
    {
      tft.setTextColor(TFT_RED);
      tft.print("Failed with state ");
      tft.println(client.state());
      tft.setTextColor(TFT_WHITE);
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  ArduinoOTA.setHostname("FX-ota");
  ArduinoOTA.begin();
  delay(500);
}

void loop()
{
  ArduinoOTA.handle(); // Upload over air. Ota handle.

  client.loop();

  uint16_t touchX, touchY;

  bool touched = touch.Pressed(); // tft.getTouch( &touchX, &touchY, 600 );
  int srvTouch = 0;

  if (touched)
  {

    if (scr == 1)
    {
      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(1);
      tft.drawCentreString("SCREEN OFF IN 5 SEC", TFT_WIDTH / 3, TFT_HEIGHT / 6, 2);
      scr = 0;
      delay(5000);
      digitalWrite(TFT_BL, LOW);
      Serial.println("Screen OFF");
    }
    else
    {
      digitalWrite(TFT_BL, HIGH);
      Serial.println("Screen ON");
      delay(1000);
      scr = 1;
    }

    touchX = touch.X();
    touchY = touch.Y();

    Serial.print("Data x ");
    Serial.println(touchX);

    Serial.print("Data y ");
    Serial.println(touchY);

    // ---=== MENU TEST ===---
    // tft.drawRect(touchX, touchY, 60, 40, TFT_RED);
    // tft.fillRect(touchX, touchY, 58, 38, TFT_LIGHTGREY);
    // tft.setTextColor(TFT_BLACK);
    // String item1 = "Menu 1";
    // tft.drawString(item1 , touchX, touchY, 1);
    // Serial.print("TEXT RECT");
    // Serial.print(tft.textWidth(item1));
    // Serial.print(" - ");
    // Serial.println(tft.fontHeight(1));
  }

  if (digitalRead(BUZZOFF_PIN) == LOW)
  {
    if (buzzMode == true)
    {
      buzzMode = false;
      Serial.println("BuzzMode Off");

      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(1);
      tft.drawCentreString("BUZZ OFF", TFT_WIDTH / 3, TFT_HEIGHT / 6, 2);
    }
    else if (buzzMode == false)
    {
      buzzMode = true;
      Serial.println("BuzzMode On");

      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(1);
      tft.drawCentreString("BUZZ ON", TFT_WIDTH / 3, TFT_HEIGHT / 6, 2);
    }
    delay(2000);
  }

  if ((digitalRead(CHANGE_SRV_PIN) == LOW) && srv == 0) // Change to server 2
  {
    tft.fillScreen(TFT_BLACK);

    tft.setTextFont(1);
    tft.drawCentreString("FIRST - 1", TFT_WIDTH / 3, TFT_HEIGHT / 6, 2);
    tft.drawRoundRect(TFT_WIDTH / 15, TFT_HEIGHT / 4, tft.textWidth(topic1) + 125, 40, 5, TFT_RED);
    // tft.fillRoundRect(TFT_WIDTH / 6, TFT_HEIGHT / 4, tft.textWidth(combinedString)+120 , 40, 3, TFT_LIGHTGREY);

    printTFT(TFT_WIDTH / 14, TFT_HEIGHT / 3, topic1, FF18, TFT_LIGHTGREY, 1, 1);

    Serial.print("Server Changed to First! ");
    // Serial.print(fx_server_con);
    Serial.println(topic1);

    client.unsubscribe(topic2);
    delay(100);
    client.subscribe(topic1);

    srv = 1;
    delay(2000);

    tft.fillScreen(TFT_BLACK);
    printTFT(0, 50, "Waiting for data...", FF18, TFT_WHITE, 1, 1);
  }

  if ((digitalRead(CHANGE_SRV_PIN) == LOW) && srv == 1) // Change to server 2
  {

    // strcpy(fx_server_con, fx_server2);
    // strcpy(fx_port_con, fx_port2);

    tft.fillScreen(TFT_BLACK);

    // String combinedString = String(fx_server_con) + ":" + String(fx_port_con);

    tft.setTextFont(1);
    tft.drawCentreString("SECOND - 2", TFT_WIDTH / 3, TFT_HEIGHT / 6, 2);
    tft.drawRoundRect(TFT_WIDTH / 15, TFT_HEIGHT / 4, tft.textWidth(topic2) + 125, 40, 5, TFT_RED);
    // tft.fillRoundRect(TFT_WIDTH / 6, TFT_HEIGHT / 4, tft.textWidth(combinedString)+118 , 38, 3, TFT_LIGHTGREY);

    printTFT(TFT_WIDTH / 14, TFT_HEIGHT / 3, topic2, FF18, TFT_LIGHTGREY, 1, 1);

    Serial.print("Server Changed to Second! ");
    // WebSerial.println(F("Server Changed to Second!"));
    // Serial.print(fx_server_con);
    Serial.println(topic2);
    client.unsubscribe(topic1);
    delay(100);
    client.subscribe(topic2);
    delay(2000);

    tft.fillScreen(TFT_BLACK);
    printTFT(0, 50, "Waiting for data...", FF18, TFT_WHITE, 1, 1);
    srv = 0;
  }
  // esp_task_wdt_reset();

  unsigned long currentMillis = millis(); // Get the current time

  // Check if the desired delay time has passed
  if (currentMillis - previousMillis >= delayTime)
  {
    // Reset the previous time
    previousMillis = currentMillis;

    if (srv == 1)
    {

      client.unsubscribe(topic1);
      delay(100);
      client.subscribe(topic2);
      srv = 0;
    }
    else if (srv == 0)
    {
      client.unsubscribe(topic2);
      delay(100);
      client.subscribe(topic1);
      srv = 1;
    }

    // client.unsubscribe(topic1);
  }
}
