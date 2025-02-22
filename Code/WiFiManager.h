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
    server.on("/", [this]() { handleRoot(); });
    server.on("/reconfigure", [this]() { handleReconfigure(); });
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
    state = WiFiState::DISCONNECTED; // Добавить эту строку
    startAPMode(); // Добавить переход в режим AP
  }
  
  void handleScheduleGet() {
	String json = "{";
		for(int i = 0; i < 7; i++) {
			json += "\"d" + String(i) + "s\":\"" + formatTime(scheduleManager.weeklySchedule[i].start) + "\",";
			json += "\"d" + String(i) + "e\":\"" + formatTime(scheduleManager.weeklySchedule[i].end) + "\"";
			if(i < 6) json += ",";
		}
		json += "}";
		server.send(200, "application/json", json);
  }

  String formatTime(uint32_t seconds) {
	  uint8_t hours = seconds / 3600;
	  uint8_t minutes = (seconds % 3600) / 60;
	  return (hours < 10 ? "0" : "") + String(hours) + ":" + 
	  (minutes < 10 ? "0" : "") + String(minutes);
  }

  void handleSchedulePost() {
	  bool success = true;
	    for(int i = 0; i < 7; i++) {
	  	  String startStr = server.arg("d" + String(i) + "s");
	  	  String endStr = server.arg("d" + String(i) + "e");
		
	  	  uint32_t start = parseTime(startStr);
	  	  uint32_t end = parseTime(endStr);
		
	  	  if(start > 86340 || end > 86340) { // Максимум 23:59
	  	  	success = false;
	  	  	break;
	  	  }
		
		    scheduleManager.weeklySchedule[i].start = start;
		    scheduleManager.weeklySchedule[i].end = end;
	    }
	
	    if(success) {
	  	  scheduleManager.save();
	  	  server.send(200, "text/plain", "OK");
	    } else {
	  	  server.send(400, "text/plain", "Ошибка формата времени");
	    }
  }

  uint32_t parseTime(String timeStr) {
	  int colonIndex = timeStr.indexOf(':');
	  if(colonIndex == -1) return 0;
	
	  uint8_t hours = timeStr.substring(0, colonIndex).toInt();
	  uint8_t minutes = timeStr.substring(colonIndex+1).toInt();
	
	  return hours * 3600 + minutes * 60;
  }
  
  WiFiState getState() const {
    if(WiFi.status() == WL_CONNECTED) return WiFiState::CONNECTED;
    if(WiFi.getMode() & WIFI_AP) return WiFiState::AP_MODE;
    return WiFiState::DISCONNECTED;
  }
  
  String getIP() const {
    if(state == WiFiState::CONNECTED) {
      return WiFi.localIP().toString();
    } else if(state == WiFiState::AP_MODE) {
      return WiFi.softAPIP().toString();
    }
    return "-";
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

  String getConnectedSSID() const {
    if(state == WiFiState::CONNECTED) {
      return WiFi.SSID().length() > 0 ? WiFi.SSID() : "-";
    }
    return "-";
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
			server.on("/schedule", HTTP_GET, [this]() { handleScheduleGet(); });
			server.on("/schedule", HTTP_POST, [this]() { handleSchedulePost(); });
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
          "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
          "<title>Умная розетка</title><style>" 
          "body {font-family: Arial; text-align: center;}"
          ".menu {margin: 50px auto; width: 300px;}"
          "a {display: block; margin: 20px; padding: 15px;"
          "background: #f0f0f0; border-radius: 5px; text-decoration: none; color: #333;}"
          "</style></head><body>"
          "<h1>Умная розетка</h1>"
          "<div class='menu'>"
          "<a href='/schedule'>Настройка расписания</a>"
          "<a href='/reconfigure'>Переподключение к сети</a>"
          "</div></body></html>");
  }
  
  void handleReconfigure() {
    server.send(200, "text/html",
          "<form action='/save' method='POST'>"
          "<h2>Настройка WiFi</h2>"
          "<label>SSID:</label><br>"
          "<input type='text' name='ssid' required><br><br>"
          "<label>Пароль:</label><br>"
          "<input type='password' name='pass'><br><br>"
          "<input type='submit' value='Сохранить'>"
          "</form>");
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