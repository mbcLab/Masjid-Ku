#include <DNSServer.h>
#include <WiFiManager.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#define WEBSERVER_H
#include <ESPAsyncWebServer.h>
#include <PZEM004Tv30.h>
#include "FirebaseESP8266.h"

AsyncWebServer server(80);
const char* firebaseURL = "firebaseURL";
const char* firebaseSecret = "firebaseSecret";
String URL = "-";
String Secret = "-";
FirebaseData firebaseData;
PZEM004Tv30 pzem(14,12);
const int relay = 5;
String relayStat;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    firebaseURL: <input type="text" name="firebaseURL">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    firebaseSecret: <input type="text" name="firebaseSecret">
    <input type="submit" value="Submit">
  </form><br>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup(){
  Serial.begin(115200);
  pinMode (relay, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  wifiManager.autoConnect("ESP8266LampuAP");
  Serial.println("Connected");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(firebaseURL)) {
      inputMessage = request->getParam(firebaseURL)->value();
      inputParam = firebaseURL;
      URL = inputMessage;
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(firebaseSecret)) {
      inputMessage = request->getParam(firebaseSecret)->value();
      inputParam = firebaseSecret;
      Secret = inputMessage;
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
    });
    server.onNotFound(notFound);
    server.begin();
    while (URL=="-" || Secret=="-"){
      Serial.println(URL);
      Serial.println(Secret);
      delay(500);
    }
  Firebase.begin("masjid-ku-53784-default-rtdb.firebaseio.com", "8DHKldLb8ULepp7PDUCsgXgvFcHnu2t8Pec9Fzx3");         //URL dan Token Firebase
  Firebase.reconnectWiFi(true);
  Firebase.setMaxRetry(firebaseData, 3);
}

void loop(){
    float voltage = pzem.voltage();
    if( !isnan(voltage) ){
        Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");
        Firebase.setString(firebaseData, "/Lampu/tegangan", String(voltage) + " V" );
    } else {
        Firebase.setString(firebaseData, "/Lampu/tegangan", "0 V" );
        Serial.println("Error reading voltage");
    }

    float current = pzem.current();
    if( !isnan(current) ){
        Serial.print("Current: "); Serial.print(current); Serial.println(" A");
        Firebase.setString(firebaseData, "/Lampu/arus", String(current) + " A");
    } else {
        Firebase.setString(firebaseData, "/Lampu/arus", "0 A");
        Serial.println("Error reading current");
    }

    float power = pzem.power();
    if( !isnan(power) ){
        Serial.print("Power: "); Serial.print(power); Serial.println(" W");
        Firebase.setString(firebaseData, "/Lampu/daya", String(power) + " Watt");
    } else {
        Firebase.setString(firebaseData, "/Lampu/daya", "0 Watt");
        Serial.println("Error reading power");
    }

    float energy = pzem.energy();
    float energyJoule = energy*pow(10,3)*3600; // p=(W/t)
    if( !isnan(energy) ){
        Serial.print("Energy: "); Serial.print(energy); Serial.println(" Wh");
        Firebase.setString(firebaseData, "/Lampu/energi", String(energy) + " Wh");  //dibagi 1000 untuk konversi menjadi kJoule
    } else {
        Firebase.setString(firebaseData, "/Lampu/energi", "0 Wh");
        Serial.println("Error reading energy");
    }

    Firebase.getString(firebaseData, "/Lampu/saklar", relayStat);
    Serial.println(relayStat);
    if (relayStat == "ON") {
      Serial.print("\n Relay ON");
      digitalWrite(relay, HIGH);  
   }
   else{
      Serial.print("\n Relay OFF");
      digitalWrite(relay, LOW);  
   }
    Serial.println();
    delay(200);
}
