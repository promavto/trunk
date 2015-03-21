/*--------------------------------------------------------------
  Program:       OscopetouchLCDmega

  Description:   Digital Oscilloscope with data  displayed
                 on Color TFT LCD with touch screen
  
  Hardware:      sainsmart mega2560 board with 3.5" tft lcd touch  module display and shield kit      
                 http://www.sainsmart.com/home-page-view/sainsmart-mega2560-board-3-5-tft-lcd-module-display-shield-kit-for-atmel-atmega-avr-16au-atmega8u2.html

  Software:      Developed using Arduino 1.0.3 software
                 This program requires the UTFT library and the
                 UTouch library from Henning Karlsen.
                 web: http://www.henningkarlsen.com/electronics
                 Version 1.00
  Date:          5 April 2014
 
  Author:        johnag    
--------------------------------------------------------------*/

#include <UTFT.h>
#include <UTouch.h>
#include <AH_AD9850.h>

//Настройка звукового генератора
#define CLK     8  // Назначение выводов генератора сигналов
#define FQUP    9  // Назначение выводов генератора сигналов
#define BitData 10 // Назначение выводов генератора сигналов
#define RESET   11 // Назначение выводов генератора сигналов
AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// настройка звукового генератора



// Declare which fonts we will be using 
extern uint8_t SmallFont[];
// Initialize Screen and touch functions
UTFT    myGLCD(ITDB32S,38,39,40,41);
UTouch  myTouch(6,5,4,3,2);
// Declare variables
char buf[12];
int x,y;
int Input = 0;
byte Sample[320];
byte OldSample[320];
int StartSample = 0; 
int EndSample = 0;
int Max = 100;
int Min = 100;
int mode = 0;
int dTime = 1;
int tmode = 0;
int Trigger = 0;
int SampleSize = 0;
int SampleTime = 0;
int dgvh;
int hpos = 50; //set 0v on horizontal  grid
int vsens = 4; // vertical sensitivity

// Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

//------------Start Subrutines------------------------------------
//--------draw buttons sub
void buttons()
{
   myGLCD.setColor(0, 0, 255);
   myGLCD.fillRoundRect (250, 1, 310, 50);
   myGLCD.fillRoundRect (250, 55, 310, 105);
   myGLCD.fillRoundRect (250, 110, 310, 160);
   myGLCD.fillRoundRect (250, 165, 310, 215);
}
//-------touchscreen position sub
void touch()
{
	while (myTouch.dataAvailable())
		{
			myTouch.read();
			x=myTouch.getX();
			y=myTouch.getY();
			delay(500);
			if ((y>=1) && (y<=50))  // Delay row
				{
				if ((x>=250) && (x<=300))  //  Delay Button
					
				    	waitForIt(250, 1, 310, 50);
					    mode= mode ++ ;
						{
						myGLCD.setColor(255, 0, 0);
						myGLCD.drawRoundRect (250, 1, 310, 50);   
						// Select delay times
						if (mode == 0) dTime = 0;
						if (mode == 1) dTime = 1;
						if (mode == 2) dTime = 2;
						if (mode == 3) dTime = 5;
						if (mode == 4) dTime = 10;
						if (mode == 5) dTime = 20;
						if (mode == 6) dTime = 30;
						if (mode == 7) dTime = 50;
						if (mode == 8) dTime = 100;
						if (mode == 9) dTime = 200;
						if (mode == 10) dTime = 500;
						if (mode > 10) mode = 0;   
    

					}
				}

			if ((y>=70) && (y<=120))  // Trigger  row
				{
					if ((x>=250) && (x<=300))  // Trigger Button
					
					waitForIt(250, 55, 310, 105);
					tmode= tmode ++;
			
						myGLCD.setColor(255, 0, 0);
						// Select Software trigger value
						myGLCD.drawRoundRect (250, 55, 310, 105);      
						if (tmode == 1) Trigger = 0;
						if (tmode == 2) Trigger = 10;
						if (tmode == 3) Trigger = 20;
						if (tmode == 4) Trigger = 30;
						if (tmode == 5) Trigger = 50;
						if (tmode > 5)tmode = 0;
					}
				}
			if ((y>=130) && (y<=180))  // H position   row
				{
					if ((x>=250) && (x<=300))  // H position Button
				
					waitForIt(250, 110, 310, 160);
					hpos= hpos ++;
   
					{
					myGLCD.setColor(255, 0, 0);
					myGLCD.drawRoundRect (250, 110, 310, 160); 
					myGLCD.clrScr();
					buttons();
					if (hpos > 60)hpos = 50;
					}
				}
		}

