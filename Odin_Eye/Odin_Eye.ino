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
// FLASH ##            #flashes
// FLASHLED # ##       LED# #flashes
// BRIGHTNESS 0-255
// SETLED # ### ### ### ###           LED# R G B W
// SETEYE R G B W


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
uint32_t eyestatus[7] = {0, 0, 0, 0, 0, 0, 0};  //Holds status of each eye LED.  32bit representing RGBW values

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
  eye.setBrightness(32);

  //Set all pixels to off (black)
  seteye();
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

        for(uint16_t i=0; (i<numflash); i++)
        {
          eye.setPixelColor(led, 0); //set all eye LED's to off
          eye.show();
          delay(500);
          seteye(); //return all eye LED to current settings
          eye.show();
          delay(500);
        } 
      }
      else
      {
        //FLASH ##  NumerofFlashes
        int numflash =message.substring(6,8).toInt();
        
        for(uint16_t i=0; i<numflash; i++)
        {
          for(uint16_t x=0; x<7; x++)
          {
            eye.setPixelColor(x, 0); //set all eye LED's to off
          }
          eye.show();
          delay(500);
          seteye();  //return all eye LED to current settings
          eye.show();
          delay(500);
        } 
      }
      
    }

    // SETLED # ### ### ### ###      LED# R G B W
    if (message.substring(0,6) == "SETLED") //Search for token SETLED
    {
      int LED = message.substring(7,8).toInt();
      int red = message.substring(9,12).toInt();
      int green = message.substring(13,16).toInt();
      int blue = message.substring(17,20).toInt();
      int white = message.substring(21,24).toInt();
      Serial.printf("LED:%d R:%d G:%d B:%d W:%d", LED, red, green, blue, white);
      eyestatus[LED]=eye.Color(green, red, blue, white);  //Green and Red order are swapped in this NeoPixel Jewel
      seteye();
    }

    //SETEYE ### ### ### ###     R G B W
    if (message.substring(0,6) == "SETEYE") //Search for token SETEYE
    {
      int red = message.substring(7,10).toInt();
      int green = message.substring(11,14).toInt();
      int blue = message.substring(15,18).toInt();
      int white = message.substring(19,22).toInt();
      Serial.printf("R:%d G:%d B:%d W:%d", red, green, blue, white);
      for(uint16_t x=0; x<7; x++)
        {
          eyestatus[x]=eye.Color(green, red, blue, white);  //Green and Red order are swapped in this NeoPixel Jewel
        }
      seteye();
    }

  }  
  newData = false;
  eye.show();

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

void seteye()
{
  for(uint16_t i=0; i<7; i++)
  {
    eye.setPixelColor(i, eyestatus[i]);
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
