
#define __SAM3X8E__
#include <Adafruit_GFX.h>   
#include <Scheduler.h>

#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>

// Declare which fonts we will be using
//UTFT myGLCD(ITDB32S,25,26,27,28);

extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// Настройка монитора

UTFT myGLCD(ITDB32S,25,26,27,28);

UTouch        myTouch(6,5,4,3,2);
//
//// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

// ADC speed one channel 480,000 samples/sec (no enable per read)
//           one channel 288,000 samples/sec (enable per read)



// analog stuff
uint32_t analogInPin = A0;
int analog_data[320], analog_data_old[320] ;
int trig_1, trig_2 ;
int i,j ;
#define ADC_scale 4
#define Vpos1 75
uint32_t ulValue = 0;
uint32_t ulChannel;
static int _readResolution = 10;
int min_ADC, max_ADC, middle_ADC;
// lsat trigger time
int trig_time ;

// display 
char time_str[20] ;
// sample rate
int dt, last_t ;
int points_lines = 1; // 1 => lines

// ADC_MR is 0x400C0004
#define ADC_MR * (volatile unsigned int *) (0x400C0004) /*adc mode word*/
#define ADC_CR * (volatile unsigned int *) (0x400C0000) /*write a 2 to start convertion*/
#define ADC_ISR * (volatile unsigned int *) (0x400C0030) /*status reg -- bit 24 is data ready*/
#define ADC_ISR_DRDY 0x01000000
#define ADC_START 2
#define ADC_LCDR * (volatile unsigned int *) (0x400C0020) /*last converted low 12 bits*/
#define ADC_DATA 0x00000FFF 


//=== setup ==============================================================
void setup(void) 
{
  Serial.begin(9600);
  Serial.println(F("TFT LCD scope"));
  
  ADC_MR |= 0x00000100 ; // ADC full speed
  
  // pooints or lines
  pinMode(30,INPUT_PULLUP);


    myGLCD.InitLCD();
	// myGLCD.setFont(SmallFont);
	myGLCD.setFont(BigFont);
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("Info: ",CENTER, 5);//

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
  
  points_lines = digitalRead(30) ;
  
  // init the scheduler
 // Scheduler.startLoop(loop2);
  
  // init the trigger state machine
  min_ADC = 100; max_ADC = 100; trig_1 = 50;  trig_2 = 150;
  
  // init the ADC
  if (analogInPin < A0) analogInPin += A0;
  ulChannel = g_APinDescription[analogInPin].ulADCChannelNumber ;
  adc_enable_channel( ADC, (adc_channel_num_t)ulChannel );   
}

//=== loop =============================================================
void loop(void) 
{
    // trigger level avg of last trace
  middle_ADC = (min_ADC + max_ADC)/2 ;
  trig_2 = 0; trig_1 = 100000; // Always traverse the while
  trig_time = millis();
  
  // trigger detect
  // first conversion
     ADC_CR = ADC_START ;
  while(!(trig_2>middle_ADC && trig_1<=middle_ADC)) 
  {
    if (millis()>trig_time+1000) break;
    
    // first conversion
     //ADC_CR = ADC_START ;
    // Wait for end of conversion
    while (!(ADC_ISR & ADC_ISR_DRDY));
    // Read the value
    trig_1 = ADC_LCDR & ADC_DATA ;
    
    // second conversion
    ADC_CR = ADC_START ;
    // Wait for end of conversion Ожидание конца преобразования
    while (!(ADC_ISR & ADC_ISR_DRDY));
    // Read the value
    trig_2 = ADC_LCDR & ADC_DATA ;
    // next conversion
     ADC_CR = ADC_START ;
  }
 
   // get loop time
   last_t = micros();
   // 1.45 microsec replacing all functions with direct register manipulation
   // ~688,000 samples/sec
   
   // start first
   ADC_CR = ADC_START ;
   
  for (i=0; i<320; i++)
  {
    // Wait for end of conversion
    while (!(ADC_ISR & ADC_ISR_DRDY));
    // Read the value
    analog_data[i] = ADC_LCDR & ADC_DATA ;
    // start next
    ADC_CR = ADC_START ;
  }
  // finish loop time

  dt = micros() - last_t;
  Serial.println(dt/320);
  //yield();
  // void Adafruit_TFTLCD::drawPixel(int16_t x, int16_t y, uint16_t color)  from TFTLCD.ccp
 for (i=0; i<319; i++)
 {
   if (points_lines)
   {
	   myGLCD.setColor( VGA_BLACK);
       myGLCD.drawLine (i, analog_data_old[i], i, analog_data_old[i+1]);
	  // myGLCD.setColor( VGA_YELLOW);
	   myGLCD.setColor( 255, 255, 255);
       myGLCD.drawLine (i, Vpos1 + (analog_data[i]>>ADC_scale), i, Vpos1 + (analog_data[i+1]>>ADC_scale));
   }
   else
   {
	    myGLCD.setColor( VGA_BLACK);
        myGLCD.drawLine (i, analog_data_old[i], i, analog_data_old[i]);
	   // myGLCD.setColor( VGA_YELLOW);
		myGLCD.setColor( 255, 255, 255);
        myGLCD.drawLine(i, Vpos1 + (analog_data[i]>>ADC_scale), i, Vpos1 + (analog_data[i]>>ADC_scale));
   }
   analog_data_old[i] = Vpos1 + (analog_data[i]>>ADC_scale) ;
   if (analog_data[i]<min_ADC) min_ADC=analog_data[i];
   if (analog_data[i]>max_ADC) max_ADC=analog_data[i];
 }
// if (!points_lines){delay(30);}
 
// yield();
} 

//================================================
// slow background task
void loop2() 
{
 /* int i, st, ed;
  delay(1000);*/
 /* tft.setCursor(0, 0);
  tft.setTextColor(WHITE);  tft.setTextSize(1);
  tft.println("ECE5030 Scope");*/
 /* st=0; ed=0;
  for (i=0; i<318; i++)
  {
   if (analog_data[i]<middle_ADC && analog_data[i+1]>=middle_ADC && st==0 ) st = i;
   if (analog_data[i]<middle_ADC && analog_data[i+1]>=middle_ADC && st>0 && i>st+2 && ed==0) ed = i;
  }*/
 
 /* tft.fillRect(0, 10, 100, 10, BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(GREEN);*/
  //sprintf(time_str, "FREQ= %d", 688000/(ed-st));
  ////tft.println(time_str);
  //sprintf(time_str, "%d ", dt);
  //Serial.println(time_str);

}

//======================================================================
//=== routines =========================================================

  

/*
unsigned long testText() {
  tft.fillScreen(BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(BLACK);

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  tft.fillScreen(BLACK);

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(BLACK);

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(BLACK);

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);

  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(BLACK);
  start = micros();
  for(y=0; y<h; y+=5) tft.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for(i=2; i<n; i+=6) {
    i2 = i / 2;
    tft.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  n = min(tft.width(), tft.height());
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                w = tft.width()  + radius,
                h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for(x=0; x<w; x+=r2) {
    for(y=0; y<h; y+=r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  n     = min(cx, cy);
  start = micros();
  for(i=0; i<n; i+=5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(0, 0, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  start = micros();
  for(i=min(cx,cy); i>10; i-=5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(0, i, i));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(i, i, 0));
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for(i=0; i<w; i+=6) {
    i2 = i / 2;
    tft.drawRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  start = micros();
  for(i=min(tft.width(), tft.height()); i>20; i-=6) {
    i2 = i / 2;
    tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(0, i, 0));
  }

  return micros() - start;
}
*/
//========================================================================
