

#include <Arduino.h>
#include <keyb4x4.h>
#include "U8glib.h"
#include <AH_AD9850.h>

//U8GLIB_ST7920_128X64_1X u8g(22, 24, 26);	// SPI Com: SCK = en = 18, MOSI = rw = 16, CS = di = 17
U8GLIB_ST7920_128X64_1X u8g(A0, A1, A2);	// SPI Com: SCK = en = 18, MOSI = rw = 16, CS = di = 17


char  txt_count1[10];  //
char  txt_count2[10]; 
char k ='\0';
uint8_t draw_state = 0;

//Настройка звукового генератора
#define CLK     8      // Назначение выводов генератора сигналов
#define FQUP    9      // Назначение выводов генератора сигналов
#define BitData 10     // Назначение выводов генератора сигналов
#define RESET   11     // Назначение выводов генератора сигналов
#define CLK1     4     // Назначение выводов генератора сигналов
#define FQUP1    5     // Назначение выводов генератора сигналов
#define BitData1 6     // Назначение выводов генератора сигналов
#define RESET1   7     // Назначение выводов генератора сигналов

int frequency_A = 300;      // Частота 1 генератора начальная
int frequency_B = 800;      // Частота 2 генератора начальная


#define led1     12  // 
#define led13    13  // 

#define Kn1      A3  // 
#define Kn2      A4  // 
#define Kn3      A5  // 



#define KEY_NONE 0
#define KEY_PREV 1
#define KEY_NEXT 2
#define KEY_SELECT 3



uint8_t uiKeyPrev   = A3;
uint8_t uiKeyNext   = A4;
uint8_t uiKeySelect = A5;


uint8_t uiKeyCodeFirst = KEY_NONE;
uint8_t uiKeyCodeSecond = KEY_NONE;
uint8_t uiKeyCode = KEY_NONE;

#define MENU_ITEMS 6
char *menu_strings[MENU_ITEMS] = { "300 - 800", "400 - 1000", "700 - 1600", "1000 - 2200" , "1400 - 2600", "1500 - 3400"};
char *menu_Num[1];                 // Вывод на дисплей номера клавиши

uint8_t menu_current = 0;
int menu_current1 = 0;
uint8_t menu_redraw_required = 0;
uint8_t last_key_code = KEY_NONE;

int display = 0;                         // Вывод № картинки


//создаем объект клавиатуры
Keyb4x4 myKeyb (32,33,34,35,36,37,38,39);
int line=0;
int column=0;
unsigned long t=0;



AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// настройка звукового генератора
AH_AD9850 AD9850_1(CLK1, FQUP1, BitData1, RESET1);// настройка звукового генератора


void u8g_prepare(void)
{
  u8g.setFont(u8g_font_6x10);
  u8g.setFontRefHeightExtendedText();
  u8g.setDefaultForegroundColor();
  u8g.setFontPosTop();
}

void uiStep(void) 
{
  uiKeyCodeSecond = uiKeyCodeFirst;
  if ( digitalRead(uiKeyPrev) == LOW )
	uiKeyCodeFirst = KEY_PREV;
  else if ( digitalRead(uiKeyNext)   == LOW )
	uiKeyCodeFirst = KEY_NEXT;
  else if ( digitalRead(uiKeySelect) == LOW )
	uiKeyCodeFirst = KEY_SELECT;
   else 
	uiKeyCodeFirst = KEY_NONE;
  
  if ( uiKeyCodeSecond == uiKeyCodeFirst )
	uiKeyCode = uiKeyCodeFirst;
  else
	uiKeyCode = KEY_NONE;
}
void drawMenu(void) 
{
  uint8_t i, h;
  u8g_uint_t w, d;

  u8g.setFont(u8g_font_6x13);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  
  h = u8g.getFontAscent()-u8g.getFontDescent();
  w = u8g.getWidth();
  for( i = 0; i < MENU_ITEMS; i++ )
	  {
		d = (w-u8g.getStrWidth(menu_strings[i]))/2;
		u8g.setDefaultForegroundColor();
		if ( i == menu_current ) 
			{
			  u8g.drawBox(0, i*h+1, w, h);
			  u8g.setDefaultBackgroundColor();
			}
		u8g.drawStr(d, i*h, menu_strings[i]);
	  }
}


