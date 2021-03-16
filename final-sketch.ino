#include <Wire.h>
#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h>

#include <Adafruit_VL6180X.h>

// screen
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

  #define TFT_CS        10
  #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         8
  
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//time of flight
Adafruit_VL6180X vl = Adafruit_VL6180X();

//neopixel

#define LED_PIN  5
#define LED_NUM 10
#define DELAYVAL 50

Adafruit_NeoPixel pixels(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

//servo

Servo servo;
int pos = 0;
int servoCount = 0;

const int photocell1Pin = A0;
const int photocell2Pin = A1;

int lastButtonState = 0;
int flag = 0;

int pressMillis = 0;
int timeMillis = 0;
int timePassed = 0;

int recordingCounter = 0;

BLEService imuService("19B10010-E8F2-537E-4F6C-D104768A1214"); // create service
BLEIntCharacteristic buttonCharacteristic("19B10010-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

void setup() {
  Serial.begin(9600);
//  while (!Serial);

  pinMode(photocell1Pin, INPUT);
  pinMode(photocell2Pin, INPUT);

//servo
  servo.attach(3);
  servo.write(0);

// screen
  tft.init(240, 240);
  tft.fillScreen(ST77XX_BLACK);

// time of flight
    if (! vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
  Serial.println("Sensor found!");

// neopixel

  pixels.begin();
  pixels.clear();

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName("ButtonLED");
  BLE.setAdvertisedService(imuService);

  imuService.addCharacteristic(buttonCharacteristic);

  // add the service
  BLE.addService(imuService);

  buttonCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    // turn on LED to indicate connection:
    digitalWrite(LED_BUILTIN, HIGH);

    
    while (central.connected()) {
      pixels.clear();
      unsigned long currentMillis = millis();
      int pc1Reading = analogRead(photocell1Pin);
      int pc2Reading = analogRead(photocell2Pin);
      uint8_t range = vl.readRange();
      uint8_t status = vl.readRangeStatus();
      
      int buttonState = 1;
      int timePassed = currentMillis - timeMillis;

      uint32_t yellow = pixels.Color(100, 60, 0);
      uint32_t red = pixels.Color(255, 0, 0);
      uint32_t off = pixels.Color(0, 0, 0);

//      Serial.print(currentMillis);
//      Serial.print(",");
//      Serial.println(timeMillis);
//      Serial.println(timePassed);
//      Serial.println(pressMillis);

      // millis to control auto turn off
      long interval = 7000;

      if (pressMillis > 0) {
        if (currentMillis - pressMillis > interval) {
           flag = 0;
           buttonCharacteristic.writeValue(flag);
           Serial.println("bye");
           Serial.println(recordingCounter);
           pressMillis = 0;
           //turn off light
            for (int i = 0; i<LED_NUM; i++) {
              pixels.setPixelColor(i, off);
              pixels.show();
            }
        }
      }
      
      if (pc1Reading < 50 && 
          pc2Reading < 50 &&
          range < 65){
        delay(100);
        buttonState = 1;
      } else {
        buttonState = 0;
      }
      
      if (buttonState != lastButtonState) {
        delay(5);
        if (buttonState == HIGH){        
          if (flag == 0) {
            pressMillis = currentMillis;
            
            // enough time passed
            if (timeMillis == 0 || timePassed >= 20000) {
              // activate
              flag = 1;
              timeMillis = currentMillis;
              // turn on light
              int counter = 0;
              for (int i=0; i<=LED_NUM; i++){
                pixels.setPixelColor(i, pixels.Color(i*3, 0, 120-i*2));
                pixels.show();
                delay(DELAYVAL);
                counter = counter + 1;
                if (counter > 0) {
                  pixels.setPixelColor(i, yellow);
                  pixels.show();
                  }
              }  
              delay(5);
              buttonCharacteristic.writeValue(flag);
              recordingCounter = recordingCounter + 1;
              Serial.println(recordingCounter);
              Serial.println("yay");
            } else {
              // not enough time passed
              for (int i=0; i<=LED_NUM; i++){
                pixels.setPixelColor(i, red);
                pixels.show();
              }
              delay (500);
              for (int i=0; i<=LED_NUM; i++){
                pixels.setPixelColor(i, off);
                pixels.show();
              }
            }
          } else { 
            flag = 0;           
            buttonCharacteristic.writeValue(flag);  
            Serial.println("nay");
            Serial.println(recordingCounter);
            // turn off light
            for (int i = 0; i<LED_NUM; i++) {
              pixels.setPixelColor(i, off);
              pixels.show();
            }
          }
        delay(5);
        }
       }
       lastButtonState = buttonState;

       // after a number of recordings, servo and screen activate
        if (recordingCounter == 3 && flag == 0) {
            buttonCharacteristic.writeValue(0);
              int servoCounter = 0;
              if (servoCounter = 0) {
                for (pos = 0; pos <= 180; pos += 1) {
                  servo.write(pos);
                  delay(30);
                  servoCounter = servoCounter + 1;
                  }
                 } else {
//                  servo.write(180);
                  textPrint();
                  delay(100);
                  }
              }
          }

// when the central disconnects, print it out:
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
    // turn off LED:
    digitalWrite(LED_BUILTIN, LOW);
    servo.write(0);
    tft.fillScreen(ST77XX_BLACK);
  }
}

void textPrint() {
  servo.write(180);
  tft.setCursor(11, 12);
  tft.setTextSize(2);
  tft.println(" ");
  tft.println("Hey, friend, nice to finally meet you :)");
  tft.println(" ");
  tft.println("Thank you for being so kind and patient when you talked to me.");
}

//void servoMovement() {
//
//}
