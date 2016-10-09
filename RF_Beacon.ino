/*
    RF Beacon

    v 1.1		201610090650Z
	
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

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
 
#define pid "9X"
#define vers "1.1"
#define pname "RF Beacon"

#define RF95_FREQ 915.0  // RFM95 is the 900ISM version, so range is 902-928, 915 center.

RH_RF95 rf95(RFM95_CS, RFM95_INT);  // Singleton instance of the radio driver

Adafruit_BME280 bme; // I2C

void setup()   {                
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

long previousMillis = 0;
long interval = 2000;  // interval between transmissions. 1000 is 1 second.

void loop()  // this is basically hitb() from RF Universal, substituting float tmp for packetnum
{
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis > interval)
    {
        float tmp = ((bme.readTemperature()*1.8)+32);
        previousMillis = currentMillis;
        char radiopacket[64] = "TNCP BEACON * ";
        itoa(tmp, radiopacket+14, 10);
        radiopacket[63] = 0;
        delay(50);
        rf95.send((uint8_t *)radiopacket, 64);
        rf95.waitPacketSent();
        Serial.println("continuous mode...");
        Serial.print("beaconing on "); Serial.println(RF95_FREQ);
        Serial.print(interval); Serial.println(" ms interval");
        Serial.println(radiopacket);
        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(50);              // wait a bit so it flickers. thats what she said.
        digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    }
}