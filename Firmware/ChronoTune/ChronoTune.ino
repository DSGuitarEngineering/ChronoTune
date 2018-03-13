/*  ChronoTune: source code for Project ChronoTune (working title)
    Copyright (C) 2018  DS Guitar Engineering

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Version:        0.2.0
Modified:       March 10, 2018
Verified:       March 10, 2018
Target uC:      ATSAMD21G18

-----------------------------------------***DESCRIPTION***------------------------------------------------
This project is based on the Chronograph, but also includes circuitry for a chromatic tuner.  The tuner
and clock will share a 4-character alphanumeric display.  The basic premise is that the pedal will
normally operate as a Chronogrpah, but instant access to a tuner will be available at any time.  Another
main goal is to allow user updateable drag-and-drop firmware via USB and UF2 bootloader.

Project ChronoTune was developed in collaboration with the Inner Circle of the 60 Cycle Hum podcast.

--------------------------------------------------------------------------------------------------------*/
//include the necessary libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6  //Neopixel pin

//initialize the Neopixel strip and the alphanumeric display
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

char displaybuffer[4] = {' ', ' ', ' ', ' '};  //buffer for marquee messages

void setup() {
  Serial.begin(9600);  //open the serial port (for debug only)

  alpha4.begin(0x70);  // pass in the address

  alpha4.clear();         //clear the display
  alpha4.writeDisplay();  //use this command to update the display

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  //display startup message
  //marquee("ChronoTune");
  //delay(300);
  //marquee("by DS Engineering");
}


void loop()
{
  alpha4.writeDigitAscii(1, 'A');
  alpha4.writeDisplay();
  delay(100);
  inTune();
  delay(1000);
  alpha4.clear();
  alpha4.writeDisplay();
  clearStrip();
  strip.show();
  delay(1500);
}

//This routine displays marquee style messages--------------------------------------------------------------
//  must pass a string; returns nothing
void marquee(String s)
{
  s.concat("    ");  //concatenate 4 blank spaces to  the beginning of the message to make the entire
                     //message scroll across the display

  for (int i=0; i < s.length(); i++){   //run loop i times, where i is the number of characters in the string

    // scroll down display
    displaybuffer[0] = displaybuffer[1];
    displaybuffer[1] = displaybuffer[2];
    displaybuffer[2] = displaybuffer[3];
    displaybuffer[3] = s.charAt(i);

    // set every digit to the buffer
    alpha4.writeDigitAscii(0, displaybuffer[0]);
    alpha4.writeDigitAscii(1, displaybuffer[1]);
    alpha4.writeDigitAscii(2, displaybuffer[2]);
    alpha4.writeDigitAscii(3, displaybuffer[3]);

    // write it out
    alpha4.writeDisplay();
    delay(150);           //delay between each character transition
  }
  alpha4.clear();
  alpha4.writeDisplay();
}

//This routine clears the Neopixel strip------------------------------------------------------------------
// no dependencies, returns nothing
void clearStrip()
{
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
    }
  strip.show();
}


//This routine displays the "in tune" animation on the Neopixel strip---------------------------------------
// no dependencies, returns nothing
void inTune()
{
  uint32_t c = strip.Color(0, 64, 0);  //low brightness green
  uint8_t wait = 50;
  strip.setPixelColor(3, c);
  strip.setPixelColor(4, c);
  strip.show();
  delay(wait);
  clearStrip();
  strip.setPixelColor(3, c);  strip.setPixelColor(4, c);
  strip.setPixelColor(2, c);
  strip.setPixelColor(5, c);
  strip.show();
  delay(wait);
  clearStrip();
  strip.setPixelColor(3, c);  strip.setPixelColor(4, c);
  strip.setPixelColor(1, c);
  strip.setPixelColor(6, c);
  strip.show();
  delay(wait);
  clearStrip();
  strip.setPixelColor(3, c);  strip.setPixelColor(4, c);
  strip.setPixelColor(0, c);
  strip.setPixelColor(7, c);
  strip.show();
  delay(wait);
  clearStrip();
  strip.setPixelColor(3, c);  strip.setPixelColor(4, c);
  strip.show();
}
