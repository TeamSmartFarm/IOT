#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
SoftwareSerial toArduino(13, 15);   //RX TX

unsigned int chipID = ESP.getChipId();
const char *MESH_PREFIX = "SmartFarm";
const char *PASSWORD = "A3B4NJshb*903dfnHx";
const int localPort = 9999;      // local port to listen on

IPAddress local_IP(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

WiFiUDP Udp;
DynamicJsonBuffer  jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

char packetBuffer[65]; //buffer to hold incoming packet

void createAP(){
  WiFi.disconnect();
  Serial.print("Configuring access point...");
  char buffer_[20];
  String ssid = String(MESH_PREFIX);
  ssid += ESP.getChipId();
  ssid.toCharArray(buffer_,20);  
  WiFi.softAP(buffer_, PASSWORD);
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void scanAPandConnect(){
// Set WiFi to station mode and disconnect from an AP if
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int n = WiFi.scanNetworks();
  while (n == 0){
    Serial.println("no networks found");
    //wait for random amount time
    delay(random(1000,5000));
    n = WiFi.scanNetworks();
  }
  
  Serial.println(" networks found");
  String ssid;
  char buffer_[20];
  int rssi = -10000;
  for (int i = 0; i < n; i++)
  {
    // Select SSID having lowest RSSI
    if (((WiFi.SSID(i)).substring(0,9)).compareTo("SmartFarm1768718") == 0 ){
      rssi = WiFi.RSSI(i);
      ssid = WiFi.SSID(i);
      break;
    }
    else if (rssi < WiFi.RSSI(i) && (((WiFi.SSID(i)).substring(0,9)).compareTo(MESH_PREFIX) == 0 )){
      rssi = WiFi.RSSI(i);
      ssid = WiFi.SSID(i);
    }
  }
  Serial.println("Connecting to :"+ssid);
  WiFi.mode(WIFI_STA);
  ssid.toCharArray(buffer_,20);  
  WiFi.begin(buffer_, PASSWORD);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  toArduino.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  createAP();
  Serial.println("\nStarting UDP...");
  Udp.begin(localPort);
}

void loop() {

  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {

    /*
    Json packet
      {
        "id":1278685,
        "count":1,
        "TTL":5,
        "sensors":[32,31,888]
      }
    */
    //Serial.println("Size of packet :"+packetSize);
    int len = Udp.read(packetBuffer, 65);
    if (len > 0) 
      packetBuffer[len] = '\0';

    JsonObject& parse = jsonBuffer.parseObject(packetBuffer);
    // Test if parsing succeeds.
    if (!parse.success()) {
      Serial.println("parseObject() failed");
      return ;
    }
    Serial.print("Content :");
    String str;
    parse.printTo(str);
    parse.prettyPrintTo(Serial);
    
    int id = parse["id"];
    int count = parse["count"];
    int TTL = parse["TTL"];
    
    parse["TTL"] = TTL - 1;
    
    if (TTL > 0){
      if (root["id"]){
        if (root["id"] < count)
          root["id"] = count;
      }
      else{
        root["id"] = count;
      }
      /*Central node ID*/
      if (chipID == 1768718){ 
        //parse.printTo(str);
        //parse.printTo(toArduino);
        Serial.println(str);
        toArduino.println(str);
        Serial.println("Data recived");
        toArduino.flush();
      }
      else{
        scanAPandConnect();
        //String s;
        parse.printTo(packetBuffer);
        //s.toCharArray(packetBuffer,45);
        sendMessage(packetBuffer);
      }
    }
    createAP();
    for (int i = 0; i<65; i++)
        packetBuffer[i] = '\0';
  }
  else if (Serial.available()){
    digitalWrite(LED_BUILTIN, LOW);
    //Serial.readString().toCharArray(packetBuffer,55);

    JsonObject& parse = jsonBuffer.parseObject(Serial.readString());
    // Test if parsing succeeds.
    if (!parse.success()) {
      Serial.println("parseObject() failed");
      //return;
    }
    parse["id"] = chipID;
    parse.printTo(packetBuffer);

    Serial.println("Content :"+String(packetBuffer));
    scanAPandConnect();
    sendMessage(packetBuffer);
    for (int i = 0; i<65; i++)
        packetBuffer[i] = '\0';   
    Serial.flush();
    createAP();
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

void sendMessage(char data[]) {
  Udp.beginPacket(local_IP,localPort);
  Udp.write(data);
  Serial.println(data);
  Udp.endPacket();
  Serial.flush();
}



