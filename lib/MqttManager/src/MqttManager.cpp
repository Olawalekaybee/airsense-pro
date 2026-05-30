#include "MqttManager.h"
#include <Arduino.h>

// config.h lives in include/ — reference it explicitly
#include "../../../include/config.h"

MqttManager* MqttManager::_instance = nullptr;

MqttManager::MqttManager() : _client(_wifiClient) {}

void MqttManager::begin(const char* clientId, MqttCallback cb) {
    _clientId     = clientId;
    _userCallback = cb;
    _instance     = this;

    _wifiClient.setInsecure();   // TLS without cert validation (fine for hobby)

    _client.setServer(MQTT_HOST, MQTT_PORT);
    _client.setKeepAlive(60);
    _client.setBufferSize(512);
    _client.setCallback(_staticCallback);

    _connect();
}

void MqttManager::loop() {
    if (!_client.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 5000UL) {   // 5 s reconnect interval
            _lastReconnectAttempt = now;
            Serial.println("[MQTT] Reconnecting...");
            _connect();
        }
    }
    _client.loop();
}

bool MqttManager::publish(const char* topic, const char* payload, bool retain) {
    if (!_client.connected()) return false;
    bool ok = _client.publish(topic, payload, retain);
    if (!ok) Serial.printf("[MQTT] Publish failed on %s\n", topic);
    return ok;
}

bool MqttManager::subscribe(const char* topic) {
    return _client.subscribe(topic);
}

bool MqttManager::connected() {
    return _client.connected();
}

void MqttManager::_connect() {
    // LWT — broker publishes "offline" if connection drops unexpectedly
    bool ok = _client.connect(
        _clientId,
        MQTT_USER, MQTT_PASS,
        "airsense/room1/status", 1, true, "offline"
    );

    if (ok) {
        Serial.printf("[MQTT] Connected as %s\n", _clientId);
        _client.publish("airsense/room1/status", "online", true);
    } else {
        Serial.printf("[MQTT] Failed, rc=%d — retrying in 5 s\n", _client.state());
    }
}

void MqttManager::_staticCallback(char* topic, uint8_t* payload, unsigned int len) {
    if (_instance && _instance->_userCallback) {
        _instance->_userCallback(topic, payload, len);
    }
}