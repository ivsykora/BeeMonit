#include <Time.h>
#include <Adafruit_CC3000.h>
#include <SPI.h>

#define ADAFRUIT_CC3000_IRQ   3 
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);
#define WLAN_SSID       "saew"          
#define WLAN_PASS       "K0y0jedz2"
#define hashAuth        "oSOxRdge"
#define WLAN_SECURITY   WLAN_SEC_WPA2

int counter = 0;
uint32_t ip = 167772196;

Adafruit_CC3000_Client client;
const unsigned long
connectTimeout  = 15L * 1000L,
responseTimeout = 15L * 1000L;
int buffer_size = 20;

void setup() {
  setUpSerial();
  setUpWifi();
}

void loop() {
  postRequest();
  delay(5000);
  counter++;

  System.gc();
}

void setUpSerial(){
  Serial.begin(115200);
  while(!Serial);
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
}

void reboot() {
  if (counter==5) {
    rebootRequest();
    void (*reboot)(void) = 0;
    reboot();
  }
}

void postRequest(){
  String PostData = "someDataToPostsomeDataToPostsomeDataToPostsomeDataToPostsomeDataToPostsomeDataToPostsomeDataToPostsomeDataToPostsomeDataToPost";

  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
  client.connected();

  client.println("POST /feed/data HTTP/1.0"); 
  client.print("X-Auth:");
  client.println(hashAuth);
  client.print("Content-Length: ");
  client.println(PostData.length());
  client.println();
  client.println(PostData);

  while(client.connected() && !client.available()) delay(1);
  while (client.connected() || client.available()) { 
    client.read();
  }

  client.close();
}

void rebootRequest(){
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
  client.connected();

  client.println("GET /feed HTTP/1.0"); 
  client.print("X-Auth:");
  client.println(hashAuth);
  client.println();

  while(client.connected() && !client.available()) delay(1);
  while (client.connected() || client.available()) { 
    client.read();
  }

  client.close();
}





