/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#include "CircuitClient.h"
#include "ArduinoJson.h"

HTTPClient http;
ESP8266WebServer server(atoi(WEBSERVER_PORT));

/**
 * Helper Functions
 **/
void _debug(char *text) {
    #ifdef DEBUG_CIRCUIT_CLIENT
        DEBUG_OUTPUT.println(text);
    #endif
}

void _debug(String text) {
    #ifdef DEBUG_CIRCUIT_CLIENT
        DEBUG_OUTPUT.println(text);
    #endif
}

void _debug(int text) {
    #ifdef DEBUG_CIRCUIT_CLIENT
        DEBUG_OUTPUT.println(text);
    #endif
}

#ifndef CIRCUIT_DOMAIN
#define CIRCUIT_DOMAIN SANDBOX_URL
#define CIRCUIT_DOMAIN_FINGERPRINT SANDBOX_FINGERPRINT
#endif


/**
 * Circuit Client
 **/
CircuitClient::CircuitClient(char* credentials, String convId)
: CircuitClient(CIRCUIT_DOMAIN, credentials, convId) {};

CircuitClient::CircuitClient(String domain, char* credentials, String convId) 
: _domain(domain)
, _credentials(credentials)
, _convId(convId)
, _server_started(false) {
    strcpy(_userId,"\0");
    _deleteAllWebHooks();
    _debug("HTTP server configured on port:");
    _debug(WEBSERVER_PORT);
    _getUserProfile();
}

void CircuitClient::setConversationId(String convId) {
    _convId = convId;
}

int CircuitClient::postTextMessage(String textMessage) {
    if (strlen(_convId.c_str()) == 0) {
        _debug("Cannot post message without conversation id");
        return -1;
    }
    _debug("Posting text message to circuit conversation...");
    String url = _getConversationUrl() + MESSAGES_ENDPOINT_URL;
    String content = String("{\"content\":\"") + textMessage + "\"}";
    _debug(url);
    _debug(content);
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    int httpCode = http.POST(content);
    _debug(httpCode);
    http.end();
    return httpCode;
}

void CircuitClient::setOnNewTextItemCallBack(void(*callback)(String)) {
    _onNewTextItemCB = callback;
    if (!_server_started) {
        _startServer();
        server.on("/newTextItem", std::bind(&CircuitClient::_handleNewTextItem, this));
    }
    // Register webhook
    http.begin(_getWebHooksUrl(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    String content = String("{\"url\":\"") + MY_WEBHOOKS_URL + ":" + WEBSERVER_PORT + "/newTextItem\",\"filter\":[\"CONVERSATION.ADD_ITEM\"]}";
    _debug(content);
    int httpCode = http.POST(content);
    _debug("Webhook for new text item ended with code: ");
    _debug(httpCode);
    http.end();
    _getAllWebhooks();
}

void CircuitClient::run() {
    server.handleClient();
}

void CircuitClient::_getUserProfile() {
    http.begin(_getUserProfileUrl(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    int httpCode = http.GET();
    _debug("Getting User Profile ended with code: ");
    _debug(httpCode);
    String payload = http.getString();
    _debug(payload);
    StaticJsonDocument<1000> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        _debug("Error deserializing message body");
        _debug(error.c_str());
        return;
    } else {
        strncpy(_userId, doc["userId"], UUID_LENGHT-1);
        if (!_userId) {
            _debug("UserId not found in profile");
            return;
        }
        _debug("User Id: ");
        _debug(_userId);
    }
    http.end();
}

void CircuitClient::_deleteAllWebHooks() {
    _debug("Delete all circuit webhooks");
    http.begin(_getWebHooksUrl(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    int httpCode = http.sendRequest("DELETE");
    _debug("Deleting all webhooks ended with code: ");
    _debug(httpCode);
    http.end();
}

void CircuitClient::_handleNotFound() {
    _debug(server.uri());
    _debug("Request not handled");
    server.send(404);
}

void CircuitClient::_handleNewTextItem(void) {
    if (server.hasArg("plain")) {
        _debug(server.arg("plain"));
        StaticJsonDocument<2000> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (error) {
            _debug("Error deserializing message body");
            _debug(error.c_str());
            server.send(500, "Error deserializing json");
            return;
        } else {
            const char* newText = doc["item"]["text"]["content"];
            if (!newText) {
                server.send(400, "No text found in body");
                return;
            }
            const char* convId = doc["item"]["convId"];
            // Does the text item belong to the configured conversation?
            if (strncmp(_convId.c_str(), convId, strlen(_convId.c_str())) == 0) {
                // Has the text item been created by other than me?
                const char *creatorId = doc["item"]["creatorId"];
                if (strncmp(_userId, creatorId, strlen(_userId)) != 0) {
                    _debug("Received Text Item: ");
                    _debug(newText);
                    if (_onNewTextItemCB != NULL) {
                        _onNewTextItemCB(newText);
                    }
                }
            }
        }
    } else {
        _debug("No body on HTTP request for new text message");
        server.send(400, "No body on request");
        return;
    }
    server.send(200);
}

String CircuitClient::_getBaseUrl() {
    return "https://" + _domain + REST_API_VERSION_URL;
}

String CircuitClient::_getWebHooksUrl() {
    return String(_getBaseUrl() + CIRCUIT_WEBHOOKS_URL);
}

String CircuitClient::_getConversationUrl() {
    return String(_getBaseUrl() + CONV_ENDPOINT_URL + _convId);
}

String CircuitClient::_getUserProfileUrl() {
    return String(_getBaseUrl() + USER_PROFILE_ENDPOINT_URL);
}

void CircuitClient::_startServer() {
    server.begin();
    server.onNotFound(std::bind(&CircuitClient::_handleNotFound, this));
    _debug("HTTP Server started...");
    _server_started = true;
}

void CircuitClient::_getAllWebhooks() {
    _debug("Get all circuit webhooks");
    http.begin(_getWebHooksUrl(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    int httpCode = http.GET();
    _debug("Get all webhooks ended with code: ");
    _debug(httpCode);
    String payload = http.getString();
    _debug(payload);
    http.end();
}