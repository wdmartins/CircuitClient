#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <CircuitClient.h>
#include <Ticker.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Access point to configure Wi-Fi
#define ACCESS_POINT_NAME "ESP8266"
#define ACCESS_POINT_PASS "esp8266"

// GPIO Definitios
#define DHT11_GPIO 2
#define BUTTON_GPIO 0

// Sensors Definitions
#define DHT_TYPE DHT11
#define DHT_TEMP_CHANGE_TRIGGER_CELSIUS 1
#define DHT_HUMI_CHANGE_TRIGGER_PERCENT 1

// Sensors Instantiation
DHT_Unified dht(DHT11_GPIO, DHT_TYPE);

// Intervals
Ticker ledTicker;
Ticker dhtTicker;
Ticker buttonTicker;

// Circuit Client declaration
CircuitClient *circuitClient;

// Setup Liquid crystal display
LiquidCrystal_I2C lcd(0x27, 16 ,2);

uint32_t dht11DelayMS;
float currentTemperature = -300;
float currentHumidity= -1;
boolean updateDht11 = false;
boolean checkButton = false;
int buttonState = 0;

void ledTick() {
  int state = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !state);
}

// Wi-Fi Configuration mode callback
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ledTicker.attach(0.2, ledTick);
}

// Circuit new text item callback
void onNewTextItemCB (String text) {
  Serial.print("Text Received: "); Serial.println(text);
  lcd.clear();
  lcd.print(text.c_str());
}

void setUpdateDht11() {
  updateDht11 = !updateDht11;
}

void changeCheckButton() {
  checkButton = !checkButton;
}

void processDhtInfo () {
  boolean report = false;
  sensors_event_t event;
  float temp;
  float humidity;

  dht.temperature().getEvent(&event);

  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    return;
  } else {
    temp = event.temperature;
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    return;
  } else {
    humidity = event.relative_humidity;
  }

  if ((currentTemperature + DHT_TEMP_CHANGE_TRIGGER_CELSIUS) < temp ||
    (currentTemperature - DHT_TEMP_CHANGE_TRIGGER_CELSIUS) > temp) {
      report = true;
  }
  if (report == false && (((currentHumidity + DHT_HUMI_CHANGE_TRIGGER_PERCENT) < humidity) ||
    ((currentHumidity - DHT_HUMI_CHANGE_TRIGGER_PERCENT > humidity)))) {
      report = true;
  }
  currentTemperature = temp;
  currentHumidity = humidity;
  if (report) {
    Serial.print("Reporting temperature: "); Serial.print(temp); Serial.print(" and humidity: "); Serial.println(currentHumidity);
    char temp[17];
    sprintf(temp, "Temp %d, Hum %d", static_cast<int>(currentTemperature), static_cast<int>(currentHumidity));
    lcd.setCursor(0,1);
    lcd.clear();
    lcd.print(temp);
    circuitClient->postTextMessage(temp);
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize Liquid crystal display
  lcd.begin();
  lcd.setCursor(0,0);
  lcd.backlight();
  lcd.print("Starting...");

  // Setup built in LED
  pinMode(BUILTIN_LED, OUTPUT);
  ledTicker.attach(0.6, ledTick);
  
  // Instantiate and setup WiFiManager
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect(ACCESS_POINT_NAME, ACCESS_POINT_PASS)) {
    Serial.println("Failed to connect and hit timeout");
    ESP.reset();
    delay(1000);  
  }
  Serial.println("Connection Sucessful");
  ledTicker.detach();
  digitalWrite(BUILTIN_LED, LOW);

  // Configure Circuit Client library
  circuitClient = new CircuitClient(CIRCUIT_DOMAIN, BASE64_CREDENTIALS);
  circuitClient->setConversationId(CIRCUIT_CONV_ID);
  circuitClient->setOnNewTextItemCallBack(onNewTextItemCB);

  // Post message to Circuit conversation
  circuitClient->postTextMessage("Hello World!");

  // Show setup finished and app is ready
  lcd.clear();
  lcd.backlight();
  lcd.print("Ready!");

  // Setup GPIO for buttong
  pinMode(BUTTON_GPIO, INPUT);

  // Initialize DTH11 Sensor
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht11DelayMS = sensor.min_delay / 1000;
  Serial.print(F("Sensor Minimun Delay:")); Serial.print(dht11DelayMS); Serial.println(F(" MS"));
  dhtTicker.attach(dht11DelayMS / 1000, setUpdateDht11);
  buttonTicker.attach(0.2, changeCheckButton);
}

void loop() {
  // Allow Circuit client to run
  circuitClient->run();

  // Check temperature
  if (updateDht11) {
    processDhtInfo();
  }

  // Check button
  if (checkButton) {
    int newButtonState = digitalRead(BUTTON_GPIO);
    if (buttonState != newButtonState) {
      Serial.println("Button State Changed");
      buttonState = newButtonState;
      if (buttonState == 1) {
        //Report temperature
        char temp[100];
        sprintf(temp, "On Demand Repot: Temperature %d, Humidity %d", static_cast<int>(currentTemperature), static_cast<int>(currentHumidity));
        circuitClient->postTextMessage(temp);
      }
    }
  }
}
