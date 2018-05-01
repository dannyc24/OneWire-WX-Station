#include <OLED.h>
/*
 * On the WIFI OLED ESP Board
 * Sensor POrts as follows
 * SDA: Pin=D2
 * SCL: Pin=D1
 * 
 */

#include <SparkFunBME280.h>
#include "Adafruit_VEML6070.h"

#include <Wire.h>

#include <OneWire.h>


#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

//Dans Weather Program mark 1..
//For ESP8266 kit

//Two Bus Systems

OneWire ds(12); //port 6
BME280 phtSensor;
#define RST_OLED 16

OLED display(4, 5);




Adafruit_VEML6070 uv = Adafruit_VEML6070();

/* Set these to your desired credentials. */
const char *ssid = "WeatherStation";
//const char *password = "weather1234";
boolean isOneWireWxEnabled = true;
boolean isDebug = true;
boolean isDSList = false;

byte addr[8];
boolean phtActive = false;
boolean uvLight = true;

ESP8266WebServer server(80);

//Weather Values 
float currPressure = 0.0f;
float currTempF = 0.0f;
int currTempFOW = 0;
float currHum = 0.0f;
float currDewPF = 0.0f;
float currWS = 0.0f;
float currAlt = 0.0f;
//char currWD;

int currWDi;
int currUV;


//Id Constants
String TEMP = "10 65 20 26 00 00";
String ANN = "1d 3c c9 00 00 00";
String RG = "1d 50 cc 00 00 00"; //Rain Gauge .. Dev:1D 50 CC 0 0 0 0 EE 


byte dswitch[] = {0x12, 0xF2, 0xC1, 0x0A, 0x00, 0x00, 0x00, 0x69};


long lasttimesnap;
uint32_t lastwssnap;

char* windDirectionArray[17] = {"x", "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};

boolean initOledClear = false;

void setup() {
  // put your setup code here, to run once:
  //Wire.begin();
  //Wire.begin(13,12);
  phtSensor.settings.commInterface = I2C_MODE;
  phtSensor.settings.I2CAddress = 0x76;
  Serial.begin(9600);
  Serial.println("DC Weather Station Mark I");
  setupOled();
  if(phtSensor.beginI2C() == false){
    phtActive = false;
    Serial.println("PHT Sensor Not Found... Deactive");
  }
  else {
    Serial.println("PHT Sensor Found.. Active");
    phtActive = true;
  }
  if(uvLight){
     uv.begin(VEML6070_1_T);  // pass in the integration time constant
     Serial.println("UV Sensor Found");
     
  }
  else {
    Serial.println("UV Sensor Not Found");
  }

  if(isOneWireWxEnabled){
    Serial.println("OneWire WX Enabled.\nSending Switch On");
    PIOON();
  }
 

//    WiFi.softAP(ssid, password);
 WiFi.softAP(ssid);
  
  IPAddress myIP = WiFi.softAPIP();
  server.begin();
  server.onNotFound(renderJsonCurrentWX);
  
}

void loop() {
  server.handleClient();
  // put your main code here, to run repeatedly:
  if(phtActive){
    pullDataFromBMPSensor();
  }
  if(uvLight){
    pullDataFromUVSensor();
  }
  if(isOneWireWxEnabled){
    scanOneWire();
  }

  
  printWxtoSerial();
  displayWxOled();
  
}


void setupOled(){
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);   // turn D2 low to reset OLED
  delay(50);
  digitalWrite(RST_OLED, HIGH);    // while OLED is running, must set D2 in high
  display.begin();
  display.print("DCWX Station",0);
  display.print("Starting.....",1);
  
}

