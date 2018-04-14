#include <ArduinoJson.h>
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

static const char ssid[] = "xxx";
static const char password[] = "xxx";
MDNSResponder mdns;

const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(6);
DynamicJsonBuffer jsonBuffer(bufferSize);
//StaticJsonBuffer<200> jsonBuffer;

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(9999);

#define NUMPIXELS 240

#define DATAPIN    13
#define CLOCKPIN   14

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:{
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    }
    case WStype_CONNECTED:{
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    }
      
    case WStype_TEXT:{ //text
//      Serial.printf("Payload: %s\r\n", payload);
      JsonObject& root = jsonBuffer.parseObject(payload);
      const char* exdata = root["_extendedData"][0];
//      size_t len = root["_extendedData"][1];
      Serial.printf("Extended Data: %s\r\n", exdata);
      handleColorData((char*)exdata, NUMPIXELS);
      break;
    }
      
    case WStype_BIN:{ //binary
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      break;
    }
     
    default:{
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
    }
  }
}

void handleColorData(char * data, size_t length){
  int head = 0;
  char* colorStr;
  while ((colorStr = strtok_r(data, ";", &data)) != NULL && head < NUMPIXELS){
//    long color = strtoul (colorStr, NULL, 16);
//    char* colorStr = "FF0000";
    uint8_t rcolor =  strtoul (colorStr, NULL, 16) >> 16;
    uint8_t gcolor =  strtoul (colorStr+2, NULL, 16) >> 8;
    uint8_t bcolor =  strtoul (colorStr+4, NULL, 16);
    uint32_t ucolor = packRGB(gcolor,
                             rcolor,
                             bcolor);
//    setColor(ucolor, 0, 1);
    strip.setPixelColor(head, ucolor);
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


void setColorRGB(uint8_t r, uint8_t g, uint8_t b, int from, int to){
  int head = from;
  while (head < to && head < NUMPIXELS){
    Serial.print("rcolor: ");
    Serial.println(r);
    Serial.print("gcolor: ");
    Serial.println(g);
    Serial.print("bcolor: ");
    Serial.println(b);
    strip.setPixelColor(head, g, r, b);
    head++;
  }
}

uint32_t packRGB(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void loop()
{
  webSocket.loop();
}
