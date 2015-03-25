/**
 * This program logs data from the Arduino ADC to a binary file.
 *
 * Samples are logged at regular intervals. Each Sample consists of the ADC
 * values for the analog pins defined in the PIN_LIST array.  The pins numbers
 * may be in any order.
 * 
 * Edit the configuration constants below to set the sample pins, sample rate,
 * and other configuration values.
 *
 * If your SD card has a long write latency, it may be necessary to use
 * slower sample rates.  Using a Mega Arduino helps overcome latency
 * problems since 13 512 byte buffers will be used.
 *
 * Each 512 byte data block in the file has a four byte header followed by up
 * to 508 bytes of data. (508 values in 8-bit mode or 254 values in 10-bit mode)
 * Each block contains an integral number of samples with unused space at the
 * end of the block.
 *
 * Data is written to the file using a SD multiple block write command.
 */
#include <SdFat.h>
#include <SdFatUtil.h>
#include <StdioStream.h>
#include "AnalogBinLogger.h"
#include "Wire.h"
#include <AH_AD9850.h>
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <RTClib.h>
#include <avr/pgmspace.h>

#define  ledPin12  12                               // Назначение светодиодов на плате
boolean ledStatus = false;
int end_measure = 0;

//Настройка звукового генератора
#define CLK     8  // Назначение выводов генератора сигналов
#define FQUP    9  // Назначение выводов генератора сигналов
#define BitData 10 // Назначение выводов генератора сигналов
#define RESET   11 // Назначение выводов генератора сигналов
AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// настройка звукового генератора

uint8_t sec = 0;       //Initialization time
uint8_t min = 0;
uint8_t hour = 0;
uint8_t dow = 1;
uint8_t date = 1;
uint8_t mon = 1;
uint16_t year = 14;
unsigned long timeF;
int flag_time = 0;

RTC_DS1307 RTC;  // define the Real Time Clock object

//=====================================================================================
//******************************* Настройки монитора и Touch панели *******************************

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// Настройка монитора

UTFT          myGLCD(ITDB32S,38,39,40,41);

UTouch        myTouch(6,5,4,3,2);

// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

boolean default_colors = true;
uint8_t menu_redraw_required = 0;

//----------------------Конец  Настройки дисплея --------------------------------
//**************************  Меню прибора ***************************************

int clockCenterX=119;
int clockCenterY=119;
int oldsec=0;
char* str[] = {"MON","TUE","WED","THU","FRI","SAT","SUN"};
char* str_mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

//-----------------------------------------------------------------------------------------------

//*********************** Переменные для цифровой клавиатуры**********************************
int x, y, z;
char stCurrent[20]     ="";                // Переменная хранения введенной строки 
//char stCurrent1[20];                       // Переменная хранения введенной строки 
int stCurrentLen       = 0;                // Переменная хранения длины введенной строки 
int stCurrentLen1      = 0;                // Переменная временного хранения длины введенной строки  
//int stCurrentLen_user  = 0;                // Переменная  хранения длины введенной строки пароля пользователя
//int stCurrentLen_telef = 0;                // Переменная  хранения длины введенной строки пароля пользователя
//int stCurrentLen_admin = 0;                // Переменная  хранения длины введенной строки пароля администратора
char stLast[20]        ="";                // Данные в введенной строке строке.
//char stLast1[20]       ="";                // Данные в введенной строке строке.
int ret                = 0;                // Признак прерывания операции

//-----------------------------------------------------------------------------------------------------

//******************Назначение переменных для хранения № опций меню (клавиш)****************************

 int but1, but2, but3, but4, but5, but6, but7, but8, but9, but10, butX, butY, but_m1, but_m2, but_m3, but_m4, but_m5, pressed_button;
// int kbut1, kbut2, kbut3, kbut4, kbut5, kbut6, kbut7, kbut8, kbut9, kbut0, kbut_save,kbut_clear, kbut_exit;
// int kbutA, kbutB, kbutC, kbutD, kbutE, kbutF;
 int m2 = 1; // Переменная номера меню


//=====================================================================================

 //+++++++++++++++++++++++++ Настройки осциллографа (Самописца) +++++++++++++++++++++++++++++++++++++++++++++++++++

 // Declare variables
char buf[12];
int x_osc,y_osc;
int Input = 0;
//byte Sample[320];
//byte OldSample[320];
int Sample[320];
int OldSample[320];
unsigned long LongFile = 0;
float StartSample = 0; 
float EndSample = 0;
int Max = 0;
int Min = 500;
int mode = 0;
int dTime = 1;
int tmode = 0;
int Trigger = 0;
int SampleSize = 0;
float SampleTime = 0;
int dgvh;
int hpos = 105; //set 0v on horizontal  grid
int vsens = 10; // vertical sensitivity чувствительность по вертикали
int port = 0;
// variables for DVM
int sum = 0;                    // sum of samples taken сумма проб, взятых
uint32_t count = 0;
uint32_t count1 = 0;
// Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
//-----------------------------------------------------------------------------------------------------------------




//***************** Назначение переменных для хранения текстов*****************************************************


char  txt_menu1_1[]          = "PE\x81\x86""CTPATOP";                                                       // "РЕГИСТРАТОР"
char  txt_menu1_2[]          = "CAMO\x89\x86""CE\x8C";                                                      // "САМОПИСЕЦ"
char  txt_menu1_3[]          = "PE\x81\x86""CT.+ CAMO\x89.";                                                // "РЕГИСТ. + САМОП."
char  txt_menu1_4[]          = "\x89O\x82K\x88\x94\x8D""EH\x86""E \x89K";                                   // "ПОДКЛЮЧЕНИЕ ПК"
char  txt_menu2_1[]          = "MENU 2-1";//"\x86H\x8BO C\x8D""ET\x8D\x86KOB";                              // ИНФО СЧЕТЧИКОВ
char  txt_menu2_2[]          = "MENU 2-2";//"\x86H\x8BO N \xA3o\xA0\xAC\x9C.";                              //
char  txt_menu2_3[]          = "MENU 2-3";//                                                   //
char  txt_menu2_4[]          = "MENU 2-4";//                                                    //
char  txt_menu3_1[]          = "MENU 3-1";//"CTEPET\x92 \x8B""A\x87\x89\x91";                               //
char  txt_menu3_2[]          = "MENU 3-2";//"\x8A""c\xA4.N ""\xA4""e\xA0""e\xA5o\xA2""a";                   // Уст. № телефона
char  txt_menu3_3[]          = "MENU 3-3";//"\x8A""c\xA4.Level Gaz";                                        //
char  txt_menu3_4[]          = "MENU 3-4";//"\x8A""c\xA4.Level Temp";                                       //
char  txt_menu4_1[]          = "MENU 4-1";//"C\x96poc \x99""a""\xA2\xA2\xABx";                              // Сброс данных
char  txt_menu4_2[]          = "MENU 4-2";//"\x8A""c\xA4.N \xA3o\xA0\xAC\x9C.";                             // Уст. № польз
char  txt_menu4_3[]          = "MENU 4-3";//"\x89""apo\xA0\xAC \xA3o\xA0\xAC\x9C.";                         // Пароль польз.
char  txt_menu4_4[]          = "MENU 4-4";//"\x89""apo\xA0\xAC a\x99\xA1\x9D\xA2.";                         // Пароль админ.
char  txt_menu5_1[]          = "MENU 5-1";//"\x86H\x8BO ZigBee";                                            // Инфо ZigBee
char  txt_menu5_2[]          = "MENU 5-2";//"";                                              //
char  txt_menu5_3[]          = "MENU 5-3";//"Set Adr Coord L";                                              // 
char  txt_menu5_4[]          = "MENU 5-4";//"Set Adr Network";                                              // 


char  txt_info1[]            = "B""\x97""o""\x99"" ""\x99""a""\xA2\xA2\xAB""x";                             // Ввод данных
char  txt_info2[]            = "\x86\xA2\xA5op\xA1""a""\xA6\x9D\xAF";                                       // Информация
char  txt_info3[]            = "Hac\xA4po\x9E\x9F""a c\x9D""c\xA4""e\xA1\xAB";                              // Настройка системы
char  txt_info4[]            = "\x8A""c\xA4""a\xA2o\x97\x9F\x9D c\x9D""c\xA4""e\xA1\xAB";                   // 
char  txt_info5[]            = "\x86\xA2\xA5op\xA1""a""\xA6\x9D\xAF ZigBee";                                // Информация ZigBee
char  txt_return[]           = "\x85""a\x97""ep\xA8\xA2\xA4\xAC \xA3poc\xA1o\xA4p";            // Завершить просмотр

char  txt_ADC_menu1[]        = "Record data";                                      //
char  txt_ADC_menu2[]        = "Convert to CSV";                                             //
char  txt_ADC_menu3[]        = "Dump to Serial";                                  //
char  txt_ADC_menu4[]        = "EXIT";                                  //
char  txt_osc_menu1[]        = "Oscilloscope";                                      //
char  txt_osc_menu2[]        = "View File";                                             //
char  txt_osc_menu3[]        = "Menu 3";                                  //
char  txt_osc_menu4[]        = "EXIT";           


char  txt_info6[]            = "Info: ";                                //Info: 
char  txt_info7[]            = "Writing:"; 
char  txt_info8[]            = "%"; 
char  txt_info9[]            = "Done: "; 
char  txt_info10[]            = "Seconds"; 
char  txt_info11[]            = "ESC->PUSH Display"; 
char  txt_info12[]            = "START"; 
char  txt_info13[]            = "Deleting tmp file"; 
char  txt_info14[]            = "Erasing all data"; 
char  txt_info15[]            = "Stop->PUSH Display"; 
char  txt_info16[]            = "File: "; 
char  txt_info17[]            = "Max block :"; 
char  txt_info18[]            = "Record time: "; 
char  txt_info19[]            = "Sam count:"; 
char  txt_info20[]            = "Samples/sec: "; 
char  txt_info21[]            = "Overruns:"; 
char  txt_info22[]            = "Sample pins:"; 
char  txt_info23[]            = "ADC bits:"; 
char  txt_info24[]            = "ADC clock kHz:"; 
char  txt_info25[]            = "Sample Rate:"; 
char  txt_info26[]            = "Sample interval:"; 
char  txt_info27[]            = "Creating new file"; 
char  txt_info28[]            = "Start record"; 
char  txt_info29[]            = ""; 
char  txt_info30[]            = ""; 


void dateTime(uint16_t* date, uint16_t* time) // Программа записи времени и даты файла
{
  DateTime now = RTC.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}




//------------------------------------------------------------------------------
// Analog pin number list for a sample.  Pins may be in any order and pin
// numbers may be repeated.
const uint8_t PIN_LIST[] = {0};
//const uint8_t PIN_LIST[] = {0, 1, 2, 3};
//------------------------------------------------------------------------------
// Sample rate in samples per second.
const float SAMPLE_RATE = 10000;  // Must be 0.25 or greater.

