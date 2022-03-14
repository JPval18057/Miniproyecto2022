/*
 * Miniproyecto
 * Código únicamente de los websockets
 */

//Libraries
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>


//****************************************************************************************
//BlinkLED
//Variables

// constants won't change. Used here to set a pin number:
const int ledPin =  2;// the number of the LED pin

// Variables that change
int ledState = LOW;             // ledState used to set the LED
unsigned long currentMillis = 0; //Stores the current time

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change
const long interval = 1000;           // interval at which to blink (milliseconds)

//Function declaration
void blinkled(void);

//****************************************************************************************
//ACCESS POINT WEBSERVER
//VARIABLES

//CONSTANTS
const char* ssid = "MINIPROYECTO";  
const char* password = "miniproyecto"; // opcional softAP()

//DECLARATIONS

//FUNCTIONS
void Webserversetup();

//JSON PROTOCOL
StaticJsonDocument<200> doc_tx;
StaticJsonDocument<200> doc_rx;

//WEBSERVER PORTS
WebServer  server(80);  //80 is de default port for webservers
WebSocketsServer webSocket = WebSocketsServer(81); //PORT 81 IS THE DEFAULT PORT FOR WS

//****************************************************************************************
//PID CONTROLLER






//Configuration of everything
void setup() {
  //WEBSERVER CONFIGURATION
  Webserversetup();
  //INDICATOR LED CONFIGURATION
  pinMode(ledPin, OUTPUT);
}

//The actual code running in the background
void loop() {
  //VERY IMPORTANT FOR THE CORRECT BEHAVIOUR OF THE SERVER
  server.handleClient();
  webSocket.loop();
  currentMillis = millis();

  //Blink BLUE BUILTIN LED
  blinkled();
}

//****************************************************************************************
/*
 *                    HERE ARE ALL THE FUNCTIONS USED IN THE CODE
 */
//****************************************************************************************

 //Blinks the led every interval
 void blinkled(){
  
    if (currentMillis - previousMillis >= interval) {

    // if the LED is off turn it on and vice-versa:
    ledState = ~ledState;

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
    
    //WEBSOCKET TEST
    String jsonString ="";
    JsonObject object = doc_tx.to<JsonObject>();
    object["rand1"] = random(100);
    object["rand2"] = random(100);
    serializeJson(doc_tx, jsonString); //WE USE DOC_TX BECAUSE WE ARE SENDING FROM ESP32 TO WEBPAGE
    Serial.println(jsonString);
    webSocket.broadcastTXT(jsonString);
    // save the last time you blinked the LED
    // At the end to have the real time
    previousMillis = currentMillis;
  }
 }

//****************************************************************************************
//CONFIGURES THE ACCESS POINT

//CONFIGURES WEB SERVER
void Webserversetup(){
  Serial.begin(115200); //USE SERIAL COMUNICATION TO SEE IF THERE'S ANYTHING WRONG
  //CREATE THE ACCESS POINT
  WiFi.softAP(ssid, password); //THIS IS THE MOST SIMPLE CONFIG
  //WiFi.softAPConfig(ip, gateway, subnet);
  IPAddress ip = WiFi.softAPIP();
  
  //Serial.print("WEBSERVER NAME: ");
  //Serial.println(ssid);
  Serial.print("IP ADDRESS: ");
  Serial.println(ip);

  //HERE YOU CAN ADD THE HANLDERS FOR ANY EVENT
  server.on("/", handleConnectionRoot);
  server.onNotFound(handleNotFound);

  //TURN ON THE SERVER
  server.begin();
  webSocket.begin(); //START WEBSOCKETS
  webSocket.onEvent(webSocketEvent);
  Serial.println("ACCESS POINT SERVER IS ON..."); //MESSAGE FOR THE USER
}

//WEBPAGE TO UPLOAD THE FIRST TIME
String webpage = "<!DOCTYPE html>\n<html>\n<head>\n<title>ESP SERVER</title>\n</head>\n<body style='background-color: #EEEEEE;'>\n\n<span style='color: #003366;'>\n\n<h1>Lets generate a random number</h1>\n<p>The first random number is: <span id = 'rand1'>-</span></p>\n<p>The second random number is: <span id = 'rand2'>-</span></p>\n<p><button type='button' id='BTN_SEND_BACK'>\nSend info to ESP32\n</button></p>\n</span>\n\n<script>\nvar Socket;\ndocument.getElementById('BTN_SEND_BACK').addEventListener('click', button_send_back);\nfunction init() {\n\tSocket = new WebSocket('ws://' + window.location.hostname + ':81/');\n\tSocket.onmessage = function(event) {\n\t\tprocessCommand(event);\n\t};\n}\nfunction button_send_back(){\n\tvar guitar = {\n\tbrand: 'Gibson',\n\ttype: 'Les Paul Studio',\n\tyear: 2010,\n\tcolor: 'white'\n\t};\n\tSocket.send(JSON.stringify(guitar));\n}\nfunction processCommand(event){\n\tvar obj = JSON.parse(event.data);\t\n\tdocument.getElementById('rand1').innerHTML = obj.rand1;\n\tdocument.getElementById('rand2').innerHTML = obj.rand2;\n\tconsole.log(obj.rand1);\n\tconsole.log(obj.rand2);\n}\nwindow.onload = function(event) {\n\tinit();\n}\n</script>\n\n</body>\n</html>";

//****************************************************************************************


//****************************************************************************************
//HANDLERS FOR THE WEBPAGE
//ROOT HANLDER (THE FIRST TIME THE SERVER STARTS)
void handleConnectionRoot() {
  server.send(200, "text/html", webpage);
}

//DISPLAYS THE TEST NOT FOUND WHEN THE PAGE IS NOT FOUND
void handleNotFound(){
  server.send(404, "text/plain", "Not found");
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length){
  switch (type) {
    case WStype_DISCONNECTED: //CLIENT DISCONNECTS TO SERVER
      Serial.println("Client DISConnected");
      case WStype_CONNECTED: //CLIENT CONNECTS TO SERVER
      break;
    Serial.println("CLIENT CONNECTED");
      // Add code to do more stuff if wanted

      break;
    case WStype_TEXT: //WHEN CLIENT SENDS DATA
      DeserializationError error = deserializeJson(doc_rx, payload);
      if (error) {
        Serial.println("Error, something went wrong...");
        return;
      } else {
        const char* g_brand = doc_rx["brand"];
        const char* g_type = doc_rx["type"];
        const int g_year = doc_rx["year"];
        const char* g_color = doc_rx["color"];
        Serial.println("Received guitar info!");
        Serial.println("Brand:"+String(g_brand));
        Serial.println("Type:"+String(g_type));
        Serial.println("Year:"+String(g_year));
        Serial.println("Color:"+String(g_color));
      }
      break;
  }
}
//****************************************************************************************

//****************************************************************************************
//PID CONTROLLERS

//****************************************************************************************

