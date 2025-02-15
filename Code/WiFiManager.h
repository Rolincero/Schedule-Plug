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
  : server(80), timeManager(tm), scheduleManager(sm) {}
  
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
  
  private:
  WebServer server;
  RTCTimeManager& timeManager;
  ScheduleManager& scheduleManager;
  Preferences prefs;
  WiFiState state = WiFiState::DISCONNECTED;
  unsigned long lastCheck = 0;
  String apSSID = "SmartPlug_Config";
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
    
    server.on("/", [this]() { handleRoot(); });
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
      if(timeManager.needsTimeSync()) {
	      timeManager.syncTime();
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
  if(server.method() == HTTP_POST) {
	bool success = true;
	
	for(int i = 0; i < 7; i++) {
	  String dayKey = String(i);
	  String startVal = server.arg("d" + dayKey + "s");
	  String endVal = server.arg("d" + dayKey + "e");
	  
	  if(!startVal.isEmpty() && !endVal.isEmpty()) {
		uint32_t start = startVal.toInt();
		uint32_t end = endVal.toInt();
		
		// Базовая валидация
		if(start > 86400 || end > 86400) {
		  success = false;
		  break;
		}
		
		scheduleManager->weeklySchedule[i].start = start;
		scheduleManager->weeklySchedule[i].end = end;
	  }
    scheduleManager.weeklySchedule[i].start = start; // Используем точку

    if(success) {
		scheduleManager.save();
		server.send(200, "text/plain", "Schedule updated");
	}

    void loadCredentials() {
	prefs.begin("wifi", true);
	storedSSID = prefs.getString("ssid", "");
	storedPass = prefs.getString("pass", "");
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
	
	server.on("/", [this]() { handleRoot(); });
	server.on("/config", HTTP_GET, [this]() { handleConfig(); });
	server.begin();
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
  }
  }
  }
  }
};