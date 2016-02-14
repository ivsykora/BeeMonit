#include <Time.h>
#include <Adafruit_CC3000.h>
#include <SPI.h>

#define ADAFRUIT_CC3000_IRQ   3 
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);
#define WLAN_SSID       "saew"          
#define WLAN_PASS       "K0y0jedz2"
#define WLAN_SECURITY   WLAN_SEC_WPA2

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
  Serial.println("loop");
  postRequest();
  delay(5000);
}

void setUpSerial(){
  Serial.begin(115200);
  while(!Serial);
  Serial.println(F("Serial initialized!"));
} 

void setUpWifi(){
  if (!cc3000.begin()){
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }

  Serial.print(F("\nAttempting to connect to ")); 
  Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }

  Serial.println(F("Connected!"));

  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }

  while (! displayConnectionDetails()) {
    delay(1000);
  }  
}

void postRequest(){
  uint32_t ip = 167772196;

  Serial.println(F("Trying to connect to server."));
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
  if (client.connected()) {
    Serial.println(F("Connected to server."));
  }

  client.println("GET /feed HTTP/1.0"); 
  client.println();

  while(client.connected() && !client.available()) delay(1);
  while (client.connected() || client.available()) { 
    char c = client.read();
    Serial.print(c);
  }

  client.close();
  Serial.println(F("Closing connection"));
}


bool displayConnectionDetails(){
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); 
    cc3000.printIPdotsRev(ipAddress);
    Serial.println();
    return true;
  }
}


