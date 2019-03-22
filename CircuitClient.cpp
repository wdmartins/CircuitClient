/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#include "CircuitClient.h"
HTTPClient http;

CircuitClient::CircuitClient(String domain, char* credentials) {
    if (!domain || !credentials) {
        Serial.println("Error initializing Circuit Client\n");
        return;
    }
    this->credentials = credentials;
    this->domain = domain;
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