void printWxtoSerial(){
  Serial.print("Temp:");
  Serial.print(currTempF,2);
  Serial.print("  ");
  Serial.print("Humd:");
  Serial.print(currHum,2);
  Serial.print(" ");
  Serial.print("Dew:");
  Serial.print(currDewPF,2);
  Serial.print(" "); 
  Serial.print("Pressure:");
  Serial.print(currPressure/100,2);
  Serial.print(" ");
  Serial.print("UV:");
  Serial.print(currUV);
  Serial.print(" ");
  Serial.print("WS:");
  Serial.print(currWS);
  Serial.print(" ");
  Serial.print("WD:");
  Serial.print(windDirectionArray[currWDi]);
  
  Serial.println(" ");
  
}
void displayWxOled(){
  if(!initOledClear){
    display.clear();
    initOledClear = true;
  }
  String l1 = "Tf:" + String(currTempF) + "Df:" + String(currDewPF);
  String l2 = "P:" + String(currPressure/100,2) + " RH:" + String(currHum);
  String l3 = "WS:" + String(currWS) + " WD:" + String(windDirectionArray[currWDi]); 
  String l4 = "SSID: " + String(ssid);
  String l5 = "IP:";
  
  
  char tl1[17];
  char tl2[17];
  char tl3[17];
  char tl4[17];
  char tl5[17];
  
  l1.toCharArray(tl1,17);
  l2.toCharArray(tl2,17);
  l3.toCharArray(tl3,17);
  l4.toCharArray(tl4,17);
  l5.toCharArray(tl5,17);
  
  
  display.print(tl1,0);
  display.print(tl2,1);
  display.print(tl3,2);
  display.print(tl4,3);
  display.print(tl5,4);
  //display.print(tl1,0,0);
 
  //display.print("0.0",0,6);
  
}


void renderJsonCurrentWX(){
  String m = "{\n";

  m += "\"TempF\":" + String(currTempF) + ",\n";
  m += "\"TempFOW\":" + String(currTempFOW) + ",\n";
  m += "\"DewpF\":" + String(currDewPF) + ",\n";
  m += "\"Pressure\":" + String(currPressure/100) + ",\n";
  m += "\"RelativeHumidity\":" + String(currHum) + ",\n";
  m += "\"WindSpeed\":" + String(currWS) + ",\n";
  m += "\"WindDirection\":\"" + String(windDirectionArray[currWDi]) + "\",\n";
  m += "\"UVI\":" + String(currUV) + ",\n";
  m += "\"RainCounter\":\"n/a\",\n";
  m += "\"Altimeter\":" + String(currAlt) + "\n";
  

  m += "}\n";
  server.send(200, "text/json", m);
}
//void scanOneWire(){
//  if(!ds.search(addr)){
//    Serial.println("OneWire Dev");
//    Serial.print("DevID:");
//    for(int a1=0; a1 < 8; a1++){
//        Serial.print(addr[a1], HEX);
//        Serial.print(" ");
//    }
//    ds.reset_search();
//    
//  }
//
//}

void scanOneWire(){
  if(!ds.search(addr)){
    if(isDSList){
      Serial.println("No Onewire Device");
    }
    else {
      //Place Holder
    }
    ds.reset_search();
    delay(250);
  }
  else {
    if(isDSList){
       Serial.println("One Wire Device");
       Serial.print("Dev:");
       for(int i=0; i < 8; i++){

          Serial.print(addr[i], HEX);
          Serial.print(" ");
      }
      Serial.println("");
    }
    else {
      
    }
   
    String id = getReadableDevId();
    onewireSensorSwitcher(id);
  }

}

void pullDataFromBMPSensor(){
  currPressure = phtSensor.readFloatPressure();
  currTempF = phtSensor.readTempF();
  currHum = phtSensor.readFloatHumidity();
  currDewPF = tempCtoF(dewpointTemp(phtSensor.readTempC(), currHum));
  currAlt = phtSensor.readFloatAltitudeFeet();
}

void pullDataFromUVSensor(){
  currUV = uv.readUV();
}

String getReadableDevId(){
  char mac[20];
  sprintf(mac,"%02x %02x %02x %02x %02x %02x\0",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7]);
  String t = String(mac);
//  Serial.println();
//  Serial.print("T:");
//  Serial.print(t);
//  Serial.println(" ");
  return t;
}

