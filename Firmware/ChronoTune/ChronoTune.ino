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
Modified:       March 12, 2018
Verified:       March 12, 2018
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

/*****************************************************************************
                          DECLARATIONS FROM TUNER CODE
******************************************************************************/

#define N 200 //Buffer Size
#define sampleF 38500//Hz
#define display_time 5000//ms


byte incomingAudio, bIndex=N-1;
int buffer[N];
long int corr[N], corrMin;
long int t_old, t_new = millis();
int i, j, minIndex,s;
int Freq=0;
boolean clipping = 0, flag = 0;


const int A  = 110,             //These are the standard note frequencies
      As = 116,                 //at the lowest octave
      B  = 123,
      C  = 131,
      Cs = 139,
      D  = 147,
      Ds = 156,
      E  = 165,
      F  = 171,
      Fs = 185,
      G  = 196,
      Gs = 208;
int note = A;
int deviation = 0;
bool dev;               //Higher or lower from the correct note
bool correct;   //To show that the guitar is in tune


/*****************************************************************************
                                  SETUP
******************************************************************************/
void setup() {

  //ADC SETUP
  noInterrupts();    //disable interrupts
 
  //set up continuous sampling of analog pin 0
  //ADCSRA = 0;                    //clear ADCSRA and ADCSRB registers
  ADC->CTRLA.reg = 0;
  //ADCSRB = 0;
  ADC->CTRLB.reg = 0;
  //ADMUX |= (1 << REFS0);   //set reference voltage to AVcc with external capacitor at AREF pin
  //ADMUX |= (1 << ADLAR);   //left align the ADC value- so we can read highest 8 bits from ADCH register only
  //ADCSRA |= (1 << ADPS2) | (1 << ADPS0);        //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  //ADCSRA |= (1 << ADATE);     //enabble auto trigger (freerun mode already set with ADCSRB = 0)
  //ADCSRA |= (1 << ADIE);      //enable interrupts when measurement complete
  //ADCSRA |= (1 << ADEN);      //enable ADC
  ADC->CTRLA.bit.ENABLE = 1;
  //ADCSRA |= (1 << ADSC);      //start ADC measurements
    
  interrupts();    //enable interrupts
  //end of ADC Setup
  
  Serial.begin(9600);  //open the serial port (for debug only)

  alpha4.begin(0x70);  // pass in the address

  alpha4.clear();         //clear the display
  alpha4.writeDisplay();  //use this command to update the display

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  //display startup message
  marquee("ChronoTune");
  delay(300);
  marquee("by DS Engineering");
}

/*****************************************************************************
                                MAIN LOOP
******************************************************************************/
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

/*****************************************************************************
                FUNCTION TO COMPUTE FUNDAMENTAL FREQUENCY
******************************************************************************/

void compute(void){
  //Autocorrelate and find minimum point
  minIndex=0;
  for(i=0; i<=N-1; i++){
    corr[i] = 0;
    for(j=0; j<=N-1; j++){
      s = j+i;
      if((j+i)>(N-1))
          s = (j+i) - (N-1);
      corr[i] = corr[i] + (int)buffer[j]*buffer[s];
    }
      if(corr[i]<corr[minIndex])
        minIndex = i;

  }

  //Calculate Frequency
  Freq = sampleF/(minIndex*2);
}

/*****************************************************************************
                        PITCH DETECTION FUNCTION
******************************************************************************/

void pitch(void){
        /*
        This function is used to find the pitch after finding frequency and display the
        outputs on the 7 segment display and the LEDs
        */

        const float t = 0.035; //tolerance band for note
        const float t2 = 0.01; //tolerance band for correct note
        int octave = 1;

        //FIND OCTAVE
        if(Freq > Gs*1)
                octave = 2;
        if(Freq > Gs*2)
                octave =4;
        if(Freq > Gs*4)
                octave = 8;

        /*FIND PITCH
          The following portion is equivalent to checking if 'Freq'
          lies within a certain band around the given note.
          The width of this band is set by 't' (a percentage value)
          Check if:
               (input_note) lies within required_note +/- (percentage*required_note)
        */

          if( Freq > (A*octave)*(1-t) && Freq < (A*octave)*(1+t) ){
                note = A;
                //Signal the 7 seg display with the letter in each of these if statements.
                //seven_seg_A();
                }
          else  if( Freq > (As*octave)*(1-t) && Freq < (As*octave)*(1+t) ){
                note = As;
                //seven_seg_As();
                }
          else  if( Freq > (B*octave)*(1-t) && Freq < (B*octave)*(1+t) ){
                note = B;
                //seven_seg_B();
                }
          else  if( Freq > (C*octave)*(1-t) && Freq < (C*octave)*(1+t) ){
                note = C;
                //seven_seg_C();
                }
          else  if( Freq > (Cs*octave)*(1-t) && Freq < (Cs*octave)*(1+t) ){
                note = Cs;
                //seven_seg_Cs();
                }
          else  if( Freq > (D*octave)*(1-t) && Freq < (D*octave)*(1+t) ){
                note = D;
                //seven_seg_D();
                }
          else  if( Freq > (Ds*octave)*(1-t) && Freq < (Ds*octave)*(1+t) ){
                note = Ds;
                //seven_seg_Ds();
                }
          else  if( Freq > (E*octave)*(1-t) && Freq < (E*octave)*(1+t) ){
                note = E;
                //seven_seg_E();
                }
          else  if( Freq > (F*octave)*(1-t) && Freq < (F*octave)*(1+t) ){
                note = F;
                //seven_seg_F();
                }
          else  if( Freq > (Fs*octave)*(1-t) && Freq < (Fs*octave)*(1+t) ){
                note = Fs;
                //seven_seg_Fs();
                }
          else  if( Freq > (G*octave)*(1-t) && Freq < (G*octave)*(1+t) ){
                note = G;
                //seven_seg_G();
                }
          else  if( Freq > (Gs*octave)*(1-t) && Freq < (Gs*octave)*(1+t) ){
                note = Gs;
                //seven_seg_Gs();
                }

          //DISPLAY
          deviation = Freq - note*octave;
          if (abs(deviation) < t2*note){
                 correct = 1;      //in tune
                 //Serial.print(" ");
                 //Serial.println("Correct");
                 }
          else{
                correct = 0;      //not in tune
                if((deviation)>0){
                  //Serial.print(" ");
                  //Serial.println("Note = deviated HIGH");
                  dev = HIGH;
                }
                else {
                        // Serial.print(" ");
                        //Serial.println("Deviated Low");
                        dev = LOW;
                }
        }
}
/***************END OF PITCH DETECTION FUNCTION****************/
