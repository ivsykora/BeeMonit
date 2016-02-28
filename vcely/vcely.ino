#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_CC3000.h>
#include <Wire.h>
#include <SPI.h>
#include<stdlib.h>

#define ADAFRUIT_CC3000_IRQ   3 
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);
#define WLAN_SSID       "weas"          
#define WLAN_PASS       "K0y0jedz2"
#define hashAuth        "oSOxRdge"
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define DHTPIN 30
#define DHTTYPE 22

DHT dht(DHTPIN, DHTTYPE);

uint32_t ip;
String postData;
char ipString[16] = "172.16.124.29";
Adafruit_BMP085 bmp;

void setup() {
  setUpSerial();
  setUpWifi();
  setUpDHT();
  setUpBMP();
  ip = parseIPV4string(ipString);
  Serial.println(F("setup done"));
}

void loop() {
  postRequest();
  delay(5000);

}

void setUpSerial(){
  Serial.begin(115200);
  while(!Serial);
  Serial.println(F("Serial setup"));
} 

void setUpWifi(){
  if (!cc3000.begin()){
    while(1);
  }

  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    while(1);
  }
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }
  Serial.println(F("Wifi setup"));
}

void setUpDHT(){
  dht.begin();
  Serial.println(F("DHT setup"));
}

void setUpBMP(){
  if (!bmp.begin()) {
    Serial.println(F("BMP setup fail"));
    while (1);
  }
  Serial.println(F("BMP setup"));
}

uint32_t parseIPV4string(char const * ipAddress) { 
  unsigned int ipbytes[4]; 
  if ( 4 != sscanf(ipAddress, "%u.%u.%u.%u", &ipbytes[0], &ipbytes[1], &ipbytes[2], &ipbytes[3]) ) {
    return 0;
  }
  return ipbytes[3] + ipbytes[2] * 0x100 + ipbytes[1] * 0x10000ul + ipbytes[0] * 0x1000000ul; 
}

void postRequest(){
  postData = "";
  postData = readDHT();
  Serial.println(F("Read DHT done"));
  postData = postData +  ";" + readBMP();
  Serial.println(F("Read BMP done"));

  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
  client.connected();

  client.println(F("POST /feed/data HTTP/1.0")); 
  client.print(F("X-Auth:"));
  client.println(hashAuth);
  client.print(F("Content-Length: "));
  client.println(postData.length());
  client.println();
  client.println(postData);

  while(client.connected() && !client.available()) delay(1);
  while (client.connected() || client.available()) { 
    client.read();
  }

  client.close();
}

String readDHT(){
  String value = pressureOrHumidityToString(dht.readHumidity());
  float temp = dht.readTemperature();
  return value;
}

String readBMP(){
  // floatToString(bmp.readTemperature()) + ";" +
  bmp.readTemperature();
  return pressureOrHumidityToString(bmp.readPressure());
}


String pressureOrHumidityToString(double floatVal){
  char charVal[sizeof(floatVal)];
  String stringVal = "";

  dtostrf(floatVal, 4, 2, charVal);

  for(int i=0;i<sizeof(charVal);i++){
    stringVal+=charVal[i];
  }

  Serial.println(stringVal);
  return stringVal;
}










