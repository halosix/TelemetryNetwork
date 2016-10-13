/*
    RF Beacon

    v 2.0        20161013054Z

    added sensor data from BME280 to radiopacket; [2 character ID][1 character TX type][2 character temp C][2 character humidity%][4 character pressure mb][packetnum++]
        ID      RV  Rover

        TX      *   beacon
                !   ping
    
    v 1.1        201610090650Z
    
    cleaned up the code a bit
    
    v 1.0       201610080405Z

    initial commit of RFBeacon
    32u4 Feather LoRa with BME280
    beacons the current temperature in fahrenheit every $interval (default 2000ms)
 */

#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_FeatherOLED.h>
Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
 
#define pid "RV"
#define vers "2.0"
#define pname "RF Beacon"

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define RF95_FREQ 915.0  // RFM95 is the 900ISM version, so range is 902-928, 915 center.

RH_RF95 rf95(RFM95_CS, RFM95_INT);  // Singleton instance of the radio driver

Adafruit_BME280 bme; // i2c, we want to keep spi open for RF

void setup()   
{                
    oledinit();

    Serial.begin(9600);
    Serial.println(pname);
    Serial.print("v ");
    Serial.println(vers);
    Serial.println("halosix technologies");
    
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

    if (!bme.begin()) 
    {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
}

#define LED 13

int16_t packetnum = 0;  // packet counter, we increment transmission
long previousMillis = 0;
long interval = 1000;  // interval between transmissions. 1000 is 1 second.

void loop()  // this is basically hitb() from RF Universal, substituting float tmp for packetnum
{
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis > interval)
    {
        float tmp = (bme.readTemperature());  // get sensor values
        float humi = (bme.readHumidity());
        float pres = ((bme.readPressure() / 100.0F)+34);
        previousMillis = currentMillis;  // reset the timer so we know when we last TX'd
        char radiopacket[64] = "RV*";  // start the radiopacket
        itoa(tmp, radiopacket+3, 10);  // add each sensor value, convert float to string and place in array
        itoa(humi, radiopacket+5, 10);
        itoa(pres, radiopacket+7, 10);
        itoa(packetnum++, radiopacket+11, 10);
        radiopacket[63] = 0;  // terminate the array with a null
        display.clearDisplay();  // clear the display and get it ready to write
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        delay(50);  // wait a bit
        rf95.send((uint8_t *)radiopacket, 64);  // here we send
        rf95.waitPacketSent();  // wait for TX to complete
        display.println("continuous mode...");
        display.print("beaconing on "); display.println(RF95_FREQ);
        display.print(interval); display.println(" ms interval");
        display.println(radiopacket);
        display.display();
        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(50);              // wait a bit so it flickers. thats what she said.
        digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    }
}

void oledinit()
{
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
    display.display();
    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
    display.println(pname); 
    display.print("v "); display.println(vers);
    display.setCursor(0,24);
    display.println("halosix technologies");
    display.display();
    delay(3000);
    display.clearDisplay();
}