void onewireSensorSwitcher(String in){
  if(in.equals(TEMP)){
    int t1 =  tempCtoF(GetTemp(0.0625));
    currTempFOW = t1;
    Serial.println("Temperature");
   
    Serial.println(t1);
  }
  else if(in.equals(ANN)){
    Serial.println("Wind Speed");
    Serial.print("Count: ");
    //Serial.print();
    processWindSpeed();
    Serial.println();
  }
  else if(in.equals(RG)){
    Serial.println("Rain Gauge");
    Serial.print("Count:");
    Serial.print(GetCount2());
    Serial.println();
    
  }
  else if(addr[0] == 0x01){
    //Class for Wind Direction
    Serial.println("Wind Direction Detected");
    currWDi = dcWindDirection(addr[1]);
    //currWD = windDirectionArray[currWDi];
  }
  else {
    Serial.println("Unknown Dev");
  }
}

void processWindSpeed(){
  uint32_t count1 = GetWindCount();
  long currTime = millis();

  long timeDiff =  abs(currTime - lasttimesnap);
  currWS = dcWindSpeed(lastwssnap,count1,timeDiff) * 2.2369;

  lastwssnap = count1;
  lasttimesnap = currTime;
  
  
}

float dcWindSpeed(uint32_t ws1, uint32_t ws2, long timediff){
  int cCount = (ws1 - ws2);
//  Serial.print("Count Difference:");
//  Serial.println(cCount);
  float f1 = abs(cCount) * 1000;
  float f2 = f1 / abs(timediff);
  float f3 = f2 / 2.0;

//  Serial.print("DC Speed:");
//  Serial.println(f3);
  return f3;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//Performs a temperature conversion request.
//
//Tested on a single DS18S20 and a single DS18B20.
//
//The DS18B20 requires a conversion factor of 0.0625 to be passed in.
//The DS18S20 requires a conversion factor of 0.5 to be passed in.
/////////////////////////////////////////////////////////////////////////////////////////////
int GetTemp(double converstionFactor)
{

  int HighByte, LowByte;
  byte present = 0;
  byte data[12];

  int TReading, SignBit, Tc_100, Whole, Fract;
//int InsideC, OutsideC;

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);               // maybe 750ms is enough, maybe not

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE,0);         // Read Scratchpad

  for (int i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }

  Tc_100 = (double)TReading * converstionFactor * 10;

  if (SignBit) // If its negative
  {
     Tc_100=0-Tc_100;
  }
  return(Tc_100);
}


/////////////////////////////////////////////////////////////////////////////////////////////
//Returns the value of the external counter A from the DS2423.
//
//The DS2423 returns the counter A count at the end of page 0x01c0 and is returned by sending
//the instruction 0xa5.
/////////////////////////////////////////////////////////////////////////////////////////////
int GetWindCount()
{
  byte data[12];
  ds.reset();
  ds.select(addr); //A DC Test
  //ds.skip();                // skip ROM, no parasite power on at the end
  ds.write(0xa5, 0);        // Read memory and counter command
  ds.write(0xc0, 0);        // Start at page 14 which has external counter A at the end...
  ds.write(0x01, 0);        // = 0x01c0 
  
  for (byte i = 0; i<32; i++)    //Read back the 32 bits in the datapage (no information for us here)
  {
    ds.read();
  }
  for (byte i = 0; i < 4; i++)  // Read back the 4 bytes of external counter A
  {
    data[i] = ds.read();
  }
  //You could check the checksum here but I'm not!
  return(data[0]);  //As we're only using the lowest byte we don't need data[1 to 3]
}

//Test Count --- Sourced From https://www.element14.com/community/groups/internet-of-things/blog/2015/01/07/old-meets-new-the-1-wire-weather-station-on-the-spark-core-part-5
uint32_t GetWindCount2(){
  byte data[38];
  ds.reset();
  ds.select(addr);

  ds.write(0xa5,0);
  ds.write(0xc0,0);
  ds.write(0x01,0);

  ds.read_bytes(data, 38);

  uint32_t count = (uint32_t)data[38];

  for(int j = 34; j >=32; j--){
    count = (count << 8) + (uint32_t)data[j];
  }
  return count;
}
int GetCount2()
{
  byte data[12];
  ds.reset();
  ds.select(addr); //DC Test to Stabilize Count retriever. 
  //ds.skip();                // skip ROM, no parasite power on at the end
  ds.write(0xa5, 0);        // Read memory and counter command
  ds.write(0xe0, 0);        // Start at page 14 which has external counter A at the end...
  ds.write(0x01, 0);        // = 0x01c0 
  
  for (byte i = 0; i<32; i++)    //Read back the 32 bits in the datapage (no information for us here)
  {
    ds.read();
  }
  for (byte i = 0; i < 4; i++)  // Read back the 4 bytes of external counter A
  {
    data[i] = ds.read();
  }
  //You could check the checksum here but I'm not!
  return(data[0]);  //As we're only using the lowest byte we don't need data[1 to 3]
}