void updateMenu(void)
{
  if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) 
  {
	return;
  }

  last_key_code = uiKeyCode;
  switch ( uiKeyCode ) 
	  {
		case KEY_NEXT:
		  menu_current++;
		  if ( menu_current >= MENU_ITEMS )
			menu_current = 0;
		  //  menu_current1 = 0;
		  menu_Num[0] = "3";
		  menu_redraw_required = 1;
		  break;
		case KEY_PREV:
		  if ( menu_current == 0 )
		  menu_current = MENU_ITEMS;
		  menu_Num[0] = "4";
		  menu_current--;
		  menu_redraw_required = 1;
		  break;
		case KEY_SELECT:
			if (menu_current == 0)
				{
					AD9850.set_frequency(0,0,300);       //set power=UP, phase=0, 1kHz frequency 
					AD9850_1.set_frequency(0,0,800);     //set power=UP, phase=0, 1kHz frequency 
				}
		    if (menu_current == 1)
				{
				    AD9850.set_frequency(0,0,400);        //set power=UP, phase=0, 1kHz frequency 
					AD9850_1.set_frequency(0,0,1000);     //set power=UP, phase=0, 1kHz frequency 
				}
			if (menu_current == 2)
				{
				    AD9850.set_frequency(0,0,700);        //set power=UP, phase=0, 1kHz frequency 
					AD9850_1.set_frequency(0,0,1600);     //set power=UP, phase=0, 1kHz frequency 
				}
			if (menu_current == 3)
				{
				   AD9850.set_frequency(0,0,1000);       //set power=UP, phase=0, 1kHz frequency 
				   AD9850_1.set_frequency(0,0,2200);     //set power=UP, phase=0, 1kHz frequency 
				}
			if (menu_current == 4)
				{
				    AD9850.set_frequency(0,0,1400);        //set power=UP, phase=0, 1kHz frequency 
					AD9850_1.set_frequency(0,0,2600);     //set power=UP, phase=0, 1kHz frequency 
				}
			if (menu_current == 5)
				{
				   AD9850.set_frequency(0,0,1500);       //set power=UP, phase=0, 1kHz frequency 
				   AD9850_1.set_frequency(0,0,3400);     //set power=UP, phase=0, 1kHz frequency 
				}

			 break;
		
	  }
}

void PinSetup(void)
{
  // configure input keys 
	pinMode(led1, OUTPUT);  
	
	pinMode(led13, OUTPUT);    
  
	digitalWrite(led1, LOW);  
	
	digitalWrite(led13, HIGH);  

	pinMode(Kn1, INPUT);  
	pinMode(Kn2, INPUT);  
	pinMode(Kn3, INPUT);  


	digitalWrite(Kn1, HIGH);  
	digitalWrite(Kn2, HIGH);
	digitalWrite(Kn3, HIGH);
	
  
  //pinMode(uiKeyPrev, INPUT);                                // set pin to input
  //digitalWrite(uiKeyPrev, HIGH);                            // turn on pullup resistors
  //pinMode(uiKeyNext, INPUT);                                // set pin to input
  //digitalWrite(uiKeyNext, HIGH);                            // turn on pullup resistors
  //pinMode(uiKeySelect, INPUT);                              // set pin to input
  //digitalWrite(uiKeySelect, HIGH);                          // turn on pullup resistors
  
}
void setup_sound()
{

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Настройка звукового генератора  
  AD9850.reset();                    //reset module
  AD9850_1.reset();                    //reset module
  delay(1000);
  AD9850.powerDown();                //set signal output to LOW
  AD9850.set_frequency(0,0,200);     //set power=UP, phase=0, 1kHz frequency 
  AD9850_1.powerDown();                //set signal output to LOW
  AD9850_1.set_frequency(0,0,2000);     //set power=UP, phase=0, 1kHz frequency 


  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
}

void setup(void)
{
	  
  u8g_prepare();

   Serial.begin(9600);
	
	//иниц. клав. 
	myKeyb.begin(false);

	PinSetup();                                // setup key detection and debounce algorithm
	menu_redraw_required = 1;     // force initial redraw

	setup_sound();
	digitalWrite(led1, HIGH);  
}

void loop(void) 
{
 
	uiStep();                                     // check for key press
	
	if (  menu_redraw_required != 0 ) 
		{
			u8g.firstPage();
			do  
			{
				  drawMenu();
			} while( u8g.nextPage() );
			menu_redraw_required = 0;
		}

	updateMenu();                            // update menu bar
  

  delay(15);
}

