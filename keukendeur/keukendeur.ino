#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WiFiUdp.h>
#include <WiFiClient.h>

#define DEURPIN 13
#define LUIKPIN 12
#define RELAIS1 4
#define RELAIS2 5
#define LEDPIN  14

const char* ssid     = "SmitsSmit";
const char* password = "";

ESP8266WebServer server(80);

void udpSendMsg(String deStatus, boolean sendHTTP = false) {
  WiFiUDP Udp;
  byte broadcastIp[] = { 255, 255, 255, 255 };

  if (sendHTTP) {
    server.send(200, "text/plain", deStatus);
  }

  Udp.beginPacket(broadcastIp, 5005);
  Udp.print(deStatus);
  Udp.endPacket();
}

void handleRoot(void) {
  server.send(200, "text/plain", "Hallo, keuken hier ( /status, /aan:[1|2], /uit:[1|2])\n");
}

void setup(void)
{
  Serial.begin(115200);

  pinMode(RELAIS1, OUTPUT);
  pinMode(RELAIS2, OUTPUT);

  pinMode(DEURPIN, INPUT_PULLUP);
  attachInterrupt(DEURPIN, deDeur, CHANGE);

  pinMode(LUIKPIN, INPUT_PULLUP);
  attachInterrupt(LUIKPIN, hetLuik, CHANGE);

  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);

  WiFi.hostname("keukendeur");
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
    digitalWrite(RELAIS1, HIGH);
    udpSendMsg(getStatus(), true);
  });

  server.on("/uit:1", []() {
    digitalWrite(RELAIS1, LOW);
    udpSendMsg(getStatus(), true);
  });

  server.on("/aan:2", []() {
    digitalWrite(RELAIS2, HIGH);
    udpSendMsg(getStatus(), true);
  });

  server.on("/uit:2", []() {
    digitalWrite(RELAIS2, LOW);
    udpSendMsg(getStatus(), true);
  });

  server.begin();
  Serial.println("HTTP server started");
  digitalWrite(LEDPIN, LOW);
}

void loop(void) {
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }
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
  String statusTekst = "";
  int huidig;

  huidig = digitalRead(DEURPIN);
  if (vorige != huidig) {
    vorige = huidig;
    statusTekst  = "{\"keukendeur\":";
    statusTekst += digitalRead(DEURPIN);
    statusTekst += "}";
    udpSendMsg(statusTekst, false);
    Serial.println(statusTekst);
  }
}

void hetLuik(void) {
  static int vorige;
  String statusTekst = "";
  int huidig;

  huidig = digitalRead(DEURPIN);
  if (vorige != huidig) {
    vorige = huidig;
    statusTekst  = "{\"kattenluik\":";
    statusTekst += digitalRead(LUIKPIN);
    statusTekst += "}";
    udpSendMsg(statusTekst, false);
  }
}

String getStatus(void) {
  String statusTekst = "";

  statusTekst  = "{\"naam\":";
  statusTekst += "\"keukendeur\"";
  statusTekst += ",\"ip\":";
  statusTekst += "\"" + WiFi.localIP().toString() + "\"";
  statusTekst += ",\"keukendeur\":";
  statusTekst += digitalRead(DEURPIN);
  statusTekst += ",\"kattenluik\":";
  statusTekst += digitalRead(LUIKPIN);
  statusTekst += ",\"aanuit1\":";
  statusTekst += digitalRead(RELAIS1);
  statusTekst += ",\"aanuit2\":";
  statusTekst += digitalRead(RELAIS2);
  statusTekst += "}";

  return statusTekst;
}

void serialWifiStatus(void) {
  Serial.println("");
  Serial.println("keuken");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
