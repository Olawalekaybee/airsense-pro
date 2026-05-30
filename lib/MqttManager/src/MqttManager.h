#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

using MqttCallback = std::function<void(const char* topic, const uint8_t* payload, unsigned int len)>;

class MqttManager {
public:
    MqttManager();

    void begin(const char* clientId, MqttCallback cb);
    void loop();
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool subscribe(const char* topic);
    bool connected();   // NOT const — PubSubClient::connected() is non-const

private:
    WiFiClientSecure _wifiClient;
    PubSubClient     _client;
    MqttCallback     _userCallback;
    const char*      _clientId = nullptr;
    unsigned long    _lastReconnectAttempt = 0;

    void _connect();
    static void _staticCallback(char* topic, uint8_t* payload, unsigned int len);
    static MqttManager* _instance;
};