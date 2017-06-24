#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

const char* ssid      = "SmitsSmit";
const char* password  = "ditisdewpakey";

unsigned int port = 1209;

char packetBuffer[255];
char ReplyBuffer[] = "acknowledged";

String dimmer = "@050";
String  kleur = "#FFFFFF";

WiFiUDP Udp;

int ledPin = 2; // GPIO2

const int   REDPIN = 14;
const int GREENPIN = 12;
const int  BLUEPIN = 13;

void setup()
{
  // set pin modes
  pinMode(  REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode( BLUEPIN, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  digitalWrite(REDPIN, HIGH);
  delay(300);
  digitalWrite(REDPIN, LOW);
  
  digitalWrite(GREENPIN, HIGH);
  delay(300);
  digitalWrite(GREENPIN, LOW);
  
  digitalWrite(BLUEPIN, HIGH);
  delay(300);
  digitalWrite(BLUEPIN, LOW);

  // begin serial and connect to WiFi
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Udp.begin(port);

}


int value = 0;

void loop()
{
  String tekst = "";
  int packetSize = Udp.parsePacket();
  if(packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remoteIp = Udp.remoteIP();
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }    
    Serial.print("Received:");
    Serial.println(packetBuffer);
    // Serial.write(packetBuffer);
    Serial.println();

    int r = 0;
    int b = 0;
    int g = 0;
    int rgb2pwm = 1023/255; // (max pwm / max kleur)
    
    // even een paar controles om te kijken wat we hebben
    // kleur: #FFFFFF
    // dimmer: @100
    
    String udpcmd = String(packetBuffer);
    
    if (udpcmd.length() == 7 && udpcmd.indexOf('#') == 0) {
      kleur = udpcmd;
      tekst = kleur;
      }
      
    else if (udpcmd.length() == 4 && udpcmd.indexOf('@') == 0) {
      dimmer = udpcmd;
      tekst = dimmer;
      }

    else if (udpcmd ==  "kleur") { tekst =  kleur; }
    else if (udpcmd == "dimmer") { tekst = dimmer; }
    else { return; }
    
    // convert color code to rgb values
    // Get rid of '#' and convert it to integer
    int kleurint = (int) strtol( &kleur[1], NULL, 16);  
    // Split them up into r, g, b values
    r = kleurint >> 16;
    g = kleurint >> 8 & 0xFF;
    b = kleurint & 0xFF;
    
    //convert dimmer value to decimal
    // Get rid of '@' and convert it to integer
    float dimmerint = strtol( &dimmer[1], NULL, 10);
    dimmerint = dimmerint/100;

    r = (int) (r*(rgb2pwm*dimmerint));
    g = (int) (g*(rgb2pwm*dimmerint));
    b = (int) (b*(rgb2pwm*dimmerint));
    
    analogWrite(  REDPIN, r);
    analogWrite(GREENPIN, g);
    analogWrite( BLUEPIN, b);

    char bericht[tekst.length()+1];
    tekst.toCharArray(bericht, tekst.length()+1); // Copy it over 
    // send a reply, to the IP address and port
    // that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());   
    Serial.print("Returned:");
    Serial.println(bericht);
    Udp.write( bericht,tekst.length());
    Udp.endPacket();
  }
}


