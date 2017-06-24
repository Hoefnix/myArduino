#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WiFiUdp.h>
#include <WiFiClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define SENSORPIN 4
#define RELAISPIN1 05
#define RELAISPIN2 12
#define RELAISPIN3 13
#define RELAISPIN4 15

const char* ssid     = "SmitsSmit";
const char* password = "";

const int   LEDPIN = 14;
unsigned long begin;

// -- debouncing
long debouncing_time = 15; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;

ESP8266WebServer server(80);

void udpSendMsg(String deStatus, boolean sendHTTP = false) {
  WiFiUDP Udp;
  byte broadcastIp[] = { 255, 255, 255, 255 };

  if (sendHTTP) {
    server.send(200, "text/plain", deStatus);
  }
  /*
    udp.beginPacketMulticast(addr, port, WiFi.localIP());
  */
  Udp.beginPacket(broadcastIp, 5005);
  Udp.print(deStatus);
  Udp.endPacket();
}

void handleRoot(void) {
  server.send(200, "text/plain", "Hallo, bureau hier");
}

void setup(void)
{
  Serial.begin(115200);
  begin = millis();

  pinMode(RELAISPIN1, OUTPUT);
  // digitalWrite(RELAISPIN1, HIGH);

  pinMode(RELAISPIN2, OUTPUT);
  // digitalWrite(RELAISPIN2, HIGH);

  pinMode(RELAISPIN3, OUTPUT);
  // digitalWrite(RELAISPIN3, HIGH);
  
  pinMode(RELAISPIN4, OUTPUT);

  pinMode(SENSORPIN, INPUT_PULLUP);
  attachInterrupt(SENSORPIN, deDeur, CHANGE);

  pinMode(  LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);

  WiFi.hostname("bureau");
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
    udpSendMsg(getStatus(), true);
  });

  server.on("/aan:1", []() {
    digitalWrite(RELAISPIN1, HIGH);
    udpSendMsg(getStatus(), true);
  });

  server.on("/aan:2", []() {
    digitalWrite(RELAISPIN2, HIGH);
    udpSendMsg(getStatus(), true);
  });
  
  server.on("/aan:3", []() {
    digitalWrite(RELAISPIN3, HIGH);
    udpSendMsg(getStatus(), true);
  });
  
  server.on("/aan:4", []() {
    digitalWrite(RELAISPIN3, HIGH);
    udpSendMsg(getStatus(), true);
  });

  server.on("/uit:1", []() {
    digitalWrite(RELAISPIN1, LOW);
    udpSendMsg(getStatus(), true);
  });

  server.on("/uit:2", []() {
    digitalWrite(RELAISPIN2, LOW);
    udpSendMsg(getStatus(), true);
  });

  server.on("/uit:3", []() {
    digitalWrite(RELAISPIN3, LOW);
    udpSendMsg(getStatus(), true);
  });

  server.on("/uit:4", []() {
    digitalWrite(RELAISPIN2, LOW);
    udpSendMsg(getStatus(), true);
  });
  
  server.begin();
  Serial.println("HTTP server started");

  digitalWrite(LEDPIN, LOW);
}

void loop(void) {
  server.handleClient();
   if (WiFi.status() == WL_DISCONNECTED) { ESP.reset(); }
  /*
    if (( millis() - begin) > (3600000) ) {
    begin = millis();
    //  3600000 ms in an hour, 86400000 ms in a day
    udpSendMsg("Tuinhuis reset now", false);
    delay(300);
    ESP.restart();
    }
  */
  analogWrite(  LEDPIN, teller() );
}

int teller() {
  static int pwm = 0;
  static int plusmin = 1;

  if (pwm >= 102400) {
    plusmin = -1;
  }
  else if (pwm <= 10) {
    plusmin = 1;
  }

  pwm += plusmin;
  return  (int) pwm / 100;
}

void deDeur(void) {
  static int vorige;
  int huidig;

  if ((long)(micros() - last_micros) >= debouncing_time * 1000) {
    last_micros = micros();
    huidig = digitalRead(SENSORPIN);
    if (vorige != huidig) {
      vorige = huidig;
      Serial.println("Sensor...");
      udpSendMsg(getStatus(), false);
    }
  }
}

String getStatus(void) {
  String statusTekst = "";

  statusTekst  = "{\"naam\":";
  statusTekst += "\"bureau\"";
  statusTekst += ",\"ip\":";
  statusTekst += "\"" + WiFi.localIP().toString() + "\"";
  statusTekst += ",\"deur\":";
  statusTekst += digitalRead(SENSORPIN);
  statusTekst += ",\"aanuit1\":";
  statusTekst += digitalRead(RELAISPIN1);
  statusTekst += ",\"aanuit2\":";
  statusTekst += digitalRead(RELAISPIN2);
  statusTekst += ",\"aanuit3\":";
  statusTekst += digitalRead(RELAISPIN3);
  statusTekst += ",\"aanuit4\":";
  statusTekst += digitalRead(RELAISPIN4);
  statusTekst += "}";

  return statusTekst;
}

void serialWifiStatus(void) {
  Serial.println("");
  Serial.println("bureau");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
