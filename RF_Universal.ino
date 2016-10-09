/*
  RF Universal
  
  v 3.0        201610090719Z
  
  boot direct to RX, press A to beacon once, press B to beacon continuously (followed by C to exit)
    
  
  v 2.4.2       201610070500Z
  
  sorted out the menus to functions, lots easier to read now. housekeeping stuff.  
  
  
  v 2.3         201610020315Z
  
  removed extraneous definitions, streamlined some stuff. added interval display to tx screen. 
  changed char radiopacket to 64 (from 20) 
  implemented TNCP for pinging
  rearranged the display a bit
  
  
  v 2.2         201609270548Z
  
  Streamlined code, took out extraneous serial debugging stuff. Removed VBATPIN, since it's tied to A9 and will never work anyway (same as button A, which we pull high to detect a press)
  Converted project name and version to #defines at beginning. Hopefully easier to implement. Since I'm planning to top most projects with an OLED, it makes sense to have a standard splash. Will refine.
  Added "c exit" labels @ 0,24 inside A and B loops. More user-friendly.
  
  
  v 2.1         201609260730Z
  
  Renamed to RF_Universal. Converted to true Universal RF code, each unit can RX and TX. Added menu screen. A to TX, B to RX, C to return to menu.
  
  
  v 1.0         201609240537Z
  
  Universal receiver designed around the Adafruit Feather 32u4 with RFM95 LoRa module.
  OLED FeatherWing, 1200mAh LiIon battery
  code polls set frequency, dumps received data directly to OLED, RSSI is displayed underneath.
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <RH_RF95.h>
#include <Adafruit_FeatherOLED.h>
Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define Apin 9
#define Bpin 6
#define Cpin 5

#define pid "1B"
#define vers "3.0_dev"
#define pname "RF Universal"

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define RF95_FREQ 915.0  // RFM95 is the 900ISM version, so range is 902-928, 915 center.

RH_RF95 rf95(RFM95_CS, RFM95_INT);  // Singleton instance of the radio driver

int txflag = 0;
int rxflag = 0;

void setup()   {                
  Serial.begin(9600);

  pinMode(Apin, INPUT_PULLUP);  // pullup them buttons
  pinMode(Bpin, INPUT_PULLUP);
  pinMode(Cpin, INPUT_PULLUP);

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

}

#define LED 13

int16_t packetnum = 0;  // packet counter, we increment transmission

long previousMillis = 0;
int16_t millic = 0;  // millis counter to insert into radiopacket
long interval = 2000;  // interval between transmissions. 1000 is 1 second.

void loop()
{

  if (txflag==1) hitb();
  if (txflag==0) listen();

}

void listen()
{
    if (rf95.available());
    {
        rxflag = 1;
        
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];       // check the buffer
        uint8_t len = sizeof(buf);
    
        if (rf95.recv(buf, &len));  // it's full, dump it
        {   
            digitalWrite(LED, HIGH);
            display.clearDisplay();
            RH_RF95::printBuffer("Received: ", buf, len);
            display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0); 
            display.println("A to TX - B to beacon");
            display.print("listening on "); display.println(RF95_FREQ);
            display.print("RSSI "); display.println(rf95.lastRssi(), DEC);
            display.println((char*)buf); 
            display.display();
            digitalWrite(LED, LOW);
        }
    }

    if (!digitalRead(Apin))
    {
        hita();
    }

    if (!digitalRead(Bpin))
    {
        hitb();
    }
}

void hita()  // send just one packet, then return to listening
{
    char radiopacket[22] = "TNCP PING ! ";
    itoa(packetnum++, radiopacket+12, 10);
    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
    radiopacket[21] = 0;
    delay(50);
    rf95.send((uint8_t *)radiopacket, 64);
    rf95.waitPacketSent();
    display.println("single shot mode...");
    display.print("beaconing on "); display.println(RF95_FREQ);
    display.setCursor(0,24); display.println(radiopacket);
    display.display();
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(50);              // wait a bit so it flickers. thats what she said.
    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW 
    delay(3000);
    txflag = 0;
}

void hitb()
{
    unsigned long currentMillis = millis();
    txflag = 1;
    if(currentMillis - previousMillis > interval)
    {
        previousMillis = currentMillis;
        char radiopacket[22] = "TNCP PING * ";
        itoa(packetnum++, radiopacket+12, 10);
        display.clearDisplay();
        display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
        radiopacket[63] = 0;
        delay(50);
        rf95.send((uint8_t *)radiopacket, 64);
        rf95.waitPacketSent();
        display.println("continuous mode...");
        display.print("beaconing on "); display.println(RF95_FREQ);
        display.print(interval); display.println(" ms interval");
        display.println(radiopacket);
        display.display();
        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(50);              // wait a bit so it flickers. thats what she said.
        digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    }
    
    if (!digitalRead(Cpin)) txflag = 0;
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