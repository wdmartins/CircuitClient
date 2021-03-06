#include <WiFiManager.h>
#include <Ticker.h>
#include <CircuitClient.h>

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

// Intervals
Ticker ledTicker;

// Circuit Client declaration
circuit::CircuitClient circuitClient(BASE64_CREDENTIALS);

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

void setup() {
  Serial.begin(115200);

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

  // Post message to Circuit conversation
  circuitClient.setConversationId(CIRCUIT_CONV_ID);
  circuitClient.postTextMessage("Hello World!");

}

void loop() {
  // Allow Circuit client to run
  circuitClient.run();
}
