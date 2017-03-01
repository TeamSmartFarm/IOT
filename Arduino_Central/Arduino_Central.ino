#include <secTimer.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#define TOTALNODE 1
#define FarmID 001

byte relay = 8;
bool moterStatus = 0;

SoftwareSerial gprsSerial(12, 13);  //RX TX
SoftwareSerial fromNodeMCU(10, 11);   //RX TX

secTimer gsmTimer; 
secTimer moistureTimer;

long moistureTime, GSMTime;

DynamicJsonBuffer  jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

struct Threshold{
  float temp;
  float light;
  float Uppermoisturelimit;     //It is used to turn off moter
  float Lowermoisturelimit;     //It is used to turn on moter
}thresholdValues;

float initialTemp, initialLight, initialMoisture;
float finalTemp, finalLight, finalMoisture;
int savedTemp[TOTALNODE],savedLight[TOTALNODE],savedMoisture[TOTALNODE],batteryLevel[TOTALNODE];
int nodes[TOTALNODE];

void intiGPRS(){
  
  Serial.println("Config SIM900...");
//  delay(2000);
  gprsSerial.flush();
  Serial.flush();

  // attach or detach from GPRS service 
  gprsSerial.println("AT+CGATT?");
  delay(100);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay(2000);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"T24.Internet\"");
  delay(2000);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=1,1");
  delay(2000);
  toSerial();
  Serial.flush();
  Serial.println("Done!...");
}

void GSMSend(){
  // initialize http service
   gprsSerial.println("AT+HTTPINIT");
   delay(2000); 
   toSerial();

   // set http param value
   /*
   String url = "AT+HTTPPARA=\"URL\",\"http://ec2-35-154-68-218.ap-south-1.compute.amazonaws.com:8000/smartfarm?farmID=";
   url += FarmID;
   url += "&initialTemperature="; 
   url += initialTemp;
   url += "&finalTemperature="; 
   url += finalTemp;
   url += "&initialLight="; 
   url += initialLight;
   url += "&finalLightt="; 
   url += finalLight;
   url += "&initialMoisture=";
   url += initialMoisture;
   url += "&finalMoisture=";
   url += finalMoisture;
   url += "&waterConsumption=";
   url += (moistureTime*0.1124);
   url += "\"";
   */
  
   //gprsSerial.println("AT+HTTPPARA=\"URL\",\"http://helpinghandgj100596.comule.com/test.php?temp=1&light=2&moisture=2\"");
   String url = "AT+HTTPPARA=\"URL\",\"http://ec2-35-154-68-218.ap-south-1.compute.amazonaws.com:8000/smartfarm?farmID=";
   url += FarmID;
   url += "&temperature="; 
   url += finalTemp;
   url += "&light="; 
   url += finalLight;
   url += "&moisture=";
   url += finalMoisture;
   url += "&waterConsumption=";
   url += (moistureTime*0.1124);
   url += "\"";
   Serial.println(""+url);
   gprsSerial.println(url);
   delay(2000);
   toSerial();

   // set http action type 0 = GET, 1 = POST, 2 = HEAD
   gprsSerial.println("AT+HTTPACTION=0");
   delay(6000);
   toSerial();

   // read server response
   gprsSerial.println("AT+HTTPREAD"); 
   delay(1000);
   toSerial();

   gprsSerial.println("");
   gprsSerial.println("AT+HTTPTERM");
   toSerial();
   delay(300);

   gprsSerial.println("");
   delay(10000);
}
void toSerial()
{
  while(gprsSerial.available()!=0)
  {
    Serial.write(gprsSerial.read());
  }
}
void GSMReceive(){
  
  
  // initialize http service
   gprsSerial.println("AT+HTTPINIT");
   delay(2000); 
   toSerial();

   // set http param value
   //use global variable here to send data
   //Will need some token for unique identity of farm
   gprsSerial.println("AT+HTTPPARA=\"URL\",\"http://helpinghandgj100596.comule.com/request.php?ID=\"");
   gprsSerial.println(FarmID);
   delay(2000);

   char json[200];
   int i = 0;
   while(gprsSerial.available()!=0)
   {
    json[i++] = gprsSerial.read();
   }
   
   JsonObject& root = jsonBuffer.parseObject(json);

   // Test if parsing succeeds.
   if (!root.success()) {
     Serial.println("parseObject() failed");
     return;
   }

   thresholdValues.temp = root["Tempreature"];
   thresholdValues.light = root["Light"];
   thresholdValues.Uppermoisturelimit = root["MoistureUpperLimit"];
   thresholdValues.Lowermoisturelimit = root["MoistureLowerLimit"];
   
   // set http action type 0 = GET, 1 = POST, 2 = HEAD
   gprsSerial.println("AT+HTTPACTION=0");
   delay(6000);
   toSerial();

   // read server response
   gprsSerial.println("AT+HTTPREAD"); 
   delay(1000);
   toSerial();

   gprsSerial.println("");
   gprsSerial.println("AT+HTTPTERM");
   toSerial();
   delay(300);

   gprsSerial.println("");
   delay(10000);
}
void setup()
{
  gprsSerial.begin(19200);
  
  //Initialy relay is in OFF state
  pinMode(relay,OUTPUT);
  digitalWrite(relay,LOW);
  
  //Threshold thresholdValues;
  gsmTimer.startTimer();
  
  Serial.begin(9600);
  fromNodeMCU.begin(9600);

  //Initialization of GPRS
  intiGPRS();

  initThreshold();
}


