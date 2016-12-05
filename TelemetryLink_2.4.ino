/*

        TelemetryLink

        v 2.4           201612050522Z       55328/262144

        cleaned up display a bit
        added ping reply capability
        
        v 2.3           201612050512Z       58320/262144

        added totally sweet progress bar.  lowermost row is now a progress bar, left to right, showing progress to next status request.
        
        
        v 2.2           201612050504Z       58320/262144

        integrated ThingSpeak and M0 WINC1500 APIs to RFM95 codebase
        hardware: M0 Feather with RFM95 LoRa and OLED Wings
        this unit listens all the time, and every <interval>, it requests a status update from the next in a list of beacons
        previously, this required two units - one to request the status updates, and a Huzzah to listen and relay to ThingSpeak. this unit combines both functions into a more stable hardware environment.
        

*/


#include <SPI.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <RH_RF95.h>
#include <Adafruit_FeatherOLED.h>
Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

#define RFM95_CS  10   // "B"  // m0 pinmapping
#define RFM95_RST 11   // "A"
#define RFM95_INT  6   // "D"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define Apin 9
#define Bpin 6
#define Cpin 5
#define VBATPIN A9

#define pid "TL"
#define vers "2.4"
#define pname "TelemetryLink"

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define RF95_FREQ 915.0 

RH_RF95 rf95(RFM95_CS, RFM95_INT);


char ssid[] = "******";
char pass[] = "******";
int status = WL_IDLE_STATUS;

unsigned long myChannelNumber = ******;
const char * myWriteAPIKey = "******";

unsigned long myChannelNumber2 = ******;
const char * myWriteAPIKey2 = "******";

unsigned long myChannelNumber3 = ******;
const char * myWriteAPIKey3 = "******";

WiFiClient client;

void setup() {

  WiFi.setPins(8,7,4,2);
  pinMode(Apin, INPUT_PULLUP);
  pinMode(Bpin, INPUT_PULLUP);
  pinMode(Cpin, INPUT_PULLUP);

  Serial.begin(9600);

  oledinit();

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  digitalWrite(RFM95_RST, LOW);   // manually reset the RF
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init())   // make sure the RF side is ready
  {
      Serial.println("RFM95 init failed");
      while (1);
  }
  Serial.println("RFM95 init passed");

  if (!rf95.setFrequency(RF95_FREQ)) 
  {
    Serial.println("setFrequency failed");
    while (1);
  }
  
  rf95.setTxPower(23, false);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi pinout probably wrong");
    while (true);
  }

  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
  display.print("WiFi connected!"); 
  display.display();

  ThingSpeak.begin(client);  // set up the connection to ThingSpeak
}

long previousMillis = 0;
long interval = 30000;  // interval between transmissions, 1000 is 1 second
int lasttx = 0;
int diff = 0;
int progmark = 0;

void loop() {

    unsigned long currentMillis = millis();

    diff = (currentMillis - previousMillis);
    progmark = map(diff, 0, interval, 0, 128);
    display.setCursor(progmark,24); display.print("-"); display.display();  // oh yeah, thats a motherfuckin progress bar! :D

    if (currentMillis - previousMillis >= interval)  // check the timer, if its interval, then beacon the next node on the list. this is extensible, just add a function at the bottom and increment lasttx++
    {
        previousMillis = currentMillis;
        if (lasttx == 0) dctx();
        else if (lasttx == 1) bdtx();
        else if (lasttx == 2) bstx();      
    }

    gateway();  // after we check the timer, we listen
 
}

