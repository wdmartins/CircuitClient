#include <Ticker.h>
#include "Secret.h"
#include <CircuitClient.h>
#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
  #include "c_types.h"
  #include "osapi.h"  
  #include <wpa2_enterprise.h>
}

typedef enum {
    EAP_TLS,
    EAP_PEAP,
    EAP_TTLS,
} eap_method_t;

// Intervals
Ticker ledTicker;

// Circuit Client declaration
CircuitClient *circuitClient;

void ledTick() {
  int state = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !state);
}

void user_set_station_config(void)
{
    char ssid[32] = WPA2_SSID;
    char password[64] = WPA2_PASSWORD;
    struct station_config sta_conf = { 0 };

    os_memcpy(sta_conf.ssid, ssid, 32);
    os_memcpy(sta_conf.password, password, 64);
    wifi_station_set_config(&sta_conf);
}
void user_set_wpa2_config(void)
{
    eap_method_t method = EAP_TLS;
    char *identity = WPA2_IDENTITY;
    char *username = WPA2_USERNAME;
    char *password = WPA2_PASSWORD;

    wifi_station_set_wpa2_enterprise_auth(1);

    //wifi_station_set_enterprise_identity(identity, os_strlen(identity));//This is an option. If not call this API, the outer identity will be "anonymous@espressif.com".

    if (method == EAP_TLS) {
        //wifi_station_set_enterprise_cert_key(client_cert, os_strlen(client_cert)+1, client_key, os_strlen(client_key)+1, NULL, 1);
        //wifi_station_set_enterprise_username(username, os_strlen(username));//This is an option for EAP_PEAP and EAP_TLS.
    }
    else if (method == EAP_PEAP || method == EAP_TTLS) {
        wifi_station_set_enterprise_username(username, os_strlen(username));
        wifi_station_set_enterprise_password(password, os_strlen(password));
        //wifi_station_set_enterprise_ca_cert(ca, os_strlen(ca)+1);//This is an option for EAP_PEAP and EAP_TTLS.
    }
}

void setup() {
  Serial.begin(115200);

  // Setup built in LED
  pinMode(BUILTIN_LED, OUTPUT);
  ledTicker.attach(0.6, ledTick);
  
  delay(100);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WPA2_SSID);

  wifi_set_opmode(STATION_MODE);
  user_set_station_config();
  user_set_wpa2_config();
  wifi_station_connect();

  int wifiStatus = WiFi.status();
  while (wifiStatus != WL_CONNECTED) {
    wifiStatus = WiFi.status();
    delay(1000);
    Serial.print("WiFi Satus: "); Serial.println(wifiStatus);
    Serial.print("Wifi ConSt: "); Serial.println(wifi_station_get_connect_status());
  }

  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  wifi_station_clear_enterprise_password();
  wifi_station_clear_enterprise_username();
  ledTicker.attach(0.2, ledTick);

  // Configure Circuit Client library
  circuitClient = new CircuitClient(CIRCUIT_DOMAIN, BASE64_CREDENTIALS);
  circuitClient->setConversationId(CIRCUIT_CONV_ID);

  // Post message to Circuit conversation
  circuitClient->postTextMessage("Hello World!");

}

void loop() {
  // Allow Circuit client to run
  circuitClient->run();
}