void loop()
{
  senseSensors(savedTemp,savedLight,savedMoisture);
  findAvg(savedTemp,savedLight,savedMoisture);
  if(takeDecision() == 1){
    //Starttimer
    Serial.println("Starting Moter");
    moistureTimer.startTimer();
    while(takeDecision() == 1){
      senseSensors(savedTemp,savedLight,savedMoisture);
      findAvg(savedTemp,savedLight,savedMoisture);
    }
    moistureTime = moistureTimer.readTimer();
    moistureTimer.stopTimer();
    Serial.println("Moter off");
    GSMSend();//Send Time as parameter also.  
    //Just for debuging purpose
    printDatatoSerial();
  }

  //Request data from server after 15 days
  if (gsmTimer.readTimer() > 1296000){
    //GSMReceive();
    Serial.println("Receiveing data from Server");
    gsmTimer.stopTimer();
    gsmTimer.startTimer();
  }
}

//Just of debuging purpose
void initThreshold(){
   thresholdValues.temp = 35;
   thresholdValues.light = 150;
   thresholdValues.Uppermoisturelimit = 400;      
   thresholdValues.Lowermoisturelimit = 600;
}

void printDatatoSerial(){
   Serial.println("FarmID"+FarmID);
   Serial.print("Temprature = ");
   Serial.println(finalTemp);
   Serial.print("Light = ");
   Serial.println(finalLight);
   Serial.print("Moisture = ");
   Serial.println(finalMoisture);
   Serial.print("Water = ");
   //water consume in 1 secound is 15000 milli-liter
   Serial.println(moistureTime*0.1229);
}

void senseSensors(int savedTemp[],int savedLight[],int savedMoisture[]){
  int i=0;
  Serial.println("Reading Sensor Values");
  for(i=0;i<TOTALNODE;){
    
      while(fromNodeMCU.available() == 0){
        //Serial.print(".");  
      }
      String readings = fromNodeMCU.readString();
      Serial.println(readings);

      int firstDelim = readings.indexOf(':');
      int tempValue = readings.substring(0,firstDelim).toInt();
      int secondDelim = readings.indexOf(':',firstDelim+1);
      int lightValue = readings.substring(firstDelim+1,secondDelim).toInt();
      int thirdDelim = readings.indexOf(':',secondDelim+1);
      int moistureValue =  readings.substring(secondDelim+1,thirdDelim).toInt();
      int forthDelim = readings.indexOf(':',thirdDelim+1);
      int id = readings.substring(thirdDelim+1,forthDelim).toInt();
      int battery = readings.substring(forthDelim+1).toInt();

      Serial.print("Mote ID:");
      Serial.println(id);
      Serial.print("Tempreature :");
      Serial.println(tempValue);
      Serial.print("Light :");
      Serial.println(lightValue);
      Serial.print("Moisture :");
      Serial.println(moistureValue);
      Serial.print("Battery :");
      Serial.println(battery);

      int j = findNode(id);
      if (j == -1){
          nodes[i] = id;
          savedTemp[i] = tempValue;
          savedLight[i] = lightValue;
          savedMoisture[i] = moistureValue;
          batteryLevel[i] = battery;
          i++;
      }
      else {
          savedTemp[j] = tempValue;
          savedLight[j] = lightValue;
          savedMoisture[j] = moistureValue;
          batteryLevel[j] = battery;
      }
      
      Serial.flush();
      fromNodeMCU.flush();
  }
  
  for (int i = 0; i<TOTALNODE; i++){
    nodes[i] = 0;
  }
}

int findNode(int id){
    for (int i = 0; i<TOTALNODE; i++){
        if (nodes[i] == id)
            return i;
    }
    return -1;
}

void findAvg(int savedTemp[],int savedLight[],int savedMoisture[]){
  int i=0;
  int temp,light,moisture;
  temp=light=moisture=0;
  Serial.println("Finding Average");
  for(i=0;i<TOTALNODE;i++){
    temp+=savedTemp[i];
    light+=savedLight[i];
    moisture+=savedMoisture[i];
  }

  finalTemp = temp/float(TOTALNODE);
  finalLight = light/float(TOTALNODE);
  finalMoisture = moisture/float(TOTALNODE);
}

int takeDecision(){
  /*
   * based on threshold value give 1 or 0 as output
  */
  Serial.println("Taking Descision");
  if (moterStatus == 0){
    //turn motor ON;
    if(finalMoisture > thresholdValues.Lowermoisturelimit){

        initialTemp = finalTemp;
        initialLight = finalLight;
        initialMoisture = finalMoisture;

        digitalWrite(relay,HIGH);
        Serial.println("Moter ---------  HIGH");
        moterStatus = 1;
        return 1;       
    }
  }
  else {
    //turn moter OFF
    if(finalMoisture <= thresholdValues.Uppermoisturelimit){
        digitalWrite(relay,LOW);          
        Serial.println("Moter ---------  LOW");
        moterStatus = 0;
        return 0;
    }
  }
  return 1;
}

