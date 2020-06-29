/*
 * 
 * LINE 119 need to have some sort of serial Terminal where we can prompt for SSID and KEY, as well as Twitch Credentials
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
 WS2812 Pin =     5 - D1 - GPIO5
 MAX7219 Data =   HMOSI - D7 - GPIO13
 MAX7219 Clock =  HSCLK - D5 - GPIO14 
 MAX7219 Load =   15 - D8 - GPIO15
 Button 1  =      4 - D2 - GPIO4 
 Button 2  =      0 - D3 - GPIO0
 */

//Powering the RGBs and the MAX7219 ONLY! with 5v (Vin) but using 3.3v logic. 
//seems to work well but causes the MCU to not reset? capacitance on the line? OR is there something wrong with this board? LM117 or usb port?
//replaced the diode on the board with a sr24 and the problem above was resolved


//setLEDxy(x,y,1); // is how to set a pixel. x=0 to 7, y = 0 to 7 right now. the 3rd term is 0 ro 1 for on and off
//lmd.display(); //must be executed after updating pixels so they get pushed to the diplay

//setPixelColor(i,c); //is how to set a rgb pixel color. where i is 0 to 89 and c is a 32bit number.
//FastLED.show(); must be exectued after updating pixels so they get pushed tp the display


#define BTN1_PIN  4 // D2 - GPIO4 
#define BTN2_PIN  0 // D3 - GPIO0
#define LOAD_PIN  15 // D8 - GPIO15
#define LED_PIN   5 // D1 - GPIO5

#include "CharTable.h"
#include <LEDMatrixDriver.hpp>
#define numCols 8
#define numRows 8
bool buffTemp[numCols][numRows]; //screen buffer-ish //screen buffer-ish (bool uses same amount of ram as byte)! this is inefficient this should be  byte[32]

//maps library to hardware// depends on board
//byte X1[16] = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};
byte X1[16] = {7,6,5,4,3,2,1,0,7,6,5,4,3,2,1,0};
byte Y2[16] = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};
byte Y3[16] = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};

//byte X1[16] = {7,6,5,4,3,2,1,0,7,6,5,4,3,2,1,0};
//byte Y2[16] = {8,15,14,13,12,11,10,9,24,31,30,29,28,27,26,25};
//byte Y3[16] = {0,7,6,5,4,3,2,1, 16,23,22,21,20,19,18,17};
byte myOffset = 4; //  "center" of screen. width/2 = 16/2 = 8


#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ALLOW_INTERRUPTS 0 //this may or may not prevent "flickering" (prevents INTERUPTS DURING show();)
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
#define NUMPIXELS  89
#define LED_TYPE  WS2812B
#define BRIGHTNESS  255     // a little dim for recording purposes
#define COLOR_ORDER GRB
// declare the LED array
CRGB leds[NUMPIXELS];

 
uint32_t Forecolor = 0x000800;
uint8_t Brange = 8; //1, 3, 5 //set the brightness for each chanel in getrand();
uint8_t green; //used in getrand
uint8_t red;   //used in getrand
uint8_t blue;  //used in getrand

const uint8_t LEDMATRIX_CS_PIN = LOAD_PIN;
const int LEDMATRIX_SEGMENTS = 1;
const int LEDMATRIX_WIDTH    = LEDMATRIX_SEGMENTS * 8;
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);

void setup() {
 lmd.setEnabled(true);
 lmd.setIntensity(2);   // 0 = low, 10 = high
  
 pinMode(LED_BUILTIN, OUTPUT);
 pinMode(BTN1_PIN,INPUT);
 pinMode(BTN2_PIN,INPUT);

 FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUMPIXELS).setCorrection(TypicalLEDStrip);
 FastLED.setBrightness(BRIGHTNESS);
 FastLED.clear(true);

 WiFi.begin(ssid,password);
 Serial.begin(115200);
 LedTest(20); //quick led hardware test
 Serial.print("Connecting to WIFI: ");
 Serial.print(SECRET_SSID);
 
 while(WiFi.status() != WL_CONNECTED) { //the events while the Wifi is not yet connected
    static uint8_t i = 0;
    if(i=10){
      Serial.print("\n ----Press 1 to enter a new SSID and Key---- \n");
    }
    //do a Serial read here. check for "1"    <--- need to write this code to read serial inputs and store them into the SSID
    //then jump to a function that will prompt for ssid and key one at a time. i.e. "Enter SSID and press Enter"
    digitalWrite(LED_BUILTIN,1);   
    Serial.print(".");
    delay(250);
    digitalWrite(LED_BUILTIN,0);
    delay(250);
    digitalWrite(LED_BUILTIN,0);
 }
 digitalWrite(LED_BUILTIN,0);  //<--- to get to here the wifi needs to connect to a SSID
 Serial.println();
 Serial.print("IP Address: "); Serial.println(WiFi.localIP());
 printStringWithShiftL("CONNECTED !!!   ", 80,2); //see chartable for valid characters. all caps a few symbols, etc