// The interval between samples in seconds, SAMPLE_INTERVAL, may be set to a
// constant instead of being calculated from SAMPLE_RATE.  SAMPLE_RATE is not
// used in the code below.  For example, setting SAMPLE_INTERVAL = 2.0e-4
// will result in a 200 microsecond sample interval.
const float SAMPLE_INTERVAL = 1.0/SAMPLE_RATE;

// Setting ROUND_SAMPLE_INTERVAL non-zero will cause the sample interval to
// be rounded to a a multiple of the ADC clock period and will reduce sample
// time jitter.
#define ROUND_SAMPLE_INTERVAL 1
//------------------------------------------------------------------------------
// ADC clock rate.
// The ADC clock rate is normally calculated from the pin count and sample
// interval.  The calculation attempts to use the lowest possible ADC clock
// rate.
//
// You can select an ADC clock rate by defining the symbol ADC_PRESCALER to
// one of these values.  You must choose an appropriate ADC clock rate for
// your sample interval. 
// #define ADC_PRESCALER 7 // F_CPU/128 125 kHz on an Uno
// #define ADC_PRESCALER 6 // F_CPU/64  250 kHz on an Uno
// #define ADC_PRESCALER 5 // F_CPU/32  500 kHz on an Uno
// #define ADC_PRESCALER 4 // F_CPU/16 1000 kHz on an Uno
// #define ADC_PRESCALER 3 // F_CPU/8  2000 kHz on an Uno (8-bit mode only)
//------------------------------------------------------------------------------
// Reference voltage.  See the processor data-sheet for reference details.
// uint8_t const ADC_REF = 0; // External Reference AREF pin.
uint8_t const ADC_REF = (1 << REFS0);  // Vcc Reference.
// uint8_t const ADC_REF = (1 << REFS1);  // Internal 1.1 (only 644 1284P Mega)
// uint8_t const ADC_REF = (1 << REFS1) | (1 << REFS0);  // Internal 1.1 or 2.56
//------------------------------------------------------------------------------
// File definitions.
//
// Maximum file size in blocks.
// The program creates a contiguous file with FILE_BLOCK_COUNT 512 byte blocks.
// This file is flash erased using special SD commands.  The file will be
// truncated if logging is stopped early.
const uint32_t FILE_BLOCK_COUNT = 256000;

// log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "ANALOG"

// Set RECORD_EIGHT_BITS non-zero to record only the high 8-bits of the ADC.
#define RECORD_EIGHT_BITS 0
//------------------------------------------------------------------------------
// Pin definitions.
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 13;

// SD chip select pin.
const uint8_t SD_CS_PIN = SS;
//------------------------------------------------------------------------------
// Buffer definitions.
//
// The logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional 
// buffers.  QUEUE_DIM must be a power of two larger than
//(BUFFER_BLOCK_COUNT + 1).
//
//#if RAMEND < 0X8FF
//#error Too little SRAM
////
//#elif RAMEND < 0X10FF
//// Use total of two 512 byte buffers.
//const uint8_t BUFFER_BLOCK_COUNT = 1;
//// Dimension for queues of 512 byte SD blocks.
//const uint8_t QUEUE_DIM = 4;  // Must be a power of two!
////
//#elif RAMEND < 0X20FF
//// Use total of five 512 byte buffers.
//const uint8_t BUFFER_BLOCK_COUNT = 4;
//// Dimension for queues of 512 byte SD blocks.
//const uint8_t QUEUE_DIM = 8;  // Must be a power of two!
////
//#elif RAMEND < 0X40FF
//// Use total of 13 512 byte buffers.
//const uint8_t BUFFER_BLOCK_COUNT = 12;
//// Dimension for queues of 512 byte SD blocks.
//const uint8_t QUEUE_DIM = 16;  // Must be a power of two!
////
//#else  // RAMEND
//// Use total of 29 512 byte buffers.
//const uint8_t BUFFER_BLOCK_COUNT = 28;
//// Dimension for queues of 512 byte SD blocks.
//const uint8_t QUEUE_DIM = 32;  // Must be a power of two!
//#endif  // RAMEND


// Use total of five 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 4;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 8;  // Must be a power of two!








//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if a reset or power failure occurs.
#define TMP_FILE_NAME "TMP_LOG.BIN"

// Size of file base name.  Must not be larger than six.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

// Number of analog pins to log.
const uint8_t PIN_COUNT = sizeof(PIN_LIST)/sizeof(PIN_LIST[0]);

// Minimum ADC clock cycles per sample interval
const uint16_t MIN_ADC_CYCLES = 15;

// Extra cpu cycles to setup ADC with more than one pin per sample.
const uint16_t ISR_SETUP_ADC = 100;

// Maximum cycles for timer0 system interrupt, millis, micros.
const uint16_t ISR_TIMER0 = 160;
//==============================================================================
SdFat sd;

SdBaseFile binFile;

char binName[13] = FILE_BASE_NAME "00.BIN";

#if RECORD_EIGHT_BITS
const size_t SAMPLES_PER_BLOCK = DATA_DIM8/PIN_COUNT;
typedef block8_t block_t;
#else  // RECORD_EIGHT_BITS
const size_t SAMPLES_PER_BLOCK = DATA_DIM16/PIN_COUNT;
typedef block16_t block_t;
#endif // RECORD_EIGHT_BITS

block_t* emptyQueue[QUEUE_DIM];
uint8_t emptyHead;
uint8_t emptyTail;

block_t* fullQueue[QUEUE_DIM];
volatile uint8_t fullHead;  // volatile insures non-interrupt code sees changes.
uint8_t fullTail;

// queueNext assumes QUEUE_DIM is a power of two
inline uint8_t queueNext(uint8_t ht) {return (ht + 1) & (QUEUE_DIM -1);}
//==============================================================================
// Interrupt Service Routines

// Pointer to current buffer.
block_t* isrBuf;

// Need new buffer if true.
volatile bool isrBufNeeded = true;

// overrun count
volatile uint16_t isrOver = 0;

// ADC configuration for each pin.
volatile uint8_t adcmux[PIN_COUNT];
volatile uint8_t adcsra[PIN_COUNT];
volatile uint8_t adcsrb[PIN_COUNT];
volatile uint8_t adcindex = 1;

// Insure no timer events are missed.
volatile bool timerError = false;
volatile bool timerFlag = false;
//------------------------------------------------------------------------------
// ADC done interrupt.
ISR(ADC_vect) 
{
  // Read ADC data.
#if RECORD_EIGHT_BITS
  uint8_t d = ADCH;
#else  // RECORD_EIGHT_BITS
  // This will access ADCL first. 
  uint16_t d = ADC;
#endif  // RECORD_EIGHT_BITS

  if (isrBufNeeded && emptyHead == emptyTail) 
	{
		// no buffers - count overrun нет буферов - рассчитывайте перерасход памяти
		if (isrOver < 0XFFFF) isrOver++;
		// Avoid missed timer error.
		timerFlag = false;
		return;
	}

  // Start ADC
  if (PIN_COUNT > 1) 
	{
		ADMUX  = adcmux[adcindex];
		ADCSRB = adcsrb[adcindex];
		ADCSRA = adcsra[adcindex];
		if (adcindex == 0) timerFlag = false;
		adcindex =  adcindex < (PIN_COUNT - 1) ? adcindex + 1 : 0;
	}
  else 
	{
		timerFlag = false;
	}
  // Check for buffer needed. Необходимо проверить буфер
  if (isrBufNeeded) 
	{   
		// Remove buffer from empty queue.  Удалить буфер из пустого очереди.
		isrBuf = emptyQueue[emptyTail];
		emptyTail = queueNext(emptyTail);
		isrBuf->count = 0;
		isrBuf->overrun = isrOver;
		isrBufNeeded = false;    
	}
  // Store ADC data.
  isrBuf->data[isrBuf->count++] = d;

  // Check for buffer full. Проверка, буфер полон?
  if (isrBuf->count >= PIN_COUNT*SAMPLES_PER_BLOCK) 
	{
		// Put buffer isrIn full queue.   Положите буфер isrIn полной очереди
		uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.  Избежать дополнительных
		fullQueue[tmp] = (block_t*)isrBuf;
		fullHead = queueNext(tmp);
		// Set buffer needed and clear overruns. Установить необходимый буффер и очистить перерасход
		isrBufNeeded = true;
		isrOver = 0;
	}
}
//------------------------------------------------------------------------------
// timer1 interrupt to clear OCF1B
ISR(TIMER1_COMPB_vect) 
{
  // Make sure ADC ISR responded to timer event.
  if (timerFlag) timerError = true;
  timerFlag = true;
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) error_P(PSTR(msg))
//------------------------------------------------------------------------------
void error_P(const char* msg) 
{
  sd.errorPrint_P(msg);
  fatalBlink();
}
//------------------------------------------------------------------------------
//
void fatalBlink() {
  while (true) {
	if (ERROR_LED_PIN >= 0) {
	  digitalWrite(ERROR_LED_PIN, HIGH);
	  delay(200);
	  digitalWrite(ERROR_LED_PIN, LOW);
	  delay(200);
	}
  }
}
//==============================================================================
#if ADPS0 != 0 || ADPS1 != 1 || ADPS2 != 2
#error unexpected ADC prescaler bits
#endif
//------------------------------------------------------------------------------
// initialize ADC and timer1

