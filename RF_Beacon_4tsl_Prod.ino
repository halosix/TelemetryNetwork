/*

        RF Beacon

        v 4.0tsl       201612050555Z       22612/28672/1047

        production version of RF Beacon
        this version of code is for 32u4 Feather with RFM95 LoRa, BME280+TSL only
        listens on RF95_FREQ, if it hears a broadcast status request to its address, it replies with weather status 


*/

#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_TSL2591.h>
#include <Adafruit_SleepyDog.h>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

#define pid "DK-WX"
#define vers "4.0"
#define pname "RF Beacon"
#define VBATPIN A9  // measure battery
#define RF95_FREQ 915.0  // RFM95 is the 900ISM version, so range is 902-928, 915 center.
#define LED 13

unsigned long previousMillis = 0;
const long interval = 10000;  // interval between transmissions. 1000 is 1 second.

RH_RF95 rf95(RFM95_CS, RFM95_INT);  // Singleton instance of the radio driver
Adafruit_BME280 bme; // use i2c, lets keep spi for rf
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)

void setup()   
{                
    Serial.begin(9600);
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
        
    if (tsl.begin()) {
        Serial.println("Found a TSL2591 sensor");
        } else {
        Serial.println("No sensor found ... check your wiring?");
        while (1);
        }
        
    rf95.setTxPower(23, false);
            configureSensor();
}

void loop() 
{

    if (rf95.available())
    {
    
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
        {
        digitalWrite(LED, HIGH);
      
        RH_RF95::printBuffer("Received: ", buf, len);

        // lcd.print((char*)buf);  // dump raw packet

        String FI = strtok((char*)buf, ",");  // start parsing the received string -  return data to the left of it as a String, passing the remainder to the next strtok()
        String PC = strtok(NULL, ",");
        String PS = strtok(NULL, ",");
        String SN = strtok(NULL, ",");
        String ST = strtok(NULL, ",");
        String DS = strtok(NULL, ",");
        String F1 = strtok(NULL, ",");
        String F2 = strtok(NULL, ";");
        
        if ((DS == "BD") && (PS == "ST"))  // bedroom only
            {
            digitalWrite(13, HIGH);   // LED on, so we can see its doing something
            int tmp = ((bme.readTemperature()*1.8)+32);  // get sensor values
            int humi = (bme.readHumidity());
            int pres = ((bme.readPressure() / 100.0F)+34);
            
            uint32_t lum = tsl.getFullLuminosity();
            uint16_t ir, full;
            ir = lum >> 16;
            full = lum & 0xFFFF;
            int lux = tsl.calculateLux(full, ir);
  
            float measuredvbat = analogRead(VBATPIN);
            measuredvbat *= .64453125; // math to get voltage to feed into batlvl
            int batlvl = map(measuredvbat, 320, 420, 5, 95);  // then take all that and map it to 5-95 to approximate battery % remaining
            int ff1 = 9;
            int ff2 = 9;
            digitalWrite(A0, HIGH);
            char rpacket[64];

            sprintf(rpacket, "TN,WX,WC,BD,TX,BC,99,99;%d,%d,%d,%d,%d,%d,%d:",tmp,humi,pres,lux,batlvl,ff1,ff2);

            delay(250);  // wait a bit
            rf95.send((uint8_t *)rpacket, strlen(rpacket));  // send the packet we previously built
            rf95.waitPacketSent();  // wait for TX to complete
            digitalWrite(13, LOW);    // LED off, with the stuff and the delay(50) there should be plenty for a visible flicker
            digitalWrite(A0, LOW);
            delay(250);
            }

       }
    }
}

void configureSensor(void)
{
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  // tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
   tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  /* Display the gain and integration time for reference sake */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         ");
  tsl2591Gain_t gain = tsl.getGain();
  switch(gain)
  {
    case TSL2591_GAIN_LOW:
      Serial.println("1x (Low)");
      break;
    case TSL2591_GAIN_MED:
      Serial.println("25x (Medium)");
      break;
    case TSL2591_GAIN_HIGH:
      Serial.println("428x (High)");
      break;
    case TSL2591_GAIN_MAX:
      Serial.println("9876x (Max)");
      break;
  }
  Serial.print  ("Timing:       ");
  Serial.print((tsl.getTiming() + 1) * 100, DEC); 
  Serial.println(" ms");
  Serial.println("------------------------------------");
  Serial.println("");
}
