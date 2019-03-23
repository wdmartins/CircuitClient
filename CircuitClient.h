/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#ifndef CircuitClient_h
#define CircuitClient_h

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <CircuitConfig.h>

#define REST_API_VERSION_URL "/rest/v2"
#define CONV_ENDPOINT_URL "/conversations/"
#define MESSAGES_ENDPOINT_URL "/messages"
#define CIRCUIT_WEBHOOKS_URL "/webhooks"

class CircuitClient {
  public:
    CircuitClient(String domain, char* credentials);
    void setConversationId(String convId);
    int postTextMessage(String text);
    void setOnNewTextItemCallBack( void (*func)(String) );
    void run();
  private:
    void deleteAllWebHooks(void);
    void (*onNewTextItemCB)(String);
    String domain;
    char * credentials;
    String convId;
    bool server_started;
    void startServer(void);
    void getAllWebhooks(void);
};

#endif
