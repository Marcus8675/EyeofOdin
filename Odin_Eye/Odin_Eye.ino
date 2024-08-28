//Eye of Odin
//
//Controller for NeoPixel Jewel used for robotic platform eye
//
//NeoPixel commands to be mirrored via USB serial port
//
// Functions
// begin() - Local Use only
// updateLength() - Local Use only
// updateType() - Local Use only
// show() - Local Use only
// delay_ns()
// setPin() - Local Use only
// setPixelColor() - USB INPUT
// fill() - USB INPUT
// ColorHSV()
// getPixelColor()
// setBrightness() - USB INPUT
// getBrightness()
// clear() - USB INPUT
// gamma32()
//
// System level commands:
//
// FLASH #flashes
// FLASHLED LED# #flashes
// BRIGHTNESS 0-255
// SETLED LED# R G B W
// SETEYE R G B W
// CLEARLED LED#
// CLEAREYE

#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "MOTOROLA-81787"
#define STAPSK "marcus8675"
#endif

#define PIN 2

const char* ssid = STASSID;
const char* password = STAPSK;
const char* hostname = "ODIN_EYE";
String message;
const byte numChars = 32;
char receivedChars[numChars];

boolean newData = false;
 
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel eye = Adafruit_NeoPixel(7, PIN, NEO_RGBW + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() 
{
  Serial.begin(115200);                         // Start serial communication 
  Serial.println("Opening the Eye");
  ArduinoOTA.setHostname(hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.onStart([]() 
  {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) 
    {
      type = "sketch";
    } 
    else 
    {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() 
  {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) 
  {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  eye.begin(); //Start NeoPixel class
  eye.setBrightness(8);

  //Set all pixels to off (black)
  for(uint16_t i=0; i<eye.numPixels(); i++) 
  {
    eye.setPixelColor(i, eye.Color(0, 0, 0, 255));
  }
  eye.show(); // Initialize all pixels to 'off'

}

void loop() 
{
  ArduinoOTA.handle(); //Check each loop pass for wifi SW update
  recvWithStartEndMarkers();
  if (newData == true) 
  {
    Serial.print("Data Received:");
    Serial.println(receivedChars);
    message=String(receivedChars);
    message.toUpperCase();
    //receivedChars=String(receivedChars).toUpperCase();
    if (message.substring(0,5) == "FLASH") //Search for token FLASH
    {
      Serial.println("FLASH");
      if (message.substring(0,8) == "FLASHLED") //Specific LED?
      {
        //FLASHLED # ##   LED#  NumerofFlashes
        int numflash =message.substring(11,13).toInt();
        int led = message.substring(9,10).toInt();
        int color = eye.getPixelColor(led); //get current color

        for(uint16_t i=0; (i<numflash); i++)
        {
          eye.setPixelColor(led,0); //LED OFF
          eye.show();
          delay(500);
          eye.setPixelColor(led,color);
          eye.show();
          delay(500);
        } 
      }
      else
      {
        //FLASH ##  NumerofFlashes
        int numflash =message.substring(6,8).toInt();
        int brightness = eye.getBrightness(); //get current brightness
        Serial.printf("numflash:%d  brightness:%d", numflash, brightness);
        for(uint16_t i=0; i<numflash; i++)
        {
          eye.setBrightness(0); //eye OFF
          eye.show();
          delay(500);
          Serial.print(".");
          eye.setBrightness(brightness);
          eye.show();
          delay(500);
        } 
      }
      
    }
    // if (String(receivedChars) == "RAINBOW") {
    //   rainbow(10);}
    // else if (String(receivedChars) == "RAINBOWCYCLE") {
    //   rainbowCycle(10);}
    // else if (String(receivedChars) == "LED1") {
    //   eye.setPixelColor(0, eye.Color(0, 0, 0, 255));} // White RGBW
    // else if (String(receivedChars) == "LED2") {
    //   eye.setPixelColor(1, eye.Color(0, 0, 0, 255));} // White RGBW
    // else if (String(receivedChars) == "LED3") {
    //   eye.setPixelColor(2, eye.Color(0, 0, 0, 255));} // White RGBW
    // else if (String(receivedChars) == "LED4") {
    //   eye.setPixelColor(3, eye.Color(0, 0, 0, 255));} // White RGBW
    // else if (String(receivedChars) == "LED5") {
    //   eye.setPixelColor(4, eye.Color(0, 0, 0, 255));} // White RGBW
    // else if (String(receivedChars) == "LED6") {
    //   eye.setPixelColor(5, eye.Color(0, 0, 0, 255));} // White RGBW
    // else if (String(receivedChars) == "LED7") {
    //   eye.setPixelColor(6, eye.Color(0, 0, 0, 255));} // White RGBW                        
    // else if (String(receivedChars) == "OFF")
    // {
    //   for(uint16_t i=0; i<eye.numPixels(); i++) 
    //   {
    //     eye.setPixelColor(i, eye.Color(0, 0, 0, 0));
    //   }
    // }
    // else {
    //   colorWipe(eye.Color(0, 255, 0, 0), 50);} // Red
  }  
  newData = false;
  eye.show();
  
  // if (msg="LED2") eye.setPixelColor(1, eye.Color(0, 0, 0, 255)); // White RGBW
  // if (msg="LED3") eye.setPixelColor(2, eye.Color(0, 0, 0, 255)); // White RGBW
  // if (msg="LED4") eye.setPixelColor(3, eye.Color(0, 0, 0, 255)); // White RGBW
  // if (msg="LED5") eye.setPixelColor(4, eye.Color(0, 0, 0, 255)); // White RGBW
  // if (msg="LED7") eye.setPixelColor(5, eye.Color(0, 0, 0, 255)); // White RGBW
  // if (msg="LED7") eye.setPixelColor(6, eye.Color(0, 0, 0, 255)); // White RGBW

  
  // // Some example procedures showing how to display to the pixels:
  // colorWipe(eye.Color(255, 0, 0), 50); // Red
  // colorWipe(eye.Color(0, 255, 0), 50); // Green
  // colorWipe(eye.Color(0, 0, 255), 50); // Blue
  // colorWipe(eye.Color(0, 0, 0, 255), 50); // White RGBW
  // // Send a theater pixel chase in...
  // theaterChase(eye.Color(127, 127, 127), 50); // White
  // theaterChase(eye.Color(127, 0, 0), 50); // Red
  // theaterChase(eye.Color(0, 0, 127), 50); // Blue
  
  // theaterChaseRainbow(50);

}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) 
{
  for(uint16_t i=0; i<eye.numPixels(); i++) 
  {
    eye.setPixelColor(i, c);
    eye.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) 
{
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<eye.numPixels(); i++) 
    {
      eye.setPixelColor(i, Wheel((i+j) & 255));
    }
    eye.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) 
{
  uint16_t i, j;

  for(j=0; j<256*5; j++) // 5 cycles of all colors on wheel
  { 
    for(i=0; i< eye.numPixels(); i++) 
    {
      eye.setPixelColor(i, Wheel(((i * 256 / eye.numPixels()) + j) & 255));
    }
    eye.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) 
{
  for (int j=0; j<10; j++) //do 10 cycles of chasing
  {  
    for (int q=0; q < 3; q++) 
    {
      for (uint16_t i=0; i < eye.numPixels(); i=i+3) 
      {
        eye.setPixelColor(i+q, c);    //turn every third pixel on
      }
      eye.show();

      delay(wait);

      for (uint16_t i=0; i < eye.numPixels(); i=i+3) 
      {
        eye.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) // cycle all 256 colors in the wheel
  {     
    for (int q=0; q < 3; q++) 
    {
      for (uint16_t i=0; i < eye.numPixels(); i=i+3) 
      {
        eye.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      eye.show();

      delay(wait);

      for (uint16_t i=0; i < eye.numPixels(); i=i+3) 
      {
        eye.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) 
{
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) 
  {
    return eye.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) 
  {
    WheelPos -= 85;
    return eye.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return eye.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