//For turning on the switch 
void PIOON()//DS2407 turn on the switch
{
byte data[12];
ds.reset(); //reset bus
Serial.println("PIOON Called");
//Serial.print(dswitch);
Serial.println("------------");
ds.select(dswitch); //select device previously discovered
ds.write(0x55); //write status command
ds.write(0x07); //select location 00:07 (2nd byte)
ds.write(0x00); //select location 00:07 (1st byte)
ds.write(0x1F); //write status data byte (turn PIO-A ON)
Serial.print ("VALUE="); //read CRC16 of command, address and data and print it; we don’t care
for (byte i = 0; i < 6; i++) {
data[i] = ds.read();
Serial.print(data[i], HEX);
Serial.print(" ");
}
ds.write(0xFF,1); //dummy byte FFh to transfer data from scratchpad to status memory, leave the bus HIGH
delay(2000); //leave the things as they are for 2 seconds

}



void PIOOFF()//DS2407 trun off the switch
{
byte data[12];
ds.reset(); //reset bus
ds.select(dswitch); //select device previously discovered
ds.write(0x55); //write status command
ds.write(0x07); //select location 00:07 (2nd byte)
ds.write(0); //select location 00:07 (1st byte)
ds.write(0x3F); //write status data byte (turn PIO-A OFF)
Serial.print ("VALUE="); //read CRC16 of command, address and data and print it; we don’t care
for (byte i = 0; i < 6; i++) {
data[i] = ds.read();
Serial.print(data[i], HEX);
Serial.print(" ");
}
ds.write(0xFF,1); //dummy byte FFh to transfer data from scratchpad to status memory, leave the bus HIGH
delay(2000); //leave the things as they are for 2 seconds

}


float dewpointTemp(float tempC, float RH){
  float c1,c2,c3;
  //Magnus CoEfficients
  if(tempC > 0.0f){
    //Above greater than 0 degrees
    c1 = 6.10780;
    c2 = 17.08085;
    c3 = 234.175;
  }
  else {
    //Less than 0 degrees c
    c1 = 6.10780;
    c2 = 17.84362;
    c3 = 234.175;
  }

  float h1 = RH / 100;

  float ps = c1 * exp((c2 * tempC)/(c3 + tempC));
  float pd = ps*h1;

  float tp = (-log(pd/c1)*c3)/(log(pd/c1)-c2);
  return tp;
}

float tempCtoF(float tempC){

  float tempOut = (tempC * 1.8) + 32;
  return tempOut;
}
int dcWindDirection(byte indevAddr){
  //This is a cheap and easy way to ident the direction of the vien.
  Serial.print("Dans Wind Direction:InDev:");
  Serial.print(indevAddr, HEX);
  Serial.println();
  int windDir = 0;
  switch(indevAddr){
    case 0xDA:
      windDir = 1;
    break;
    
    case 0xDB:
      windDir = 3;
    break;

    case 0xDD:
      windDir = 5;
    break;

    case 0xE1:
      windDir = 7;
    break;

    case 0xE0:
      windDir = 9;
    break;

    case 0xDF:
      windDir = 11;
    break;

    case 0xDE:
      windDir = 13;
    break;

    case 0xD9:
      windDir = 15;
    break;
    default: 
      windDir = 0;

    
  }
    Serial.print("Func Out Wind Dir:");
    Serial.println(windDir);
    return windDir;
}


