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

Version:        0.1.0
Modified:       February 4, 2018
Verified:       February 4, 2018
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
#include <FlashAsEEPROM.h>
#include "RTClib.h"

//hardware setup
RTC_DS3231 rtc;                                    //declare the RTC, actual RTC is DS3232
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();  //declare the display

//set pin numbers
const byte FSW = 2;                 //footswitch pin, active low
const byte SQW = 4;                 //RTC square wave input pin
const byte CLN = 13;                //colon LEDs, active high

//declare variables
char displaybuffer[4] = {' ', ' ', ' ', ' '};      //buffer for marquee messages

boolean buttonDown = false;         //button's current debounced reading
boolean buttonHeld = false;         //button hold detector
unsigned long buttonTimer = 0;      //timer for debouncing button
int debounce = 40UL;                //debounce time in milliseconds

byte SQWstate;                      //stores the state of the SQW pin

byte swHrs = 0;                     //stores the hours value of the stopwatch
byte swMin = 0;                     //stores the minutes value of the stopwatch
byte swSec = 0;                     //stores the seconds value of the stopwatch
boolean runSW = false;              //trigger for incrementing the stopwatch in the main loop

byte ctdnMin;                       //stores the minutes value of the countdown timer (EEPROM)
byte ctdnSec;                       //stores the seconds value of the countdown timer (EEPROM)
boolean runTimer = false;           //trigger for decrementing the timer in the main loop

byte clkHour = 0;                   //stores the hour of the current time
byte clkMin = 0;                    //stores the minute of the current time

byte menu = 0;                      //menu register stores the current position in the menu tree
boolean setupMode = false;          //specifies whether startup should go to setup mode or normal
byte warning;                       //temporarily holds the value of the countdown timer warning

//boolean drawColon = false;          //lights the colon segments on the display when true


//***********************************************************************************************************************
//---------------------------------------------------------SETUP---------------------------------------------------------
//***********************************************************************************************************************


void setup() {
  Serial.begin(9600);                                     //open the serial port (for debug only)

  //check EEPROM for errors and load default values if errors are found
  if(EEPROM.read(0) > 99) EEPROM.update(0, B00000001);    //initialize countdown timer minutes to 1
  if(EEPROM.read(1) > 59) EEPROM.update(1, B00000000);    //initialize countdown timer seconds to 0
  if(EEPROM.read(2) > 1)  EEPROM.update(2, B00000000);    //initialize clock hour format to "12hr"
      //0 = 12hr, 1 = 24hr
  if(EEPROM.read(3) > 1)  EEPROM.update(3, B00000000);    //initialize countdown timer warning to "on"
      //0 = on, 1 = off
  if(EEPROM.read(4) > 10) EEPROM.update(4, B00000001);    //initialize warning time to 1min
  EEPROM.commit();

  //setup I/O pin modes
  pinMode (FSW, INPUT_PULLUP);                            //footswitch is NO between pin and ground
  pinMode (SQW, INPUT_PULLUP);                            //RTC SQW pin is open drain; requires pullup

  pinMode (CLN, OUTPUT);                                  //colon LEDs, active high
  digitalWrite(CLN, LOW);                                 //initialize colon off

  alpha4.begin(0x70);                                     //display driver is at I2C address 70h

  alpha4.clear();                                         //clear the display
  alpha4.writeDisplay();                                  //update the display with new data


  //if the footswitch is held down, start in setup mode
    if(digitalRead(FSW) == LOW)
    {
      marquee("SETUP MODE");
      //writeLeftRaw(0x5B, 0x0F);     //S,t
      //writeRightRaw(0x1C, 0x67);    //u,P
      delay(1000);
      setupMode = true;
      //load the first menu item
      marquee("Warning Threshold");
      //WriteDisp(0x09, 0x0C);                                // do not decode left, decode right
      //writeLeftRaw(0x1F, 0x0E);     //b,L
      alpha4.writeDigitAscii(0, 'B');
      alpha4.writeDigitAscii(1, 'L');
      drawColon(true);
      warning = EEPROM.read(4);
      writeRight(warning);
      delay(500);
      blinkRight(warning);
      //set the next menu item
      menu = B01010000;
    }


  //if the setup mode was not initiated boot in normal mode
  if(setupMode == false)
  {
    //display startup message
    char4("DSGE");
    delay(1000);
    marquee("ChronoTune");
    //delay(500);
    //marquee("by DS Engineering");

    writeClk();
  }
}