void adcInit(metadata_t* meta) 
{
  uint8_t adps;  // prescaler bits for ADCSRA 
  uint32_t ticks = F_CPU*SAMPLE_INTERVAL + 0.5;  // Sample interval cpu cycles.

  if (ADC_REF & ~((1 << REFS0) | (1 << REFS1))) 
	  {
		error("Invalid ADC reference");
	  }
#ifdef ADC_PRESCALER
  if (ADC_PRESCALER > 7 || ADC_PRESCALER < 2) 
	  {
		error("Invalid ADC prescaler");
	  }
  adps = ADC_PRESCALER;
#else  // ADC_PRESCALER
  // Allow extra cpu cycles to change ADC settings if more than one pin.
  int32_t adcCycles = (ticks - ISR_TIMER0)/PIN_COUNT;
//					  - (PIN_COUNT > 1 ? ISR_SETUP_ADC : 0); // Уточнить для чего
					  
  for (adps = 7; adps > 0; adps--) 
	  {
		 if (adcCycles >= (MIN_ADC_CYCLES << adps)) break;
	  }
#endif  // ADC_PRESCALER
  meta->adcFrequency = F_CPU >> adps;
  if (meta->adcFrequency > (RECORD_EIGHT_BITS ? 2000000 : 1000000)) {
	error("Sample Rate Too High");
  }
#if ROUND_SAMPLE_INTERVAL
  // Round so interval is multiple of ADC clock.
  ticks += 1 << (adps - 1);
  ticks >>= adps;
  ticks <<= adps;
#endif  // ROUND_SAMPLE_INTERVAL

  if (PIN_COUNT > sizeof(meta->pinNumber)/sizeof(meta->pinNumber[0])) {
	error("Too many pins");
  }
  meta->pinCount = PIN_COUNT;
  meta->recordEightBits = RECORD_EIGHT_BITS;
  
  for (int i = 0; i < PIN_COUNT; i++) {
	uint8_t pin = PIN_LIST[i];
	if (pin >= NUM_ANALOG_INPUTS) error("Invalid Analog pin number");
	meta->pinNumber[i] = pin;
	
   // Set ADC reference and low three bits of analog pin number.   
	adcmux[i] = (pin & 7) | ADC_REF;
	if (RECORD_EIGHT_BITS) adcmux[i] |= 1 << ADLAR;
	
	// If this is the first pin, trigger on timer/counter 1 compare match B.
	adcsrb[i] = i == 0 ? (1 << ADTS2) | (1 << ADTS0) : 0;
#ifdef MUX5
	if (pin > 7) adcsrb[i] |= (1 << MUX5);
#endif  // MUX5
	adcsra[i] = (1 << ADEN) | (1 << ADIE) | adps;
	adcsra[i] |= i == 0 ? 1 << ADATE : 1 << ADSC;
  }
   Serial.flush();
  // Setup timer1
  TCCR1A = 0;
  uint8_t tshift;
  if (ticks < 0X10000) {
	// no prescale, CTC mode
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
	tshift = 0;
  } else if (ticks < 0X10000*8) {
	// prescale 8, CTC mode
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
	tshift = 3;
  } else if (ticks < 0X10000*64) {
	// prescale 64, CTC mode
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
	tshift = 6;
  } else if (ticks < 0X10000*256) {
	// prescale 256, CTC mode
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12);
	tshift = 8;
  } else if (ticks < 0X10000*1024) {
	// prescale 1024, CTC mode
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) | (1 << CS10);
	tshift = 10;
  } else {
	error("Sample Rate Too Slow");
  }
  // divide by prescaler
  ticks >>= tshift;
  // set TOP for timer reset
  ICR1 = ticks - 1;
  // compare for ADC start
  OCR1B = 0;
  
  // multiply by prescaler
  ticks <<= tshift;
  
  // Sample interval in CPU clock ticks.
  meta->sampleInterval = ticks;
  meta->cpuFrequency = F_CPU;
  float sampleRate = (float)meta->cpuFrequency/meta->sampleInterval;
  Serial.print(F("Sample pins:"));
  myGLCD.print(txt_info22,LEFT, 25);
  myGLCD.setColor(VGA_YELLOW);
  for (unsigned int i = 0; i < meta->pinCount; i++) 
  {
	Serial.print(' ');
	Serial.print(meta->pinNumber[i], DEC);
	myGLCD.printNumI(meta->pinNumber[i], 230+(20*i), 25);// 
  }
 
  Serial.println(); 
  Serial.print(F("ADC bits: "));
  Serial.println(meta->recordEightBits ? 8 : 10);
  Serial.print(F("ADC clock kHz: "));
  Serial.println(meta->adcFrequency/1000);
  Serial.print(F("Sample Rate: "));
  Serial.println(sampleRate);  
  Serial.print(F("Sample interval usec: "));
  Serial.println(1000000.0/sampleRate, 4); 
  Serial.flush();
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(txt_info23,LEFT, 45);
  myGLCD.setColor(VGA_YELLOW);
  myGLCD.printNumI(meta->recordEightBits ? 8 : 10, RIGHT, 45);// 
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(txt_info24,LEFT, 65);
  myGLCD.setColor(VGA_YELLOW);
  myGLCD.printNumI(meta->adcFrequency/1000, RIGHT, 65);// 
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(txt_info25,LEFT, 85);
  myGLCD.setColor(VGA_YELLOW);
  myGLCD.printNumI(sampleRate, RIGHT, 85);// 
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(txt_info26,LEFT, 105);
  myGLCD.setColor(VGA_YELLOW);
  myGLCD.printNumI(1000000.0/sampleRate, RIGHT, 105);// 
  myGLCD.setColor(255, 255, 255);

 }
//------------------------------------------------------------------------------
// enable ADC and timer1 interrupts
void adcStart() 
{
  // initialize ISR
  isrBufNeeded = true;
  isrOver = 0;
  adcindex = 1;

  // Clear any pending interrupt.
  ADCSRA |= 1 << ADIF;
  
  // Setup for first pin.
  ADMUX = adcmux[0];
  ADCSRB = adcsrb[0];
  ADCSRA = adcsra[0];

  // Enable timer1 interrupts.
  timerError = false;
  timerFlag = false;
  TCNT1 = 0;
  TIFR1 = 1 << OCF1B;
  TIMSK1 = 1 << OCIE1B;
}
//------------------------------------------------------------------------------
void adcStop() {
  TIMSK1 = 0;
  ADCSRA = 0;
}
//------------------------------------------------------------------------------
// Convert binary file to CSV file.
void binaryToCsv() 
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info6,CENTER, 5);//
	uint8_t lastPct = 0;
	block_t buf;
	metadata_t* pm;
	uint32_t t0 = millis();
	char csvName[13];
	StdioStream csvStream;
  
	if (!binFile.isOpen()) {
	Serial.println(F("No current binary file"));
	return;
	}
	binFile.rewind();
	if (!binFile.read(&buf , 512) == 512) error("Read metadata failed");
	// Create a new CSV file.
	strcpy(csvName, binName);
	strcpy_P(&csvName[BASE_NAME_SIZE + 3], PSTR("CSV"));

	if (!csvStream.fopen(csvName, "w")) {
	error("open csvStream failed");  
	}
	Serial.println();
	Serial.print(F("Writing: "));
	Serial.print(csvName);
	Serial.println(F(" - type any character to stop"));
//	Serial.flush();
	myGLCD.print(txt_info7,LEFT, 25);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(csvName,RIGHT, 25);// 
	myGLCD.setColor(255, 255, 255);
	
	pm = (metadata_t*)&buf;
	csvStream.print(F("Interval,"));
	float intervalMicros = 1.0e6*pm->sampleInterval/(float)pm->cpuFrequency;
	csvStream.print(intervalMicros, 4);
	csvStream.println(F(",usec"));
	for (uint8_t i = 0; i < pm->pinCount; i++) {
	if (i) csvStream.putc(',');
	csvStream.print(F("pin"));
	csvStream.print(pm->pinNumber[i]);
	}
	csvStream.println(); 
	uint32_t tPct = millis();
	while (!Serial.available() && binFile.read(&buf, 512) == 512) 
	{
	//	uint16_t i;
		if (buf.count == 0) break;
		if (buf.overrun) 
			{
				csvStream.print(F("OVERRUN,"));
				csvStream.println(buf.overrun);     
			}
		for (uint16_t j = 0; j < buf.count; j += PIN_COUNT) 
			{
				for (uint16_t i = 0; i < PIN_COUNT; i++) 
					{
						if (i) csvStream.putc(',');
						csvStream.print(buf.data[i + j]);     
					}
				csvStream.println();
			}
		if ((millis() - tPct) > 1000) 
			{
				uint8_t pct = binFile.curPosition()/(binFile.fileSize()/100);
				if (pct != lastPct) 
					{
						tPct = millis();
						lastPct = pct;
						Serial.print(pct, DEC);
						Serial.println('%');

						myGLCD.setColor(VGA_YELLOW);
						myGLCD.printNumI(pct, 5, 75);// 
						myGLCD.print(txt_info8,40, 75);//
						myGLCD.setColor(255, 255, 255);

					}
			}
		if (Serial.available()) break;
	}
	csvStream.fclose();  
	Serial.print(F("Done: "));
	Serial.print(0.001*(millis() - t0));
	Serial.println(F(" Seconds"));
	Serial.flush();
	myGLCD.print(txt_info9,5, 110);//
	myGLCD.printNumF((0.001*(millis() - t0)),2, 85, 110);// 
	myGLCD.print(txt_info10, 210, 110);//
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11, CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	delay(2000);
	Draw_menu_ADC1();
}
//------------------------------------------------------------------------------
// read data file and check for overruns
void checkOverrun() {
  bool headerPrinted = false;
  block_t buf;
  uint32_t bgnBlock, endBlock;
  uint32_t bn = 0;
  
  if (!binFile.isOpen()) {
	Serial.println(F("No current binary file"));
	return;
  }
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
	error("contiguousRange failed");
  }
  binFile.rewind();
  Serial.println();
  Serial.println(F("Checking overrun errors - type any character to stop"));
  if (!binFile.read(&buf , 512) == 512) {
	error("Read metadata failed");
  }
  bn++;
  while (binFile.read(&buf, 512) == 512) {
	if (buf.count == 0) break;
	if (buf.overrun) {
	  if (!headerPrinted) {
		Serial.println();
		Serial.println(F("Overruns:"));
		Serial.println(F("fileBlockNumber,sdBlockNumber,overrunCount"));
		headerPrinted = true;
	  }
	  Serial.print(bn);
	  Serial.print(',');
	  Serial.print(bgnBlock + bn);
	  Serial.print(',');
	  Serial.println(buf.overrun);
	}
	bn++;
  }
  if (!headerPrinted) {
	Serial.println(F("No errors found"));
  } else {
	Serial.println(F("Done"));
  }
}
//------------------------------------------------------------------------------
// dump data file to Serial
void dumpData() 
{
	Serial.println();
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info12,CENTER, 40);
	block_t buf;
	if (!binFile.isOpen()) 
		{
			Serial.println(F("No current binary file"));
			return;
		}
	binFile.rewind();
	if (binFile.read(&buf , 512) != 512) 
	{
	error("Read metadata failed");
	}
	Serial.println();
	//Serial.println(F("Type any character to stop"));
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info15,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	delay(1000);
	while (!myTouch.dataAvailable() && binFile.read(&buf , 512) == 512) 
	{
		//waitForIt(1, 1, 319, 239);
		if (buf.count == 0) break;
		if (buf.overrun) 
			{
				Serial.print(F("OVERRUN,"));
				Serial.println(buf.overrun);
			}
		for (uint16_t i = 0; i < buf.count; i++) 
		{
			Serial.print(buf.data[i], DEC);
			if ((i+1)%PIN_COUNT) 
				{
					Serial.print(',');
				}
			else 
			{
				Serial.println();
			}
		}
	}
	Serial.println(F("Done"));
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(txt_info9,CENTER, 80);
	myGLCD.setColor(255, 255, 255);
	delay(500);
	while (myTouch.dataAvailable()){}
	Draw_menu_ADC1();
}
//------------------------------------------------------------------------------
void dumpData_Osc() 
{
	Serial.println();
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info12,CENTER, 40);
	block_t buf;
	count1 = 0;
	int xpos = 0;
	if (!binFile.isOpen()) 
		{
			Serial.println(F("No current binary file"));
			return;
		}
	binFile.rewind();
	if (binFile.read(&buf , 512) != 512) 
	{
		error("Read metadata failed");
	}
	Serial.println();
	//Serial.println(F("Type any character to stop"));
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info15,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	//DrawGrid();
	delay(1000);
	myGLCD.clrScr();
	LongFile = 0;
	DrawGrid();
	/*int Long_Screen = (count/PIN_COUNT)/240;
	float Long_Screen1 = ((count/PIN_COUNT)/240)/240;*/

	while (!myTouch.dataAvailable() && binFile.read(&buf , 512) == 512) 
	{
		//myGLCD.clrScr();
		//waitForIt(1, 1, 319, 239);
		if (buf.count == 0) break;
		if (buf.overrun) 
			{
				Serial.print(F("OVERRUN,"));
				Serial.println(buf.overrun);
			}
		  
		for (uint16_t i = 0; i < buf.count; i++) 
		{
				Sample[xpos] = buf.data[i]*5/100;
				xpos++;
			if(xpos == 240)
				{
					for (int xpos1 = 0; xpos1<240; xpos1++)
						{
							myGLCD.setColor( 0, 0, 0);       		// Erase previous display Стереть предыдущий экран
							myGLCD.drawLine (xpos1 + 1, 255-OldSample[ xpos1 + 1]* vsens-hpos, xpos1 + 2, 255-OldSample[ xpos1 + 2]* vsens-hpos);
							if (xpos1 == 0) myGLCD.drawLine (xpos1 + 1, 1, xpos1 + 1, 239);
							myGLCD.setColor( 255, 255, 255);  	//Draw the new data
							myGLCD.drawLine (xpos1, 255-Sample[ xpos1]* vsens-hpos, xpos1 + 1, 255-Sample[ xpos1 + 1]* vsens-hpos);
							OldSample[xpos1] = Sample[xpos1];
							
						}
			
					xpos = 0;
					DrawGrid();
					count1++;
					myGLCD.printNumI(count/PIN_COUNT, RIGHT, 220);// 
					myGLCD.setColor(VGA_LIME);
					myGLCD.printNumI(count1*240, LEFT, 220);// 
				}

			if ((i+1)%PIN_COUNT) 
				{
			//		Serial.print(',');
				}
			else 
				{
			//		Serial.println();
				}
		}
	}
	Serial.println(F("Done"));
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(txt_info9,CENTER, 80);
	myGLCD.setColor(255, 255, 255);
	delay(500);
	while (myTouch.dataAvailable()){}
	Draw_menu_ADC1();
}
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;

