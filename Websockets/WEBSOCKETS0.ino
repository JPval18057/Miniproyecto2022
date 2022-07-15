/*
 * Miniproyecto
 * Código únicamente de los websockets
 * con lecturas de sensor y comunicación con servidor
 */

//Libraries
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

//Libraries for the IMU
#include <Wire.h>
#include <MPU6050_light.h>


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
void blinkled();

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
//                                 PID CONTROLLER
/*
 * create variables for ki, kp and kd
 * must be double data type
 */
//Constants
float kp_local = 1.0;
float kd_local = 0.0;
float ki_local = 0.0;
/*
 * This ones are not equal to the analogic constants
 * They have to absorb Ts
 */
//Processing variables start at 0
float ek = 0; //Substraction between the measurement and the reference
float ek_1 = 0; //previous  value for ek
float Ek = 0; //Sum of all the values of ek for the integration of ki
float eD = 0; //delta e for the derivation
const float ref = 0.5; //constant value, it is recomended to be a const float type
float yk = 0; //sensor reading
float uk = 0; //result of the diff eq
float uk_1 = 0; //previous result of the diff eq

//casting variables
uint8_t uk_integrer = 0; //8 bit variable that writes to DAC1
float unscale = 0; //Variable that stores the mapped value
const float lower_limit = -70.0; //Min measurement angle (deg)
const float high_limit = +70.0; //Max measurement angle (deg)

//Process function
void control();
//****************************************************************************************




//****************************************************************************************
//                                IMU VARIABLES
//IMU WIRE OBJECT
MPU6050 mpu(Wire);
//Variables

//Function declaration
void IMU_config();
//****************************************************************************************


//****************************************************************************************
//                                TIMER0 CONFIGURATION
hw_timer_t * timer0 = NULL; //reiniciar timer0
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;
int interrupt_counter = 0;

//INTERRUPT ON TIMER0
//TIMER0 IS THE TIMER THAT CONTROLS THE PID
void IRAM_ATTR onTimer0(){
  //INTERRUPTION ON TIMER0
  portENTER_CRITICAL_ISR(&timerMux0);
  /*
   * Your code goes here
   */
  control();
  interrupt_counter++;
  portEXIT_CRITICAL_ISR(&timerMux0);
}

//CONFIGURATION TIMER 0
void TIMER0_CONFIG();

//PID CONTROLLER
void TIMER0_PID();

//****************************************************************************************

//****************************************************************************************
//                                TIMER1 CONFIGURATION
hw_timer_t * timer1 = NULL; //reiniciar timer1
portMUX_TYPE timerMux1 = portMUX_INITIALIZER_UNLOCKED;
int interrupt_counter2 = 0;

//INTERRUPT ROUTINE TIMER 1
void IRAM_ATTR onTimer1(){
  //INTERRUPTION ON TIMER1  
  portENTER_CRITICAL_ISR(&timerMux1);
  interrupt_counter2++;
  portEXIT_CRITICAL_ISR(&timerMux1);
}

//CONFIGURATION TIMER 1
void TIMER1_CONFIG();

//TIMER 1 SUBROUTINE
void TIMER1_SERVER();

//****************************************************************************************



//****************************************************************************************
//Configuration of everything
void setup() {
  //WEBSERVER CONFIGURATION
  Webserversetup();
  //IMU CONFIGURATION
  IMU_config();
  //TIMERS CONFIGURATION
  TIMER0_CONFIG();
  TIMER1_CONFIG();
  //INDICATOR LED CONFIGURATION
  pinMode(ledPin, OUTPUT);
}

//****************************************************************************************


//****************************************************************************************
//The actual code running in the background
void loop() {
  //VERY IMPORTANT FOR THE CORRECT BEHAVIOUR OF THE SERVER
  server.handleClient();
  webSocket.loop();
  currentMillis = millis();
  //IMPORTANT FOR THE INTEGRATION OF THE GYRO INFORMATION AND COMPLEMENTARY FILTER
  mpu.update();

  //Blink BLUE BUILTIN LED
  blinkled(); //used for the initial stages but now a timer is used for more precision
  TIMER1_SERVER();
  /*
   * HERE YOU CALL A FUNCTION TO TAKE THE DATA FROM THE SENSOR
   * TURN IT INTO A DOUBLE AND SCALE IT UP IF DESIRED
   * PROCESS THE DATA
   * SCALE IT FROM LAST STEP TO 0-255 (DIVIDE BY 5 AND MULTIPLY BY 255)
   * CAST IT TO INTEGRER
   * USE dacWrite(DAC1, DATA)
   * USE TIMER 0 FOR THE PID CONTROLLER
   * USE TIMER 1 FOR SENDING DATA TO SERVER
   */
   TIMER0_PID();
}
//****************************************************************************************







