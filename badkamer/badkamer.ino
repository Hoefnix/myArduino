/*
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include "FS.h"

#define DHTTYPE DHT11
#define DHTPIN  2

const char* ssid     = "Smits.Smit";
const char* password = "";

const char* host = "api.thingspeak.com";
const char* thingspeak_key = "0Y0TM8Y14DTM54P1";
const char* ventilator	= "192.168.178.205";
const char* serverpiip	= "192.168.178.100";

int drempelwaarde = 45;
int minutes = 0;
int serversecs = 0;

ESP8266WebServer server(80);

// Initialize DHT sensor
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01

DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

float humidity, temp;  // Values read from sensor
String webString = "";   // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor

void handle_root() {
  String drempel = server.arg("drempel");
  if (drempel != "" ) {
    drempelwaarde = drempel.toInt();
    File filew = SPIFFS.open("/drempelwaarde", "w");
    if (!filew) {
      Serial.println("file open failed");
      server.send(200, "text/plain", "Fout bij schrijven nieuwe drempelwaarde");
    }
    else {
      filew.println(drempelwaarde);
      filew.close();
      Serial.println("Nieuwe drempelwaarde (" + drempel + ") geschreven");
      server.send(200, "text/plain", "Nieuwe drempelwaarde (" + drempel + ") geschreven");
    }
  }
  else {
    server.send(200, "text/plain", "Hallo, hier is de badkamer( /status voor meetwaarden)");
  }
  delay(100);
}

void setup(void)
{
  // You can open the Arduino IDE Serial Monitor window to see what the code is doing
  Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable
  dht.begin();           // initialize temperature sensor

  WiFi.mode(WIFI_STA);
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("DHT Badkamer Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_root);

  server.on("/temp", []() { // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
    webString = "Temperature: " + String((int)temp) + " C"; // Arduino has a hard time with float to string
    server.send(200, "text/plain", webString);            // send to someones browser when asked
  });

  server.on("/humidity", []() { // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();           // read sensor
    webString = "Humidity: " + String((int)humidity) + "%";
    server.send(200, "text/plain", webString);               // send to someones browser when asked
  });

  server.on("/status", []() { // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor

    webString  = "{\"naam\":";
    webString += "\"badkamer\"";
    webString += ",\"temperatuur\":";
    webString += temp;
    webString += ",\"luchtvochtigheid\":";
    webString += humidity;
    webString += ",\"drempelwaarde\":";
    webString += drempelwaarde;
    webString += "}";

    server.send(200, "text/plain", webString);            // send to someones browser when asked
  });

  server.begin();
  Serial.println("HTTP server started");

  // check drempelwaarde
  SPIFFS.begin();
  if (!SPIFFS.exists("/drempelwaarde")) {
    Serial.println("formatting...");
    if (SPIFFS.format()) {
      File filew = SPIFFS.open("/drempelwaarde", "w");
      if (!filew) {
        Serial.println("file open failed");
      }
      else {
        Serial.println("Standaard drempelwaarde geschreven");
        filew.println(drempelwaarde);
        filew.close();
      }
    }
  }
  else {
    if (SPIFFS.exists("/drempelwaarde")) {
      File filer = SPIFFS.open("/drempelwaarde", "r");
      if (filer) {
        String drempel = filer.readStringUntil('\n');
        drempelwaarde = drempel.toInt();
        Serial.print("Gelezen drempelwaarde: ");
        Serial.println(drempelwaarde);
      }
      else {
        Serial.print("fout bij openen /drempelwaarde");
      }
    }
  }
}

void loop(void)
{
  server.handleClient();
  delay(1000); // miliseconden, 1000 = 1 sec
  ventileren();
}

void ventileren() {
  // Serial.println(serversecs);
  if (++serversecs > 60) {  // eens per minuut checken of ventilator aan moet
    serversecs = 0;
    gettemperature();
    if (humidity > drempelwaarde) {
      // Use WiFiClient class to create TCP connections
      WiFiClient clntvent;
      const int httpPort = 80;
      if (!clntvent.connect(ventilator, httpPort)) {
        Serial.println("connection failed");
        return;
      }

      Serial.println("De ventilator zou nu aan moeten");
      // This will send the request to the server
      clntvent.print(String("GET ") + "?aan" + " HTTP/1.1\r\n" +
                     "Host: " + ventilator + "\r\n" +
                     "Connection: close\r\n\r\n");
      delay(10);
      // Read all the lines of the reply from server and print them to Serial
      while (clntvent.available()) {
        String line = clntvent.readStringUntil('\r');
        Serial.print(line);
      }
      serverpi();
    }
    thingspeak();
  }
}
void serverpi()
{
  const int httpPort = 1208;
    WiFiClient clntvent;

  if (!clntvent.connect(serverpiip, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.println("Update status");
  // This will send the request to the server
  clntvent.print(String("GET ") + "/status" + " HTTP/1.1\r\n" +
                 "Host: " + serverpiip + "\r\n" +
                 "Connection: close\r\n\r\n");
  delay(10);
  // Read all the lines of the reply from server and print them to Serial
  while (clntvent.available()) {
    String line = clntvent.readStringUntil('\r');
    Serial.print(line);
  }
}

void thingspeak()
{
  // iedere vijf minuten, vandaar de teller
  if (++minutes > 4) {
    minutes = 0;

    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    gettemperature();

    String url = "/update?key=";
    url += thingspeak_key;
    url += "&field4=";
    url += temp;
    url += "&field3=";
    url += humidity;

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  }
}

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor
    previousMillis = currentMillis;

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();  // Read humidity (percent)
    temp = dht.readTemperature();   // Read temperature
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}

