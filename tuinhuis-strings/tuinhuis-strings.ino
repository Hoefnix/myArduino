#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WiFiUdp.h>
#include <WiFiClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define DEURPIN 4
#define RELAISPIN 12
#define ONE_WIRE_BUS 2  // DS18B20 pin

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

const char* ssid     = "Smits.Smit";
const char* password = "";
IPAddress ip(192, 168, 178, 111);

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
  server.send(200, "text/plain", "Hallo, tuinhuis hier ( /status voor meetwaarden)");
}

void setup(void)
{
  Serial.begin(115200);
  begin = millis();

  pinMode(RELAISPIN, OUTPUT);
  pinMode(DEURPIN, INPUT_PULLUP);
  attachInterrupt(DEURPIN, deDeur, CHANGE);

  pinMode(  LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);

  WiFi.hostname("tuinhuis");
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

  server.on("/aan", []() {
    digitalWrite(RELAISPIN, HIGH);
    udpSendMsg(getStatus(), true);
  });

  server.on("/uit", []() {
    digitalWrite(RELAISPIN, LOW);
    udpSendMsg(getStatus(), true);
  });

  server.begin();
  Serial.println("HTTP server started");
  
  digitalWrite(LEDPIN, LOW);
}

void loop(void) {
  server.handleClient();
  if (( millis()-begin) > (3600000) ) {
    begin = millis();
    //  3600000 ms in an hour, 86400000 ms in a day
    udpSendMsg("Tuinhuis reset now", false);
    delay(300);
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


float deTemperatuur(void) {
  float temp;
  for (int i = 0; i <= 25; i++) {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    if (!(temp == 85.0 || temp == (-127.0))) break;
  }
  return temp;
}

void deDeur(void) {
  static int vorige;
  int huidig;

  if ((long)(micros() - last_micros) >= debouncing_time * 1000) {
    last_micros = micros();
    huidig = digitalRead(DEURPIN);
    if (vorige != huidig) {
      vorige = huidig;
        Serial.println("Deur...");

      udpSendMsg(getStatus(), false);
    }
  }
}

String getStatus(void) {
  String statusTekst = "";

  statusTekst  = "{\"naam\":";
  statusTekst += "\"tuinhuis\"";
  //statusTekst += "\"schuur\"";
  statusTekst += ",\"ip\":";
  statusTekst += "\"" + WiFi.localIP().toString() + "\"";
  statusTekst += ",\"deur\":";
  statusTekst += digitalRead(  DEURPIN);
  statusTekst += ",\"aanuit\":";
  statusTekst += digitalRead(RELAISPIN);
  statusTekst += ",\"temperatuur\":";
  statusTekst += deTemperatuur();
  statusTekst += "}";

  return statusTekst;
}

void serialWifiStatus(void) {
  Serial.println("");
  Serial.println("tuinhuis");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