char ipCharArray[20];
IPAddress ip = WiFi.localIP();
sprintf(ipCharArray, "%d.%d.%d.%d   ", ip[0], ip[1], ip[2], ip[3]);
 printStringWithShiftL(ipCharArray, 80,2); //Print he local IP on the LED matrix
 
 webSocket.begin("irc-ws.chat.twitch.tv", 80, "/");  // server address, port, and URL path
 webSocket.onEvent(webSocketEvent);  // event handler
 webSocket.setReconnectInterval(5000);  // try ever 5000 again if connection has failed
}



void loop() {
 // webSocket.loop(); //uncomment this to use websockets
 PollButtons();
}



void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    // If the websocket connection is succesful
    case WStype_CONNECTED:
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

    delay(500);
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
    //setRgb(i, r, g, b);
    setPixelColor(i,Color(g,r,b));
    FastLED.show();
  }
}

void setPixelColor(int i, uint32_t c){ //input a 32-bit color. write the g,r,b to led[i] color buffer
  leds[i].g = ((c >> 16) & 0xFF); //Green
  leds[i].r = ((c >> 8) & 0xFF); //Red
  leds[i].b = ((c >> 0) & 0xFF); //Blue
}
uint32_t Color(uint8_t g, uint8_t r, uint8_t b) {
  return ((uint32_t)g << 16) | ((uint32_t)r <<  8) | b;
}



void PollButtons(){ //button 1 interupt FALLING EDGE
  if(!digitalRead(BTN1_PIN)){
    //cli();//stop interrupts
    lmd.clear();
    while (digitalRead(BTN1_PIN) == LOW) delay(10);
    // do stuff
    Serial.printf("Button 1\n");
     printStringWithShiftL("1", 10,2); 
  }

  if(!digitalRead(BTN2_PIN)){
    lmd.clear();
    while (digitalRead(BTN2_PIN) == LOW) delay(10);
    //do stuff
     Serial.printf("Button 2\n");
     printStringWithShiftL("2", 10,2); 
  }
  
}





void BtnTest(){
  if(digitalRead(BTN1_PIN)==LOW){ //BTN1 Test
    Serial.println("BTN1 Pressed");
    for(int x=0; x<8; x++){
      int y= 4;
       lmd.setPixel(x,y,1);
       lmd.display();
       delay(10);
      }
   }
  
  
  if(digitalRead(BTN2_PIN)==LOW){ //BTN2 Test
    Serial.println("BTN2 Pressed");
    for(int x=0; x<8; x++){
      int y= 4;
       lmd.setPixel(y,x,1);
       lmd.display();
       delay(10);
      }
   }
   ClearMatrix();
}

void ClearMatrix(){
for (int x=0; x<8; x++){
  for (int y=0; y<8; y++){
    lmd.setPixel(x,y,0);
  }
}
lmd.display();
}


void rotateScreen(float angPercent, int dir){ //rotates all pixels theta rads counter clockwise
//dir: 1 for CCW, -1 for CW
storeScreen();
 int angleIndex = 0;
 for(float i=0; i<=2*angPercent; i+=.05){
  PollButtons();
  for(int x=0; x<16; x++){
    for(int y=0; y<16; y++){
        int myXT = (int) (myOffset+(((x-myOffset)*cos(M_PI*i))-(dir)*((y-myOffset)*sin(M_PI*i))));
        int myYT = (int) (myOffset+(((x-myOffset)*(dir)*sin(M_PI*i))+((y-myOffset)*cos(M_PI*i))));
        SetLEDxy(myXT,myYT,buffTemp[x][y]);               
    }
    }
  lmd.display();
  }
  if(ceilf(angPercent) == angPercent) printStored(); //if angle to rotate is integer value then fix rotation error by printing the tempbuff
}

void storeScreen(){ //copy every mono led state to a buffer called bitmap
  for(int x=0; x<16; x++){
    for(int y=0; y<16; y++){
      buffTemp[x][y] = GetLEDxy(x,y);
    }
  }
}

void printStored(){ //print the buffer called bitmap
  for(int x=0; x<16; x++){
    for(int y=0; y<16; y++){
      SetLEDxy(x,y,buffTemp[x][y]);
    }
  }
lmd.display();
}


void SetLEDxy(int x, int y, bool c){ //just pass in cartesian (x,y) pairs
    if(x<8){
    lmd.setPixel(Y2[y], X1[x], c);
    }else{
    lmd.setPixel(Y3[y], X1[x], c);
    }
}

bool GetLEDxy(int x, int y){ //just pass in cartesian (x,y) pairs
    if(x<8){
      return lmd.getPixel(Y2[y], X1[x]);
    }else{
      return lmd.getPixel(Y3[y], X1[x]);
    }
}



