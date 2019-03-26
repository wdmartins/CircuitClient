#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <CircuitClient.h>
#include <Ticker.h>

// Access point to configure Wi-Fi
#define ACCESS_POINT_NAME "ESP8266"
#define ACCESS_POINT_PASS "esp8266"

Ticker ticker;
// Circuit Client declaration
CircuitClient *circuitClient;

// Setup Liquid crystal display
LiquidCrystal_I2C lcd(0x27, 16 ,2);

void tick() {
  int state = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !state);
}

// Wi-Fi Configuration mode callback
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick);
}

// Circuit new text item callback
void onNewTextItemCB (String text) {
  Serial.println("Text Received: ");
  Serial.println(text);
  lcd.clear();
  lcd.print(text.c_str());
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
  ticker.attach(0.6, tick);
  
  // Instantiate and setup WiFiManager
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect(ACCESS_POINT_NAME, ACCESS_POINT_PASS)) {
    Serial.println("Failed to connect and hit timeout");
    ESP.reset();
    delay(1000);  
  }
  Serial.println("Connection Sucessful");
  ticker.detach();
  digitalWrite(BUILTIN_LED, LOW);

  // Configure Circuit Client library
  circuitClient = new CircuitClient(CIRCUIT_DOMAIN, BASE64_CREDENTIALS);
  circuitClient->setConversationId(CIRCUIT_CONV_ID);
  circuitClient->postTextMessage("Hello World!");
  circuitClient->setOnNewTextItemCallBack(onNewTextItemCB);

  // Show setup finished and app is ready
  lcd.clear();
  lcd.backlight();
  lcd.print("Ready!");

  
}

void loop() {
  // Allow Circuit client to run
  circuitClient->run();
}
