
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "Adafruit_SSD1306.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define MQTT_SERVER      "mqtt.leonrenkema.nl"
#define MQTT_SERVERPORT  1883

#define MQTT_DELAY 20000

#define SSID "##"
#define SSID_PASSWORD "##"

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 30

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT);
//Adafruit_MQTT_Subscribe temperatureChannel = Adafruit_MQTT_Subscribe(&mqtt, "telegram/temp");
Adafruit_MQTT_Publish temperatureChannel = Adafruit_MQTT_Publish(&mqtt, "telegram/temp");

#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// https://github.com/milesburton/Arduino-Temperature-Control-Library

OneWire  ds(15);  // on pin 10 (a 4.7K resistor is necessary)


void powerUsageCallBack(char *data, uint16_t len) {
  Serial.print(data);
  clearDisplay();
  showLine(0,0, data);
}

void setup() {

  Serial.begin(115200);

  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  connectWifi();

  //powerUsageChannel.setCallback(powerUsageCallBack);
  
  //mqtt.subscribe(&powerUsageChannel);
  
}

uint32_t currentMilis = 0;
uint32_t lastUpdateMilis = 0;
uint32_t screensaverMilis = 0;
bool screen = false;

uint8_t row = 1;

void loop() {

  char Buf[80];
  
  currentMilis = millis();
  esp_task_wdt_reset();
  
  MQTT_connect();

  mqtt.processPackets(10000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

  float temperature = scanOnewire();

  if (temperature != -100) {
    String tempString = String(temperature) + " C";
    int stringLength = tempString.length()+1;
    char charBuffer[stringLength];
    tempString.toCharArray(charBuffer, stringLength);
    
    // Clear the buffer
    if (screen) {
      clearDisplay();
      showLine(0,1, charBuffer);
    }
    
    if (currentMilis - lastUpdateMilis > MQTT_DELAY) {
      String temperatureString = String("sensors,location=kantoor value=") + temperature + "";
      temperatureString.toCharArray(Buf, sizeof(Buf));
      temperatureChannel.publish(Buf);

      lastUpdateMilis = currentMilis;
    }
  }

  // if (currentMilis - screensaverMilis > 5000) {
  //   if (screen) {
  //     clearDisplay();
  //     screen = false;
  //   } else {
  //     screen = true;
  //   }
  //   screensaverMilis = currentMilis;
  // }
  
}