//----------wait for touch sub 
void waitForIt(int x1, int y1, int x2, int y2)
{
  while (myTouch.dataAvailable())
  myTouch.read();
}
//----------draw grid sub
void DrawGrid()
{

  myGLCD.setColor( 0, 200, 0);
  for(  dgvh = 0; dgvh < 5; dgvh ++)
	  {
		  myGLCD.drawLine( dgvh * 50, 0, dgvh * 50, 240);
		  myGLCD.drawLine(  0, dgvh * 50, 245 ,dgvh * 50);
	  }
  myGLCD.drawLine( 245, 0, 245, 240);
  myGLCD.drawLine( 0, 239, 245, 239);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (250, 1, 310, 50);
  myGLCD.drawRoundRect (250, 55, 310, 105);
  myGLCD.drawRoundRect (250, 110, 310, 160);
  myGLCD.drawRoundRect (250, 165, 310, 215);
 
  }
  // ------ Wait for input to be greater than trigger sub
void trigger()
{
   while (Input < Trigger)
   { 
     Input = analogRead(A0)*5/100;
   }
}

//---------------End Subrutines ----------------------


 void setup() {
   myGLCD.InitLCD();
   myGLCD.clrScr();
   myTouch.InitTouch();
   myTouch.setPrecision(PREC_MEDIUM);
   buttons();
   pinMode(0, INPUT); 
    // set up the ADC
  ADCSRA &= ~PS_128;  // remove bits set by Arduino library

  // you can choose a prescaler from below.
  // PS_16, PS_32, PS_64 or PS_128
  ADCSRA |= PS_64;    // set our own prescaler 
  
  	// Настройка звукового генератора  
	AD9850.reset();                    //reset module
	delay(1000);
	AD9850.powerDown();                //set signal output to LOW
	AD9850.set_frequency(0,0,500);    //set power=UP, phase=0, 1kHz frequency 
  
  
 }
void loop() 
{
   
   while(1) {
   DrawGrid();
   touch();
   trigger();

 // Collect the analog data into an array
 
 StartSample = micros();
 for( int xpos = 0;
 xpos < 240; xpos ++) { Sample[ xpos] = analogRead(A0)*5/102;
 delayMicroseconds(dTime);
 }
  EndSample = micros();
  
// Display the collected analog data from array
for( int xpos = 0; xpos < 239;
xpos ++)
{
// Erase previous display
myGLCD.setColor( 0, 0, 0);
myGLCD.drawLine (xpos + 1, 255-OldSample[ xpos + 1]* vsens-hpos, xpos + 2, 255-OldSample[ xpos + 2]* vsens-hpos);
if (xpos == 0) myGLCD.drawLine (xpos + 1, 1, xpos + 1, 239);
// Draw the new data
myGLCD.setColor (255, 255, 255);
myGLCD.drawLine (xpos, 255-Sample[ xpos]* vsens-hpos, xpos + 1, 255-Sample[ xpos + 1]* vsens-hpos);
}
// Determine sample voltage peak to peak
Max = Sample[ 100];
Min = Sample[ 100];
for( int xpos = 0;
xpos < 240; xpos ++)
{
OldSample[ xpos] = Sample[ xpos];
if (Sample[ xpos] > Max) Max = Sample[ xpos];
if (Sample[ xpos] < Min) Min = Sample[ xpos];
}
// display the sample time, delay time and trigger level
myGLCD.setBackColor( 0, 0, 255);
myGLCD.setFont( SmallFont);
myGLCD.setBackColor( 0, 0, 255);
myGLCD.print("Delay", 260, 5);
myGLCD.print("    ", 270, 20);
myGLCD.print(itoa ( dTime, buf, 10), 270, 20);
myGLCD.print("Trig.", 260, 60);
myGLCD.print("   ", 270, 75);
myGLCD.print(itoa( Trigger, buf, 10), 270, 75);
myGLCD.print("H Pos.", 260, 120);
myGLCD.print( itoa( hpos, buf, 10), 270, 135);
//myGLCD.setBackColor( 0, 0, 0);
SampleTime =( EndSample-StartSample)/1000;
myGLCD.print("Sec.", 205, 210);
myGLCD.print(" ", 280, 30);
myGLCD.print(itoa( SampleTime, buf, 10), 205, 220);
// Range of 0 to 64 * 78 = 4992 mV
SampleSize =( Max-Min)*78;
myGLCD.print("mVolt", 5, 210);
myGLCD.print( itoa( SampleSize, buf, 10),5, 220);
myGLCD.print(itoa( analogRead(A0)*4.15/10.23, buf, 10),110 ,220);

}}


