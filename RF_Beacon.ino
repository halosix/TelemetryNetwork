/*
    RF Beacon

    HARDWARE
        designed for and tested on 32u4 Feather with RFM95 LoRa, BME280, and TCS34725
        pinout: payload board, 220uF cap across rails, BME280 and TCS34725 to 3v3 GND SDA SCL, red LED across rails, green LED to A0
        this will probably work with any arduino-compatible board, note that A9 is connected to vBat on the Feather
        

    v 3.3           201610190221Z       27666/28672/1739

    condensed the battery level math
    added a sanity check to lux so cT doesnt overflow the display
    added a sanity check to lux so lux doesnt overflow the display
    rewrote the display for better consistency
    NOTE: 3.2 code ran on two devices (BY and RV) for ~24 hours with no issues, battery capacity seems adequate
    
    
    v 3.2           201610170626Z       27672/28672/1741

    removed currentMillis / interval stuff, replaced with radio.sleep() and Watchdog.sleep(), added #include Adafruit_SleepyDog.h [for power reasons, we'll see if it matters]
    
    
    v 3.1           201610170528Z       27678/28672/1743

    changed battery voltage reporting around
    
    
    v 3.0           201610170322Z       28166/28672/1763

    nearing production-quality code
    added TCS34725 sensor, removed UV
   
    
    v 2.2            201610151946Z       17794/28672/924
    
    cleaned up radiopacket build sequence, changed to sprintf
    
    
    v 2.0           201610130540Z

    added sensor data from BME280 to radiopacket; [2 character ID][1 character TX type][2 character temp C][2 character humidity%][4 character pressure mb][packetnum++]
               
    
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
#include <Adafruit_SleepyDog.h>

Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define Apin 9
#define Bpin 6
#define Cpin 5
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define pid "RV-WX"
#define vers "3.3"
#define pname "RF Beacon"
#define VBATPIN A9  // measure battery
#define RF95_FREQ 915.0  // RFM95 is the 900ISM version, so range is 902-928, 915 center.
#define LED 13

long interval = 10000;  // interval between transmissions. 1000 is 1 second.

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

    if (!bme.begin()) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1); }
        
    if (tcs.begin()) {
        Serial.println("Found sensor");
        } else {
        Serial.println("No TCS34725 found ... check your connections");
        while (1);
        }
    rf95.setTxPower(23, false);
}

void loop()  // just transmit rpacket[] every $interval - no listening period
{
    display.setTextSize(1); display.setTextColor(WHITE);
    
        digitalWrite(13, HIGH);   // LED on, so we can see its doing something
        int tmp = ((bme.readTemperature()*1.8)+32);  // get sensor values
        int humi = (bme.readHumidity());
        int pres = ((bme.readPressure() / 100.0F)+34);
        uint16_t r, g, b, c, colorTemp, lux;
        tcs.getRawData(&r, &g, &b, &c);
        colorTemp = tcs.calculateColorTemperature(r, g, b);
        lux = tcs.calculateLux(r, g, b);

        if (lux<=10) colorTemp = 1;  // low light levels make the color temp go wonky
  
        float measuredvbat = analogRead(VBATPIN);
        measuredvbat *= .64453125; // math to get voltage to feed into batlvl
        int batlvl = map(measuredvbat, 320, 420, 5, 95);  // then take all that and map it to 5-95 to approximate battery % remaining
                
        display.clearDisplay();
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        display.print(pname); display.print(" "); display.println(vers);
        display.setCursor(0,16); display.print(tmp); display.print(" "); display.print(humi); display.print(" "); display.print(pres); display.print(" "); display.print(colorTemp); display.print(" "); display.println(lux); 
        display.print("bat. "); display.print(batlvl); display.print("%    "); display.print("int "); display.print(interval/1000); display.println(" s");
        display.setCursor(0,24);
                
        digitalWrite(A0, HIGH);
        char rpacket[64];
        sprintf(rpacket, "RV-WX:%d,%d,%d,%d,%d,%d:",tmp,humi,pres,colorTemp,lux,batlvl);  // use sprintf to build our packet! char to 64, then mask it out
        delay(50);  // wait a bit
        rf95.send((uint8_t *)rpacket, strlen(rpacket));  // send the packet we previously built
        rf95.waitPacketSent();  // wait for TX to complete
        digitalWrite(13, LOW);    // LED off, with the stuff and the delay(50) there should be plenty for a visible flicker
        digitalWrite(A0, LOW);
        
    display.display();

    rf95.sleep();
    Watchdog.sleep(interval);
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