//****************************************************************************************
/*
 *                    HERE ARE ALL THE FUNCTIONS USED IN THE CODE
 */
//****************************************************************************************

 //Blinks the led every interval
 void blinkled(){
  
    if (currentMillis - previousMillis >= interval) {   
    ledState = ~ledState;
    digitalWrite(ledPin, ledState);
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
String webpage = "<!DOCTYPE html>\n<html>\n<head>\n<title>ESP SERVER</title>\n<script src=\"https://code.highcharts.com/highcharts.js\"></script>\n<script src=\"https://code.highcharts.com/modules/exporting.js\"></script>\n<!-- optional -->\n<script src=\"https://code.highcharts.com/modules/offline-exporting.js\"></script>\n<script src=\"https://code.highcharts.com/modules/export-data.js\"></script>\n</head>\n<body style='background-color: #EEEEEE;'>\n\n<span style='color: #003366;'>\n\n<h1>ESP32 WEBSERVER FOR MPU6050</h1>\n<p>Datos normalizados a g=9.81</p>\n<p>Aceleracion eje x (m/s²): <span id = 'rand1'>-</span></p>\n<p>Aceleracion eje y (m/s²): <span id = 'rand2'>-</span></p>\n<p>Aceleracion eje z (m/s²): <span id = 'rand3'>-</span></p>\n<h2>Gyro</h2>\n<p>Gyro x (rad/s): <span id = 'rand4'>-</span></p>\n<p>Gyro y (rad/s): <span id = 'rand5'>-</span></p>\n<p>Gyro z (rad/s): <span id = 'rand6'>-</span></p>\n<h1>Valores PID</h1>\n\n<form>\n  <label for=\"kp\">Kp:</label>\n  <input type=\"number\" id=\"kp\" name=\"quantity\" step=\"0.000001\">\n  <label for=\"kd\">Kd:</label>\n  <input type=\"number\" id=\"kd\" name=\"quantity\" step=\"0.000001\">\n  <label for=\"ki\">Ki:</label>\n  <input type=\"number\" id=\"ki\" name=\"quantity\" step=\"0.000001\">\n</form>\n\n\n<p><button type='button' id='BTN_SEND_BACK'>\nSend info to ESP32\n</button></p>\n</span>\n<!-- AJAX CODE TO SEND INFORMATION TO ESP32 -->\n<script>\n//INCOMING DATA ARRAY\n/*\tThis array has to go to local webpage storage to be used later in the graph\n *\tThen it needs to be downloadable\n */\nlet incoming_data = [];\n//WEBSOCKET CODE\nvar Socket;\ndocument.getElementById('BTN_SEND_BACK').addEventListener('click', button_send_back);\nfunction init() {\n\tSocket = new WebSocket('ws://' + window.location.hostname + ':81/');\n\tSocket.onmessage = function(event) {\n\t\tprocessCommand(event);\n\t};\n}\nfunction button_send_back(ev){\n\tev.preventDefault(); //To prevent the form from reloading and reseting the input boxes\n\tvar guitar = {\n\tbrand: 'Gibson',\n\ttype: 'Les Paul Studio',\n\tyear: 2010,\n\tcolor: 'white'\n\t};\n\tlet PID = {\n\t\tkp: document.getElementById('kp').value,\n\t\tkd: document.getElementById('kd').value,\n\t\tki: document.getElementById('ki').value\n\t};\n\tSocket.send(JSON.stringify(PID));\n\tconsole.log(PID);\n}\nfunction processCommand(event){\n\tvar obj = JSON.parse(event.data);\t\n\tdocument.getElementById('rand1').innerHTML = obj.rand1;\n\tdocument.getElementById('rand2').innerHTML = obj.rand2;\n\tdocument.getElementById('rand3').innerHTML = obj.rand3;\n\tdocument.getElementById('rand4').innerHTML = obj.rand4;\n\tdocument.getElementById('rand5').innerHTML = obj.rand5;\n\tdocument.getElementById('rand6').innerHTML = obj.rand6;\n\t//Put the incoming sensor values to an array for easier graphing\n\tincoming_data.push(obj.rand7);\n\t//incoming_data.push(obj.rand2); //just grahping the pitch angle\n\t//console.log(incoming_data);\n\t//console.log(obj.rand1);\n\t//console.log(obj.rand2);\n\t\n\t//SAVING TO localStorage\n\t//THE JSON OBJECT NEEDS TO BE A STRING\n\t//REMEMBER TO PARSE IT WHEN YOU GET IT \n\t//let variable = localStorage.getItem(\"name_of_data\");\n\t//let object = JSON.parse(variable);\n\t//NOW YOU CAN USE THE DATA INSIDE OF THE OBJECT IN YOUR CODE\n\tlocalStorage.setItem(\"Data_esp\", JSON.stringify(incoming_data));\n}\nwindow.onload = function(event) {\n\tinit();\n}\n</script>\n\n<!--BUTTON FOR CONFIGURABLE N -->\n<form>\n<div>\n\t<label for=\"N_datos\">Cantidad de datos:</label>\n\t<input type=\"number\" id=\"N_datos\" name=\"quantity\" step=\"1\" value=\"40\">\n\t<button type='button' id='update_graph'>\n\tUpdate_graph\n\t</button>\n</div>\n</form>\n\n\n<!-- // IMU GRAPH -->\n<div id=\"container\" style=\"width:50%; height:300px;\"></div>\n<script>\nlet chart; //global chart\nlet memory; //global memory\nlet data; //global memory\nlet N_datos;\n//Event listener for the information shown on graph\ndocument.getElementById('update_graph').addEventListener('click', update_graph);\n\tfunction update_graph(ev1) {\n\t//ev1.preventDefault(); //Prevents page from reloading\n\tN_datos = document.getElementById('N_datos').value;\t\n\t}\n//ASK FOR DATA FROM localStorage EVERY SECOND\nsetInterval(function requestData() {\t\n\tmemory = localStorage.getItem(\"Data_esp\");\n\tdata = JSON.parse(memory);\n\tlet data_size = data.length - 1; //Size of the array\n\tconst point = data[data_size]; //Obtain the latest point in the graph\n\tconst series = chart.series[0],\n\t\tshift = series.data.length > N_datos; //Start shifting data if there is more than 20 entrys\n\t\t//THE NUMBER 20 IS THE NUMBER OF DATA SHOWN IN THE GRAPH\n\t\t\n\t//add the point to the graph\n\tchart.series[0].addPoint(point, true, shift);\n\t\n\t//console.log(data);\n}, 500);\n\n\ndocument.addEventListener('DOMContentLoaded', function () {\n        chart = Highcharts.chart('container', {\n            chart: {\n                type: 'spline',\n\t\t\t\tzoomType: 'x',\n\t\t\t\tpanning: true,\n\t\t\t\tpankey: 'shift',\n            },\n\t\t\tcredits: {\n\t\t\t\tenabled: false\n\t\t\t}\n\t\t\t,\n            title: {\n                text: 'Angulo del balancin'\n            },\n\t\t\tsubtitle: {\n\t\t\t\ttext: 'Click and drag to zoom in. Hold down shift key to pan.'\n\t\t\t},\t\t\t\n            xAxis: {\n                //categories: []\n\t\t\t\tscrollbar: {\n\t\t\t\tenabled: true\n\t\t\t\t}\n            },\n            yAxis: {\n                title: {\n                    text: 'Angulo (deg)'\n                }\n            },\n            series: [{\n                name: 'Angulo Z (deg)',\n\t\t\t\tshowInNavigator: true,\n                data: [] //incoming_data\n            }]\n\t\t\t\n        });\n    });\n\n</script>\n\n</body>\n</html>";

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
        const float kp = doc_rx["kp"];
        const float kd = doc_rx["kd"];
        const float ki = doc_rx["ki"];
        //const char* g_color = doc_rx["color"];
        Serial.println("Received PID info!");
        Serial.print("kp:");
        Serial.println(kp,5);
        Serial.print("kd:");
        Serial.println(kd,5);
        Serial.print("ki:");
        Serial.println(ki,5);
        //Serial.println("Color:"+String(g_color));
        //Serial.println(data, Format)
        //Data can be any type of data
        //Format is BIN,DEC,HEX,OCT for integrers and the decimal places for float and doubles.

        /*
         * HERE YOU ADD THE CODE TO MODIFY THE CONSTANTS
         * KP = kp;
         * KD = kd;
         * KI = ki;
         */
         kp_local = kp;
         kd_local = kd;
         ki_local = ki;
      }
      break;
  }
}
//****************************************************************************************

