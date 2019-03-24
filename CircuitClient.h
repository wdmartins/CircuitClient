/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#ifndef CIRCUIT_CLIENT_H
#define CIRCUIT_CLIENT_H

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
#define DEBUG_CIRCUIT_CLIENT
#define DEBUG_OUTPUT Serial

class CircuitClient {
  public:
    CircuitClient(String domain, char* credentials);
    
    void setConversationId(String convId);
    int postTextMessage(String text);
    void setOnNewTextItemCallBack( void (*func)(String) );
    void run();
    const char * getConversationId();

  protected:
    void _deleteAllWebHooks(void);
    void (*_onNewTextItemCB)(String);
    String _domain;
    char * _credentials;
    String _convId;
    bool _server_started;
    void _startServer(void);
    void _getAllWebhooks(void);
    void _handleNotFound(void);
    void _onNewTextItem(void);
};

#endif //CIRCUIT_CLIENT_H
