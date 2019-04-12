/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#ifndef CIRCUIT_CLIENT_H
#define CIRCUIT_CLIENT_H

#include "Arduino.h"
#include <CircuitConfig.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

using namespace std;

// Circuit Domains Definitions
#define SANDBOX_URL "circuitsandbox.net"
#define SANDBOX_FINGERPRINT "E2:D8:96:33:4F:F7:A3:66:AF:EF:A2:04:11:9C:39:D8:D6:DD:DA:95"

#define REST_API_VERSION_URL "/rest/v2"
#define CONV_ENDPOINT_URL "/conversations/"
#define MESSAGES_ENDPOINT_URL "/messages"
#define CIRCUIT_WEBHOOKS_URL "/webhooks"
#define USER_PROFILE_ENDPOINT_URL "/users/profile"
#define USER_PRESENCE_ENDPOINT_URL "/users/presence"
#define USER_PRESENCE_WEBHOOK_URL "/presence"
#define UUID_LENGHT 61

#define DEBUG_CIRCUIT_CLIENT
#define DEBUG_OUTPUT Serial

#define PRESENCE_BUSY "BUSY"
#define PRESENCE_AVAILABLE "AVAILABLE"
#define PRESENCE_AWAY "AWAY"
#define PRESENCE_DND "DND"

typedef void (*fptr)(String);

class CircuitClient {
  public:
    CircuitClient(string domain, string credentials, string convId);
    CircuitClient(string credentials, string convId);
    
    void init(void);
    void setConversationId(string convId);
    int postTextMessage(string text);
    void setOnNewTextItemCallBack( void (*func)(String) );
    const char *getUserPresence(char* userId);
    void setOnUserPresenceChange(char* userId, void (*func)(String) );
    void run();

  protected:
    void (*_onNewTextItemCB)(String);
    void (*_onUserPresenceChangeCB)(String);
    string _domain;
    string _credentials;
    char _userId[UUID_LENGHT];
    string _convId;
    bool _server_started;
    void _deleteAllWebHooks(void);
    void _getAllWebhooks(void);
    string _getBaseUrl(void);
    string _getConversationUrl(void);
    string _getPresenceWebHooksUrl(void);
    void _getUserProfile(void);
    string _getUserPresenceUrl(char *);
    string _getUserProfileUrl(void);
    string _getWebHooksUrl(void);
    void _handleNewTextItem(void);
    void _handleNotFound(void);
    void _handleUserPresenceChange(void);
    void _init(string domain, string credentials, string convId);
    void _startServer(void);
};

#endif //CIRCUIT_CLIENT_H