void logData() 
{
	uint32_t bgnBlock, endBlock;
	// Allocate extra buffer space.
	block_t block[BUFFER_BLOCK_COUNT];
  
	Serial.println();
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info12, CENTER, 2);

	// Initialize ADC and timer1.
	adcInit((metadata_t*) &block[0]);
  
	// Find unused file name.
	if (BASE_NAME_SIZE > 6)  // Проверка длины базовой части имени файла
		{
			error("FILE_BASE_NAME too long");
		}
	while (sd.exists(binName)) 
	 {
		if (binName[BASE_NAME_SIZE + 1] != '9') 
		  {
				binName[BASE_NAME_SIZE + 1]++;
		  }
		else 
		 {
			binName[BASE_NAME_SIZE + 1] = '0';
			if (binName[BASE_NAME_SIZE] == '9') 
				{
					error("Can't create file name");
				}
			binName[BASE_NAME_SIZE]++;
		 }
	 }
	// Delete old tmp file.
	if (sd.exists(TMP_FILE_NAME)) 
	 {
		Serial.println(F("Deleting tmp file"));
		myGLCD.print(txt_info13,LEFT, 135);//
		if (!sd.remove(TMP_FILE_NAME)) 
			{
				error("Can't remove tmp file");
			}
	 }
	// Create new file.
	Serial.println(F("Creating new file"));
	myGLCD.print(txt_info27,LEFT, 155);//"Сброс"
	binFile.close();
	if (!binFile.createContiguous(sd.vwd(),	TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT)) 
		{
	    	error("createContiguous failed");
		}
	// Get the address of the file on the SD.
	if (!binFile.contiguousRange(&bgnBlock, &endBlock)) 
		{
    		error("contiguousRange failed");
		}
	// Use SdFat's internal buffer.
	uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
	if (cache == 0) error("cacheClear failed"); 
 
	// Flash erase all data in the file.
	Serial.println(F("Erasing all data"));
	myGLCD.print(txt_info14,LEFT, 175);//"Сброс"
	uint32_t bgnErase = bgnBlock;
	uint32_t endErase;
	while (bgnErase < endBlock)
	{
		endErase = bgnErase + ERASE_SIZE;
		if (endErase > endBlock) endErase = endBlock;
		if (!sd.card()->erase(bgnErase, endErase))
			{
				error("erase failed");
			}
		bgnErase = endErase + 1;
	}
	// Start a multiple block write.
	if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) 
		{
    		error("writeBegin failed");
		}
	// Write metadata.
	if (!sd.card()->writeData((uint8_t*)&block[0])) 
		{
		error("Write metadata failed");
		} 
	// Initialize queues.  Инициализация очереди.
	emptyHead = emptyTail = 0;
	fullHead = fullTail = 0;
  
	// Use SdFat buffer for one block.  Используйте SdFat буфер для одного блока
	emptyQueue[emptyHead] = (block_t*)cache;
	emptyHead = queueNext(emptyHead);
  
	// Put rest of buffers in the empty queue.
	for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) 
		{
			emptyQueue[emptyHead] = &block[i];
			emptyHead = queueNext(emptyHead);
		}
	// Give SD time to prepare for big write.
	delay(100);
	Serial.println(F("Logging - type any character to stop"));
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info15, CENTER, 200);
	// Wait for Serial Idle.
	Serial.flush();
	delay(10);
	uint32_t bn = 1;
	uint32_t t0 = millis();
	uint32_t t1 = t0;
	uint32_t overruns = 0;
	//uint32_t count = 0;
	uint32_t maxLatency = 0;
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(txt_info28, CENTER, 135);
	myGLCD.setColor(255, 255, 255);
	// Start logging interrupts.



	adcStart();
	while (1) 
		{

		if (fullHead != fullTail) 
			{
				// Get address of block to write.  Получить адрес блока, чтобы написать
				block_t* pBlock = fullQueue[fullTail];
	  
				// Write block to SD. Написать блок SD
				uint32_t usec = micros();
				if (!sd.card()->writeData((uint8_t*)pBlock)) 
					{
	     				error("write data failed");
					}
				usec = micros() - usec;
				t1 = millis();
				if (usec > maxLatency) maxLatency = usec;
				count += pBlock->count;
	  
				// Add overruns and possibly light LED. 
				if (pBlock->overrun) 
				{
					overruns += pBlock->overrun;
					if (ERROR_LED_PIN >= 0) 
						{
							digitalWrite(ERROR_LED_PIN, HIGH);
						}
				}
				// Move block to empty queue.
				emptyQueue[emptyHead] = pBlock;
				emptyHead = queueNext(emptyHead);
				fullTail = queueNext(fullTail);
				bn++;
				if (bn == FILE_BLOCK_COUNT) 
				{
				// File full so stop ISR calls.
				adcStop();
				break;
				}
			}
		if (timerError) 
			{
				error("Missed timer event - rate too high");
			}
		if (myTouch.dataAvailable())
		//if (Serial.available()) 
			{
				// Stop ISR calls.
				adcStop();
				if (isrBuf != 0 && isrBuf->count >= PIN_COUNT) {
				// Truncate to last complete sample.
				isrBuf->count = PIN_COUNT*(isrBuf->count/PIN_COUNT);
				// Put buffer in full queue.
				fullQueue[fullHead] = isrBuf;
				fullHead = queueNext(fullHead);
				isrBuf = 0;
				}
				if (fullHead == fullTail) break;
			}
		}
	if (!sd.card()->writeStop()) {
	error("writeStop failed");
	}
	// Truncate file if recording stopped early.
	if (bn != FILE_BLOCK_COUNT) {    
	Serial.println(F("Truncating file"));
	if (!binFile.truncate(512L * bn)) {
		error("Can't truncate file");
	}
	}
	if (!binFile.rename(sd.vwd(), binName)) {
		error("Can't rename file");
	}
	Serial.print(F("File renamed: "));
	Serial.println(binName);
	Serial.print(F("Max block write usec: "));
	Serial.println(maxLatency);
	Serial.print(F("Record time sec: "));
	Serial.println(0.001*(t1 - t0), 3);
	Serial.print(F("Sample count: "));
	Serial.println(count/PIN_COUNT);
	Serial.print(F("Samples/sec: "));
	Serial.println((1000.0/PIN_COUNT)*count/(t1-t0));
	Serial.print(F("Overruns: "));
	Serial.println(overruns);
	Serial.println(F("Done"));

	delay(100);
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info6,CENTER, 5);//
	myGLCD.print(txt_info16,LEFT, 25);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(binName,RIGHT , 25);
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info17,LEFT, 45);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumI(maxLatency, RIGHT, 45);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info18,LEFT, 65);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumF(0.001*(t1 - t0),2, RIGHT, 65);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info19,LEFT, 85);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumI(count/PIN_COUNT, RIGHT, 85);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info20,LEFT, 105);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumF((1000.0/PIN_COUNT)*count/(t1-t0),2, RIGHT, 105);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info21,LEFT, 125);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumI(overruns, RIGHT, 125);// 
	myGLCD.setColor(255, 255, 255);

	delay(500);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	while (!myTouch.dataAvailable()){}
	while (myTouch.dataAvailable()){}
	Draw_menu_ADC1();

}

void Draw_menu_ADC1()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 255);
	for (int x=0; x<4; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (30, 20+(50*x), 290,60+(50*x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+(50*x), 290,60+(50*x));
		}
	myGLCD.print( txt_ADC_menu1, CENTER, 30);     // "Record ADC data"
	myGLCD.print( txt_ADC_menu2, CENTER, 80);      // "Convert to CSV"
	myGLCD.print( txt_ADC_menu3, CENTER, 130);     // "Data to Serial"
	myGLCD.print( txt_ADC_menu4, CENTER, 180);      // "Error details"
}
void menu_ADC()
{
	// discard any input
	while (Serial.read() >= 0) {} // Удалить все символы из буфера
//	Draw_menu_ADC1();
	char c;

	Serial.println();
	Serial.println(F("type:"));
	Serial.println(F("c - convert file to CSV")); 
	Serial.println(F("d - dump data to Serial"));  
	Serial.println(F("e - overrun error details"));
	Serial.println(F("r - record ADC data"));



	while (true)
		{
		delay(10);
		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((x>=30) && (x<=290))       // Upper row
					{
					if ((y>=20) && (y<=60))    // Button: 1
						{
							waitForIt(30, 20, 290, 60);
							logData();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							binaryToCsv();
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
							 dumpData();
						}
					if ((y>=170) && (y<=220))  // Button: 4
						{
							waitForIt(30, 170, 290, 210);
							break;
						}
				}
			}
	   }
}