//***********************************************************************************************************************
//-------------------------------------------------------MAIN LOOP-------------------------------------------------------
//***********************************************************************************************************************


void loop() {
  writeClk();
  alpha4.blinkRate(0);
  delay(250);
}


//***********************************************************************************************************************
//-------------------------------------------------------SUBROUTINES-----------------------------------------------------
//***********************************************************************************************************************


//This routine writes the current time to the display--------------------------------------------------------------
void writeClk()
{
  DateTime now = rtc.now();                     // Get the RTC info
  clkHour = now.hour();                         // Get the hour
  if(EEPROM.read(2) == 0)
  {
    if(clkHour > 12) clkHour -= 12;             // if 12hr format is selected, convert the hours
    if(clkHour == 0) clkHour = 12;
  }
  clkMin = now.minute();                        // Get the minutes
  drawColon(true);                              //turn on the colon
  //WriteDisp(0x09, 0x0F);                        // Set all digits to "BCD decode".
  writeLeft(clkHour);                           //push the hours value to the display
  writeRight(clkMin);                           //push the minutes value to the display
}


//This routine writes numbers to the left side of the display--------------------------------------------------------
void writeLeft(byte x)
{
  char y = (x / 10) + '0';                      //calculate left digit and convert number to character
  char z = (x % 10) + '0';                      //calculate right digit and convert number to character
  alpha4.writeDigitAscii(0, y);                 //write the left number
  alpha4.writeDigitAscii(1, z);                 //write the right number

  //if the clock is active and in 12 hour format
  if((clkHour < 10) && (menu == 0 || menu == B00010000 || menu == B00100000) && (EEPROM.read(2) == 0))
  {
    alpha4.writeDigitAscii(0, ' ');             // turn off the first digit if time is less than 10:00
  }

  /*if(drawColon == false) {alpha4.writeDigitAscii(2, x % 10);}
  if(drawColon == true)
  {
    x = (x % 10);
    x = (x |= B10000000);
    alpha4.writeDigitAscii(2, x);
  }*/
  alpha4.writeDisplay();
}


//This routine writes numbers to the right side of the display------------------------------------------------------
void writeRight(byte x)
{
  char y = (x / 10) + '0';                      //calculate left digit and convert number to character
  char z = (x % 10) + '0';                      //calculate right digit and convert number to character
  alpha4.writeDigitAscii(2, y);                 //write the left number
  alpha4.writeDigitAscii(3, z);                 //write the right number
  /*alpha4.writeDigitAscii(3, y % 10);
  if(drawColon == false) {alpha4.writeDigitAscii(3, y / 10);}
  if(drawColon == true)
  {
    y = (y / 10);
    y = (y |= B10000000);
    alpha4.writeDigitAscii(2, y);
  }*/
  alpha4.writeDisplay();
}


//This routine displays marquee style messages--------------------------------------------------------------
//  must pass a string; returns nothing
void marquee(String s)
{
  s.concat("    ");     //concatenate 4 blank spaces to  the beginning of the message to make the entire
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


//This routine displays static 4-character messages-------------------------------------------------------------------
//  must pass a string; returns nothing
void char4(String s)
{
  alpha4.writeDigitAscii(0, s.charAt(0));
  alpha4.writeDigitAscii(1, s.charAt(1));
  alpha4.writeDigitAscii(2, s.charAt(2));
  alpha4.writeDigitAscii(3, s.charAt(3));
  alpha4.writeDisplay();
}


//This routine blinks the left side of the display--------------------------------------------------------------------
void blinkLeft(byte x)
{
  alpha4.writeDigitAscii(0, ' ');
  alpha4.writeDigitAscii(1, ' ');
  alpha4.writeDisplay();
  delay(500);
  writeLeft(x);
}


//This routine blinks the right side of the display--------------------------------------------------------------------
void blinkRight(byte x)
{
  alpha4.writeDigitAscii(2, ' ');
  alpha4.writeDigitAscii(3, ' ');
  alpha4.writeDisplay();
  delay(500);
  writeRight(x);
}


//This routine turns the colon LEDs on or off---------------------------------------------------------------------------
void drawColon(boolean x)
{
  if (x == true) digitalWrite(CLN, HIGH);
  if (x == false) digitalWrite(CLN, LOW);
}


//This routine addresses and sends data to the RTC chip (write operation)-----------------------------------------------
void writeRTC(byte cmd, byte data) {
  Wire.beginTransmission(0x68);      //7-bit address for DS3231
  Wire.write(cmd);
  Wire.write(data);
  Wire.endTransmission();
}
