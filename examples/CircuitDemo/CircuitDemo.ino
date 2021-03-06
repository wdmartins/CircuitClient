#include <WiFiManager.h>
#include <Ticker.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal_I2C.h>
#include <CircuitClient.h>

using namespace circuit;
// Access point to configure Wi-Fi
#define ACCESS_POINT_NAME "ESP8266"
#define ACCESS_POINT_PASS "esp8266"

// Circuit Client Configuration.
#ifndef CIRCUIT_CONV_ID
#define CIRCUIT_CONV_ID "Add Circuit Conversation Id here"
#endif
#ifndef BASE64_CREDENTIALS
#define BASE64_CREDENTIALS "Add your b64 encoded circuit credentials"
#endif
// Circuit User Id to monitor presence
#define USER_ID "c6b8bea7-79b0-4263-9c06-00149ef2db35"


// GPIO Definitios
#define DHT11_GPIO 2
#define BUTTON_GPIO 14
#define LED_RED_GPIO 12
#define LED_BLUE_GPIO 13
#define LED_GREEN_GPIO 15

// Sensors Definitions
#define DHT_TYPE DHT11
#define DHT_TEMP_CHANGE_TRIGGER_CELSIUS 1
#define DHT_HUMI_CHANGE_TRIGGER_PERCENT 1

// Sensors Instantiation
DHT_Unified dht(DHT11_GPIO, DHT_TYPE);

// Colors 
#define LED_OFF -1
#define LED_RED 0
#define LED_GREEN 1
#define LED_BLUE 2

// Intervals
Ticker ledTicker;
Ticker dhtTicker;
Ticker buttonTicker;
Ticker textDisplayTicker;

// Circuit Client
CircuitClient circuitClient(BASE64_CREDENTIALS);

// Setup Liquid crystal display
LiquidCrystal_I2C lcd(0x27, 16 ,2);

// Global variables
uint32_t dht11DelayMS;
float currentTemperature = -300;
float currentHumidity= -1;
boolean updateDht11 = false;
boolean displayingText = false;
boolean checkButton = false;
int buttonState = 0;
int currentRGBLEDColor = -1;

// Builtin LED flashing
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

// Circuit text display timeout
void stopDisplaying() {
  textDisplayTicker.detach();
  displayingText = false;
  char tempText[17];
  displayTemp(tempText);
}

// Circuit new text item callback
void onNewTextItemCB (string text) {
  Serial.print("Text Received: "); Serial.println(text.c_str());
  displayingText = true;
  lcd.clear();
  lcd.print(text.c_str());
  textDisplayTicker.attach(5, stopDisplaying);
}

void onUserPresenceChangeCB(string userPresence) {
  Serial.print("User presecene is now: "); Serial.println(userPresence.c_str());
  showPresence(userPresence);
}
void setUpdateDht11() {
  updateDht11 = !updateDht11;
}

void changeCheckButton() {
  checkButton = !checkButton;
}

// Display temperature and humidity to LCD display
void displayTemp(char *tempText) {
  sprintf(tempText, "Temp %d, Hum %d", static_cast<int>(currentTemperature), static_cast<int>(currentHumidity));
  lcd.setCursor(0,1);
  lcd.clear();
  lcd.print(tempText);
}

// Query temperature sensor and process changes
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
    char tempText[17];
    displayTemp(tempText);
    circuitClient.postTextMessage(tempText);
  }
}

// Set RGB Led Color
int setLedColor(int color) {
  if (color == LED_RED) {
    digitalWrite(LED_GREEN_GPIO, HIGH);
    digitalWrite(LED_RED_GPIO, LOW);using namespace std;

    digitalWrite(LED_BLUE_GPIO, HIGH);
  } else if (color == LED_GREEN) {
    digitalWrite(LED_GREEN_GPIO, LOW);
    digitalWrite(LED_RED_GPIO, HIGH);
    digitalWrite(LED_BLUE_GPIO, HIGH);
  } else if (color == LED_BLUE) {
    digitalWrite(LED_GREEN_GPIO, HIGH);
    digitalWrite(LED_RED_GPIO, HIGH);
    digitalWrite(LED_BLUE_GPIO, LOW);
  } else {
    // Turn Off
    digitalWrite(LED_GREEN_GPIO, HIGH);
    digitalWrite(LED_RED_GPIO, HIGH);
    digitalWrite(LED_BLUE_GPIO, HIGH);
  }
  return color;
}

// Show Presence
void showPresence(string presence) {
  if (kPresenceAvailable.compare(presence) == 0) {
    setLedColor(LED_GREEN);
  } else if (kPresenceBusy.compare(presence) == 0) {
    setLedColor(LED_RED);
  } else {
    setLedColor(LED_BLUE);
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

  // Initialize Circuit Client
  circuitClient.init();
  circuitClient.setOnNewTextItemCallBack(onNewTextItemCB);
  circuitClient.setOnUserPresenceChange(USER_ID, onUserPresenceChangeCB);

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

  // Setup GPIOs for RGBLED
  pinMode(LED_RED_GPIO, OUTPUT);
  pinMode(LED_BLUE_GPIO, OUTPUT);
  pinMode(LED_GREEN_GPIO, OUTPUT);

  currentRGBLEDColor = setLedColor(LED_OFF);

  // Post message to Circuit conversation
  circuitClient.setConversationId(CIRCUIT_CONV_ID);
  circuitClient.postTextMessage("Hello World!");

  // Get User Presence
  const char* userPresence = circuitClient.getUserPresence(USER_ID);
  Serial.print("Initial User Presence: "); Serial.println(userPresence);
  showPresence(userPresence);
 
  // Show setup finished and app is ready
  lcd.clear();
  lcd.backlight();
  lcd.print("Ready!");

}

void loop() {
  // Allow Circuit client to run
  circuitClient.run();

  // Check temperature
  if (updateDht11 && !displayingText) {
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
        sprintf(temp, "On Demand Report: Temperature %d, Humidity %d", static_cast<int>(currentTemperature), static_cast<int>(currentHumidity));
        circuitClient.postTextMessage(temp);
      }
    }
  }
}
