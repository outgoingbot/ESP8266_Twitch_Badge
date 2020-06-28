/*
 todo
 create gmail and twitch accounts to share
 needs SPI for RGB leds and SPI for led matrix. SPI for fast led might not be required. will led matrix lib support esp8266 spi?
 user enter SSID, passwords, twitch name, etc (USB?)
 decide chat keywords and methods. i.e donate, subscribe, moderators, etc
 fun stuff: patters, colors, scroll text, blink
 design boards


 -update firmware over wifi?
 -run webserver?

 HARDWARE
 WS2812 Pin = 5 - D1 - GPIO5
 MAX7219 Data = HMOSI - D7 - GPIO13
 MAX7219 Clock = HSCLK - D5 - GPIO14 
 MAX7219 Load = 15 - D8 - GPIO15
 
 */

//Powering the RGBs and the MAX7219 ONLY! with 5v (Vin) but using 3.3v logic. seems to work well

#define BTN1_PIN  4 // D2 - GPIO4 
#define BTN2_PIN  0 // D3 - GPIO0
#define LOAD_PIN  15 // D8 - GPIO15
#define LED_PIN   5 // D1 - GPIO5


#include <LEDMatrixDriver.hpp>
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ALLOW_INTERRUPTS 0
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include "secret.h"

// Initialize network parameters
char ssid[] = SECRET_SSID; 
char password[] = SECRET_PASS;

// Declare websocket client class variable
WebSocketsClient webSocket;

// Allocate the JSON document
StaticJsonDocument<200> doc;

// Parameters for Twitch channel
const char twitch_oauth_token[] = TWITCH_OAUTH_TOKEN;
const char twitch_nick[] = TWITCH_NICK;    
const char twitch_channel[] = TWITCH_CHANNEL;

// Define necessary parameters for controlling the WS2812B LEDs
#define N_PIXELS  89
#define LED_TYPE  WS2812B
#define BRIGHTNESS  255     // a little dim for recording purposes
#define COLOR_ORDER GRB

uint32_t Forecolor = 0x000800;
uint8_t Brange = 8; //1, 3, 5 //set the brightness for each chanel in getrand();
uint8_t green; //used in getrand
uint8_t red;   //used in getrand
uint8_t blue;  //used in getrand

// declare the LED array
CRGB leds[N_PIXELS];

const uint8_t LEDMATRIX_CS_PIN = LOAD_PIN;
// Number of 8x8 segments you are connecting
const int LEDMATRIX_SEGMENTS = 1;
const int LEDMATRIX_WIDTH    = LEDMATRIX_SEGMENTS * 8;
// The LEDMatrixDriver class instance
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);

void setup() {
lmd.setEnabled(true);
lmd.setIntensity(2);   // 0 = low, 10 = high
  
pinMode(LED_BUILTIN, OUTPUT);
pinMode(BTN1_PIN,INPUT);
pinMode(BTN2_PIN,INPUT);
  
  
 FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, N_PIXELS).setCorrection(TypicalLEDStrip);
 FastLED.setBrightness(BRIGHTNESS);
 FastLED.clear(true);



  // Connect to WiFi
  WiFi.begin(ssid,password);
  Serial.begin(115200);
  while(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN,1);   
    Serial.print(".");
    delay(250);
    digitalWrite(LED_BUILTIN,0);
    delay(250);
    digitalWrite(LED_BUILTIN,0);
  }
  digitalWrite(LED_BUILTIN,0);  //On Board LED On when WIFI connected
  Serial.println();
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());

  // server address, port, and URL path
  webSocket.begin("irc-ws.chat.twitch.tv", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);

  // initialize the FastLED object

}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    // If the websocket connection is succesful, try to join the IRC server
    case WStype_CONNECTED:
      LedTest();
      Serial.printf("[WSc] Connected to: %s\n", payload);
      webSocket.sendTXT("PASS " + String(twitch_oauth_token) + "\r\n");
      webSocket.sendTXT("NICK " + String(twitch_nick) + "\r\n");
      webSocket.sendTXT("JOIN " + String(twitch_channel) + "\r\n");
      break;
    // If we get a response, print it to Serial
    case WStype_TEXT: {
      Serial.printf("> %s\n", payload);
      String payload_str = String((char*) payload);
      // Search for the beginning on the JSON-encoded message (":!")
      int quote_start = payload_str.indexOf(":!");
      // If the message is addressed to the chat bot
      if(quote_start > 0) {
        int quote_end = payload_str.length();
        String pixel_str = payload_str.substring(quote_start+2, quote_end);
        Serial.println(pixel_str);
        parseMessage(pixel_str);
      }
      break;
    }
    // Handle disconnecting from the websocket
    case WStype_DISCONNECTED:
    LedTest();
      Serial.printf("[WSc] Disconnected!\n");
      webSocket.sendTXT("PART " + String(TWITCH_CHANNEL) + "\r\n");
      break;
  }
}

