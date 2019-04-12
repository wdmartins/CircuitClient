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
        DEBUG_OUTPUT.print("CIRCUIT:"); DEBUG_OUTPUT.println(text);
    #endif
}

void _debug(string text) {
    #ifdef DEBUG_CIRCUIT_CLIENT
        DEBUG_OUTPUT.print("CIRCUIT:"); DEBUG_OUTPUT.println(text.c_str());
    #endif
}

// void _debug(String text) {
//     #ifdef DEBUG_CIRCUIT_CLIENT
//         DEBUG_OUTPUT.print("CIRCUIT:"); DEBUG_OUTPUT.println(text);
//     #endif
// }

void _debug(int text) {
    #ifdef DEBUG_CIRCUIT_CLIENT
        DEBUG_OUTPUT.print("CIRCUIT:"); DEBUG_OUTPUT.println(text);
    #endif
}

#ifndef CIRCUIT_DOMAIN
#define CIRCUIT_DOMAIN SANDBOX_URL
#define CIRCUIT_DOMAIN_FINGERPRINT SANDBOX_FINGERPRINT
#endif


/**
 * Circuit Client
 **/
CircuitClient::CircuitClient(string credentials, string convId)
: CircuitClient(CIRCUIT_DOMAIN, credentials, convId) {};

CircuitClient::CircuitClient(string domain, string credentials, string convId) 
: _domain(domain)
, _credentials(credentials)
, _convId(convId)
, _server_started(false) {}

void CircuitClient::init() {
    strcpy(_userId,"\0");
    _deleteAllWebHooks();
    _debug("HTTP server configured on port:");
    _debug(WEBSERVER_PORT);
    _getUserProfile();
}
void CircuitClient::setConversationId(string convId) {
    _convId = convId;
}

