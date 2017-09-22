#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>
#include <string.h>

//extern "C" {
//#include "user_interface.h"
//}

static const char ssid[] = "Abraham Linksys";
static const char password[] = "not my real password";
MDNSResponder mdns;

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(9999);

#define NUMPIXELS 60

#define DATAPIN    13
#define CLOCKPIN   14

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
//  uint32_t free = system_get_free_heap_size();
//  Serial.println("free mem: ");
//  Serial.println(free);
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
      
    case WStype_TEXT: //text
      handleColorData((char*)payload, length);
      break;
      
    case WStype_BIN: //binary
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      break;
     
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleColorData(char * data, size_t length){
  int head = 0;
  char* colorStr;
  while ((colorStr = strtok_r(data, ";", &data)) != NULL && head < NUMPIXELS){
    long color = strtoul (colorStr, NULL, 16);
    strip.setPixelColor(head, color);
    head++;
  }
  //show lights
  strip.show();
  
  //clear memory
  delete[] data;
  data=NULL;
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("espWebSock", WiFi.localIP())) {
    Serial.println("MDNS responder started");
//    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 9999);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.print("Connect to http://espWebSock.local or http://");
  Serial.println(WiFi.localIP());

  //setup strip
  strip.begin();
  strip.show();

  //start websocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
}
