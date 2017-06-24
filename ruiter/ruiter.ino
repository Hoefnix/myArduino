#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WiFiUdp.h>
#include <WiFiClient.h>

#define RELAISPIN1 00
#define RELAISPIN2 02

const char* ssid     = "RuiterWiFi";
const char* password = "";

unsigned long begin;

// -- debouncing
long debouncing_time = 15; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;

ESP8266WebServer server(80);

void handleRoot(void) {
  server.send(200, "text/plain", WiFi.localIP().toString() + "/status\n"+WiFi.localIP().toString() + "/aan:[1/2]\n"+WiFi.localIP().toString() + "/uit:[1/2]\n");

}

void setup(void)
{
  Serial.begin(115200);
  begin = millis();

  pinMode(RELAISPIN1, OUTPUT);
  pinMode(RELAISPIN2, OUTPUT);

  WiFi.hostname("schakelaar");
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

  server.on("/status", []() {
    server.send(200, "text/plain", getStatus());
  });

  server.on("/aan:1", []() {
    digitalWrite(RELAISPIN1, HIGH);
    server.send(200, "text/plain", getStatus());
  });

  server.on("/aan:2", []() {
    digitalWrite(RELAISPIN2, HIGH);
    server.send(200, "text/plain", getStatus());
  });

  server.on("/uit:1", []() {
    digitalWrite(RELAISPIN1, LOW);
    server.send(200, "text/plain", getStatus());
  });

  server.on("/uit:2", []() {
    digitalWrite(RELAISPIN2, LOW);
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

  statusTekst  = "{";
  statusTekst +=  "\"aan1\":";
  statusTekst += digitalRead(RELAISPIN1);
  statusTekst += ",\"aan2\":";
  statusTekst += digitalRead(RELAISPIN2);

  statusTekst += "}";

  return statusTekst;
}

void serialWifiStatus(void) {
  Serial.println("");
  Serial.println("ruiter");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());  
}
