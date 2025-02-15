#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "RTCTimeManager.h"

class WiFiManager {
public:
  enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    AP_MODE
  };

  WiFiManager() : server(80) {}

  void init(RTCTimeManager* timeMgr = nullptr) {
    this->timeManager = timeMgr;
    loadCredentials();
    beginConnection();
  }

  void handleClient() {
    server.handleClient();
    
    if(state == WiFiState::CONNECTING && millis() - lastCheck > 10000) {
      checkConnection();
    }
  }

  WiFiState getState() const {
    return state;
  }

  String getIP() const {
    return WiFi.localIP().toString();
  }

private:
  WebServer server;
  RTCTimeManager* timeManager = nullptr;
  Preferences prefs;
  WiFiState state = WiFiState::DISCONNECTED;
  unsigned long lastCheck = 0;
  String apSSID = "SmartPlug_Config";
  String apPass = "configure123";
  String storedSSID;
  String storedPass;

  void loadCredentials() {
    prefs.begin("wifi", true);
    storedSSID = prefs.getString("ssid", "");
    storedPass = prefs.getString("pass", "");
    prefs.end();
  }

  void saveCredentials(const String& ssid, const String& pass) {
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
  }

  void beginConnection() {
    if(storedSSID.length() > 0) {
      connectToWiFi(storedSSID.c_str(), storedPass.c_str());
    } else {
      startAPMode();
    }
  }

  void connectToWiFi(const char* ssid, const char* pass) {
    WiFi.begin(ssid, pass);
    state = WiFiState::CONNECTING;
    lastCheck = millis();
    
    server.on("/", std::bind(&WiFiManager::handleRoot, this));
    server.on("/config", HTTP_GET, std::bind(&WiFiManager::handleConfig, this));
    server.on("/schedule", HTTP_POST, std::bind(&WiFiManager::handleSchedule, this));
    server.begin();
  }

  void startAPMode() {
    WiFi.softAP(apSSID.c_str(), apPass.c_str());
    state = WiFiState::AP_MODE;
    
    server.on("/", std::bind(&WiFiManager::handleAPRoot, this));
    server.on("/save", HTTP_GET, std::bind(&WiFiManager::handleAPSave, this));
    server.begin();
  }

  void checkConnection() {
    if(WiFi.status() == WL_CONNECTED) {
      state = WiFiState::CONNECTED;
      if(timeManager && timeManager->needsTimeSync()) {
        timeManager->syncTime();
      }
    } else {
      WiFi.reconnect();
    }
    lastCheck = millis();
  }

  // HTTP Handlers
  void handleRoot() {
    server.send(200, "text/html", 
      "<h1>Smart Plug</h1>"
      "<a href='/config'>Settings</a>");
  }

  void handleConfig() {
    server.send(200, "text/html",
      "<form action='/save'>"
      "SSID: <input name='ssid'><br>"
      "Password: <input name='pass'><br>"
      "<input type='submit'></form>");
  }

  void handleAPRoot() {
    server.send(200, "text/html",
      "<h1>WiFi Setup</h1>"
      "<form action='/save'>"
      "SSID: <input name='ssid'><br>"
      "Password: <input name='pass'><br>"
      "<input type='submit'></form>");
  }

  void handleAPSave() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    
    if(ssid.length() > 0) {
      saveCredentials(ssid, pass);
      server.send(200, "text/plain", "Settings saved. Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }

  void handleSchedule() {
    // Обработка расписания из веб-интерфейса
    server.send(200, "text/plain", "Schedule updated");
  }
};