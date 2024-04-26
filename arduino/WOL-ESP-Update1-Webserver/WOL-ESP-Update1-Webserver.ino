/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

unsigned long turnOnTime;
unsigned long resetTime;
const char* ssid = "XXXXXX";
const char* password = "XXXXXX";
bool wol, resetButton = false;
int pinRelay = 12;
// Create an instance of the server
// specify the port to listen on as an argument
ESP8266WebServer server(80);

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
 
  server.send ( 404, "text/plain", message );
}
void handleRoot() {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  snprintf ( temp, 400,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
  </body>\
</html>",

    hr, min % 60, sec % 60
  );
  server.send ( 200, "text/html", temp );

}
void control() {
if(turnOnTime + 1000 > millis() && wol)
      {
         digitalWrite(pinRelay, 1);
      }
      else if(!resetButton)
      {
          digitalWrite(pinRelay, 0);
          wol = false;
      }
      if(resetTime  + 7000 > millis() && resetButton)
      {
        digitalWrite(pinRelay,1);
      }
      else if(!wol)
      {
        digitalWrite(pinRelay,0);
        resetButton = false;
      }
}
void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare GPIO2
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, 0);
  
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );

  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }

  server.on ( "/", handleRoot );
  server.on ( "/wol", []() {
    turnOnTime = millis();
    wol = true;
    String s = "WOL Received";
    Serial.println("Wol Received");
    server.send ( 200, "text/plain", s );
  } );
  server.on ( "/reset", []() {
    resetTime = millis();
    resetButton = true;
    String s = "Reset Received";
    Serial.println("Reset Received");
    server.send ( 200, "text/plain", s );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
}

void loop() {
    server.handleClient();
    control();
}

