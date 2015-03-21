// IMPORTANT: Adafruit_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h FOR SETUP.
#define __SAM3X8E__

#include <Adafruit_GFX.h>   
#include <Adafruit_TFTLCD.h> 
#include <Scheduler.h>
#include <UTFT.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];

//UTFT myGLCD(ITDB32S,25,26,27,28);

extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// Настройка монитора

UTFT myGLCD(ITDB32S,25,26,27,28);

//UTouch        myTouch(6,5,4,3,2);
//
//// Finally we set up UTFT_Buttons :)
//UTFT_Buttons  myButtons(&myGLCD, &myTouch);

boolean default_colors = true;
uint8_t menu_redraw_required = 0;

// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// ADC speed one channel 480,000 samples/sec (no enable per read)
//           one channel 288,000 samples/sec (enable per read)

// analog stuff
uint32_t analogInPin5 = A5;
int analog_data[320], analog_data_old[320] ;
int trig_1, trig_2 ;
int i,j ;
#define ADC_scale 4
#define Vpos1 75
uint32_t ulValue = 0;
uint32_t ulChannel;
//static int _readResolution = 10;

int min_ADC, max_ADC, middle_ADC;
// lsat trigger time  ПОСЛЕДНИЙ Время срабатывания
int trig_time ;

// display 
//char time_str[20] ;
// sample rate частота дискретизации
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
 // analogReadResolution(12) ;
  // points or lines точки или линии
  pinMode(30,INPUT_PULLUP);

  myGLCD.InitLCD();
 // myGLCD.setFont(SmallFont);
  myGLCD.setFont(BigFont);
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Info: ",CENTER, 5);//
 
  // init the trigger state machine  инициализации триггера
  min_ADC = 100; max_ADC = 100; trig_1 = 50;  trig_2 = 150;
  
  // init the ADC  инициализировать АЦП
  if (analogInPin5 < A0) analogInPin5 += A0;  // 
  ulChannel = g_APinDescription[analogInPin5].ulADCChannelNumber ;
  adc_enable_channel( ADC, (adc_channel_num_t)ulChannel );   

  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Info: ",CENTER, 5);//
}

//======================================================================

void loop(void) 
{
  // trigger level avg of last trace  Уровень запуска AVG последнего следа
  middle_ADC = (min_ADC + max_ADC)/2 ;
  trig_2 = 0; trig_1 = 100000; // Always traverse the while  Всегда пройти время,
  trig_time = millis();
  
  // trigger detect триггера обнаружения
  // first conversion первое преобразование
  ADC_CR = ADC_START ;
  while(!(trig_2>middle_ADC && trig_1<=middle_ADC)) 
  {
    if (millis()>trig_time+1000) break;
    
    // first conversion первое преобразование
     ADC_CR = ADC_START ;
    // Wait for end of conversion Ожидание конца преобразования
    while (!(ADC_ISR & ADC_ISR_DRDY));
    // Read the value Считайте значение
    trig_1 = ADC_LCDR & ADC_DATA ;
    
    // second conversion второй преобразования
    ADC_CR = ADC_START ;
    // Wait for end of conversion Ожидание конца преобразования
    while (!(ADC_ISR & ADC_ISR_DRDY));
    // Read the value Считайте значение
    trig_2 = ADC_LCDR & ADC_DATA ;
    // next conversion следующего преобразования
     ADC_CR = ADC_START ;
  }
 
   // get loop time получить время цикла
   last_t = micros();
   // 1.45 microsec replacing all functions with direct register manipulation
   // 1,45 мкс Замена всех функций с прямой манипуляции регистра
   // ~688,000 samples/sec  ~ 688 000 выборок / сек
   
   // start first начать сначала
   ADC_CR = ADC_START ;
   
  for (i=0; i<320; i++)
  {
    // Wait for end of conversion  Ожидание конца преобразования
    while (!(ADC_ISR & ADC_ISR_DRDY));
    // Read the value  Считайте значение
    analog_data[i] = ADC_LCDR & ADC_DATA ;
    // start next  начать в следующем
    ADC_CR = ADC_START ;
  }
  // finish loop time время окончания цикла
  dt = micros() - last_t;
  
  //yield();
  // void Adafruit_TFTLCD::drawPixel(int16_t x, int16_t y, uint16_t color)  from TFTLCD.ccp
 for (i=0; i<319; i++)
 {
   if (points_lines)
   {
	  myGLCD.setColor(VGA_BLACK);
     // myGLCD.setColor(VGA_BLUE);
      myGLCD.drawLine(i, analog_data_old[i], i, analog_data_old[i+1]);
	  myGLCD.setColor(VGA_YELLOW);
      myGLCD.drawLine(i, Vpos1 + (analog_data[i]>>ADC_scale), i, Vpos1 + (analog_data[i+1]>>ADC_scale));
   }
   else
   {
	  myGLCD.setColor(VGA_BLACK);
      myGLCD.drawLine(i, analog_data_old[i], i, analog_data_old[i]);
	  myGLCD.setColor(VGA_YELLOW);
      myGLCD.drawLine(i, Vpos1 + (analog_data[i]>>ADC_scale), i, Vpos1 + (analog_data[i]>>ADC_scale));
   }
   analog_data_old[i] = Vpos1 + (analog_data[i]>>ADC_scale) ;
   if (analog_data[i]<min_ADC) min_ADC=analog_data[i];
   if (analog_data[i]>max_ADC) max_ADC=analog_data[i];
 }
 if (!points_lines){delay(30);}
 //myGLCD.clrScr();
 yield();
}