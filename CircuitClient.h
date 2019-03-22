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
#include <CircuitConfig.h>

#define REST_API_VERSION_URL "/rest/v2"
#define CONV_ENDPOINT_URL "/conversations/"
#define MESSAGES_ENDPOINT_URL "/messages"

class CircuitClient {
  public:
    CircuitClient(String domain, char* credentials);
    void setConversationId(String convId);
    int postTextMessage(String text);

  private:
    String domain;
    char * credentials;
    String convId;
};

#endif
