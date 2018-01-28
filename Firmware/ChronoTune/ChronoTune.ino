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

Version:        0.0.1
Modified:       January 27, 2018
Verified:       January 27, 2018
Target uC:      ATSAMD21G18

-----------------------------------------***DESCRIPTION***
This project is based on the Chronograph, but also includes circuitry for a chromatic tuner.  The tuner
and clock will share a 4-character alphanumeric display.  The basic premise is that the pedal will
normally operate as a Chronogrpah, but instant access to a tuner will be available at any time.  Another
main goal is to allow user updateable drag-and-drop firmware via USB and UF2 bootloader.

--------------------------------------------------------------------------------------------------------*/
//include the necessary libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

char displaybuffer[4] = {' ', ' ', ' ', ' '};

void setup() {
  Serial.begin(9600);
  
  alpha4.begin(0x70);  // pass in the address

  alpha4.clear();
  alpha4.writeDisplay();

  marquee("Project ChronoTune");
  delay(500);
  marquee("by DS Engineering");
}


void loop() {
   
}

//This routine displays marquee style messages--------------------------------------------------------------
void marquee(String s)
{
  s.concat("    ");
  for (int i=0; i < s.length(); i++){
    
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
 
    // write it out!
    alpha4.writeDisplay();
    delay(200);
  }
  alpha4.clear();
  alpha4.writeDisplay();
}

