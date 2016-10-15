/*
	RF BaseStation
	
	v 2.0			201610151937Z
	
	init both displays, listen on RF95_FREQ, and display partially parsed output to lcd0
	changed receive logic over to strtok, this allows us to parse the received string
	
	v 1.0			201610130715Z
	
	initial commit of RF Base Station
	custom version of RF Universal, optimzed for the Metro board
	hardware: 
		Metro board
		2x i2c backpacks
		2x 16x2 character displays
		RFM95 LoRa breakout - SPI, CS10 RST9 INT2
		3x SPST buttons to d3 d4 d5
		6.3v 220uF capacitor across power rails feeding RF
		
*/

#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

#define LED 13
#define Apin 3
#define Bpin 4
#define Cpin 5

Adafruit_LiquidCrystal lcd(0);
Adafruit_LiquidCrystal lcdb(1);

int txflag = 0;
int rxflag = 0;

void setup() 
{

  pinMode(Apin, INPUT_PULLUP);  // pullup them buttons
  pinMode(Bpin, INPUT_PULLUP);
  pinMode(Cpin, INPUT_PULLUP);
  
  lcd.setBacklight(HIGH);
  lcd.begin(16, 2);
  lcdb.setBacklight(HIGH);
  lcdb.begin(16, 2);
    
  pinMode(LED, OUTPUT);     
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

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

    lcd.clear();
    lcd.print("listening...");

    lcdb.clear();
    lcdb.print("waiting...");
}

int16_t packetnum = 0;  // packet counter, we increment transmission

long previousMillis = 0;
int16_t millic = 0;  // millis counter to insert into radiopacket
long interval = 2000;  // interval between transmissions. 1000 is 1 second.

void loop()
{

  if (txflag==1) hita();
  if (txflag==0) listen(); 

}

void listen()
{
  if (rf95.available())
  {
    rxflag = 1;
    
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      
      RH_RF95::printBuffer("Received: ", buf, len);

      // lcd.print((char*)buf);  // dump raw packet

      String SN = strtok((char*)buf, ".");  // start parsing the received string - here we stop at . and return data to the left of it as a String, passing the remainder to the next strtok()
      int TP = atoi(strtok(NULL, ",")); // which is here! same as above, but we stop at , for temperature
      int HD = atoi(strtok(NULL, ",")); // still , this is for humidity
      int PR = atoi(strtok(NULL, ",")); // still , this one for pressure in millibars
      int BT = atoi(strtok(NULL, ":")); // ending with : to return the remote battery voltage

      float BTF = BT /= 1024;  // we sent the voltage as vbat*2*3.3 to avoid sending decimals, here we /=1024 to return actual voltage
    
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(SN);
      lcd.setCursor(6, 0);
      lcd.print(TP);
      lcd.setCursor(9, 0);
      lcd.print(HD);
      lcd.setCursor(12, 0);
      lcd.print(PR);
      lcd.setCursor(0, 1);
      lcd.print(BTF);
      lcd.setCursor(4, 1);
      lcd.print("v");
      lcd.setCursor(7, 1);
      lcd.print("RSSI");
      lcd.setCursor(12, 1);
      lcd.print(rf95.lastRssi(), DEC);
      
      digitalWrite(LED, LOW);

    }
    
    else { Serial.println("Receive failed"); }
  }
  
    if (!digitalRead(Apin))
    {
        hita();
    }

/*    if (!digitalRead(Bpin))
    {
        hitb();
    }*/
}

void hita()
{
    unsigned long currentMillis = millis();
    txflag = 1;
    if(currentMillis - previousMillis > interval)
    {
        previousMillis = currentMillis;
        char radiopacket[22] = "";
        itoa(packetnum++, radiopacket+12, 10);
        lcd.clear();
        lcd.setCursor(0,0);
        radiopacket[63] = 0;
        delay(50);
        rf95.send((uint8_t *)radiopacket, 64);
        rf95.waitPacketSent();
        lcd.print("TX on "); lcd.println(RF95_FREQ);
        lcd.println(radiopacket);
        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(50);              // wait a bit so it flickers. thats what she said.
        digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    }
    
    if (!digitalRead(Cpin)) txflag = 0;
}