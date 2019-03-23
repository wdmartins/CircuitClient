/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#include "CircuitClient.h"
HTTPClient http;
ESP8266WebServer server(WEBSERVER_PORT);

void handleNotFound() {
    Serial.println(server.uri());
    Serial.println("Request not handled");
}
void onNewTextItem(void);

CircuitClient::CircuitClient(String domain, char* credentials) {
    if (!domain || !credentials) {
        Serial.println("Error initializing Circuit Client\n");
        return;
    }
    this->credentials = credentials;
    this->domain = domain;
    this->server_started = false;
    this->deleteAllWebHooks();
    Serial.println("HTTP server configured on port:");
    Serial.println(WEBSERVER_PORT);
}

void CircuitClient::setConversationId(String convId) {
    this->convId = convId;
}

int CircuitClient::postTextMessage(String textMessage) {
    if (!this->convId) {
        Serial.println("Cannot post message without conversation id");
        return -1;
    }
    Serial.println("Posting text message to circuit conversation...");
    String url = "https://" + this->domain + REST_API_VERSION_URL + CONV_ENDPOINT_URL +
        this->convId + MESSAGES_ENDPOINT_URL;
    String content = String("{\"content\":\"") + textMessage + "\"}";
    Serial.println(url);
    Serial.println(content);
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(this->credentials);
    int httpCode = http.POST(content);
    Serial.println(httpCode);
    http.end();
    return httpCode;
}

void CircuitClient::deleteAllWebHooks() {
    Serial.println("Delete all circuit webhooks");
    String url = "https://" + this->domain + REST_API_VERSION_URL + CIRCUIT_WEBHOOKS_URL;
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(this->credentials);
    int httpCode = http.sendRequest("DELETE");
    Serial.println("Deleting all webhooks ended with code: ");
    Serial.println(httpCode);
    http.end();
}

void CircuitClient::setOnNewTextItemCallBack(void(*callback)(String)) {
    if (!this->convId) {
        Serial.println("Cannot register for text items without conversation...");
        return;
    }
    // if (this->onNewTextItemCB != NULL) {
    //     Serial.println("There is already a webhook for new text item");
    //     return;
    // }
    this->onNewTextItemCB = callback;
    if (!server_started) {
        this->startServer();
        server.on("/newTextItem", onNewTextItem);
    }
    // Register webhook
    String url = "https://" + this->domain + REST_API_VERSION_URL + CIRCUIT_WEBHOOKS_URL;
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(this->credentials);
    String content = String("{\"url\":\"") + MY_WEBHOOKS_URL + ":8000" + "/newTextItem\",\"filter\":[\"CONVERSATION.ADD_ITEM\"]}";
    Serial.println(content);
    int httpCode = http.POST(content);
    Serial.println("Webhook for new text item ended with code: ");
    Serial.println(httpCode);
    http.end();
    this->getAllWebhooks();
}

void onNewTextItem(void) {
    if (server.hasArg("json")) {
        Serial.println(server.arg("json"));
        // TODO: Invoke callbaclk
    } else {
        Serial.println("No body on HTTP request for new text message");
    }
    server.send(200);
}

void CircuitClient::startServer() {
    server.begin();
    server.onNotFound(handleNotFound);
    Serial.println("HTTP Server started...");
    this->server_started = true;
}

void CircuitClient::run() {
    server.handleClient();
}

void CircuitClient::getAllWebhooks() {
    Serial.println("Delete all circuit webhooks");
    String url = "https://" + this->domain + REST_API_VERSION_URL + CIRCUIT_WEBHOOKS_URL;
    http.begin(url, CIRCUIT_DOMAIN_FINGERPRINT);
    http.setAuthorization(this->credentials);
    int httpCode = http.GET();
    Serial.println("Get all webhooks ended with code: ");
    Serial.println(httpCode);
    String payload = http.getString();
    Serial.println(payload);
    http.end();
}