//************************** Аналоговые часы ****************************************************
int bcd2bin(int temp)//BCD  to decimal
{
	int a,b,c;
	a=temp;
	b=0;
	if(a>=16)
	{
		while(a>=16)
		{
			a=a-16;
			b=b+10;
			c=a+b;
			temp=c;
		}
	}
	return temp;
}
void clock_read()
{
		Wire.beginTransmission(0x68);//Send the address of DS1307
		Wire.write(0);//Sending address	
		Wire.endTransmission();//The end of the IIC communication
		Wire.requestFrom(0x68, 7);//IIC communication is started, you can continue to access another address (address auto increase) and the number of visits
		sec = bcd2bin(Wire.read());//read time
		min = bcd2bin(Wire.read());
		hour = bcd2bin(Wire.read());
		dow = Wire.read();
		date = bcd2bin(Wire.read());
		mon = bcd2bin(Wire.read());
		year = bcd2bin(Wire.read()) + 2000;
		delay(10);
	//  Wire.endTransmission();//The end of the IIC communication
}
void clock_print_serial()
{
	/*
	  Serial.print(date, DEC);
	  Serial.print('/');MLML
	  Serial.print(mon, DEC);
	  Serial.print('/');
	  Serial.print(year, DEC);//Serial display time
	  Serial.print(' ');
	  Serial.print(hour, DEC);
	  Serial.print(':');
	  Serial.print(min, DEC);
	  Serial.print(':');
	  Serial.print(sec, DEC);
	  Serial.println();
	  Serial.print(" week: ");
	  Serial.print(dow, DEC);
	  Serial.println();
	  */
}
void drawDisplay()
{
  // Clear screen
  myGLCD.clrScr();
  
  // Draw Clockface
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  for (int i=0; i<5; i++)
  {
	myGLCD.drawCircle(clockCenterX, clockCenterY, 119-i);
  }
  for (int i=0; i<5; i++)
  {
	myGLCD.drawCircle(clockCenterX, clockCenterY, i);
  }
  
  myGLCD.setColor(192, 192, 255);
  myGLCD.print("3", clockCenterX+92, clockCenterY-8);
  myGLCD.print("6", clockCenterX-8, clockCenterY+95);
  myGLCD.print("9", clockCenterX-109, clockCenterY-8);
  myGLCD.print("12", clockCenterX-16, clockCenterY-109);
  for (int i=0; i<12; i++)
  {
	if ((i % 3)!=0)
	  drawMark(i);
  }  
  clock_read();
  drawMin(min);
  drawHour(hour, min);
  drawSec(sec);
  oldsec=sec;

  // Draw calendar
  myGLCD.setColor(255, 255, 255);
  myGLCD.fillRoundRect(240, 0, 319, 85);
  myGLCD.setColor(0, 0, 0);
  for (int i=0; i<7; i++)
  {
	myGLCD.drawLine(249+(i*10), 0, 248+(i*10), 3);
	myGLCD.drawLine(250+(i*10), 0, 249+(i*10), 3);
	myGLCD.drawLine(251+(i*10), 0, 250+(i*10), 3);
  }

  // Draw SET button
  myGLCD.setColor(64, 64, 128);
  myGLCD.fillRoundRect(260, 200, 319, 239);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(260, 200, 319, 239);
  myGLCD.setBackColor(64, 64, 128);
  myGLCD.print("SET", 266, 212);
  myGLCD.setBackColor(0, 0, 0);
  
  myGLCD.setColor(64, 64, 128);
  myGLCD.fillRoundRect(260, 140, 319, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(260, 140, 319, 180);
  myGLCD.setBackColor(64, 64, 128);
  myGLCD.print("RET", 266, 150);
  myGLCD.setBackColor(0, 0, 0);

}
void drawMark(int h)
{
  float x1, y1, x2, y2;
  
  h=h*30;
  h=h+270;
  
  x1=110*cos(h*0.0175);
  y1=110*sin(h*0.0175);
  x2=100*cos(h*0.0175);
  y2=100*sin(h*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x2+clockCenterX, y2+clockCenterY);
}
void drawSec(int s)
{
  float x1, y1, x2, y2;
  int ps = s-1;
  
  myGLCD.setColor(0, 0, 0);
  if (ps==-1)
  ps=59;
  ps=ps*6;
  ps=ps+270;
  
  x1=95*cos(ps*0.0175);
  y1=95*sin(ps*0.0175);
  x2=80*cos(ps*0.0175);
  y2=80*sin(ps*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x2+clockCenterX, y2+clockCenterY);

  myGLCD.setColor(255, 0, 0);
  s=s*6;
  s=s+270;
  
  x1=95*cos(s*0.0175);
  y1=95*sin(s*0.0175);
  x2=80*cos(s*0.0175);
  y2=80*sin(s*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x2+clockCenterX, y2+clockCenterY);
}
void drawMin(int m)
{
  float x1, y1, x2, y2, x3, y3, x4, y4;
  int pm = m-1;
  
  myGLCD.setColor(0, 0, 0);
  if (pm==-1)
  pm=59;
  pm=pm*6;
  pm=pm+270;
  
  x1=80*cos(pm*0.0175);
  y1=80*sin(pm*0.0175);
  x2=5*cos(pm*0.0175);
  y2=5*sin(pm*0.0175);
  x3=30*cos((pm+4)*0.0175);
  y3=30*sin((pm+4)*0.0175);
  x4=30*cos((pm-4)*0.0175);
  y4=30*sin((pm-4)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);

  myGLCD.setColor(0, 255, 0);
  m=m*6;
  m=m+270;
  
  x1=80*cos(m*0.0175);
  y1=80*sin(m*0.0175);
  x2=5*cos(m*0.0175);
  y2=5*sin(m*0.0175);
  x3=30*cos((m+4)*0.0175);
  y3=30*sin((m+4)*0.0175);
  x4=30*cos((m-4)*0.0175);
  y4=30*sin((m-4)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);
}
void drawHour(int h, int m)
{
  float x1, y1, x2, y2, x3, y3, x4, y4;
  int ph = h;
  
  myGLCD.setColor(0, 0, 0);
  if (m==0)
  {
	ph=((ph-1)*30)+((m+59)/2);
  }
  else
  {
	ph=(ph*30)+((m-1)/2);
  }
  ph=ph+270;
  
  x1=60*cos(ph*0.0175);
  y1=60*sin(ph*0.0175);
  x2=5*cos(ph*0.0175);
  y2=5*sin(ph*0.0175);
  x3=20*cos((ph+5)*0.0175);
  y3=20*sin((ph+5)*0.0175);
  x4=20*cos((ph-5)*0.0175);
  y4=20*sin((ph-5)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);

  myGLCD.setColor(255, 255, 0);
  h=(h*30)+(m/2);
  h=h+270;
  
  x1=60*cos(h*0.0175);
  y1=60*sin(h*0.0175);
  x2=5*cos(h*0.0175);
  y2=5*sin(h*0.0175);
  x3=20*cos((h+5)*0.0175);
  y3=20*sin((h+5)*0.0175);
  x4=20*cos((h-5)*0.0175);
  y4=20*sin((h-5)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);
}
void printDate()
{
  clock_read();
  myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.print(str[dow-1], 256, 8);
  if (date<10)
	myGLCD.printNumI(date, 272, 28);
  else
	myGLCD.printNumI(date, 264, 28);

  myGLCD.print(str_mon[mon-1], 256, 48);
  myGLCD.printNumI(year, 248, 65);
}
void clearDate()
{
  myGLCD.setColor(255, 255, 255);
  myGLCD.fillRect(248, 8, 312, 81);
}
void AnalogClock()

{
	 int x, y;

	drawDisplay();
	printDate();
 
  clock_read();
  
  while (true)
  {

	if (oldsec!=sec)
	{
	  if ((sec==0) and (min==0) and (hour==0))
	  {
		clearDate();
		printDate();
	  }
	  if (sec==0)
	  {
		drawMin(min);
		drawHour(hour, min);
	  }
	  drawSec(sec);
	  oldsec=sec;
	}

	if (myTouch.dataAvailable())
	{
	  myTouch.read();
	  x=myTouch.getX();
	  y=myTouch.getY();
	  if (((y>=200) && (y<=239)) && ((x>=260) && (x<=319))) //установка часов
	  {
		myGLCD.setColor (255, 0, 0);
		myGLCD.drawRoundRect(260, 200, 319, 239);
		setClock();
	  }

	  if (((y>=140) && (y<=180)) && ((x>=260) && (x<=319))) //Возврат
	  {
		myGLCD.setColor (255, 0, 0);
		myGLCD.drawRoundRect(260, 140, 319, 180);
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		break;
	  }
	}
	
	delay(10);
	clock_read();
  }

}

void draw_Glav_Menu()
{
  but1 = myButtons.addButton( 10,  20, 250,  35, txt_menu1_1);
  but2 = myButtons.addButton( 10,  65, 250,  35, txt_menu1_2);
  but3 = myButtons.addButton( 10, 110, 250,  35, txt_menu1_3);
  but4 = myButtons.addButton( 10, 155, 250,  35, txt_menu1_4);
  butX = myButtons.addButton( 279, 199,  40,  40, "W", BUTTON_SYMBOL); // кнопка Часы 
  but_m1 = myButtons.addButton( 10, 199, 45,  40, "1");
  but_m2 = myButtons.addButton( 61, 199, 45,  40, "2");
  but_m3 = myButtons.addButton( 112, 199, 45,  40, "3");
  but_m4 = myButtons.addButton( 163, 199, 45,  40, "4");
  but_m5 = myButtons.addButton( 214, 199, 45,  40, "5");
  myGLCD.setColor(VGA_BLACK);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("                      ", CENTER, 0); 
  switch (m2) 
	{
	case 1:
			myGLCD.print(txt_info1, CENTER, 0);
			break;
		case 2:
			myGLCD.print(txt_info2, CENTER, 0);
			break;
		case 3:
			myGLCD.print(txt_info3, CENTER, 0);
			break;
		case 4:
			myGLCD.print(txt_info4, CENTER, 0);
			break;
		case 5:
			myGLCD.print(txt_info5, CENTER, 0);
			break;
		}
   myButtons.drawButtons();
}
void swichMenu() // Тексты меню в строках "txt....."
	
{
	
	 m2=1;                                                    // Устанивить первую странице меню
	 while(1) 
	   {
		 myButtons.setTextFont(BigFont);                      // Установить Большой шрифт кнопок  

			if (myTouch.dataAvailable() == true)              // Проверить нажатие кнопок
			  {
				pressed_button = myButtons.checkButtons();    // Если нажата - проверить что нажато
					 if (pressed_button==butX)                // Нажата вызов часы
						  {  
							 AnalogClock();
							 myGLCD.clrScr();
							 myButtons.drawButtons();         // Восстановить кнопки
							 print_up();                      // Восстановить верхнюю строку
						  }
		 
					 if (pressed_button==but_m1)              // Нажата 1 страница меню
						  {
							  myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_BLUE); // Голубой фон меню
							  myButtons.drawButtons();        // Восстановить кнопки
							  default_colors=true;
							  m2=1;                                                // Устанивить первую странице меню
							  myButtons.relabelButton(but1, txt_menu1_1, m2 == 1);
							  myButtons.relabelButton(but2, txt_menu1_2, m2 == 1);
							  myButtons.relabelButton(but3, txt_menu1_3, m2 == 1);
							  myButtons.relabelButton(but4, txt_menu1_4, m2 == 1);
							  myGLCD.setColor(0, 255, 0);
							  myGLCD.setBackColor(0, 0, 0);
							  myGLCD.print("                      ", CENTER, 0); 
							  myGLCD.print(txt_info1, CENTER, 0);            // "Ввод данных"
		
						  }
					if (pressed_button==but_m2)
						  {
							  myButtons.setButtonColors(VGA_WHITE, VGA_RED, VGA_YELLOW, VGA_BLUE, VGA_TEAL);
							  myButtons.drawButtons();
							  default_colors=false;
							  m2=2;
							  myButtons.relabelButton(but1, txt_menu2_1 , m2 == 2);
							  myButtons.relabelButton(but2, txt_menu2_2 , m2 == 2);
							  myButtons.relabelButton(but3, txt_menu2_3 , m2 == 2);
							  myButtons.relabelButton(but4, txt_menu2_4 , m2 == 2);
							  myGLCD.setColor(0, 255, 0);
							  myGLCD.setBackColor(0, 0, 0);
							  myGLCD.print("                      ", CENTER, 0); 
							  myGLCD.print(txt_info2, CENTER, 0);            // Информация
						 }

				   if (pressed_button==but_m3)
						 {
							  myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_GREEN);
							  myButtons.drawButtons();
							  default_colors=false;
							  m2=3;
							  myButtons.relabelButton(but1, txt_menu3_1 , m2 == 3);
							  myButtons.relabelButton(but2, txt_menu3_2 , m2 == 3);
							  myButtons.relabelButton(but3, txt_menu3_3 , m2 == 3);
							  myButtons.relabelButton(but4, txt_menu3_4 , m2 == 3);
							  myGLCD.setColor(0, 255, 0);
							  myGLCD.setBackColor(0, 0, 0);
							  myGLCD.print("                      ", CENTER, 0); 
							  myGLCD.print(txt_info3, CENTER, 0);            // Информация
						}
				   if (pressed_button==but_m4)
						{
							  myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_RED);
							  myButtons.drawButtons();
							  default_colors=false;
							  m2=4;
							  myButtons.relabelButton(but1, txt_menu4_1 , m2 == 4);
							  myButtons.relabelButton(but2, txt_menu4_2 , m2 == 4);
							  myButtons.relabelButton(but3, txt_menu4_3 , m2 == 4);
							  myButtons.relabelButton(but4, txt_menu4_4 , m2 == 4);
							  myGLCD.setColor(0, 255, 0);
							  myGLCD.setBackColor(0, 0, 0);
							  myGLCD.print("                      ", CENTER, 0); 
							  myGLCD.print(txt_info4, CENTER, 0);            // 
						}

				   if (pressed_button==but_m5)
						{
							  myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_NAVY);
							  myButtons.drawButtons();
							  default_colors=false;
							  m2=5;
							  myButtons.relabelButton(but1, txt_menu5_1 , m2 == 5);
							  myButtons.relabelButton(but2, txt_menu5_2 , m2 == 5);
							  myButtons.relabelButton(but3, txt_menu5_3 , m2 == 5);
							  myButtons.relabelButton(but4, txt_menu5_4 , m2 == 5);
							  myGLCD.setColor(0, 255, 0);
							  myGLCD.setBackColor(0, 0, 0);
							  myGLCD.print("                      ", CENTER, 0);  
							  myGLCD.print(txt_info5, CENTER, 0);            // 
						}
	
				   //*****************  Меню №1  **************

				   if (pressed_button==but1 && m2 == 1)
					   {
						//   Serial.print("Menu 1-1");
							 Draw_menu_ADC1();
							 menu_ADC();
							 myGLCD.clrScr();
							 myButtons.drawButtons();;
							 print_up();
					   }
	  
				   if (pressed_button==but2 && m2 == 1)
					   {
						  //  Serial.print("Menu 1-2");
							Draw_menu_Osc();
							menu_Oscilloscope();
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					   }
	  
				   if (pressed_button==but3 && m2 == 1)
					   {
						//   pass_test_start();  // Нарисовать цифровую клавиатуру
						//   klav123();          // Считать информацию с клавиатуры
						//if (ret == 1)        // Если "Возврат" - закончить
						//	 {
						//		goto bailout31;  // Перейти на окончание выполнения пункта меню
						//	 }
						//else                 // Иначе выполнить пункт меню
						//	 {
						//		pass_test();     // Проверить пароль
						//	 }
						//if ( ( pass1 == 1)||( pass2 == 1) || ( pass3 == 1)) // если верно - выполнить пункт меню
						//	 {
						//		myGLCD.clrScr();   // Очистить экран
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//	   colwater_save_start(); // если верно - выполнить пункт меню
						//	 }
						//else  // Пароль не верный - сообщить и закончить
						//	 {
						//		txt_pass_no_all();
						//	 }

							bailout31: // Восстановить пункты меню
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					   }
				   if (pressed_button==but4 && m2 == 1)
					   {
						//	pass_test_start();  // Нарисовать цифровую клавиатуру
						//	klav123();          // Считать информацию с клавиатуры
						//if (ret == 1)        // Если "Возврат" - закончить
						//	 {
						//		goto bailout41;  // Перейти на окончание выполнения пункта меню
						//	 }
						//else                 // Иначе выполнить пункт меню
						//	 {
						//		pass_test();     // Проверить пароль
						//	 }
						//if ( ( pass1 == 1)||( pass2 == 1) || ( pass3 == 1)) // если верно - выполнить пункт меню
						//	 {
						//		myGLCD.clrScr();   // Очистить экран
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		hotwater_save_start(); // если верно - выполнить пункт меню
						//	 }
						//else  // Пароль не верный - сообщить и закончить
						//	 {
						//		txt_pass_no_all();
						//	 }

							bailout41: // Восстановить пункты меню
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					   }

				 //*****************  Меню №2  **************


				   if (pressed_button==but1 && m2 == 2)
					  {
						//	print_info();
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }

				  if (pressed_button==but2 && m2 == 2)
					  {
						//   info_nomer_user();
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }
	  
				  if (pressed_button==but3 && m2 == 2)
					  {
						//	pass_test_start();
						//	klav123();
						//if (ret == 1)
						//   {
						//	   goto bailout32;
						//   }
						//else
						//   {
						//	   pass_test();
						//   }
						//if ( ( pass2 == 1) || ( pass3 == 1))
						//	{
						//		myGLCD.clrScr();
						//		myGLCD.print(txt_pass_ok, RIGHT, 208);
						//		delay (500);
						//		XBee_Setup();
						//	}
						// else
						//	{
						//		txt_pass_no_all();
						//	}

							bailout32:
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }
				  if (pressed_button==but4 && m2 == 2)
					  {
						//pass_test_start();
						//	klav123();
						//if (ret == 1)
						//   {
						//	   goto bailout42;
						//   }
						//else
						//   {
						//	   pass_test();
						//   }
						//if ( ( pass2 == 1) || ( pass3 == 1))
						//	{
						//		myGLCD.clrScr();
						//		myGLCD.print(txt_pass_ok, RIGHT, 208);
						//		delay (500);
						//	//	XBee_Setup();
						//	}
						// else
						//	{
						//		txt_pass_no_all();
						//	}

							bailout42:
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }
		
				//*****************  Меню №3  **************
				   if (pressed_button==but1 && m2 == 3) // Первый пункт меню 3
					  {
						//	pass_test_start();  // Нарисовать цифровую клавиатуру
						//	klav123();          // Считать информацию с клавиатуры
						//if (ret == 1)        // Если "Возврат" - закончить
						//	{
						//	   goto bailout13;  // Перейти на окончание выполнения пункта меню
						//	}
						//else                 // Иначе выполнить пункт меню
						//   {
						//		pass_test();     // Проверить пароль
						//   }
						//if (  ( pass1 == 1) || ( pass2 == 1) || ( pass3 == 1)) // если верно - выполнить пункт меню
						//   {
						//		myGLCD.clrScr();   // Очистить экран
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		eeprom_clear == 0;
						//		system_clear_start(); // если верно - выполнить пункт меню
						//   }
						//else  // Пароль не верный - сообщить и закончить
						//   {
						//		txt_pass_no_all();
						//   }

							 bailout13: // Восстановить пункты меню
							 myGLCD.clrScr();
							 myButtons.drawButtons();
							 print_up();
					  }

			 //--------------------------------------------------------------
				   if (pressed_button==but2 && m2 == 3)  // Второй пункт меню 3
					  {
		/*					
						  pass_test_start();
							 klav123();
						if (ret == 1)
						   {
							   goto bailout23;
						   }
						else
						   {
							   pass_test();
						   }
						if ( ( pass1 == 1)||( pass2 == 1) || ( pass3 == 1))
						   {
								myGLCD.clrScr();
								myGLCD.print(txt_pass_ok, RIGHT, 208);
								set_n_telef();
								delay (500);
						   }
						else
						   {
								txt_pass_no_all();
						   }
*/
							bailout23:
						
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
				   
					  }

			   //------------------------------------------------------------------

				   if (pressed_button==but3 && m2 == 3)  // Третий пункт меню 3
					  { 
	/*						pass_test_start();
							klav123();
						if (ret == 1)
						   {
							   goto bailout33;
						   }
						else
						   {
							   pass_test();
						   }
						if ( ( pass2 == 1) || ( pass3 == 1))
							{
								myGLCD.clrScr();
								myGLCD.print(txt_pass_ok, RIGHT, 208);
								delay (500);
								set_warm_gaz();
							}
						 else
							{
								txt_pass_no_all();
							}*/

							bailout33:
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }

	 //------------------------------------------------------------------
				   if (pressed_button==but4 && m2 == 3) // Четвертый пункт меню 3
					  {
						//	pass_test_start();
						//	klav123();
						//if (ret == 1)
						//	{
						//	   goto bailout43;
						//	}
						//else
						//	{
						//	   pass_test();
						//	}
						//if ( ( pass2 == 1) || ( pass3 == 1))
						//	{
						//		myGLCD.clrScr();
						//		myGLCD.print(txt_pass_ok, RIGHT, 208);
						//		delay (500);
						//		set_warm_temp();
						//	}
						//else
						//	{
						//		txt_pass_no_all();
						//		//myGLCD.clrScr();
						//		//myGLCD.setColor(255, 255, 255);
						//		//myGLCD.setBackColor(0, 0, 0);
						//		//myGLCD.print(txt_pass_no, RIGHT, 208);
						//		//delay (1000);
						//	}
							bailout43:
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }

				   //*****************  Меню №4  **************

				   if (pressed_button==but1 && m2 == 4) // Сброс данных
					  {
						//	pass_test_start();  // Нарисовать цифровую клавиатуру
						//	klav123();          // Считать информацию с клавиатуры
						//if (ret == 1)        // Если "Возврат" - закончить
						//	{
						//	   goto bailout14;  // Перейти на окончание выполнения пункта меню
						//	}
				  ////   else                 // Иначе выполнить пункт меню
					 //  //   {
						//	   pass_test();     // Проверить пароль
					 //  //   }
						//if ( ( pass2 == 1) || ( pass3 == 1)) // если верно - выполнить пункт меню
						//	{
						//		myGLCD.clrScr();   // Очистить экран
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		eeprom_clear = 1; // Разрешить стереть информации
						//		system_clear_start(); // если верно - выполнить пункт меню
						//	}
						//else  // Пароль не верный - сообщить и закончить
						//	{
						//		txt_pass_no_all();
						//	}

							bailout14: // Восстановить пункты меню
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
				   
					  }

				   if (pressed_button==but2 && m2 == 4)
					  {
						//	pass_test_start();
						//	klav123();
						//if (ret == 1)
						//	{
						//	   goto bailout24;
						//	}
						//else
						//   {
						//	   pass_test();
						//   }
						//if ( ( pass1 == 1)||( pass2 == 1) || ( pass3 == 1))
						//   {
						//		myGLCD.clrScr();
						//		myGLCD.print(txt_pass_ok, RIGHT, 208);
						//		delay (500);
						//		set_n_user_start();
						//   }
						//else
						//   {
						//		txt_pass_no_all();
						//   }

							bailout24:
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }

				   if (pressed_button==but3 && m2 == 4) // Ввод пароля пользователя
					  {
						//int  stCurrentLen_pass_user = i2c_eeprom_read_byte( deviceaddress,adr_pass_user-2);  //считать длину пароля  из памяти
						//if (stCurrentLen_pass_user == 0)
						//	{ 
						//		 pass1 = 1;
						//		 goto pass_cross_user; 
						//	}
						//	 pass_test_start();
						//	 klav123();
						//if (ret == 1)
						//	{
						//	   goto bailout34;
						//	}
						//  pass_test();
						//  pass_cross_user:

						//if ( ( pass1 == 1)||( pass2 == 1) || ( pass3 == 1))
						//	{
						//		myGLCD.clrScr();
						//		myGLCD.print(txt_pass_ok, RIGHT, 208);
						//		delay (500);
						//		set_pass_user_start();
						//	}
						//else
						//	{
						//		txt_pass_no_all();
						//	}

							bailout34:
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }
				   if (pressed_button==but4 && m2 == 4) // Смена пароля администратора
					  {
						//int stCurrentLen_pass_admin = i2c_eeprom_read_byte( deviceaddress,adr_pass_admin-2);
						//if (stCurrentLen_pass_admin == 0)
						//	{  
						//	   pass2 = 1;
						//	   pass3 = 1;
						//	   goto pass_cross_admin; 
						//	}
						//	//pass_test_start();
						//	//klav123();
						//if (ret == 1)
						//	 {
						//	   goto bailout44;
						//	 }
//							 pass_test();
//							 pass_cross_admin:
//				  
//						if (( pass2 == 1) || ( pass3 == 1))
//							{
//								myGLCD.clrScr();
//								myGLCD.print(txt_pass_ok, RIGHT, 208);
//								delay (500);
//							//	set_pass_admin_start();
//							}
//						else
//							{
////							txt_pass_no_all();
//							}

							bailout44:
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }
					//*****************  Меню №5  **************

				   if (pressed_button==but1 && m2 == 5) // Сброс данных
					  {
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }
				   if (pressed_button==but2 && m2 == 5)
					  {
						//	pass_test_start();  // Нарисовать цифровую клавиатуру
						//	klav123();          // Считать информацию с клавиатуры
						//if (ret == 1)        // Если "Возврат" - закончить
						//	{
						//	   goto bailout25;  // Перейти на окончание выполнения пункта меню
						//	}
						//else                 // Иначе выполнить пункт меню
						//   {
						//	   pass_test();     // Проверить пароль
						//   }
						//if ( ( pass2 == 1) || ( pass3 == 1)) // если верно - выполнить пункт меню
						//   {
						//	  myGLCD.clrScr();   // Очистить экран
						//	  myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//	  delay (500);
						//	  ZigBee_SetH(); // если верно - выполнить пункт меню
						//	  reset_klav();
						//   }
						//else  // Пароль не верный - сообщить и закончить
						//   {
						//	  txt_pass_no_all();
						//   }

						//bailout25:
						//	myButtons.drawButtons();
						//	print_up();
					  }

				   if (pressed_button==but3 && m2 == 5) // Ввод пароля пользователя
					  {
						//  pass_test_start();  // Нарисовать цифровую клавиатуру
						//  klav123();          // Считать информацию с клавиатуры
						if (ret == 1)        // Если "Возврат" - закончить
						   {
							  goto bailout35;  // Перейти на окончание выполнения пункта меню
						   }
						else                 // Иначе выполнить пункт меню
						   {
						//	   pass_test();     // Проверить пароль
						   }
						//if ( ( pass2 == 1) || ( pass3 == 1)) // если верно - выполнить пункт меню
						//   {
						//	  myGLCD.clrScr();   // Очистить экран
						//	  myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//	  delay (500);
						//	  ZigBee_SetL(); // если верно - выполнить пункт меню
						//	  reset_klav();
						//	}
						//else  // Пароль не верный - сообщить и закончить
						//	{
						//	  txt_pass_no_all();
						//	}

						bailout35:
							myButtons.drawButtons();
							print_up();
					  }

				   if (pressed_button==but4 && m2 == 5) // Смена пароля администратора
					  {
				   
						//	pass_test_start();  // Нарисовать цифровую клавиатуру
						//	klav123();          // Считать информацию с клавиатуры
						if (ret == 1)        // Если "Возврат" - закончить
							{
							   goto bailout45;  // Перейти на окончание выполнения пункта меню
							}
						else                 // Иначе выполнить пункт меню
							{
						//	   pass_test();     // Проверить пароль
							}
						//if ( ( pass2 == 1) || ( pass3 == 1)) // если верно - выполнить пункт меню
						//	{
						//		myGLCD.clrScr();   // Очистить экран
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		ZigBee_Set_Network();
						//		reset_klav();
						//	}
						//else  // Пароль не верный - сообщить и закончить
						//	{
						//		txt_pass_no_all();
						//	}

						bailout45:
							myButtons.drawButtons();
							print_up();
					  }
				  } 
	   }
}
void print_up()                             // Печать верхней строчки над меню
{
		myGLCD.setColor(0, 255, 0);
					myGLCD.setBackColor(0, 0, 0);
					myGLCD.print("                      ", CENTER, 0); 
				 switch (m2) 
				   {
					case 1:
						  myGLCD.print(txt_info1, CENTER, 0);
						  break;
					 case 2:
						  myGLCD.print(txt_info2, CENTER, 0);
						  break;
					 case 3:
						  myGLCD.print(txt_info3, CENTER, 0);
						  break;
					 case 4:
						  myGLCD.print(txt_info4, CENTER, 0);
						  break;
					 case 5:
						  myGLCD.print(txt_info5, CENTER, 0);
						  break;
				   }
}
void klav123() // ввод данных с цифровой клавиатуры
{
	ret = 0;

	while (true)
	  {
		if (myTouch.dataAvailable())
		{
			  myTouch.read();
			  x=myTouch.getX();
			  y=myTouch.getY();
	  
		if ((y>=10) && (y<=60))         // Upper row
		  {
			if ((x>=10) && (x<=60))     // Button: 1
			  {
				  waitForIt(10, 10, 60, 60);
				  updateStr('1');
			  }
			if ((x>=70) && (x<=120))   // Button: 2
			  {
				  waitForIt(70, 10, 120, 60);
				  updateStr('2');
			  }
			if ((x>=130) && (x<=180))  // Button: 3
			  {
				  waitForIt(130, 10, 180, 60);
				  updateStr('3');
			  }
			if ((x>=190) && (x<=240))  // Button: 4
			  {
				  waitForIt(190, 10, 240, 60);
				  updateStr('4');
			  }
			if ((x>=250) && (x<=300))  // Button: 5
			  {
				  waitForIt(250, 10, 300, 60);
				  updateStr('5');
			  }
		  }

		 if ((y>=70) && (y<=120))  // Center row
		   {
			 if ((x>=10) && (x<=60))  // Button: 6
				{
				  waitForIt(10, 70, 60, 120);
				  updateStr('6');
				}
			 if ((x>=70) && (x<=120))  // Button: 7
				{
				  waitForIt(70, 70, 120, 120);
				  updateStr('7');
				}
			 if ((x>=130) && (x<=180))  // Button: 8
				{
				  waitForIt(130, 70, 180, 120);
				  updateStr('8');
				}
			 if ((x>=190) && (x<=240))  // Button: 9
				{
				  waitForIt(190, 70, 240, 120);
				  updateStr('9');
				}
			 if ((x>=250) && (x<=300))  // Button: 0
				{
				  waitForIt(250, 70, 300, 120);
				  updateStr('0');
				}
			}
		  if ((y>=130) && (y<=180))  // Upper row
			 {
			 if ((x>=10) && (x<=130))  // Button: Clear
				{
				  waitForIt(10, 130, 120, 180);
				  stCurrent[0]='\0';
				  stCurrentLen=0;
				  myGLCD.setColor(0, 0, 0);
				  myGLCD.fillRect(0, 224, 319, 239);
				}
			 if ((x>=250) && (x<=300))  // Button: Exit
				{
				  waitForIt(250, 130, 300, 180);
				  myGLCD.clrScr();
				  myGLCD.setBackColor(VGA_BLACK);
				  ret = 1;
				  stCurrent[0]='\0';
				  stCurrentLen=0;
				  break;
				}
			 if ((x>=130) && (x<=240))  // Button: Enter
				{
				  waitForIt(130, 130, 240, 180);
				 if (stCurrentLen>0)
				   {
				   for (x=0; x<stCurrentLen+1; x++)
					 {
						stLast[x]=stCurrent[x];
					 }
						stCurrent[0]='\0';
						stLast[stCurrentLen+1]='\0';
		//				i2c_eeprom_write_byte(deviceaddress,adr_stCurrentLen1,stCurrentLen);
						stCurrentLen1 = stCurrentLen;
						stCurrentLen=0;
						myGLCD.setColor(0, 0, 0);
						myGLCD.fillRect(0, 208, 319, 239);
						myGLCD.setColor(0, 255, 0);
						myGLCD.print(stLast, LEFT, 208);
						break;
					}
				  else
					{
						myGLCD.setColor(255, 0, 0);
						myGLCD.print("\x80\x8A\x8B\x8B""EP \x89\x8A""CTO\x87!", CENTER, 192);//"БУФФЕР ПУСТОЙ!"
						delay(500);
						myGLCD.print("                ", CENTER, 192);
						delay(500);
						myGLCD.print("\x80\x8A\x8B\x8B""EP \x89\x8A""CTO\x87!", CENTER, 192);//"БУФФЕР ПУСТОЙ!"
						delay(500);
						myGLCD.print("                ", CENTER, 192);
						myGLCD.setColor(0, 255, 0);
					}
				 }
			  }
		  }
	   } 
} 

