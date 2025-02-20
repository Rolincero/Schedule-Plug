#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "RTCTimeManager.h"
#include "ScheduleManager.h"

class WiFiManager {
public:
  enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    AP_MODE
  };
  
  WiFiManager(RTCTimeManager& tm, ScheduleManager& sm) 
  : server(80), timeManager(tm), scheduleManager(sm) {
    apSSID = "SmartPlug_" + String(ESP.getEfuseMac(), HEX);
  }
  
  void init() {
    loadCredentials();
    beginConnection();
  }
  
  void handleClient() {
    server.handleClient();
    if(state == WiFiState::CONNECTING && millis() - lastCheck > 10000) {
      checkConnection();
    }
  }
  
  void resetCredentials() {
    prefs.begin("wifi", false);
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
    WiFi.disconnect();
  }
  
  WiFiState getState() const {
    return state;
  }
  
  String getIP() const {
    return WiFi.localIP().toString();
  }

  String getAPSSID() const {
    return apSSID;
  }

  String getAPPassword() const {
    return apPass;
  }

  String getAPIP() const {
    return WiFi.softAPIP().toString();
  }
  
  void loadCredentials() {
    prefs.begin("wifi", true);
    storedSSID = prefs.getString("ssid", "");
    storedPass = prefs.getString("pass", "");
    prefs.end();
  }
  
private:
  WebServer server;
  RTCTimeManager& timeManager;
  ScheduleManager& scheduleManager;
  Preferences prefs;
  WiFiState state = WiFiState::DISCONNECTED;
  unsigned long lastCheck = 0;
  String apSSID;
  String apPass = "configure123";
  String storedSSID;
  String storedPass;
  
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
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      state = WiFiState::CONNECTED;
      server.on("/", [this]() { handleRoot(); });
      server.on("/config", HTTP_GET, [this]() { handleConfig(); });
      server.on("/schedule", HTTP_POST, [this]() { handleSchedule(); });
      server.begin();
      if(timeManager.needsTimeSync()) {
        timeManager.syncTime();
      }
    } else {
      Serial.println("Connection Failed!");
      startAPMode();
    }
  }
  
  void startAPMode() {
    WiFi.softAP(apSSID.c_str(), apPass.c_str());
    state = WiFiState::AP_MODE;
    
    server.on("/", [this]() { handleAPRoot(); });
    server.on("/save", HTTP_GET, [this]() { handleAPSave(); });
    server.begin();
  }
  
  void checkConnection() {
    if(WiFi.status() == WL_CONNECTED) {
      state = WiFiState::CONNECTED;
    } else {
      WiFi.reconnect();
    }
    lastCheck = millis();
  }
  
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
      "<p>Connect to AP:</p>"
      "<p><b>SSID:</b> " + apSSID + "</p>"
      "<p><b>Password:</b> " + apPass + "</p>"
      "<form action='/save'>"
      "Your WiFi SSID: <input name='ssid'><br>"
      "Password: <input name='pass'><br>"
      "<input type='submit' value='Save'>"
      "</form>");
  }
  
  void handleAPSave() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    
    if(ssid.length() > 0) {
      saveCredentials(ssid, pass);
      server.send(200, "text/plain", "Settings saved. Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "SSID cannot be empty");
    }
  }
  
  void handleSchedule() {
    if(server.method() == HTTP_POST) {
      bool success = true;
      
      for(int i = 0; i < 7; i++) {
        String dayKey = String(i);
        String startVal = server.arg("d" + dayKey + "s");
        String endVal = server.arg("d" + dayKey + "e");
        
        if(!startVal.isEmpty() && !endVal.isEmpty()) {
          uint32_t start = startVal.toInt();
          uint32_t end = endVal.toInt();
          
          if(start > 86400 || end > 86400) {
            success = false;
            break;
          }

          if(success) {
			      scheduleManager.save(); // Убедиться, что есть вызов save()
			      server.send(200, "text/plain", "Schedule updated");
		      }
          
          scheduleManager.weeklySchedule[i].start = start;
          scheduleManager.weeklySchedule[i].end = end;
        }
      }
      
      if(success) {
        scheduleManager.save();
        server.send(200, "text/plain", "Schedule updated");
      } else {
        server.send(400, "text/plain", "Invalid schedule data");
      }
    }
  }
};