void drawBitmap(char* image){ //draws a bitmap from a hex array defined at top
  byte Tempbuff = 0;
  int mySide = 1;
  int myYY = 0;
  for(int x=0; x<32; x++){
    Tempbuff = pgm_read_byte(&(image[x]));
    uint8_t b = 0b10000000;
    if(x%2==0){
      mySide = 0;
      if(x) myYY++;
      }else{
      mySide = 8;
    }
    for (int k = 0; k < 8; k++) {
      if (!(Tempbuff&b)) SetLEDxy(mySide+k,numRows-1-myYY,1);
      b = b >> 1;
    }
  }
  lmd.display();
}


void FillScreen(bool c){
    for(int x=0; x<16; x++){
    for(int y=0; y<16; y++){
    SetLEDxy(x,y,c);
    }
  }
}

//Extract characters for Static Text
void printString(char* s, int x, int y){
  while (*s != 0) {
    printChar(*s, x, y);
    s++;
    x+=4;
  }
}

// Put extracted character on Display
void printChar(char c, int x, int y){
  byte mybuff;
  if (c < 32) return; //error check for ascii values less than ASCII  < ' ' (space) = 32 >
  c -= 32;      //align the char ascii with the CharTable
  for (int j = 0; j < 3; j++) {
    mybuff = pgm_read_byte(&(CH[3*c+j]));
    uint8_t b = 0b00000001;
    for (int k = y; k < y+5; k++) {
      if (mybuff&b) SetLEDxy(j+x,k,1);
      b = b << 1;
    }

  }
}

//Extract characters for Scrolling text -PHASE THESE OUT? same as above but with shiftL()
void printStringWithShiftL(char* s, int shift_speed, int textpos) {
  while (*s != 0) {
    printCharWithShiftL(*s, shift_speed, textpos);
    s++;
  }
}

// Put extracted character on Display
void printCharWithShiftL(char c, int shift_speed, int textpos) {
byte mybuff;
  if (c < 32) return; //error check for ascii values less than ASCII  < ' ' (space) = 32 >
  c -= 32; //align the char ascii with the CharTable
  for (int j = 0; j <= 2; j++) {
  mybuff = pgm_read_byte(&(CH[3*c+j]));
    uint8_t b = 0b00000001;
    for (int k = textpos; k < textpos+5; k++) {
      if (mybuff&b) SetLEDxy(numCols-1,k,1);
      b = b << 1;
    }
    lmd.display();
    ShiftLEDLeft(); //shift every column one column left (one space between letters)
    BlankColumn(numCols-1);
    delay(shift_speed);
  }
  ShiftLEDLeft(); // (one space after word)
}

//shift mono leds Left function
void ShiftLEDLeft () {   //Shift columns right 1 palce
  for ( int x = 1; x <= numCols-1; x++) { //read one behind left
    for (int y = 0; y <= numRows-1; y++) { //read each Led value
      SetLEDxy(x-1,y,GetLEDxy(x,y));
    }
  }
  lmd.display();
}

//shift mono leds Left function
void ShiftLEDDown () {   //Shift columns right 1 palce
  for (int y = numRows-2; y >= 0; y--) { //read one behind left
    for (int x = 0; x <= numCols-1; x++) { //read each Led value
      SetLEDxy(x,y+1,GetLEDxy(x,y));
    }
  }
  lmd.display();
}

void BlankColumn(int c) {    //wipes the coulmn 0 to clear out jjunk, to get ready for shifting
  for (int w = 0; w <= numRows-1; w++) {
    SetLEDxy(c,w,0);
  }
}

void BlankRow(int c) {    //wipes the coulmn 0 to clear out jjunk, to get ready for shifting
  for (int w = 0; w <= numCols-1; w++) {
    SetLEDxy(w,c,0);
  }
}

/*
 * //need to change this to read the leds[] array
void RotRGBRight(){ //shifts all RGB pixels right 1 place
    for(int i=NUMPIXELS-1; i>0; i--){
    setPixelColor(i, getPixelColor(i-1)); // Moderately bright green color.
  }
}
*/
void ColorWipe(uint32_t c, int s){
  for(int i=0;i<NUMPIXELS;i++){
    setPixelColor(i, c); // Moderately bright green color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    delay(s); // Delay for a period of time (in milliseconds).
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

void LedTest(int s){
  for(int x=0; x<numCols-1; x++){
    for(int y=0; y<numRows-1; y++){
      SetLEDxy(y,x,1);
      lmd.display();
      delay(s);
      SetLEDxy(y,x,0);
      lmd.display();
      delay(s);
    }
  }
  
  ColorWipe(0x0A0000,10);
  delay(300);
  ColorWipe(0x000A00,10);
  delay(300);
  ColorWipe(0x00000A,10);
  delay(300);
  ColorWipe(0,10);
  
  for(int x=numCols-1; x>=0; x--){
    for(int y=numRows-1; y>=0; y--){
      SetLEDxy(y,x,0);
      lmd.display();
      delay(s);
    }
  }
}
