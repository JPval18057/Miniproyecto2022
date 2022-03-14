
//Incluimos las librerías
#include <WiFi.h>
#include <WebServer.h>
//Websockets para actualizar automáticamente
#include <SocketIOclient.h>
#include <WebSockets.h>
#include <WebSockets4WebServer.h>
#include <WebSocketsClient.h>
#include <WebSocketsServer.h>
#include <WebSocketsVersion.h>



//Definitions
#define LED 2

//Variables
// Creamos nuestra propia red -> SSID & Password
const char* ssid = "Miniproyecto_Juampa";  
const char* password = "miniproyecto"; // opcional softAP()

//Variables del programa
double t0 = 0;
double t1 = 0;
double interval = 500;
int led_value = LOW;

//Variables de comunicación


// Definimos la IP local (a veces genera conflictos al usar pines)
/*
IPAddress ip(192,168,4,22);         //(192, 168, 1, 1)
IPAddress gateway(192,168,4,9);     //(192, 168, 1, 1)
IPAddress subnet(255,255,255,0);    //(255, 255, 255, 0)
*/

WebServer server(80);  // puerto por defecto 80
WebSocketsServer webSocket = webSocketsServer(81); //puerto estándar para websockets





void setup() {
  Serial.begin(115200);
 
  // Creamos el punto de acceso
  WiFi.softAP(ssid, password); // Tiene mas parametros opcionales
  //WiFi.softAPConfig(ip, gateway, subnet);
  IPAddress ip = WiFi.softAPIP();
  
 
  //Mostramos el nombre, contraseña e ip de la red
  Serial.print("Nombre de mi red esp32: ");
  Serial.println(ssid);
  Serial.print("Ip: ");
  Serial.println(ip);
  Serial.print("Contraseña: ");
  Serial.println(password);

  //Iniciamos el servidor web
  server.on("/", handleConnectionRoot);
  server.onNotFound(handleNotFound);

  //Encendemos la página
  server.begin();
  Serial.println("Servidor HTTP iniciado");
  //Configuración de websockets
  webSocket.begin();
  
  //Configuración de hardware
  //Pines digitales
  pinMode(LED, OUTPUT);
}
 
void loop() {
  server.handleClient();
  webSocket.loop();
  
  //Blue LED to show it's working
  t1 = millis();
  if (t1 - t0 >= interval){
    led_value = ~led_value;
    digitalWrite(LED, led_value);
    //update the websocket
    String str = String(led_value);
    const int str_len = str.length() +1;
    char char_array[str_len];
    webSocket.broadcastTXT(String(led_value));
    str.toCharArray(char_array, str_len);
    
    //Reset the cycle
    t0 = t1;
  }
}
 
// Nuestra respuesta en html
String Webpage0 = "<!DOCTYPE html>\n<html>\n<head>\n<title>MINIPROYECTO</title>\n</head>\n\n<body style='background-color: #FFFFFF;'>\n\n<span style='color: #003366;'>\n<h1>Websocket test</h1>\n<p>Luz encendida: <span id ='led'>-</p>\n\n</span>\n\n</body>\n<script>\n\tvar Socket;\n\tfunction init({\n\t\tSocket = new WebSocket('ws://' + window.location.hostname + ':81/');\n\t\tSocket.onmessage = function(event) {\n\t\t\tprocessCommand(event);\n\t\t};\n\t)\n\tfunction processCommand(event) {\n\tdocument.getElementById('led').innerHTML = event.data;\n\tconsole.log(event.data);\n\t}\n\twindow.onload = function(event) {\n\t\tinit();\n\t}\n</script>\n\n</html>";
/*
 * Recordar que la función interval se ejecuta automáticamente en una página web
 * Se puede usar para actualizar la página web cada 500ms
 */
 
// Responder a la url raíz (root /)
void handleConnectionRoot() {
  server.send(200, "text/html", Webpage0);
}

void handleNotFound(){
  server.send(404, "text/plain", "Not found");
}