void updateStr(int val)
{
  if (stCurrentLen<20)
  {
	stCurrent[stCurrentLen]=val;
	stCurrent[stCurrentLen+1]='\0';
	stCurrentLen++;
	myGLCD.setColor(0, 255, 0);
	myGLCD.print(stCurrent, LEFT, 224);
  }
  else
  {   // Вывод строки "ПЕРЕПОЛНЕНИЕ!"
	myGLCD.setColor(255, 0, 0);
	myGLCD.print("\x89""EPE""\x89O\x88HEH\x86""E!", CENTER, 224);// ПЕРЕПОЛНЕНИЕ!
	delay(500);
	myGLCD.print("              ", CENTER, 224);
	delay(500);
	myGLCD.print("\x89""EPE""\x89O\x88HEH\x86""E!", CENTER, 224);// ПЕРЕПОЛНЕНИЕ!
	delay(500);
	myGLCD.print("              ", CENTER, 224);
	myGLCD.setColor(0, 255, 0);
  }
}
// Draw a red frame while a button is touched
void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
  myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}
void reset_klav()
{
		myGLCD.clrScr();
		myButtons.deleteAllButtons();
		but1 = myButtons.addButton( 10,  20, 250,  35, txt_menu5_1);
		but2 = myButtons.addButton( 10,  65, 250,  35, txt_menu5_2);
		but3 = myButtons.addButton( 10, 110, 250,  35, txt_menu5_3);
		but4 = myButtons.addButton( 10, 155, 250,  35, txt_menu5_4);
		butX = myButtons.addButton(279, 199,  40,  40, "W", BUTTON_SYMBOL); // кнопка Часы 
		but_m1 = myButtons.addButton(  10, 199, 45,  40, "1");
		but_m2 = myButtons.addButton(  61, 199, 45,  40, "2");
		but_m3 = myButtons.addButton(  112, 199, 45,  40, "3");
		but_m4 = myButtons.addButton(  163, 199, 45,  40, "4");
		but_m5 = myButtons.addButton(  214, 199, 45,  40, "5");

}
//++++++++++++++++++++++++++ Конец меню прибора ++++++++++++++++++++++++++++++++++++
void Draw_menu_Osc()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 255);
	for (int x=0; x<4; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (30, 20+(50*x), 290,60+(50*x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+(50*x), 290,60+(50*x));
		}
	myGLCD.print( txt_osc_menu1, CENTER, 30);     // 
	myGLCD.print( txt_osc_menu2, CENTER, 80);      
	myGLCD.print( txt_osc_menu3, CENTER, 130);     
	myGLCD.print( txt_osc_menu4, CENTER, 180);      
}
void menu_Oscilloscope()
{
		// discard any input
	while (Serial.read() >= 0) {} // Удалить все символы из буфера

	char c;

	while (true)
		{
		delay(10);
		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((x>=30) && (x<=290))       // Upper row
					{
					if ((y>=20) && (y<=60))    // Button: 1
						{
							waitForIt(30, 20, 290, 60);
							myGLCD.clrScr();
							DrawGrid();
							buttons();
							ADCSRA &= ~PS_128;  //
							ADCSRA |= PS_128;    // set our own prescaler 
							oscilloscope();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							dumpData_Osc();
							Draw_menu_Osc();
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
						
						}
					if ((y>=170) && (y<=220))  // Button: 4
						{
							waitForIt(30, 170, 290, 210);
							Draw_menu_Osc();
							break;
						}
				}
			}
	   }

}
void trigger()
{
	while (Input < Trigger)
	{
		Input = analogRead(port)*5/10;
	}
}
void oscilloscope()
{
	while(1) 
	{
		 DrawGrid();
		 touch();
		 trigger();

		// Collect the analog data into an array
 
		StartSample = micros();
		for( int xpos = 0;	xpos < 240; xpos ++) 
		{
			Sample[xpos] = analogRead(port)*5/100;
			delayMicroseconds(dTime);

		}

		EndSample = micros();
  
		// Display the collected analog data from array
		for( int xpos = 0; xpos < 239;	xpos ++)
			{
				// Erase previous display Стереть предыдущий экран
				myGLCD.setColor( 0, 0, 0);
				myGLCD.drawLine (xpos + 1, 255-OldSample[ xpos + 1]* vsens-hpos, xpos + 2, 255-OldSample[ xpos + 2]* vsens-hpos);
				if (xpos == 0) myGLCD.drawLine (xpos + 1, 1, xpos + 1, 239);
				//Draw the new data
				myGLCD.setColor( 255, 255, 255);
				myGLCD.drawLine (xpos, 255-Sample[ xpos]* vsens-hpos, xpos + 1, 255-Sample[ xpos + 1]* vsens-hpos);
			}
		// Determine sample voltage peak to peak
		Max = Sample[ 100];
		Min = Sample[ 100];
		for( int xpos = 0;	xpos < 240; xpos ++)
			{
			OldSample[xpos] = Sample[ xpos];
		/*	if (Sample[ xpos] > Max) Max = Sample[ xpos];
			if (Sample[ xpos] < Min) Min = Sample[ xpos];*/
			}
		// display the sample time, delay time and trigger level
		myGLCD.setBackColor( 0, 0, 255);
		myGLCD.setFont( SmallFont);
		myGLCD.setColor (255, 255,255);
		//myGLCD.setBackColor( 0, 0,255);
		
		myGLCD.print("Delay", 260, 5);
		myGLCD.print("     ", 270, 20);
		myGLCD.print(itoa ( dTime, buf, 10), 270, 20);
		myGLCD.print("Trig.", 260, 60);
		myGLCD.print("   ", 270, 75);
		myGLCD.print(itoa( Trigger, buf, 10), 270, 75);
		SampleTime =( EndSample/1000-StartSample/1000);
		myGLCD.print("mSec.", 260, 170);
		myGLCD.print("      ", 260, 190);
		myGLCD.printNumF(SampleTime, 1, 260, 190);
	/*	if (port == 0)myGLCD.print("Pulse", 260, 120);
		if (port == 1)myGLCD.print("Temp", 260, 120);
		if (port == 2)myGLCD.print("GSR", 260, 120);
		myGLCD.print( itoa( port, buf, 10), 270, 135);*/

	/*	myGLCD.setBackColor( 0, 0, 0);
		myGLCD.setFont( BigFont);
		myGLCD.print("Pulse", 10, 175);
		myGLCD.print("Temp", 100, 175);
		myGLCD.print("GSR", 180, 175);
		myGLCD.setColor (0, 255, 0);

		myGLCD.print(itoa( analogRead(A0)*4.15/10.23, buf, 10), 10, 200);
		myGLCD.print(itoa( analogRead(A1)*4.15/10.23, buf, 10),100, 200);
		myGLCD.print(itoa( analogRead(A2)*4.15/10.23, buf, 10),180 ,200);*/
	}


}

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
	  delay(10);
	  myTouch.read();
	  x_osc=myTouch.getX();
	  y_osc=myTouch.getY();

	  if ((x_osc>=250) && (x_osc<=310))  //  Delay Button
	  {
		  if ((y_osc>=1) && (y_osc<=50))  // Delay row
			  {
				waitForIt(250, 1, 310, 50);
				mode ++ ;
				// Select delay times you can change values to suite your needs
				if (mode == 0) dTime = 1;
				if (mode == 1) dTime = 10;
				if (mode == 2) dTime = 20;
				if (mode == 3) dTime = 50;
				if (mode == 4) dTime = 100;
				if (mode == 5) dTime = 200;
				if (mode == 6) dTime = 300;
				if (mode == 7) dTime = 500;
				if (mode == 8) dTime = 1000;
				if (mode == 9) dTime = 5000;
				if (mode == 10) dTime = 10000;
				if (mode > 10) mode = 0;   
				 Serial.println(dTime);

			  }
		 if ((y_osc>=55) && (y_osc<=105))  // Trigger  row
			 {
				waitForIt(250, 55, 310, 105);
				tmode ++;
				if (tmode == 1) Trigger = 0;
				if (tmode == 2) Trigger = 10;
				if (tmode == 3) Trigger = 20;
				if (tmode == 4) Trigger = 30;
				if (tmode == 5) Trigger = 50;
				if (tmode > 5)tmode = 0;
				Serial.println(Trigger);
			 }
		 if ((y_osc>=110) && (y_osc<=160))  // Port select   row
			 {
				waitForIt(250, 110, 310, 160);
				return;
			 }

	  }
   }
}
//----------wait for touch sub 
void waitForIt_osc(int x1, int y1, int x2, int y2)
{
  while (myTouch.dataAvailable())
  myTouch.read();
}
void DrawGrid()
{

  myGLCD.setColor( 0, 200, 0);
  for(  dgvh = 0; dgvh < 4; dgvh ++)
  {
	  myGLCD.drawLine( dgvh * 50, 0, dgvh * 50, 150);
	  myGLCD.drawLine(  0, dgvh * 50, 245 ,dgvh * 50);
  }
  myGLCD.drawLine( 200, 0, 200, 150);
  myGLCD.drawLine( 245, 0, 245, 150);
  myGLCD.setColor(255, 255, 255);
  
  myGLCD.drawRoundRect (250, 1, 310, 50);
  myGLCD.drawRoundRect (250, 55, 310, 105);
  myGLCD.drawRoundRect (250, 110, 310, 160);
  myGLCD.drawRoundRect (250, 165, 310, 215);
}

//------------------------------------------------------------------------------
void setup(void) 
{
  if (ERROR_LED_PIN >= 0) 
  {
	pinMode(ERROR_LED_PIN, OUTPUT);
  }
  Serial.begin(9600);
   Serial.println(F("Start Setup: "));
  
  // Read the first sample pin to init the ADC.
  analogRead(PIN_LIST[0]);
  
  Serial.print(F("FreeRam Setup: "));
  Serial.println(FreeRam());

  // initialize file system.
  if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) {
	sd.initErrorPrint();
	fatalBlink();
  }

	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Настройка звукового генератора  
	AD9850.reset();                    //reset module
	delay(1000);
	AD9850.powerDown();                //set signal output to LOW
	AD9850.set_frequency(0,0,500);    //set power=UP, phase=0, 1kHz frequency 

	Wire.begin();
	if (!RTC.begin())
	{
	//Serial.println("RTC failed");
	while(1);
	};
	// set date time callback function
	SdFile::dateTimeCallback(dateTime); 
	/*Draw_menu_ADC1();
	menu_ADC();*/

	Serial.println("End ADC");
}
//------------------------------------------------------------------------------
void loop()
{
	draw_Glav_Menu();
	swichMenu();
}
