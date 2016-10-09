/*
    BME280 Tester

    v 1.0               201610090641Z

    pretty simple - connect a BME280 to 32u4 Feather i2c bus, attach a Feather OLED, and it'll display temperature, pressure, pressure altitude, and humidity.

 */

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_FeatherOLED.h>

Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define Apin 9
#define Bpin 6
#define Cpin 5

#define vers "1.0"
#define pname "BME280 Tester"

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

void setup() {
    Serial.begin(9600);
    Serial.println(F("BME280 test"));

    pinMode(Apin, INPUT_PULLUP);
    pinMode(Bpin, INPUT_PULLUP);
    pinMode(Cpin, INPUT_PULLUP);

    oledinit();
  
    if (!bme.begin()) 
    {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
    }
}

void loop() 
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
  
    display.print("Temp = ");
    display.print(((bme.readTemperature()*1.8)+32));
    display.println(" *F");

    display.print("Pres = ");

    display.print((bme.readPressure() / 100.0F)+34);
    display.println(" mb");

    display.print("Alt = ");
    display.print((bme.readAltitude(SEALEVELPRESSURE_HPA))*3.2808);
    display.println(" ft");

    display.print("Humi = ");
    display.print(bme.readHumidity());
    display.println(" %");
    display.display();

    Serial.println();
    delay(2000);
}

void oledinit()
{

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
    display.display();
    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
    display.println(pname); 
    display.print("v ");
    display.println(vers);
    display.setCursor(0,24);
    display.println("halosix technologies");
    display.display();
    delay(3000);
    display.clearDisplay();

}