# TelemetryNetwork
Arduino powered telemetry network

BME280_Tester.ino
        a simple graphical tester for the BME280 sensor. Attach BME280 to i2c bus, OLED to hardware SPI. Tested with 32u4 Feather.
        Displays temperature, pressure, pressure altitude, and humidity.
	
RF_Beacon.ino
        broadcasts current temperature. Attach BME280 to i2c bus. Tested with 32u4 Feather.
	
RF_Universal.ino
        Testing suite for RFM95 LoRa.  Boots to receive mode, A to single-shot and B to broadcast pings. 
        RX mode displays received packet and RSSI.  Tested with 32u$ Feather, RFM95 LoRa, OLED Featherwing.
