/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Global definitions
//Wifi  Definitions
const char* ssid     = "****";
const char* password = "****";
IPAddress local_IP(*, *, *, *);  //Set fixed local IP
IPAddress gateway(*, *, *, *);  //Router
IPAddress subnet(*, *, *, *);
IPAddress primaryDNS(*, *, *, *); 
IPAddress secondaryDNS(*, *, *, *); 

//Sonos server data
const char* SonosHost = "*.*.*.*";  //Sonos Server IP
const uint16_t SonosPort = *;  //Sonos service port
#define ROOM_CLIP "/room/clip/clip1.mp3"  //optional clips see Sonos API
#define HOME_CLIP "/clipall/clip2.mp3"    //another optional clips see Sonos API
const String SoundUri = ROOM_CLIP;  //Selected clip to play

//Local Server for Gate Open requests
AsyncWebServer server(80);
#define GATE_OPEN_URI "/open"

//Logger Remote Server
const char* LoggerHost = "*.*.*.*";  //Sonos Server IP
const uint16_t LoggerPort = *;  //Sonos service port
const String LogUri = "/Door-Bell/log";  //Simple logger service definition

//NTP server and time zone settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 3600;

//Digital pins defintions
//Bell digital pin
#define BELL_PIN 2 //Bell Digital Pin
byte BellPin = BELL_PIN;
#define BELL_DELAY 5000  //Bell delay between presses
unsigned long bell_time = 0; //time counter to set delay
bool Bell = false;  //Bell flag

//Gate open digital Pin
#define OPEN_PIN 4    //Gate OPen pin
byte OpenPin = OPEN_PIN;
bool OpenGate = false; //Gate open flag

//Generic utils
//Wifi event manager
void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection...Restart ESP");
        logit("WiFi lost connection...Restart ESP");
        ESP.restart();
        break;
    }
}
//Simple wrapper for HTTP Requests (get/post) 
void HttpRequest(String HttpServer,uint16_t ServerPort,String ReqUri,String PostData = "") { //If post data is "" its a get request
    bool httpget = (PostData == "");
    HTTPClient http;
    http.begin(HttpServer,ServerPort,ReqUri);
    int httpCode;
    if (httpget) {  
        httpCode = http.GET();  //Make the request
    } else {
        http.addHeader("Content-Type", "text/plain");        //Specify content-type header
        httpCode = http.POST(String(PostData) + "\r\n");   //Send the actual POST request
    }
 
    if (httpCode > 0) { //Check for the returning code
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      }
    else {
      Serial.println("Error on HTTP request");
      Serial.println(httpCode);
    }
 
    http.end(); //Free the resources
}

//Get Local Time Time
String LocalTime()
{
  struct tm timeinfo;
  char lt[20];
  int n;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "Time Error";
  }
  strftime(lt, 20, "%F %H:%M:%S",&timeinfo);
  return String(lt);
  //Serial.println(&timeinfo, "%F %H:%M:%S");
}

//Log data to logger server
void logit(String message) {
   HttpRequest(LoggerHost,LoggerPort,LogUri,LocalTime() + "-" + message);
}   

//Application utils
// Event handler fo Bell press detection 
void BellHandler()
{
  //Resong for only raising flag it to keep handler short and loose all unhandled conncutive interrupts
  noInterrupts(); //block interrupts and only raise global flag here
  Bell =true;
  interrupts();
}

//Wrapper to Sonos play command
void play_sonos(String play){
   HttpRequest(SonosHost,SonosPort,play,"");
}

//Wrapper to open gate command
void open_gate()
{
  digitalWrite(OPEN_PIN, HIGH);   // Set gate relay pin HIGH
  //Wait for 0.5 sec (we dont mind blocking the loop since no need to handle bell events while we are openenig the gate 
  delay(500);
  digitalWrite(OPEN_PIN, LOW);
  OpenGate = false; 
}

void bell_pressed()    //manage bell pressed 
{
  if(millis() - bell_time > BELL_DELAY)  {  //check if we passed allowed delay between bells
     play_sonos(SoundUri);  //Send Sonos play request
     bell_time = millis(); //save the time
  } else {  // the request is not valid delay not passed
     Serial.println("Bell delay not passed");
  }
  noInterrupts();
  Bell = false;  //reset ball event flag
  interrupts();
}

void setup()
{
    //initialize serial terminal
    Serial.begin(115200);
    
    // Connect to a WiFi network
    //Configure wifi STA
    if (!WiFi.config(local_IP, gateway, subnet,primaryDNS,secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }
    WiFi.onEvent(WiFiEvent);  //Define event handler
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);  //Start wifi STA
    while (WiFi.status() != WL_CONNECTED) {  //1 second delay loop until connect
        delay(1000);
        Serial.print(".");
    }
    
    //Initializing digital pins
    pinMode(BellPin, INPUT_PULLUP);
    pinMode(OpenPin, OUTPUT);
    
    //NTP time configuration (get NTP time offsetted to local time zone) 
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    //Define Server response event
    server.on(GATE_OPEN_URI, HTTP_GET, [](AsyncWebServerRequest *request){  //We set the URI to "/open" but can be any
      request->send(200, "text/plain", "Ok Open Relay Sent....");  //first respond 
      OpenGate = true; //raise flag
    });
    
    //Start local server to handle open gate requests
    Serial.println("Starting local server on port 80");
    Serial.println("Send http://Local Server IP/open to open gate");
    Serial.println("Go To http://Logger Server IP:PORT to see logs");
    server.begin();
    
    // set bell pin interrupt handler 
    attachInterrupt(digitalPinToInterrupt(BellPin), BellHandler, CHANGE);
}

void loop()
{
    //Test if Bell pressed
    if (Bell) {
        Serial.println("Bell Pressed...Playing Sonos Clip");
        logit("Bell Pressed...Play Sonos Clip");
        bell_pressed();
    }

    //Test is OPen Gate reques
    if (OpenGate) {
        Serial.println("Open-Gate Request...Activating Gate Relay....");
        logit("Open-Gate Request...Activating Gate Relay....");
        open_gate();
    }
    
}

