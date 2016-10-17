/*
    RF Beacon

    v 3.0           201610170322Z       28166/28672/1763

    nearing production-quality code
    added TCS34725 sensor, removed UV
    current radiopacket format
        BY-WX.77,42,1012,2400,128,4000:
        from-type.temp,humi,pres,colorTemp,lux,vBat:
    
    
    v 2.2            201610151946Z       17794/28672/924
    
    cleaned up radiopacket build sequence, changed to sprintf
    
    v 2.0           201610130540Z

    added sensor data from BME280 to radiopacket; [2 character ID][1 character TX type][2 character temp C][2 character humidity%][4 character pressure mb][packetnum++]
        ID      RV  Rover
                BY  Back yard
                FY  Front yard
                MC  Mission Control
                
        TX      *   weather beacon
                #   general beacon
                !   command
                @   ping              
                
    v 1.1           201610090650Z
    
    cleaned up the code a bit
    
    v 1.0           201610080405Z

    initial commit of RFBeacon
    32u4 Feather LoRa with BME280
    beacons the current temperature in fahrenheit every $interval (default 2000ms)
 */

/* Sketch uses 23,932 bytes (83%) of program storage space. Maximum is 28,672 bytes.
Global variables use 1,664 bytes of dynamic memory. WITH OLED */

#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_FeatherOLED.h>

Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define Apin 9
#define Bpin 6
#define Cpin 5

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
 
#define pid "BY"
#define vers "3.0_dev"
#define pname "RF Beacon"

#define VBATPIN A9  // measure battery
#define RF95_FREQ 915.0  // RFM95 is the 900ISM version, so range is 902-928, 915 center.

RH_RF95 rf95(RFM95_CS, RFM95_INT);  // Singleton instance of the radio driver

Adafruit_BME280 bme; // use i2c, lets keep spi for rf

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

void setup()   
{                
    Serial.begin(9600);
    oledinit();  
    pinMode(A0, OUTPUT);
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);
    digitalWrite(RFM95_RST, LOW);   // manually reset the RF
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);

    while (!rf95.init()) {
        Serial.println("RFM95 init failed");
        while (1); }
    
    Serial.println("RFM95 init passed");

    if (!rf95.setFrequency(RF95_FREQ)) {
        Serial.println("setFrequency failed");
        while (1); }
  
    rf95.setTxPower(23, false);

    if (!bme.begin()) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1); }
        
    if (tcs.begin()) {
        Serial.println("Found sensor");
        } else {
        Serial.println("No TCS34725 found ... check your connections");
        while (1);
        }
}

#define LED 13

int16_t packetnum = 0;  // packet counter, we increment transmission
long previousMillis = 0;
long interval = 10000;  // interval between transmissions. 1000 is 1 second.

void loop()  // just transmit rpacket[] every $interval - no listening period (yet)
{
    unsigned long currentMillis = millis();
//    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(WHITE);
    int countdown = (((interval-(currentMillis-previousMillis)))/1000); 
    int progress = map(countdown, 0, 100, 16, 128);
    if(countdown==0) { display.setCursor(0,24); display.print("TX"); digitalWrite(A0, HIGH); } else { display.setCursor(countdown*10,24); display.print("<"); }          // display.setCursor(progress,24); display.print(countdown); }
    
    if(currentMillis - previousMillis > interval)
    {
        digitalWrite(13, HIGH);   // LED on, so we can see its doing something
        previousMillis = currentMillis;  // reset the timer so we know when we last TX'd
        int tmp = ((bme.readTemperature()*1.8)+32);  // get sensor values
        int humi = (bme.readHumidity());
        int pres = ((bme.readPressure() / 100.0F)+34);
        uint16_t r, g, b, c, colorTemp, lux;
        tcs.getRawData(&r, &g, &b, &c);
        colorTemp = tcs.calculateColorTemperature(r, g, b);
        lux = tcs.calculateLux(r, g, b);
  
        int measuredvbat = analogRead(VBATPIN);
        measuredvbat *= 2;    // voltage divider was used, so double the measurement
        measuredvbat *= 3.3;  // multiply by aref (3.3) - send this so i dont have to worry about decimals. /= 1024 to get voltage on the rx end.
        float cvbat = measuredvbat/=1024;
        display.clearDisplay();
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.print(tmp); display.print(" "); display.print(humi); display.print(" "); display.print(pres); display.print(" "); display.print(colorTemp); display.print(" "); display.println(lux); 
        display.println("T  H  Pr   cT    lx");
        display.print(cvbat); display.print(" v | "); display.print("int "); display.print(interval); display.println(" ms");
        
        
        char rpacket[64];
        sprintf(rpacket, "BY-WX.%d,%d,%d,%d,%d,%d:",tmp,humi,pres,colorTemp,lux,measuredvbat);  // use sprintf to build our packet! char to 64, then mask it out
        delay(50);  // wait a bit
        rf95.send((uint8_t *)rpacket, strlen(rpacket));  // send the packet we previously built
        rf95.waitPacketSent();  // wait for TX to complete
        digitalWrite(13, LOW);    // LED off, with the stuff and the delay(50) there should be plenty for a visible flicker
        digitalWrite(A0, LOW);
    }

    display.display();
}

void oledinit()
{

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
    display.display();
    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
    display.println(pname); 
    Serial.println(pname);
    display.print("v ");
    Serial.print("v ");
    display.println(vers);
    Serial.println(vers);
    display.setCursor(0,24);
    display.println("halosix technologies");
    Serial.println("halosix technologies");
    display.display();
    delay(3000);
    display.clearDisplay();
    
}