// start of macros dbg and dbgi
#define dbg(myFixedText, variableName) \
  Serial.print( F(#myFixedText " "  #variableName"=") ); \
  Serial.println(variableName);
// usage: dbg("1:my fixed text",myVariable);
// myVariable can be any variable or expression that is defined in scope

#define dbgi(myFixedText, variableName,timeInterval) \
  do { \
    static unsigned long intervalStartTime; \
    if ( millis() - intervalStartTime >= timeInterval ){ \
      intervalStartTime = millis(); \
      Serial.print( F(#myFixedText " "  #variableName"=") ); \
      Serial.println(variableName); \
    } \
  } while (false);
// end of macros dbg and dbgi


// I wrote some basic documentation about receiving the UDP-messages with python at the end of the file
#include <WiFi.h>
#include <SafeString.h>

#define MaxMsgLength 1024
createSafeString(UDP_Msg_SS,MaxMsgLength); 
uint8_t UDP_Msg_uint8_Buffer[MaxMsgLength + 1]; // for some strange reasons on ESP32 the udp.write-function needs an uint8_t-array

#define MaxHeaderLength 32 
createSafeString(Header_SS,MaxHeaderLength);

#define MaxTimeStampLength 64 
createSafeString(TimeStamp_SS,MaxTimeStampLength);


char HeaderDelimiter = '$'; // must match the delimiter defined in the python-code 


const char *ssid     = ""; 

const char *password = "";

IPAddress    remoteIP     (192, 168, 178, 160); // receiver-IP
unsigned int remotePort = 4210;                 // receiver port to listen on must match the portnumber the receiver is listening to

WiFiUDP Udp;

const char* ntpServer = "fritz.box";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 7200;

#include <time.h>                   // time() ctime()
time_t now;                         // this is the epoch
tm myTimeInfo;                      // the structure tm holds time information in a more convient way


boolean TimePeriodIsOver (unsigned long &expireTime, unsigned long TimePeriod) {
  unsigned long currentMillis  = millis();
  if ( currentMillis - expireTime >= TimePeriod )
  {
    expireTime = currentMillis; // set new expireTime
    return true;                // more time than TimePeriod) has elapsed since last time if-condition was true
  }
  else return false;            // not expired
}

const byte OnBoard_LED = 2;
int BlinkTime = 500;

void BlinkHeartBeatLED(int IO_Pin, int BlinkPeriod) {
  static unsigned long MyBlinkTimer;
  pinMode(IO_Pin, OUTPUT);

  if ( TimePeriodIsOver(MyBlinkTimer, BlinkPeriod) ) {
    digitalWrite(IO_Pin, !digitalRead(IO_Pin) );
  }
}

unsigned long TestTimer;
unsigned long UDP_SendTimer;

int myCounter = 0;
int HeaderNr  = 0;


void PrintFileNameDateTime()
{
  Serial.print("Code running comes from file ");
  Serial.println(__FILE__);
  Serial.print(" compiled ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
}

void showTime() {
  time(&now);                       // read the current time
  localtime_r(&now, &myTimeInfo);           // update the structure tm with the current time
  Serial.print("year:");
  Serial.print(myTimeInfo.tm_year + 1900);  // years since 1900
  Serial.print("\tmonth:");
  Serial.print(myTimeInfo.tm_mon + 1);      // January = 0 (!)
  Serial.print("\tday:");
  Serial.print(myTimeInfo.tm_mday);         // day of month
  Serial.print("\thour:");
  Serial.print(myTimeInfo.tm_hour);         // hours since midnight  0-23
  Serial.print("\tmin:");
  Serial.print(myTimeInfo.tm_min);          // minutes after the hour  0-59
  Serial.print("\tsec:");
  Serial.print(myTimeInfo.tm_sec);          // seconds after the minute  0-61*
  Serial.print("\twday");
  Serial.print(myTimeInfo.tm_wday);         // days since Sunday 0-6
  if (myTimeInfo.tm_isdst == 1)             // Daylight Saving Time flag
    Serial.print("\tDST");
  else
    Serial.print("\tstandard");
    
  Serial.println();
}


void StoreTimeStampIntoSS(SafeString& p_RefToSS, tm p_myTimeInfo) {

  time(&now);                               // read the current time
  localtime_r(&now, &myTimeInfo);           // update the structure tm with the current time

  //p_RefToSS = " ";
  p_RefToSS  = myTimeInfo.tm_year + 1900;
  p_RefToSS += ".";

  // month
  if (p_myTimeInfo.tm_mon + 1 < 10) {
    p_RefToSS += "0";
  }  
  p_RefToSS += myTimeInfo.tm_mon + 1;

  p_RefToSS += ".";

  // day
  if (p_myTimeInfo.tm_mday + 1 < 10) {
    p_RefToSS += "0";
  }    
  p_RefToSS += myTimeInfo.tm_mday;

  p_RefToSS += "; ";

  // hour
  if (p_myTimeInfo.tm_hour < 10) {
    p_RefToSS += "0";
  }    
  p_RefToSS += myTimeInfo.tm_hour;
  p_RefToSS += ":";

  // minute
  if (p_myTimeInfo.tm_min < 10) {
    p_RefToSS += "0";
  }    
  p_RefToSS += myTimeInfo.tm_min;
  
  p_RefToSS += ":";

  // second
  if (p_myTimeInfo.tm_sec < 10) {
    p_RefToSS += "0";
  }    
  p_RefToSS += myTimeInfo.tm_sec;
  //p_RefToSS += ",";  
}


void connectToWifi() {
  Serial.print("Connecting to "); 
  Serial.println(ssid);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    BlinkHeartBeatLED(OnBoard_LED, 333);
    delay(332);
    Serial.print(".");
  }
  Serial.print("\n connected.");
  Serial.println(WiFi.localIP() );

}

void synchroniseWith_NTP_Time() {
  Serial.print("configTime uses ntpServer ");
  Serial.println(ntpServer);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("synchronising time");
  
  while (myTimeInfo.tm_year + 1900 < 2000 ) {
    time(&now);                       // read the current time
    localtime_r(&now, &myTimeInfo);
    BlinkHeartBeatLED(OnBoard_LED, 100);
    delay(100);
    Serial.print(".");
  }
  Serial.print("\n time synchronsized \n");
  showTime();    
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n Setup-Start \n");
  PrintFileNameDateTime();
  Serial.print("InitSensor() done \n");
  
  connectToWifi();
  synchroniseWith_NTP_Time();
  Header_SS = "Header"; 
}

void PrintMsg() {
  Serial.print("UDP_Msg_SS #");
  Serial.print(UDP_Msg_SS);
  Serial.println("#");
}

void loop() {
  BlinkHeartBeatLED(OnBoard_LED, BlinkTime);

  if (TimePeriodIsOver(UDP_SendTimer, 2000) ) {
    Serial.print("Send Message to #");
    Serial.print(remoteIP);
    Serial.print(":");
    Serial.println(remotePort);

    UDP_Msg_SS = "";

    UDP_Msg_SS = Header_SS;
    UDP_Msg_SS += HeaderDelimiter;

    StoreTimeStampIntoSS(TimeStamp_SS,myTimeInfo);
    UDP_Msg_SS += TimeStamp_SS;

    UDP_Msg_SS += ",my Testdata1,";
    UDP_Msg_SS += 123;
    UDP_Msg_SS += ",my Testdata2,";

    UDP_Msg_SS += 789;
    UDP_Msg_SS += ",";
    
    UDP_Msg_SS += myCounter++; 

    dbg("Send UDP_Msg #",UDP_Msg_SS);
    dbg("length:",UDP_Msg_SS.length());
    Udp.beginPacket(remoteIP, remotePort);
    Udp.write((const uint8_t*)UDP_Msg_SS.c_str(), UDP_Msg_SS.length() );  
    Udp.endPacket();
  }    
}
