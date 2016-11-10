/*
    RF Telemetry Gateway

    v 1.7       201611100137Z   62048/262144

    huzzah had stability issues, migrated to m0 feather with WINC1500 wifi + radio feather. much more stable.
    this code has run stable for ~48 hours with no crashes or resets.
    
    v 1.0       201611070648Z   299625/1044464/39848/81920

    Based on a HUZZAH Feather, paired with an RFM95 Featherwing.  Listens on selected frequency, parses received data, and exports it to Adafruit IO.


    Code uses examples provided by Adafruit.  
    Adafruit invests time and resources providing this open source code.
    Please support Adafruit and open source hardware by purchasing
    products from Adafruit!
*/

#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_FeatherOLED.h>

Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define RFM95_CS  10   // "B"  // pinmapping for m0
#define RFM95_RST 11   // "A"
#define RFM95_INT  6   // "D"
#define RF95_FREQ 915.0
RH_RF95 rf95(RFM95_CS, RFM95_INT);
#define pid "RF-GW"
#define vers "1.7_dev"
#define pname "RF Gateway"

#define Apin 9  // buttons on OLED
#define Bpin 6
#define Cpin 5

#define LED 13  // blinky

#define IO_USERNAME    "your_username"
#define IO_KEY         "your_key"
#define WIFI_SSID       "your_ssid"
#define WIFI_PASS       "your_password"
#include "AdafruitIO_WiFi.h"
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// setup the feeds
AdafruitIO_Feed *DTP = io.feed("deck-temperature");
AdafruitIO_Feed *DHD = io.feed("deck-humidity");
AdafruitIO_Feed *DPR = io.feed("deck-pressure");
AdafruitIO_Feed *DLX = io.feed("deck-lux");
AdafruitIO_Feed *DBT = io.feed("deck-battery");
AdafruitIO_Feed *UTP = io.feed("upstairs-temperature");
AdafruitIO_Feed *UHD = io.feed("upstairs-humidity");
AdafruitIO_Feed *UPR = io.feed("upstairs-pressure");
AdafruitIO_Feed *ULX = io.feed("upstairs-lux");

void setup() 
{  
  
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  pinMode(Apin, INPUT_PULLUP);
  pinMode(Bpin, INPUT_PULLUP);
  pinMode(Cpin, INPUT_PULLUP);

  oledinit();
  
  Serial.begin(9600);
  delay(100);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(23, false);

  // connect to io.adafruit.com
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(250);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0, 0);
  display.print(io.statusText()); display.display();
  
}

void loop()
{

  io.run();
  
  if (rf95.available())
  {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      RH_RF95::printBuffer("Received: ", buf, len);

      /* considering adding a version # in the packet, could have different functions for different versions */

      String SN = strtok((char*)buf, ":");  // start parsing the received string -  return data to the left of it as a String, passing the remainder to the next strtok()
      int TP = atoi(strtok(NULL, ",")); // parses out temperature
      int HD = atoi(strtok(NULL, ",")); // parses out humidity
      int PR = atoi(strtok(NULL, ",")); // parses out pressure in millibars
      int CT = atoi(strtok(NULL, ",")); // parses out color temp
      int LX = atoi(strtok(NULL, ",")); // parses out lux
      int BT = atoi(strtok(NULL, ":")); // ending with : to return the remote battery level
      
      if (SN=="UP-WX")  // upstairs unit, BME280 and TSL, so no colortemp. plugged in, so i dont care about battery level.
        { 
            digitalWrite(LED, HIGH);
            UTP->save(TP);
            UHD->save(HD);
            UPR->save(PR);
            ULX->save(LX);
        }

        else if (SN=="BY-WX")  // deck unit, BME280 and TCS, powered via battery/solar
        { 
            digitalWrite(LED, HIGH);
            DTP->save(TP);
            DHD->save(HD);
            DPR->save(PR);
            DLX->save(LX);
            DBT->save(BT);
        }
            display.clearDisplay(); display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0, 0);
            display.print(SN); display.print(" - "); display.print(BT); display.print("% RSSI "); display.println(rf95.lastRssi(), DEC);
            display.print(TP); display.print("* "); display.print(HD); display.print("% "); display.print(PR); display.println(" mb");
            display.print(CT); display.print(" K  "); display.print(LX); display.println(" lux");
            display.display();
            digitalWrite(LED, LOW);  
    }
    else { Serial.println("Receive failed"); }
  }
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
