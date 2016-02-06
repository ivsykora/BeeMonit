#include <SD.h>
#include <Time.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>


#define ADAFRUIT_CC3000_IRQ   3 
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);
#define WLAN_SSID       "qwerty"          
#define WLAN_PASS       "qwerty"
#define WLAN_SECURITY   WLAN_SEC_WPA2

Adafruit_CC3000_Client client;
const unsigned long
connectTimeout  = 15L * 1000L,
responseTimeout = 15L * 1000L;

const unsigned long EVERY_FIFTEEN_MINUTES [] = {
  0, 15, 30, 45};  
const unsigned int BYTE_LENGTH_OF_UNSIGNED_LONG = 4;

Sd2Card card;
const int chipSelect = 4;
SdVolume volume;
SdFile root;

#define LISTEN_PORT           80
#define MAX_ACTION            10
#define MAX_PATH              64
#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20 
#define TIMEOUT_MS            500
Adafruit_CC3000_Server httpServer(LISTEN_PORT);
uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;
char action[MAX_ACTION+1];
char path[MAX_PATH+1];

void setup() {
  setUpSerial();
  setUpWifi();
  setUpTime();
  setUpSDCard();
  setUpHttpServer();

  writeToLog(getTimeAsString());
}

void loop() {
  checkIfRequestAndHandleIt();
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

void setUpTime(){
  while (timeStatus()==timeNotSet){
    unsigned long t = getTimeFromNTP();
    if (t) {
      setTime(t);
    }
  }
}

void setUpSDCard (){
  Serial.println(F("\nInitializing SD Card"));

  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);

  delay(500);

  while(!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    delay(2000);
    Serial.print("Initializing SD card again...");
  }
  Serial.println("initialization done.");

  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println(F("initialization failed. Things to check:"));
    Serial.println(F("* is a card is inserted?"));
    Serial.println(F("* Is your wiring correct?"));
    Serial.println(F("* did you change the chipSelect pin to match your shield or module?"));
    return;
  } 
  else {
    Serial.println(F("Wiring is correct and a card is present.")); 
  }

  if (!volume.init(card)) {
    Serial.println(F("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card"));
    return;
  }

  delay(500);
}

void setUpHttpServer(){
  httpServer.begin();
  Serial.println(F("\nStarted HTTP server!"));
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
    Serial.print(F("\nNetmask: ")); 
    cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); 
    cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); 
    cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); 
    cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

unsigned long getTimeFromNTP() {
  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;

  Serial.print(F("Locating time server..."));

  if(cc3000.getHostByName("pool.ntp.org", &ip)) {
    static const char PROGMEM
      timeReqA[] = {227,  0,  6, 236},
    timeReqB[] = {49, 78, 49,  52};

    Serial.println(F("\r\nAttempting connection..."));
    startTime = millis();
    do {
      client = cc3000.connectUDP(ip, 123);
    } 
    while((!client.connected()) &&
      ((millis() - startTime) < connectTimeout));

    if(client.connected()) {
      Serial.print(F("connected!\r\nIssuing request..."));

      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      client.write(buf, sizeof(buf));

      Serial.print(F("\r\nAwaiting response..."));
      memset(buf, 0, sizeof(buf));
      startTime = millis();
      while((!client.available()) &&
        ((millis() - startTime) < responseTimeout));
      if(client.available()) {
        client.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
          ((unsigned long)buf[41] << 16) |
          ((unsigned long)buf[42] <<  8) |
          (unsigned long)buf[43]) - 2208988800UL;
        Serial.print(F("OK\r\n"));
      }
      client.close();
    }
  }
  if(!t) Serial.println(F("error"));
  return t;
}

void writeToLog(String message) {
  File log;
  String _logName = getPathToLog();
  char logName[(_logName.length()+1)];
  _logName.toCharArray(logName, sizeof(logName));
  
  existsOrCreateLogFolder();

  if(!SD.exists(logName)){
    log = SD.open(logName, FILE_WRITE);
    log.close();
    Serial.println(F("Creating log file for today "));
  }

  log = SD.open(logName, FILE_WRITE);
  if (log) {
    log.println(message);
  } 
  else {
    Serial.println(F("Failed to open log."));
  }
  log.close();
}

void existsOrCreateLogFolder () {
  String _logFolder = String(year());
  char logFolder [sizeof(_logFolder)];
  _logFolder.toCharArray(logFolder, sizeof(logFolder));
  
  if (!SD.exists(logFolder)) {
    SD.mkdir(logFolder);
  }
}

void checkIfRequestAndHandleIt(){
  Adafruit_CC3000_ClientRef client = httpServer.available();
  if (client) {
    Serial.println("Connection");
    String _data = getDataFromLog();
    char data [(_data.length()+1)];
    _data.toCharArray(data, sizeof(data));
    Serial.println("Read data from log file.");
    
    bufindex = 0;
    memset(&buffer, 0, sizeof(buffer));
    memset(&action, 0, sizeof(action));
    memset(&path,   0, sizeof(path));
    unsigned long endtime = millis() + TIMEOUT_MS;
    bool parsed = false;
    while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE)) {
      if (client.available()) {
        buffer[bufindex++] = client.read();
      }
      parsed = parseRequest(buffer, bufindex, action, path);
    }
    if (parsed) {
      Serial.println("Parsed request -> responding");
      if (strcmp(action, "GET") == 0) {
        client.fastrprintln(F("HTTP/1.1 200 OK"));
        client.fastrprintln(F("Content-Type: text/plain"));
        client.fastrprintln(F("Connection: close"));
        client.fastrprintln(F("Server: Adafruit CC3000"));
        client.fastrprintln(F(""));
        client.fastrprintln("");

      }
      else {
        client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
        client.fastrprintln(F(""));
      }
    }
    delay(100);
    client.close();
    Serial.println("Connection closed");
  }
}

bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path) {
  if (bufSize < 2)
    return false;
  if (buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n') {
    parseFirstLine((char*)buf, action, path);
    return true;
  }
  return false;
}

void parseFirstLine(char* line, char* action, char* path) {
  char* lineaction = strtok(line, " ");
  if (lineaction != NULL)
    strncpy(action, lineaction, MAX_ACTION);
  char* linepath = strtok(NULL, " ");
  if (linepath != NULL)
    strncpy(path, linepath, MAX_PATH);
}

String getDataFromLog() {
  String data = "";
  
  File log;
  String _logName = getPathToLog();
  char logName[(_logName.length()+1)];
  _logName.toCharArray(logName, sizeof(logName));
  
  log = SD.open(logName);

  if (log) {
    char character;
    while (log.available()) {
      character = log.read();
      data.concat(character);
    }
    log.close();
  }
  
  
  return data;
}

// /yyyy/MM-dd
String getPathToLog () {
  return String(year()) + "/" + computeDateTimeDigit(month()) + "-" + computeDateTimeDigit(day());
}

// yyyy-MM-dd hh:mm:ss
String getTimeAsString () {
  return  getDateAsString() + " " + computeDateTimeDigit(hour()) + ":" + computeDateTimeDigit(minute()) + ":" + computeDateTimeDigit(second());
}
// yyyy-MM-dd
String getDateAsString () {
  return String(year()) + "-" + computeDateTimeDigit(month()) + "-" + computeDateTimeDigit(day());
}

String computeDateTimeDigit (int digit) {
  String s ="";
  if (digit<10){
    s = "0";
  }
  s = s + digit;
  return s;
}




