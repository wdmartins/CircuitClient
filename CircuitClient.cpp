/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#include "CircuitClient.h"
#include "ArduinoJson.h"

HTTPClient http;
ESP8266WebServer server(WEBSERVER_PORT);
CircuitClient *client;

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

void handleNotFound() {
    _debug(server.uri());
    _debug("Request not handled");
}

void onNewTextItem(void) {
    if (server.hasArg("plain")) {
        _debug(server.arg("plain"));
        StaticJsonDocument<2000> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (error) {
            _debug("Error deserializing message body");
            _debug(error.c_str());
            _debug(server.arg("plain"));
            server.send(500, "Error deserializing json");
            return;
        } else {
            const char* newText = doc["item"]["text"]["content"];
            if (!newText) {
                server.send(400, "No text found in body");
                return;
            }
            const char* convId = doc["item"]["convId"];
            if (strncmp(client->getConversationId(), convId, strlen(client->getConversationId())) == 0) {
                _debug("Received Text Item: ");
                _debug(newText);
                // TODO: Invoke callbaclk
            }
        }
    } else {
        _debug("No body on HTTP request for new text message");
        server.send(400, "No body on request");
        return;
    }
    server.send(200);
}

CircuitClient::CircuitClient(String domain, char* credentials)
: _domain(domain)
, _credentials(credentials)
, _server_started(false) {
    _deleteAllWebHooks();
    _debug("HTTP server configured on port:");
    _debug(WEBSERVER_PORT);
    client = this;
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
    String url = "https://" + _domain + REST_API_VERSION_URL + CONV_ENDPOINT_URL +
        _convId + MESSAGES_ENDPOINT_URL;
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

void CircuitClient::_deleteAllWebHooks() {
    _debug("Delete all circuit webhooks");
    String url = "https://" + _domain + REST_API_VERSION_URL + CIRCUIT_WEBHOOKS_URL;
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    int httpCode = http.sendRequest("DELETE");
    _debug("Deleting all webhooks ended with code: ");
    _debug(httpCode);
    http.end();
}

void CircuitClient::setOnNewTextItemCallBack(void(*callback)(String)) {
    _onNewTextItemCB = callback;
    if (!_server_started) {
        _startServer();
        server.on("/newTextItem", onNewTextItem);
    }
    // Register webhook
    String url = "https://" + _domain + REST_API_VERSION_URL + CIRCUIT_WEBHOOKS_URL;
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    String content = String("{\"url\":\"") + MY_WEBHOOKS_URL + ":80" + "/newTextItem\",\"filter\":[\"CONVERSATION.ADD_ITEM\"]}";
    _debug(content);
    int httpCode = http.POST(content);
    _debug("Webhook for new text item ended with code: ");
    _debug(httpCode);
    http.end();
    _getAllWebhooks();
}

void CircuitClient::_startServer() {
    server.begin();
    server.onNotFound(handleNotFound);
    _debug("HTTP Server started...");
    _server_started = true;
}

void CircuitClient::run() {
    server.handleClient();
}

void CircuitClient::_getAllWebhooks() {
    _debug("Get all circuit webhooks");
    String url = "https://" + _domain + REST_API_VERSION_URL + CIRCUIT_WEBHOOKS_URL;
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(_credentials);
    int httpCode = http.GET();
    _debug("Get all webhooks ended with code: ");
    _debug(httpCode);
    String payload = http.getString();
    _debug(payload);
    http.end();
}

const char *CircuitClient::getConversationId() {
    return _convId.c_str();
}

