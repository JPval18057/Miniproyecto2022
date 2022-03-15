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
/*
 * create variables for ki, kp and kd
 * must be double data type
 */





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
  /*
   * HERE YOU CALL A FUNCTION TO TAKE THE DATA FROM THE SENSOR
   * TURN IT INTO A DOUBLE AND SCALE IT UP IF DESIRED
   * PROCESS THE DATA
   * SCALE IT FROM LAST STEP TO 0-255 (DIVIDE BY 5 AND MULTIPLY BY 255)
   * CAST IT TO INTEGRER
   * USE dacWrite(DAC1, DATA)
   */
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
    /*
     * HERE YOU GRAB THE VALUES FROM THE SENSOR AND SEND THEM INSTEAD OF RANDOM(100)
     */
    object["rand1"] = random(100);
    object["rand2"] = random(100);
    serializeJson(doc_tx, jsonString); //WE USE DOC_TX BECAUSE WE ARE SENDING FROM ESP32 TO WEBPAGE
    //Serial.println(jsonString); //DISPLAY IN THE SERIAL MONITOR
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
String webpage = "<!DOCTYPE html>\n<html>\n<head>\n<title>ESP SERVER</title>\n<script src=\"https://code.highcharts.com/highcharts.js\"></script>\n</head>\n<body style='background-color: #EEEEEE;'>\n\n<span style='color: #003366;'>\n\n<h1>ESP32 WEBSERVER FOR MPU6050</h1>\n<p>Angulo normalizado eje y: <span id = 'rand1'>-</span></p>\n<p>Angulo normalizado eje x: <span id = 'rand2'>-</span></p>\n<p>Angulo normalizado eje z: <span id = 'rand3'>-</span></p>\n<h2>Gyro</h2>\n<p>Gyro x (rad/s): <span id = 'rand2'>-</span></p>\n<p>Gyro y (rad/s): <span id = 'rand2'>-</span></p>\n<p>Gyro z (rad/s): <span id = 'rand2'>-</span></p>\n<h1>Valores PID</h1>\n\n<form>\n  <label for=\"kp\">Kp:</label>\n  <input type=\"number\" id=\"kp\" name=\"quantity\" step=\"0.00001\">\n  <label for=\"kd\">Kd:</label>\n  <input type=\"number\" id=\"kd\" name=\"quantity\" step=\"0.00001\">\n  <label for=\"ki\">Ki:</label>\n  <input type=\"number\" id=\"ki\" name=\"quantity\" step=\"0.00001\">\n</form>\n\n<p><button type='button' id='BTN_SEND_BACK'>\nSend info to ESP32\n</button></p>\n</span>\n//AJAX CODE TO SEND INFORMATION TO ESP32\n<script>\nvar Socket;\ndocument.getElementById('BTN_SEND_BACK').addEventListener('click', button_send_back);\nfunction init() {\n\tSocket = new WebSocket('ws://' + window.location.hostname + ':81/');\n\tSocket.onmessage = function(event) {\n\t\tprocessCommand(event);\n\t};\n}\nfunction button_send_back(ev){\n\tev.preventDefault();\n\tvar guitar = {\n\tbrand: 'Gibson',\n\ttype: 'Les Paul Studio',\n\tyear: 2010,\n\tcolor: 'white'\n\t};\n\tlet PID = {\n\t\tkp: document.getElementById('kp').value,\n\t\tkd: document.getElementById('kd').value,\n\t\tki: document.getElementById('ki').value\n\t};\n\tSocket.send(JSON.stringify(PID));\n\tconsole.log(PID);\n}\nfunction processCommand(event){\n\tvar obj = JSON.parse(event.data);\t\n\tdocument.getElementById('rand1').innerHTML = obj.rand1;\n\tdocument.getElementById('rand2').innerHTML = obj.rand2;\n\tconsole.log(obj.rand1);\n\tconsole.log(obj.rand2);\n}\nwindow.onload = function(event) {\n\tinit();\n}\n</script>\n\n// IMU GRAPH\n<div id=\"container\" style=\"width:50%; height:300px;\"></div>\n<script>\ndocument.addEventListener('DOMContentLoaded', function () {\n        const chart = Highcharts.chart('container', {\n            chart: {\n                type: 'line'\n            },\n            title: {\n                text: 'Ángulo del balancín'\n            },\n            xAxis: {\n                categories: []\n            },\n            yAxis: {\n                title: {\n                    text: 'Ángulo (rad)'\n                }\n            },\n            series: [{\n                name: 'Eje x',\n                data: [1, 0, 4, 5, 1, 8]\n            }, {\n                name: 'Eje y',\n                data: [5, 7, 3, 7, 9, 3]\n            }]\n        });\n    });\n</script>\n\n</body>\n</html>";

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
        const double kp = doc_rx["kp"];
        const double kd = doc_rx["kd"];
        const double ki = doc_rx["ki"];
        //const char* g_color = doc_rx["color"];
        Serial.println("Received PID info!");
        Serial.println("kp:"+String(kp));
        Serial.println("kd:"+String(kd));
        Serial.println("ki:"+String(ki));
        //Serial.println("Color:"+String(g_color));

        /*
         * HERE YOU ADD THE CODE TO MODIFY THE CONSTANTS
         * KP = kp;
         * KD = kd;
         * KI = ki;
         */
      }
      break;
  }
}
//****************************************************************************************

//****************************************************************************************
//PID CONTROLLERS
/*
 * To DO list
 * Make a function that processes the data
 * Figure out how to scale the data
 * Difference equation
 * Unscale data
 * write to dac1
 * for the final version of the code turn off every serial comunication
 * except for the incoming data of the webpage
 * if necesary use a timer to process data
 * REMEMBER TO TURN DOWN VOLTAGE TO 6V APROX. TO LOWER THE SPEED
 */
//****************************************************************************************
