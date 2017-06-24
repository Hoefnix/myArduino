

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "FS.h"

#define inputPin  2

const char* ssid     = "Smits.Smit";
const char* password = "ditisdewpakey";

const char* host = "api.thingspeak.com";
const char* thingspeak_key = "0Y0TM8Y14DTM54P1";
const char* serverpiip	= "192.168.178.120";

int vertragingswaarde = 45;
int minutes = 0;
int serversecs = 0;
int prvPIRstate = 0;
int aanofuit = 0;

#define RELAISPIN1 12
#define RELAISPIN2 13
#define RELAISPIN3 14
#define RELAISPIN4 15
#define RELAISPIN5 16

// NETWORK: Static IP details...
IPAddress ip(192, 168, 178, 29);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

String webString = "";   // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last movement was read
unsigned long timeUntilShutdown = 0;

void handle_aanofuit() {
  String aanuit1 = server.arg("1");
  String aanuit2 = server.arg("2");
  String aanuit3 = server.arg("3");
  String aanuit4 = server.arg("4");
  String aanuit5 = server.arg("5");

  if (aanuit1 != "" ) {
    digitalWrite(RELAISPIN1, (aanuit1 == "aan") ? HIGH : LOW);
  }
  if (aanuit2 != "" ) {
    digitalWrite(RELAISPIN2, (aanuit2 == "aan") ? HIGH : LOW);
  }
  if (aanuit3 != "" ) {
    digitalWrite(RELAISPIN3, (aanuit3 == "aan") ? HIGH : LOW);
  }
  if (aanuit4 != "" ) {
    digitalWrite(RELAISPIN4, (aanuit4 == "aan") ? HIGH : LOW);
  }
  if (aanuit5 != "" ) {
    digitalWrite(RELAISPIN5, (aanuit5 == "aan") ? HIGH : LOW);
  }

  server.send(200, "text/plain", myStatus());
  serverpi("/status");
}

void handle_root() {
  String vertraging = server.arg("uitschakelvertraging");

  if (vertraging != "" ) {
    vertragingswaarde = vertraging.toInt();
    File filew = SPIFFS.open("/vertragingswaarde", "w");
    if (!filew) {
      Serial.println("file open failed");
      server.send(200, "text/plain", "Fout bij schrijven nieuwe uitschakelvertraging");
    }
    else {
      filew.println(vertragingswaarde);
      filew.close();
      server.send(200, "text/plain", myStatus());
    }
  }
  else {
    server.send(200, "text/plain", "Hallo, hier is het bureau\n - /status\n - /aanuit?[1-5]=[aan/uit]\n - ?uitschakelvertraging=[nn]\n");
  }
  delay(100);
}

void setup(void)
{
  // You can open the Arduino IDE Serial Monitor window to see what the code is doing
  Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable

  pinMode(RELAISPIN1, OUTPUT);
  pinMode(RELAISPIN2, OUTPUT);
  pinMode(RELAISPIN3, OUTPUT);
  pinMode(RELAISPIN4, OUTPUT);
  pinMode(RELAISPIN5, OUTPUT);

  WiFi.mode(WIFI_STA);
  // WiFi.config(ip, gateway, subnet); // Static IP Setup Info Here...
  WiFi.begin(ssid, password);       // Connect to WiFi network
  Serial.print("\n\r \n\rWorking to connect");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Woonkamer timer");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_root);

  server.on("/aanuit", handle_aanofuit);

  server.on("/status", []() {                       // if you add this subdirectory to your webserver call, you get text below :)
    server.send(200, "text/plain", myStatus());     // send to someones browser when asked

  });

  server.begin();
  Serial.println("HTTP server started");
  prvPIRstate = digitalRead(inputPin);

  // check vertragingswaarde
  SPIFFS.begin();
  if (!SPIFFS.exists("/vertragingswaarde")) {
    Serial.println("formatting...");
    if (SPIFFS.format()) {
      File filew = SPIFFS.open("/vertragingswaarde", "w");
      if (!filew) {
        Serial.println("file open failed");
      }
      else {
        Serial.println("Standaard uitschakelvertraging geschreven");
        filew.println(vertragingswaarde);
        filew.close();
      }
    }
  }
  else {
    if (SPIFFS.exists("/vertragingswaarde")) {
      File filer = SPIFFS.open("/vertragingswaarde", "r");
      if (filer) {
        String vertraging = filer.readStringUntil('\n');
        vertragingswaarde = vertraging.toInt();
        Serial.print("Gelezen vertragingswaarde: ");
        Serial.println(vertragingswaarde);
      }
      else {
        Serial.print("fout bij openen /vertragingswaarde");
      }
    }
  }
}

void loop(void)
{
  int curPIRstate;
  int verschil;
  unsigned long currentMillis = millis();

  verschil = currentMillis - previousMillis;
  timeUntilShutdown = (vertragingswaarde - (verschil / 60000));

  if (verschil >= (vertragingswaarde * 60000)) {
    if (aanofuit == 1) {
      Serial.println("Zet uit");
      serverpi("/?bureau:piruit");      //serverpi("/?woonkamer:piruit");
      aanofuit = 0;
    }
    previousMillis = currentMillis;     // reset timer
  }

  server.handleClient();

  delay(1000);                          // miliseconden, 1000 = 1 sec
  curPIRstate = digitalRead(inputPin);  // read input value
  // Serial.println("1. status PIR: " + String(curPIRstate));
  if (curPIRstate != prvPIRstate) {
    // wacht een een seconde en lees nog een keer om evt piekjes er uit te filteren
    delay(1000);                          // miliseconden, 1000 = 1 sec
    curPIRstate = digitalRead(inputPin);  // read input value
    Serial.println("2. status PIR: " + String(curPIRstate));
    if (curPIRstate != prvPIRstate) {
      Serial.println("Timer reset");
      previousMillis = currentMillis;     // reset timer
      if (aanofuit == 0) {
        Serial.println("Zet aan");
        serverpi("/?bureau:piraan");      // serverpi("/?woonkamer:piraan");
        aanofuit = 1;
      }
      prvPIRstate = curPIRstate;
    }
  }
}

void serverpi(String opdracht)
{
  WiFiClient client;
  const int httpPort = 1208;

  if (!client.connect(serverpiip, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  client.print(String("GET ") + opdracht + " HTTP/1.1\r\n" +
               "Host: " + serverpiip + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
}

String myStatus() {

  webString  = "{\"naam\":";
  webString += "\"bureau\"";
  webString += ",\"aanuit1\":";
  webString +=  digitalRead(RELAISPIN1);
  webString += ",\"aanuit2\":";
  webString +=  digitalRead(RELAISPIN2);
  webString += ",\"aanuit3\":";
  webString +=  digitalRead(RELAISPIN3);
  webString += ",\"aanuit4\":";
  webString +=  digitalRead(RELAISPIN4);
  webString += ",\"aanuit5\":";
  webString +=  digitalRead(RELAISPIN5);
  webString += ",\"uitover\":";
  webString += timeUntilShutdown;
  webString += ",\"vertraging\":";
  webString += vertragingswaarde;
  webString += "}";

  return webString;            // send to someones browser when asked

}