int CircuitClient::postTextMessage(string textMessage) {
    if (strlen(_convId.c_str()) == 0) {
        _debug("Cannot post message without conversation id");
        return -1;
    }
    _debug("Posting text message to circuit conversation...");
    string url = _getConversationUrl() += MESSAGES_ENDPOINT_URL;
    string content = "{\"content\":\"";
    content += textMessage;
    content += "\"}";
    _debug(url);
    _debug(content);
    http.begin(url.c_str(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials.c_str());
    int httpCode = http.POST(content.c_str());
    _debug(httpCode);
    http.end();
    return httpCode;
}

void CircuitClient::setOnNewTextItemCallBack(void(*callback)(String)) {
    _onNewTextItemCB = callback;
    if (!_server_started) {
        _startServer();
    }
    server.on("/newTextItem", std::bind(&CircuitClient::_handleNewTextItem, this));
    // Register webhook
    http.begin(_getWebHooksUrl().c_str(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials.c_str());
    string content = "{\"url\":\"";
    content += MY_WEBHOOKS_URL;
    content += ":";
    content += WEBSERVER_PORT;
    content += "/newTextItem\",\"filter\":[\"CONVERSATION.ADD_ITEM\"]}";
    _debug(content.c_str());
    int httpCode = http.POST(content.c_str());
    _debug("Webhook for new text item ended with code: ");
    _debug(httpCode);
    http.end();
}

const char *CircuitClient::getUserPresence(char *userId) {
    http.begin(_getUserPresenceUrl(userId).c_str(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials.c_str());
    int httpCode = http.GET();
    _debug("Getting User Presence ended with code: ");
    _debug(httpCode);
    string payload = http.getString().c_str();
    _debug(payload);
    http.end();
    StaticJsonDocument<1000> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        _debug("Error deserializing message body");
        _debug(error.c_str());
        return "Unknown";
    }
    return doc[0]["state"];
}

void CircuitClient::setOnUserPresenceChange(char* userId, void (*callback)(String) ) {
    _onUserPresenceChangeCB = callback;
    if (!_server_started) {
        _startServer();
    }
    server.on("/userpresencechange", std::bind(&CircuitClient::_handleUserPresenceChange, this));
    // Register webhook
    http.begin(_getPresenceWebHooksUrl().c_str(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials.c_str());
    string content = "{\"url\":\"";
    content += MY_WEBHOOKS_URL; content += ":"; content += WEBSERVER_PORT;
    content += "/userpresencechange\",\"userIds\":[\""; content += userId; content += "\"]}";
    _debug(content);
    int httpCode = http.POST(content.c_str());
    _debug("Webhook for user presence change ended with code: ");
    _debug(httpCode);
    http.end();
    _getAllWebhooks();
}

void CircuitClient::run() {
    server.handleClient();
}

void CircuitClient::_getUserProfile() {
    http.begin(_getUserProfileUrl().c_str(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials.c_str());
    int httpCode = http.GET();
    _debug("Getting User Profile ended with code: ");
    _debug(httpCode);
    string payload = http.getString().c_str();
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
    http.begin(_getWebHooksUrl().c_str(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials.c_str());
    int httpCode = http.sendRequest("DELETE");
    _debug("Deleting all webhooks ended with code: ");
    _debug(httpCode);
    http.end();
}

void CircuitClient::_handleNotFound() {
    _debug(server.uri().c_str());
    _debug("Request not handled");
    server.send(404);
}

void CircuitClient::_handleNewTextItem(void) {
    if (server.hasArg("plain")) {
        _debug(server.arg("plain").c_str());
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
            _debug("Text is for conversation id: ");
            _debug(convId);
            _debug("Configured ConvId: ");
            _debug(_convId.c_str());
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
            } else {
                _debug("Text is not for the configured conversation.");
            }
        }
    } else {
        _debug("No body on HTTP request for new text message");
        server.send(400, "No body on request");
        return;
    }
    server.send(200);
}

void CircuitClient::_handleUserPresenceChange(void) {
    _debug("Handling user presence change");
    if (server.hasArg("plain")) {
        _debug(server.arg("plain").c_str());
        StaticJsonDocument<2000> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (error) {
            _debug("Error deserializing message body");
            _debug(error.c_str());
            server.send(500, "Error deserializing json");
            return;
        } else {
            const char* state = doc["presenceState"]["state"];
            _debug(state);
            if (!state) {
                server.send(400, "No state found in body");
                return;
            }
            _debug(state);
            if (_onUserPresenceChangeCB != NULL) {
                _onUserPresenceChangeCB(state);
            }
        }
    } else {
        _debug("No body on HTTP request for new text message");
        server.send(400, "No body on request");
        return;
    }
    server.send(200);
}

string CircuitClient::_getBaseUrl() {
    string url("https://");
    url += _domain;
    url += REST_API_VERSION_URL;
    return url;
}

string CircuitClient::_getWebHooksUrl() {
    string url(_getBaseUrl());
    return url += CIRCUIT_WEBHOOKS_URL;
}
string CircuitClient::_getPresenceWebHooksUrl() {
    string url(_getBaseUrl());
    url += CIRCUIT_WEBHOOKS_URL;
    return url += USER_PRESENCE_WEBHOOK_URL;
}

string CircuitClient::_getConversationUrl() {
    string url(_getBaseUrl());
    url += CONV_ENDPOINT_URL;
    return url += _convId;
}

string CircuitClient::_getUserProfileUrl() {
    string url(_getBaseUrl());
    return url += USER_PROFILE_ENDPOINT_URL;
}

string CircuitClient::_getUserPresenceUrl(char *userId) {
    string url(_getBaseUrl());
    url += USER_PRESENCE_ENDPOINT_URL;
    url += "?userIds=";
    return url += userId;
}

void CircuitClient::_startServer() {
    server.begin();
    server.onNotFound(std::bind(&CircuitClient::_handleNotFound, this));
    _debug("HTTP Server started...");
    _server_started = true;
}

void CircuitClient::_getAllWebhooks() {
    _debug("Get all circuit webhooks");
    http.begin(_getWebHooksUrl().c_str(), CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials.c_str());
    int httpCode = http.GET();
    _debug("Get all webhooks ended with code: ");
    _debug(httpCode);
    String payload = http.getString();
    _debug(payload.c_str());
    http.end();
}