void gateway()
{
      if (rf95.available())
  {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
      RH_RF95::printBuffer("Received: ", buf, len);

        String FI = strtok((char*)buf, ",");  // start parsing the received string -  return data to the left of it as a String, passing the remainder to the next strtok()
        String PC = strtok(NULL, ",");
        String PS = strtok(NULL, ",");
        String SN = strtok(NULL, ",");
        String ST = strtok(NULL, ",");
        String DS = strtok(NULL, ",");
        String F1 = strtok(NULL, ",");
        String F2 = strtok(NULL, ";");

      if ((FI == "TN") && (PC == "CP") && (PS == "PG") && (DS == "TL"))  // listens for a TNCP ping addressed to it
      { tncppg(); }

      if ((FI == "TN") && (PC == "WX") && (PS == "WC") && (SN == "BD") && (DS == "BC"))  // bedroom, BME and TSL
      {
        display.clearDisplay();
        
        int UTP = atoi(strtok(NULL, ",")); // parses out temperature
        int UHD = atoi(strtok(NULL, ",")); // parses out humidity
        int UPR = atoi(strtok(NULL, ",")); // parses out pressure in millibars
        int ULX = atoi(strtok(NULL, ",")); // parses out lux

        ThingSpeak.setField(1, UTP);
        ThingSpeak.setField(2, UHD);
        ThingSpeak.setField(3, UPR);
        ThingSpeak.setField(4, ULX);
        ThingSpeak.writeFields(myChannelNumber2, myWriteAPIKey2);
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.println("Bedroom"); 
        display.print(UTP); display.print(" deg "); display.print(UHD); display.println("% humidity"); 
        display.print(ULX); display.println(" lx.");
        display.display();
      }
      else if ((FI == "TN") && (PC == "WX") && (PS == "WC") && (SN == "DC") && (DS == "BC"))  // deck, BME and TCS
      {
        display.clearDisplay();
        
        int DTP = atoi(strtok(NULL, ",")); // parses out temperature
        int DHD = atoi(strtok(NULL, ",")); // parses out humidity
        int DPR = atoi(strtok(NULL, ",")); // parses out pressure in millibars
        int DCT = atoi(strtok(NULL, ",")); // parses out color temp
        int DLX = atoi(strtok(NULL, ",")); // parses out lux
        int DBT = atoi(strtok(NULL, ":")); // ending with : to return the remote battery level

        ThingSpeak.setField(1, DTP);
        ThingSpeak.setField(2, DHD);
        ThingSpeak.setField(3, DPR);
        ThingSpeak.setField(4, DCT);
        ThingSpeak.setField(5, DLX);
        ThingSpeak.setField(6, DBT);
        ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.println("Deck"); 
        display.print(DTP); display.print(" deg "); display.print(DHD); display.println("% humidity"); 
        display.print(DLX); display.print(" lx. "); display.print(DCT); display.println(" K cT");
        display.display();
      }

      else if ((FI == "TN") && (PC == "WX") && (PS == "WC") && (SN == "BS") && (DS == "BC"))  // basement, BME and TCS
      {
        display.clearDisplay();
        
        int BTP = atoi(strtok(NULL, ",")); // parses out temperature
        int BHD = atoi(strtok(NULL, ",")); // parses out humidity
        int BPR = atoi(strtok(NULL, ",")); // parses out pressure in millibars
        int BCT = atoi(strtok(NULL, ",")); // parses out color temp
        int BLX = atoi(strtok(NULL, ",")); // parses out lux
        int BBT = atoi(strtok(NULL, ":")); // ending with : to return the remote battery level

        ThingSpeak.setField(1, BTP);
        ThingSpeak.setField(2, BHD);
        ThingSpeak.setField(3, BPR);
        ThingSpeak.setField(4, BCT);
        ThingSpeak.setField(5, BLX);
        ThingSpeak.setField(6, BBT);
        ThingSpeak.writeFields(myChannelNumber3, myWriteAPIKey3);

        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.println("Basement"); 
        display.print(BTP); display.print(" deg "); display.print(BHD); display.println("% humidity"); 
        display.print(BLX); display.print(" lx."); display.print(BCT); display.println(" K cT");
        display.display();
      }

    }
    else {
      Serial.println("Receive failed");
    }
  }
}

void dctx()  // requesting deck node to transmit status
{
        lasttx = 1;
        char radiopacket[64];
        sprintf(radiopacket, "TN,CP,ST,GW,TX,DC,99,99;");
        display.clearDisplay();
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.println("status request");
        display.println("to DECK");
        display.println("TNCP ST DECK");
        display.display();
        rf95.send((uint8_t *)radiopacket, strlen(radiopacket));
        rf95.waitPacketSent();
        digitalWrite(13, HIGH); 
        digitalWrite(13, LOW);
}

void bdtx()  // requesting bedroom node to transmit status
{
        lasttx = 2;
        char radiopacket[64];
        sprintf(radiopacket, "TN,CP,ST,GW,TX,BD,99,99;");
        display.clearDisplay();
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.println("status request");
        display.println("to BEDROOM");
        display.println("TNCP ST BEDROOM");
        display.display();
        rf95.send((uint8_t *)radiopacket, strlen(radiopacket));
        rf95.waitPacketSent();
        digitalWrite(13, HIGH); 
        digitalWrite(13, LOW);
}

void bstx()  // requesting basement node to transmit status
{
        lasttx = 0;
        char radiopacket[64];
        sprintf(radiopacket, "TN,CP,ST,GW,TX,BS,99,99;");
        display.clearDisplay();
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.println("status request");
        display.println("to BASEMENT");
        display.println("TNCP ST BASEMENT");
        display.display();
        rf95.send((uint8_t *)radiopacket, strlen(radiopacket));
        rf95.waitPacketSent();
        digitalWrite(13, HIGH); 
        digitalWrite(13, LOW);
}

void tncppg()  // responding to a ping
{
        int PGRSSI = (rf95.lastRssi(), DEC);
        char radiopacket[64];
        sprintf(radiopacket, "TN,CP,PR,TL,TX,BC,%d,99;",PGRSSI);
        display.clearDisplay();
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.println("sending ping reply");
        display.print("TNCP PING REPLY"); display.println(PGRSSI);
        display.display();
        rf95.send((uint8_t *)radiopacket, strlen(radiopacket));
        rf95.waitPacketSent();
        digitalWrite(13, HIGH); 
        digitalWrite(13, LOW);
}

void oledinit()
{

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
    display.display();
    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
    display.print(pname); display.print(" "); display.println(vers);
    display.setCursor(0,16); display.print("device ID ["); display.print(pid); display.println("]");
    display.println("halosix technologies");
    display.display();
    delay(5000);
    display.clearDisplay();
    
}

