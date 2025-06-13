#ifndef AVISHA_OTA_H
#define AVISHA_OTA_H

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

class AViShaOTA {
private:
    WebServer* server;
    String hostname;
    String otaPassword;
    int serverPort;
    bool mdnsEnabled;
    bool serialDebug;
    
    // Callback functions
    void (*onStartCallback)();
    void (*onEndCallback)();
    void (*onProgressCallback)(unsigned int progress, unsigned int total);
    void (*onErrorCallback)(ota_error_t error);
    void (*onWiFiConnectedCallback)();
    void (*onWiFiDisconnectedCallback)();
    
    // Internal methods
    void handleRoot();
    void handleUpdate();
    void setupWebServer();
    void setupArduinoOTA();
    static void wifiEventHandler(WiFiEvent_t event);
    
    // Static instance for WiFi event handler
    static AViShaOTA* instance;
    
    // HTML template
    const char* getUploadHTML();

public:
    // Constructor
    AViShaOTA(const String& hostname = "AViShaOTA_ESP32", int port = 80);
    
    // Destructor
    ~AViShaOTA();
    
    // Configuration methods
    void setOTAPassword(const String& password);
    void setHostname(const String& name);
    void enableMDNS(bool enable = true);
    void enableSerialDebug(bool enable = true);
    
    // Callback setters
    void onStart(void (*callback)());
    void onEnd(void (*callback)());
    void onProgress(void (*callback)(unsigned int progress, unsigned int total));
    void onError(void (*callback)(ota_error_t error));
    void onWiFiConnected(void (*callback)());
    void onWiFiDisconnected(void (*callback)());
    
    // Main methods
    bool begin(const char* ssid, const char* password);
    void handle();
    
    // Utility methods
    String getLocalIP();
    String getUploadURL();
    bool isConnected();
    void restart();
    
    // Static methods for callbacks
    void handleWiFiEvent(WiFiEvent_t event);
};

#endif
