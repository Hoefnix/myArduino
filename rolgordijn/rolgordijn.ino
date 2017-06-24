#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WiFiUdp.h>
#include <WiFiClient.h>

#define OPNEER 16
#define AANUIT 12

const char* ssid     = "SmitsSmit";
const char* password = "";

int positie; // 0 is volledig opgerold, 100 is volledig uitgerold

ESP8266WebServer server(80);

void handleRoot(void) {
  server.send(200, "text/plain", argumenten() + "\n");
}

void setup(void)
{
  Serial.begin(115200);

  pinMode(OPNEER, OUTPUT);
  pinMode(AANUIT, OUTPUT);

  WiFi.hostname("rolgordijn");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  Serial.print("\n\r \n\rWorking to connect");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  serialWifiStatus();

  server.on("/", handleRoot );

  server.on("/help", [](){
      server.send(200, "text/plain", WiFi.localIP().toString() + "/status\n" + WiFi.localIP().toString() + "?position=[0/100]\n" + WiFi.localIP().toString() + "?initial=[0/100]\n");
  });

  server.on("/status", []() {
    server.send(200, "text/plain", getStatus());
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  if (WiFi.status() == WL_DISCONNECTED) {
    ESP.reset();
  }
}

String getStatus(void) {
  String statusTekst = "";

  statusTekst  = "{position:";
  statusTekst +=  (String)positie;
  statusTekst += "}";

  return statusTekst;
}

String argumenten(void) {
  int percentage;

  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "position") {
      percentage = server.arg(i).toInt();
      if (percentage >= 0 && percentage <= 100) {
        server.send(200, "text/plain", "{position:" + (String)percentage + "}");
        if (positie > percentage ) {
          digitalWrite(OPNEER, LOW); // neer
          digitalWrite(AANUIT, HIGH);
          while (positie > percentage) {
            delay(1000);                  // waits for a second
            positie -= 1;
            Serial.println(positie);
          }
        }

        else if (positie < percentage ) {
          digitalWrite(OPNEER, HIGH); // op
          digitalWrite(AANUIT, HIGH);
          while (positie < percentage) {
            delay(1000);                  // waits for a second
            positie += 1;
            Serial.println(positie);
          }
        }
        digitalWrite(AANUIT, LOW);
      }
      else {
        server.send(200, "text/plain", "{position:" + (String)positie + "}");
      }
    }
  else if (server.argName(i) == "initial") {
    percentage = server.arg(i).toInt();
    if (percentage >= 0 && percentage <= 100) {
      positie = percentage;
      server.send(200, "text/plain", "{position:" + (String)percentage + "}");
    }
  }

  }
  return "";
}

void serialWifiStatus(void) {
  Serial.println("");
  Serial.println("rolgrodijn");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}
