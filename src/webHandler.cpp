#include <LittleFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "webHandler.h"
#include "heartMonitor.h"

static AsyncWebServer server(80);
static const int WIFI_TIMEOUT_MS = 20000; // 20 second timeout for WiFi connection

static bool initializeWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  long now = millis();

  Serial.print("Attempting to connect to WiFi");
  while (WiFi.status() != WL_CONNECTED && millis() - now < WIFI_TIMEOUT_MS) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi, check connection.");
    return false;

  } else {
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    return true;
  }
}

static bool initializeFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return false;
  }
  return true;
}

bool initializeWebServer() {
    // Guard clauses
    // @todo: better error handling
    if (!initializeFS()) {
      return false;
    }
  
    if (!initializeWifi()) {
      return false;
    }
  
    server.serveStatic("/uPlot.min.css", LittleFS, "/uPlot.min.css");
    server.serveStatic("/uPlot.iife.min.js", LittleFS, "/uPlot.iife.min.js");
    server.serveStatic("/app.js", LittleFS, "/app.js");
  
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(LittleFS, "/index.html", "text/html");
    });
  
    server.on("/api/measurements", HTTP_GET, [](AsyncWebServerRequest* request) {
      static const size_t MAX_BATCH_SIZE = 32;
      HeartMonitorMeasurements batch[MAX_BATCH_SIZE];

      size_t count = drainMeasurements(batch, MAX_BATCH_SIZE);
      if (count == 0) {
        batch[0] = getMeasurements();
        count = 1;
      }

      String jsonResponse = "{\"measurements\":[";
      for (size_t i = 0; i < count; ++i) {
        if (i > 0) {
          jsonResponse += ",";
        }
        jsonResponse += "{";
        jsonResponse += "\"timestamp\":" + String(batch[i].timestamp) + ",";
        jsonResponse += "\"irValue\":" + String(batch[i].irValue) + ",";
        jsonResponse += "\"beatsPerMinute\":" + String(batch[i].beatsPerMinute, 2) + ",";
        jsonResponse += "\"averageBPM\":" + String(batch[i].averageBPM, 2);
        jsonResponse += "}";
      }
      jsonResponse += "]}";

      request->send(200, "application/json", jsonResponse);
    });

    server.begin();
    Serial.println("HTTP server started");
  
    return true;
  }
