/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#include "CircuitClient.h"
#include "ArduinoJson.h"

using namespace circuit;

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
    _urlBuilder = new UrlBuilder(_domain);
    _http = new HttpWrapper(BASE64_CREDENTIALS, true);
    //TODO: Add server fingerprint to CircuitClient constructor
    _http->setServerFingerprint(kSandBoxFingerprint);
    _http->setHttpClient(&http);
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
    if (_convId.length() == 0) {
        _debug("Cannot post message without conversation id");
        return -1;
    }
    _debug("Posting text message to circuit conversation...");
    string content = "{\"content\":\"" + textMessage + "\"}";
    return _http->POST(_urlBuilder->getMessagesUrl(_convId), content);
}

void CircuitClient::setOnNewTextItemCallBack(void(*callback)(string)) {
    _onNewTextItemCB = callback;
    if (!_server_started) {
        _startServer();
    }
    server.on("/newTextItem", std::bind(&CircuitClient::_handleNewTextItem, this));
    // Register webhook
    string content = "{\"url\":\"";
    content += MY_WEBHOOKS_URL;
    content += ":";
    content += WEBSERVER_PORT;
    content += "/newTextItem\",\"filter\":[\"CONVERSATION.ADD_ITEM\"]}";
    _http->POST(_urlBuilder->getWebHooksUrl(), content);
}

const char *CircuitClient::getUserPresence(char *userId) {
    _http->GET(_urlBuilder->getUserPresenceUrl(userId));
    string payload = _http->getPayload();
    StaticJsonDocument<1000> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        _debug("Error deserializing message body");
        _debug(error.c_str());
        return "Unknown";
    }
    return doc[0]["state"];
}

void CircuitClient::setOnUserPresenceChange(char* userId, void (*callback)(string) ) {
    _onUserPresenceChangeCB = callback;
    if (!_server_started) {
        _startServer();
    }
    server.on("/userpresencechange", std::bind(&CircuitClient::_handleUserPresenceChange, this));
    // Register webhook
    string content = "{\"url\":\"";
    content += MY_WEBHOOKS_URL; content += ":"; content += WEBSERVER_PORT;
    content += "/userpresencechange\",\"userIds\":[\""; content += userId; content += "\"]}";
    _http->POST(_urlBuilder->getPresenceWebHooksUrl(), content);
}

void CircuitClient::run() {
    server.handleClient();
}

void CircuitClient::_getUserProfile() {
    int httpCode = _http->GET(_urlBuilder->getUserProfileUrl());
    if (httpCode == 200) {
        string payload = _http->getPayload();
        StaticJsonDocument<1000> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            _debug("Error deserializing message body");
            _debug(error.c_str());
            return;
        } else {
            strncpy(_userId, doc["userId"], kUUIDLength-1);
            if (!_userId) {
                _debug("UserId not found in profile");
                return;
            }
            _debug("User Id: ");
            _debug(_userId);
        }
    }
}

void CircuitClient::_deleteAllWebHooks() {
    _debug("Delete all circuit webhooks");
    _http->DELETE(_urlBuilder->getWebHooksUrl());
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

void CircuitClient::_startServer() {
    server.begin();
    server.onNotFound(std::bind(&CircuitClient::_handleNotFound, this));
    _debug("HTTP Server started...");
    _server_started = true;
}

void CircuitClient::_getAllWebhooks() {
    // Method not exposed. For debugging purposes only 
    _debug("Get all circuit webhooks");
    _http->GET(_urlBuilder->getWebHooksUrl());
}

/*------------------------------------------------------------------------------------------*/
/* HTTP Wrapper                                                                             */
/*------------------------------------------------------------------------------------------*/
HttpWrapper::HttpWrapper(bool debug)
    :_debugEnabled(debug) {}

HttpWrapper::HttpWrapper(string b64EncodedCredentials, bool debug)
    :_credentials(b64EncodedCredentials),
    _debugEnabled(debug) {}

HttpWrapper::HttpWrapper(string username, string password, bool debug) {
    _debugEnabled = debug;
    //TODO: Encode credentials
    // _credentials = ....
}

void HttpWrapper::setHttpClient(HTTPClient *http) {
    _http = http;
}

void HttpWrapper::setCredentials(string b64EncodedCredentials) {
    _credentials = b64EncodedCredentials;
}

void HttpWrapper::setCredentials(string username, string password) {
    //TODO: Encode credentials
    // _credentials = ....
}

void HttpWrapper::setServerFingerprint(string fingerprint) {
    _serverFingerprint = fingerprint;
}

void HttpWrapper::setDebug() {
    _debugEnabled = true;
}

void HttpWrapper::resetDebug() {
    _debugEnabled = false;
}

int HttpWrapper::PUT(string url, string content) {
    //TODO
    return 501;
}

int HttpWrapper::POST(string url, string content) {
    string debugText;
    if (_debugEnabled) {
        debugText = "POST: url [" + url + "] content [" + content + "]";
        _debug(debugText);
    }
    _http->begin(url.c_str(), _serverFingerprint.c_str());
    _http->setAuthorization(_credentials.c_str());
    int httpCode = _http->POST(content.c_str());
    if (_debugEnabled) {
        debugText = "Result: HTTP ";
        _debug(debugText);
        _debug(httpCode);
    }
    _http->end();
    return httpCode;
}

int HttpWrapper::GET(string url) {
    string debugText;
    if (_debugEnabled) {
        debugText = "GET: url [" + url + "]";
        _debug(debugText);
    }
    _http->begin(url.c_str(), _serverFingerprint.c_str());
    _http->setAuthorization(_credentials.c_str());
    int httpCode = _http->GET();
    _lastPayload = _http->getString().c_str();
    if (_debugEnabled) {
        debugText = "Result: HTTP ";
        _debug(debugText);
        _debug(httpCode);
        debugText = "Payload: [" + _lastPayload + "]"; 
        _debug(debugText);
    }
    _http->end();
    return httpCode;
}

string HttpWrapper::getPayload() {
    return _lastPayload;
}

int HttpWrapper::DELETE(string url) {
    string debugText;
    if (_debugEnabled) {
        debugText = "DELETE: url [" + url + "]";
        _debug(debugText);
    }
    http.begin(url.c_str(), _serverFingerprint.c_str());
    http.setAuthorization(_credentials.c_str());
    int httpCode = http.sendRequest("DELETE");
    if (_debugEnabled) {
        debugText = "Result: HTTP ";
        _debug(debugText);
        _debug(httpCode);
    }
    http.end();
    return httpCode;
}
/*------------------------------------------------------------------------------------------*/
/* UrlBuilder                                                                               */
/*------------------------------------------------------------------------------------------*/
UrlBuilder::UrlBuilder(string domain, string protocol, string restApiVersion) {
    _baseUrl = protocol + "://" + domain + restApiVersion;
}

string UrlBuilder::getWebHooksUrl() {
    return _baseUrl + kCircuitWebHooksUrl;
}

string UrlBuilder::getPresenceWebHooksUrl() {
    return _baseUrl + kCircuitWebHooksUrl + kUserPresenceWebhookUrl;
}

string UrlBuilder::getUserProfileUrl() {
    return _baseUrl + kUserProfileEndpointUrl;
}

string UrlBuilder::getUserPresenceUrl(string userId) {
    return _baseUrl + kUserPresenceEndpointUrl + "?userIds=" + userId;
}

string UrlBuilder::getMessagesUrl(string convId) {
    return _getConversationUrl(convId) + kMessagesEndpointUrl;
}

string UrlBuilder::_getConversationUrl(string convId) {
    return _baseUrl + kConvEndpointUrl + convId;
}