//****************************************************************************************
//                      
void IMU_config(){
  //START THE WIRE I2C LIBRARY
  Wire.begin();  
  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while(status!=0){ } // stop everything if could not connect to MPU6050
  
  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets(true,true); // gyro and accelero
  Serial.println("Done!\n");
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
 * The angle of reference is -1.9 degrees on Z. (horizontal position)
 */
//****************************************************************************************
//                    TIMER CONFIG

void TIMER0_CONFIG(){
  //CONFIG TIMER0
  //TIMER 0 IS THE PID TIMER AND NEEDS TO BE AS FAST AS POSIBLE try a period of 1ms
  Serial.println("start timer0");
  timer0 = timerBegin(0, 80, true); // timer 0, MWDT clock period = 12.5 ns * TIMGn_Tx_WDT_CLK_PRESCALE -> 12.5 ns * 80 -> 1000 ns = 1 us, countUp
  timerAttachInterrupt(timer0, &onTimer0, true); // edge (not level) triggered 
  timerAlarmWrite(timer0, 500000, true); // 250000 * 1 us = 250 ms, autoreload true
  timerAlarmEnable(timer0);
}

void TIMER1_CONFIG(){
  //TIMER 1 SENDS DATA TO SERVER EVERY 1 SECOND but after testing increase the frequency to 500ms
  //CONFIG TIMER1
  Serial.println("start timer1");
  timer1 = timerBegin(1, 80, true);  // timer 1, MWDT clock period = 12.5 ns * TIMGn_Tx_WDT_CLK_PRESCALE -> 12.5 ns * 80 -> 1000 ns = 1 us, countUp
  timerAttachInterrupt(timer1, &onTimer1, true); // edge (not level) triggered 
  timerAlarmWrite(timer1, 1000000, true); // 250000 * 1 us = 1000 ms, autoreload true
  timerAlarmEnable(timer1);
}

//****************************************************************************************
//                    TIMER PID AND SERVER
/*
 * When you read this remember that you have to move the code from millis()
 * to TIMER1_SERVER
 * and read the sensors and process the data in TIMER0_PID
 */
void TIMER1_SERVER() {
  if (interrupt_counter2==1) {
    //Serial.println("onTimer1: Server");
    //WEBSOCKET TEST
    String jsonString ="";
    JsonObject object = doc_tx.to<JsonObject>();
    /*
     * HERE YOU GRAB THE VALUES FROM THE SENSOR AND SEND THEM INSTEAD OF RANDOM(100)
     */
    object["rand1"] = mpu.getAccX();//aceleration x
    object["rand2"] = mpu.getAccY();//aceleration y
    object["rand3"] = mpu.getAccZ();//aceleration z
    object["rand4"] = mpu.getGyroX();//Gyro x
    object["rand5"] = mpu.getGyroY();//Gyro y
    object["rand6"] = mpu.getGyroZ();//Gyro z    
    object["rand7"] = mpu.getAngleZ();//Angle Z
    serializeJson(doc_tx, jsonString); //WE USE DOC_TX BECAUSE WE ARE SENDING FROM ESP32 TO WEBPAGE
    //Serial.println(jsonString); //DISPLAY IN THE SERIAL MONITOR
    webSocket.broadcastTXT(jsonString);
    interrupt_counter2 = 0;
  }
    
}

void TIMER0_PID() {
  if (interrupt_counter==1) {
    /*    
     *     CODE TO EXECUTE WITH INTERRUPT GOES HERE
     */
    interrupt_counter = 0;
  }
  
}
//****************************************************************************************
//                                   PID CONTROLLER CORE

void control(){
  //Here you scale the values
  yk = mpu.getAngleZ(); //Get the angle value, it is already a float
  
  //Everything from here on has to be a float and scaled
  //Equation of differences
  ek = ref - yk;
  eD = ek - ek_1;
  Ek = Ek + ek;
  uk = kp_local*ek + ki_local*Ek + kd_local*eD; //Difference eq for control
  //updates in code
  ek_1 = ek;
    
  //Here you convert from a float to an integrer of 8 bits
  unscale = map(uk, lower_limit, high_limit, 0, 255); //Adjust the range
  uk_integrer = (uint8_t) unscale; //adjust the data type
  
  //Use that integrer to write to DAC1
  //ANALOG OUTPUT OF THE CONTROLLER THAT GOES INTO THE DRIVER
  //dacWrite(DAC1, uk_integrer);
  //Just for debugging
  //Serial.println(uk_integrer, DEC); //show the pid result in the console
  /*
   * Remainder:
   * 0 -> -6V and 255 -> 6V
   * 127 -> 0V
   * Always check the physical voltages to ensure best performance
   * Check the gain
   * Check the adjustment pot (1.65V)
   * Check both ends of the signal 0 and 255 and make sure they are in range.
   */
}


//****************************************************************************************
