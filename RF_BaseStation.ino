/*
    RF BaseStation
    
    v 2.3           201610250511Z       13890/32256/1031

    modified display to output "lux < 25" if that is the case. this prevents weird color temps (due to low light levels) from being shown.

    
    v 2.1           201610190401Z       11668/32256/981

    removed buttons, don't really need them anymore
    simplified code, removed unused timers, added splash to LCD
    v 2.0           201610151937Z
    
    init both displays, listen on RF95_FREQ, and display partially parsed output to lcd0
    changed receive logic over to strtok, this allows us to parse the received string
    
    v 1.0           201610130715Z
    
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
#define pid "RF-BS"
#define vers "2.2"
#define pname "RF BaseStation"
#define LED 13

Adafruit_LiquidCrystal lcd(0);
Adafruit_LiquidCrystal lcdb(1);

void setup() 
{  
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

  lcdinit();

  lcd.setCursor(0,0); lcd.print("listening...");
  lcdb.setCursor(0,1); lcdb.print("listening...");

  rf95.setTxPower(23, false);
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

      String SN = strtok((char*)buf, ":");  // start parsing the received string -  return data to the left of it as a String, passing the remainder to the next strtok()
      int TP = atoi(strtok(NULL, ",")); // parses out temperature
      int HD = atoi(strtok(NULL, ",")); // parses out humidity
      int PR = atoi(strtok(NULL, ",")); // parses out pressure in millibars
      int CT = atoi(strtok(NULL, ",")); // parses out color temp
      int LX = atoi(strtok(NULL, ",")); // parses out lux
      int BT = atoi(strtok(NULL, ":")); // ending with : to return the remote battery level
      
      if (SN=="RV-WX") 
        { 
            lcdb.clear();
            lcdb.setCursor(0, 0);
            lcdb.print(SN);
            lcdb.setCursor(6, 0);
            lcdb.print(TP);
            lcdb.setCursor(9, 0);
            lcdb.print(HD);
            lcdb.setCursor(12, 0);
            lcdb.print(PR);
            
            if (LX>=25)  
            {   lcdb.setCursor(0, 1);
                lcdb.print(CT);
                lcdb.setCursor(4, 1);
                lcdb.print("K");
                lcdb.setCursor(6, 1);
                lcdb.print(LX);
            }
            else { lcdb.setCursor(0,1); lcdb.print("lux < 25"); }
               
            lcdb.setCursor(12,1);
            lcdb.print(BT);
            lcdb.print("%");
     
            digitalWrite(LED, LOW);
        }

        else if (SN=="BY-WX") 
        { 
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(SN);
            lcd.setCursor(6, 0);
            lcd.print(TP);
            lcd.setCursor(9, 0);
            lcd.print(HD);
            lcd.setCursor(12, 0);
            lcd.print(PR);

            if (LX>=25)  
            {   
                lcd.setCursor(0, 1);
                lcd.print(CT);
                lcd.setCursor(4, 1);
                lcd.print("K");
                lcd.setCursor(6, 1);
                lcd.print(LX);
            }
            
            else { lcd.setCursor(0,1); lcd.print("lux < 25"); }
            
            lcd.setCursor(12,1);
            lcd.print(BT);
            lcd.print("%");
     
            digitalWrite(LED, LOW);
        }
    }
    
    else { Serial.println("Receive failed"); }
  }
  
}

void lcdinit()
{
    lcd.setBacklight(HIGH);
    lcd.begin(16, 2);
    lcdb.setBacklight(HIGH);
    lcdb.begin(16, 2);
  
    lcd.clear(); 
    lcd.setCursor(0, 0); lcd.print("RF BaseStation");
    lcd.setCursor(0, 1); lcd.print(vers);

    lcdb.clear(); 
    lcdb.setCursor(0, 0); lcdb.print(RF95_FREQ); lcdb.print(" MHz");
    lcdb.setCursor(11,1); lcdb.print("[h6t]");

    delay(5000);

    lcd.clear(); lcdb.clear();
}
    