void parseMessage(String pixel_str) {
  // Attempt to deserialize the string
  DeserializationError error = deserializeJson(doc, pixel_str);
  // Test to see if the string is a valid JSON-formatted one
  if(error) {
    Serial.print("deserializeJson failed: ");
    Serial.println(error.c_str());
    return;
  }
  // Assume that a valid string is of the form {"led":"11,255,255,255"}
  // for turning pixel #12 to full white
  if(doc.containsKey("led")) {
    String val = doc["led"];
    uint8_t i, r, g, b;
    uint8_t result = sscanf(val.c_str(), "%d,%d,%d,%d", &i, &r, &g, &b);
    setRgb(i, r, g, b);
  }
}

void setRgb(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
  if(i < N_PIXELS) {
    leds[i].r = r;
    leds[i].g = g;
    leds[i].b = b;
    FastLED.show();
  }
}

void LedTest(){
for(int i=0; i<N_PIXELS; i++){
  digitalWrite(LED_BUILTIN,0);
  delay(10);
  getrand();
  setRgb(i,green,red,blue);
  digitalWrite(LED_BUILTIN,1);
  delay(10);
}
 
for(int x=0; x<8; x++){
     for(int y=0; y<8; y++){
      lmd.setPixel(x,y,1);
      lmd.display();
      delay(10);
     }
}
for(int x=0; x<8; x++){
     for(int y=0; y<8; y++){
      lmd.setPixel(x,y,0);
      lmd.display();
      delay(10);
     }
}

 for(int i=0; i<N_PIXELS; i++){
  setRgb(i,0,0,0);
  delay(10);
 }
}

void BtnTest(){
if(digitalRead(BTN1_PIN)==LOW){
  for(int x=0; x<8; x++){
     for(int y=0; y<8; y++){
      lmd.setPixel(x,y,1);
      lmd.display();
      delay(10);
     }
  }
  for(int x=0; x<8; x++){
   for(int y=0; y<8; y++){
    lmd.setPixel(x,y,0);
    lmd.display();
    delay(10);
   }
  }
}
if(digitalRead(BTN2_PIN)==LOW){
for(int i=0; i<N_PIXELS; i++){
  digitalWrite(LED_BUILTIN,0);
  delay(100);
  getrand();
  setRgb(i,green,red,blue);
  digitalWrite(LED_BUILTIN,1);
  delay(100);
}
} 
}



void getrand() { // trying to make it //the goal is to get a random forecolor that is not white, then find the opposite of
      switch (random(0, 3)) {
        case 0:
          green = (random(0, Brange + 1) * 2);
          red = (random(0, Brange + 1) * 2);
          blue = 0;
          if (green == 0 || red == 0) { //prevents all black values
            blue = 1;
          };
          break;

        case 1:
          green = (random(0, Brange + 1) * 2);
          blue = (random(0, Brange + 1) * 2);
          red = 0; //32
          if (green == 0 || blue == 0) {
            red = 1;
          };
          break;

        case 2:
          red = (random(0, Brange + 1) * 2);
          blue = (random(0, Brange + 1) * 2);
          green = 0; //32
          if (red == 0 || blue == 0) {
            green = 1;
          };
          break;
      }
      Forecolor = (65536 * green) + (256 * red) + (1 * blue);
}


void loop() {
  webSocket.loop();
}
