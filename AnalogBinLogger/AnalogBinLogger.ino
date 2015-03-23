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

 //***************** Назначение переменных для хранения текстов*****************************************************


char  txt_menu1_1[]          = "PE\x81\x86""CTPATOP";                                                       // "РЕГИСТРАТОР"
char  txt_menu1_2[]          = "CAMO\x89\x86""CE\x8C";                                                      // "САМОПИСЕЦ"
char  txt_menu1_3[]          = "PE\x81\x86""CT.+ CAMO\x89.";                                                // "РЕГИСТ. + САМОП."
char  txt_menu1_4[]          = "\x89O\x82K\x88\x94\x8D""EH\x86""E \x89K";                                   // "ПОДКЛЮЧЕНИЕ ПК"
char  txt_menu2_1[]          = "MENU 2-1";//"\x86H\x8BO C\x8D""ET\x8D\x86KOB";                              // ИНФО СЧЕТЧИКОВ
char  txt_menu2_2[]          = "MENU 2-2";//"\x86H\x8BO N \xA3o\xA0\xAC\x9C.";                              //
char  txt_menu2_3[]          = "MENU 2-3";//"Setup XBee";                                                   //
char  txt_menu2_4[]          = "MENU 2-4";//"menu2_4";                                                      //
char  txt_menu3_1[]          = "MENU 3-1";//"CTEPET\x92 \x8B""A\x87\x89\x91";                               //
char  txt_menu3_2[]          = "MENU 3-2";//"\x8A""c\xA4.N ""\xA4""e\xA0""e\xA5o\xA2""a";                   // Уст. № телефона
char  txt_menu3_3[]          = "MENU 3-3";//"\x8A""c\xA4.Level Gaz";                                        //
char  txt_menu3_4[]          = "MENU 3-4";//"\x8A""c\xA4.Level Temp";                                       //
char  txt_menu4_1[]          = "MENU 4-1";//"C\x96poc \x99""a""\xA2\xA2\xABx";                              // Сброс данных
char  txt_menu4_2[]          = "MENU 4-2";//"\x8A""c\xA4.N \xA3o\xA0\xAC\x9C.";                             // Уст. № польз
char  txt_menu4_3[]          = "MENU 4-3";//"\x89""apo\xA0\xAC \xA3o\xA0\xAC\x9C.";                         // Пароль польз.
char  txt_menu4_4[]          = "MENU 4-4";//"\x89""apo\xA0\xAC a\x99\xA1\x9D\xA2.";                         // Пароль админ.
char  txt_menu5_1[]          = "MENU 5-1";//"\x86H\x8BO ZigBee";                                            // Инфо ZigBee
char  txt_menu5_2[]          = "MENU 5-2";//"Set Adr Coord H";                                              //
char  txt_menu5_3[]          = "MENU 5-3";//"Set Adr Coord L";                                              // 
char  txt_menu5_4[]          = "MENU 5-4";//"Set Adr Network";                                              // 


char  txt_info1[]            = "B""\x97""o""\x99"" ""\x99""a""\xA2\xA2\xAB""x";                             // Ввод данных
char  txt_info2[]            = "\x86\xA2\xA5op\xA1""a""\xA6\x9D\xAF";                                       // Информация
char  txt_info3[]            = "Hac\xA4po\x9E\x9F""a c\x9D""c\xA4""e\xA1\xAB";                              // Настройка системы
char  txt_info4[]            = "\x8A""c\xA4""a\xA2o\x97\x9F\x9D c\x9D""c\xA4""e\xA1\xAB";                   // 
char  txt_info5[]            = "\x86\xA2\xA5op\xA1""a""\xA6\x9D\xAF ZigBee";                                // Информация ZigBee
char  txt_return[]           = "\x85""a\x97""ep\xA8\xA2\xA4\xAC \xA3poc\xA1o\xA4p";            // Завершить просмотр

char  txt_sys_menu1[]        = "Record data";                                      //
char  txt_sys_menu2[]        = "Convert to CSV";                                             //
char  txt_sys_menu3[]        = "Dump to Serial";                                  //
char  txt_sys_menu4[]        = "Error details";                                  //

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
char  txt_info28[]            = ""; 
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
const uint8_t PIN_LIST[] = {0, 1, 2, 3};
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
// Набор RECORD_EIGHT_BITS ненулевым, чтобы записывать только высокие 8-бит АЦП
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
//Определения буфера.
//
// Регистратор будет использовать буфер SdFat плюс BUFFER_BLOCK_COUNT дополнительная
// Буферы. QUEUE_DIM должны быть степенью двойки больше, чем
// (BUFFER_BLOCK_COUNT + 1).
//
#if RAMEND < 0X8FF
#error Too little SRAM
//
#elif RAMEND < 0X10FF
// Use total of two 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 1;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 4;  // Must be a power of two!
//
#elif RAMEND < 0X20FF
// Use total of five 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 4;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 8;  // Must be a power of two!
//
#elif RAMEND < 0X40FF
// Use total of 13 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 12;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 16;  // Must be a power of two!
//
#else  // RAMEND
// Use total of 29 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 28;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 32;  // Must be a power of two!
#endif  // RAMEND
//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if a reset or power failure occurs.
#define TMP_FILE_NAME "TMP_LOG.BIN"

// Size of file base name.  Must not be larger than six.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

// Number of analog pins to log.  Количество аналоговых булавки, чтобы войти
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
#if RECORD_EIGHT_BITS  // если RECORD_EIGHT_BITS не равно "0" - записывать только старшие 8 бит
  uint8_t d = ADCH;
#else  // RECORD_EIGHT_BITS
  // This will access ADCL first.  Это будет иметь доступ ADCL в первую очередь.
  uint16_t d = ADC;                     // Запись двух байтов   
#endif  // RECORD_EIGHT_BITS

  if (isrBufNeeded && emptyHead == emptyTail) 
  {
	// no buffers - count overrun
	if (isrOver < 0XFFFF) isrOver++;
	
	// Avoid missed timer error.
	timerFlag = false;
	return;
  }
  // Start ADC
  if (PIN_COUNT > 1) 
	  {
		ADMUX = adcmux[adcindex];
		ADCSRB = adcsrb[adcindex];
		ADCSRA = adcsra[adcindex];
		if (adcindex == 0) timerFlag = false;
		adcindex =  adcindex < (PIN_COUNT - 1) ? adcindex + 1 : 0;
	  } 
  else 
	  {
		timerFlag = false;
	  }
  // Check for buffer needed.  Проверьте буфера, необходимого
  if (isrBufNeeded) 
	  {   
		// Remove buffer from empty queue.  Удалить буфер из пустого очереди.
		isrBuf = emptyQueue[emptyTail];
		emptyTail = queueNext(emptyTail);
		isrBuf->count = 0;
		isrBuf->overrun = isrOver;
		isrBufNeeded = false;    
	  }
  // Store ADC data.  Запись результата в буффер 
  isrBuf->data[isrBuf->count++] = d;

  // Check for buffer full.   Проверьте заполнения буфера.
  if (isrBuf->count >= PIN_COUNT*SAMPLES_PER_BLOCK) 
  {
	// Put buffer isrIn full queue.   Положите буфер isrIn полной очереди.
	uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead. 
	fullQueue[tmp] = (block_t*)isrBuf;
	fullHead = queueNext(tmp);
   
	// Set buffer needed and clear overruns. Установить буфер необходимости и четкие перерасход.
	isrBufNeeded = true;
	isrOver = 0;
  }
}
//------------------------------------------------------------------------------
// timer1 interrupt to clear OCF1B  timer1 прервать, чтобы очистить OCF1B
ISR(TIMER1_COMPB_vect) 
{
  // Make sure ADC ISR responded to timer event.  // Убедитесь в том, ADC ISR ответил на таймер событие
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
void fatalBlink() 
{
  while (true) 
  {
	if (ERROR_LED_PIN >= 0) 
	{
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
					  - (PIN_COUNT > 1 ? ISR_SETUP_ADC : 0);
					  
  for (adps = 7; adps > 0; adps--) 
  {
	 if (adcCycles >= (MIN_ADC_CYCLES << adps)) break;
  }
#endif  // ADC_PRESCALER
  meta->adcFrequency = F_CPU >> adps;
  if (meta->adcFrequency > (RECORD_EIGHT_BITS ? 2000000 : 1000000)) 
  {
	error("Sample Rate Too High");
  }
#if ROUND_SAMPLE_INTERVAL
  // Round so interval is multiple of ADC clock.
  ticks += 1 << (adps - 1);
  ticks >>= adps;
  ticks <<= adps;
#endif  // ROUND_SAMPLE_INTERVAL

  if (PIN_COUNT > sizeof(meta->pinNumber)/sizeof(meta->pinNumber[0])) 
  {
	error("Too many pins");
  }
  meta->pinCount = PIN_COUNT;
  meta->recordEightBits = RECORD_EIGHT_BITS;
  
  for (int i = 0; i < PIN_COUNT; i++) 
  {
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
  for (unsigned int i = 0; i < meta->pinCount; i++) 
  {
	Serial.print(' ');
	Serial.print(meta->pinNumber[i], DEC);
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
}
void adcInit1(metadata_t* meta) 
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
void adcStop() 
{
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
	if (binFile.read(&buf , 512) != 512) {
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
		for (uint16_t i = 0; i < buf.count; i++) {
		Serial.print(buf.data[i], DEC);
		if ((i+1)%PIN_COUNT) {
		Serial.print(',');
		} else {
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
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;
void logData() 
{
  uint32_t bgnBlock, endBlock;
  
  // Allocate extra buffer space.  Выделять дополнительное место буфера.
  block_t block[BUFFER_BLOCK_COUNT];  // В зависимости от выделенной памяти
  
  Serial.println();
  
  // Initialize ADC and timer1.   Инициализация АЦП и Timer1.
  adcInit((metadata_t*) &block[0]);
  
  // Find unused file name. Найти неиспользованные имя файла.
  if (BASE_NAME_SIZE > 6) 
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
	  if (binName[BASE_NAME_SIZE] == '9') {
		error("Can't create file name");
	  }
	  binName[BASE_NAME_SIZE]++;
	}
  }
  // Delete old tmp file.  Удалить старый файл TMP.
  if (sd.exists(TMP_FILE_NAME)) 
  {
	Serial.println(F("Deleting tmp file"));
	if (!sd.remove(TMP_FILE_NAME)) 
	{
	  error("Can't remove tmp file");
	}
  }
  // Create new file.  Создать новый файл.
  Serial.println(F("Creating new file"));
  binFile.close();
  if (!binFile.createContiguous(sd.vwd(),
	TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT)) 
  {
	error("createContiguous failed");
  }
  // Get the address of the file on the SD.  Получить адрес файла на SD.
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) 
  {
	error("contiguousRange failed");
  }
  // Use SdFat's internal buffer. Используйте внутренний буфер SdFat
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0) error("cacheClear failed"); 
 
  // Flash erase all data in the file. стереть все данные в файле.
  Serial.println(F("Erasing all data"));
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) 
  {
	endErase = bgnErase + ERASE_SIZE;
	if (endErase > endBlock) endErase = endBlock;
	if (!sd.card()->erase(bgnErase, endErase)) {
	  error("erase failed");
	}
	bgnErase = endErase + 1;
  }
  // Start a multiple block write. Старт несколько блоков записи.
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) 
  {
	error("writeBegin failed");
  }
  // Write metadata.   Написать метаданные.  Записать параметры настройки АЦП
  if (!sd.card()->writeData((uint8_t*)&block[0]))
  {
	error("Write metadata failed");
  } 
  // Initialize queues. Инициализация очереди.
  emptyHead = emptyTail = 0;
  fullHead = fullTail = 0;
  
  // Use SdFat buffer for one block. Используйте SdFat буфер для одного блока.
  emptyQueue[emptyHead] = (block_t*)cache;
  emptyHead = queueNext(emptyHead);
  
  // Put rest of buffers in the empty queue. Поместите остальные буферов в пустую очередь.
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) // В зависимости от выделенной памяти
  {
	emptyQueue[emptyHead] = &block[i];
	emptyHead = queueNext(emptyHead);
  }
  // Give SD time to prepare for big write. Дайте SD времени, чтобы подготовиться к большой записи.
  delay(1000);
  Serial.println(F("Logging - type any character to stop"));
  // Wait for Serial Idle.
  Serial.flush();
  delay(10);
  uint32_t bn = 1;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;

  // Start logging interrupts.  Начало регистрации прерываний.
  adcStart();
  while (1) 
  {
	if (fullHead != fullTail) 
	{
	  // Get address of block to write.  Получить адрес блока, чтобы написать
	  block_t* pBlock = fullQueue[fullTail];
	  
	  // Write block to SD. 
	  uint32_t usec = micros();
	  if (!sd.card()->writeData((uint8_t*)pBlock)) 
	  {
		error("write data failed");
	  }
	  usec = micros() - usec;
	  t1 = millis();
	  if (usec > maxLatency) maxLatency = usec;
	  count += pBlock->count;
	  
	  // Add overruns and possibly light LED. Добавить перерасхода средств и, возможно
	  if (pBlock->overrun) 
	  {
		overruns += pBlock->overrun;
		if (ERROR_LED_PIN >= 0) 
		{
		  digitalWrite(ERROR_LED_PIN, HIGH);
		}
	  }
	  // Move block to empty queue.  Переместить блок в пустой очереди.
	  emptyQueue[emptyHead] = pBlock;
	  emptyHead = queueNext(emptyHead);
	  fullTail = queueNext(fullTail);
	  bn++;
	  if (bn == FILE_BLOCK_COUNT) 
	  {
		// File full so stop ISR calls.  Файл полон, таким образом остановить ISR звонки.
		adcStop();
		break;
	  }
	}
	if (timerError) 
	{
	  error("Missed timer event - rate too high");
	}
	if (Serial.available()) 
	{
	  // Stop ISR calls.
	  adcStop();
	  if (isrBuf != 0 && isrBuf->count >= PIN_COUNT) 
	  {
		// Truncate to last complete sample. Обрезать К последнему полную выборку.
		isrBuf->count = PIN_COUNT*(isrBuf->count/PIN_COUNT);
		// Put buffer in full queue. Положите буфер в полном очереди.
		fullQueue[fullHead] = isrBuf;
		fullHead = queueNext(fullHead);
		isrBuf = 0;
	  }
	  if (fullHead == fullTail) break;
	}
  }
  if (!sd.card()->writeStop()) 
  {
	error("writeStop failed");
  }
  // Truncate file if recording stopped early.  Обрезать файл, если запись остановлена рано.
  if (bn != FILE_BLOCK_COUNT) 
  {    
	Serial.println(F("Truncating file"));
	if (!binFile.truncate(512L * bn))
	{
	  error("Can't truncate file");
	}
  }
  if (!binFile.rename(sd.vwd(), binName)) 
  {
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
}
void logData1() 
{
	uint32_t bgnBlock, endBlock;
	// Allocate extra buffer space.
	block_t block[BUFFER_BLOCK_COUNT];
  
	Serial.println();
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info12, CENTER, 2);


  
	// Initialize ADC and timer1.
	adcInit1((metadata_t*) &block[0]);
  
	// Find unused file name.
	if (BASE_NAME_SIZE > 6) {
	error("FILE_BASE_NAME too long");
	}
	while (sd.exists(binName)) {
	if (binName[BASE_NAME_SIZE + 1] != '9') {
		binName[BASE_NAME_SIZE + 1]++;
	} else {
		binName[BASE_NAME_SIZE + 1] = '0';
		if (binName[BASE_NAME_SIZE] == '9') {
		error("Can't create file name");
		}
		binName[BASE_NAME_SIZE]++;
	}
	}
	// Delete old tmp file.
	if (sd.exists(TMP_FILE_NAME)) {
	Serial.println(F("Deleting tmp file"));
	myGLCD.print(txt_info13,LEFT, 135);//
	if (!sd.remove(TMP_FILE_NAME)) {
		error("Can't remove tmp file");
	}
	}
	// Create new file.
	Serial.println(F("Creating new file"));
	myGLCD.print(txt_info27,LEFT, 155);//"Сброс"
	binFile.close();
	if (!binFile.createContiguous(sd.vwd(),
	TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT)) {
	error("createContiguous failed");
	}
	// Get the address of the file on the SD.
	if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
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
	while (bgnErase < endBlock) {
	endErase = bgnErase + ERASE_SIZE;
	if (endErase > endBlock) endErase = endBlock;
	if (!sd.card()->erase(bgnErase, endErase)) {
		error("erase failed");
	}
	bgnErase = endErase + 1;
	}
	// Start a multiple block write.
	if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) {
	error("writeBegin failed");
	}
	// Write metadata.
	if (!sd.card()->writeData((uint8_t*)&block[0])) {
	error("Write metadata failed");
	} 
	// Initialize queues.
	emptyHead = emptyTail = 0;
	fullHead = fullTail = 0;
  
	// Use SdFat buffer for one block.
	emptyQueue[emptyHead] = (block_t*)cache;
	emptyHead = queueNext(emptyHead);
  
	// Put rest of buffers in the empty queue.
	for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) 
		{
			emptyQueue[emptyHead] = &block[i];
			emptyHead = queueNext(emptyHead);
		}
	// Give SD time to prepare for big write.
	delay(1500);
	Serial.println(F("Logging - type any character to stop"));
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info15, CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	// Wait for Serial Idle.
	Serial.flush();
	delay(10);
	uint32_t bn = 1;
	uint32_t t0 = millis();
	uint32_t t1 = t0;
	uint32_t overruns = 0;
	uint32_t count = 0;
	uint32_t maxLatency = 0;

	// Start logging interrupts.
	adcStart();
	while (1) 
		{
		if (fullHead != fullTail) 
			{
				// Get address of block to write.
				block_t* pBlock = fullQueue[fullTail];
	  
				// Write block to SD.
				uint32_t usec = micros();
				if (!sd.card()->writeData((uint8_t*)pBlock)) {
				error("write data failed");
				}
				usec = micros() - usec;
				t1 = millis();
				if (usec > maxLatency) maxLatency = usec;
				count += pBlock->count;
	  
				// Add overruns and possibly light LED. 
				if (pBlock->overrun) {
				overruns += pBlock->overrun;
				if (ERROR_LED_PIN >= 0) {
					digitalWrite(ERROR_LED_PIN, HIGH);
				}
				}
				// Move block to empty queue.
				emptyQueue[emptyHead] = pBlock;
				emptyHead = queueNext(emptyHead);
				fullTail = queueNext(fullTail);
				bn++;
				if (bn == FILE_BLOCK_COUNT) {
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

void start_ADC(void) // Эталон программы
{
	  // discard any input
  while (Serial.read() >= 0) {}
  Serial.println();
  Serial.println(F("type:"));
  Serial.println(F("c - convert file to CSV")); 
  Serial.println(F("d - dump data to Serial"));  
  Serial.println(F("e - overrun error details"));
  Serial.println(F("r - record ADC data"));

  while(!Serial.available()) {}
  char c = tolower(Serial.read());
  if (ERROR_LED_PIN >= 0) {
	digitalWrite(ERROR_LED_PIN, LOW);
  }
  // Read any extra Serial data.
  do {
	delay(10);
  } while (Serial.read() >= 0);
  
  if (c == 'c') {
	binaryToCsv();
  } else if (c == 'd') {
	dumpData();
  } else if (c == 'e') {    
	checkOverrun();
  } else if (c == 'r') {
	logData();
  } else {
	Serial.println(F("Invalid entry"));
  }

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
	myGLCD.print( txt_sys_menu1, CENTER, 30);     // "Record ADC data"
	myGLCD.print( txt_sys_menu2, CENTER, 80);      // "Convert to CSV"
	myGLCD.print( txt_sys_menu3, CENTER, 130);     // "Data to Serial"
	myGLCD.print( txt_sys_menu4, CENTER, 180);      // "Error details"
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

	//while(!Serial.available()) {}  //  Ожидать ввода символа
	// c = tolower(Serial.read());

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
						    c = 'r';
							Serial.println(F("r - record ADC data"));
							break;
						}
					if ((y>=70) && (y<=110))   // Button: 1
						{
							waitForIt(30, 70, 290, 110);
							c = 'c';
							Serial.println(F("c - convert file to CSV")); 
							break;
						}
					if ((y>=120) && (y<=160))  // Button: 1
						{
						waitForIt(30, 120, 290, 160);
							 c = 'd';
							 break;
						}
					if ((y>=170) && (y<=220))  // Button: 1
						{
							waitForIt(30, 170, 290, 210);
							c = 'e';
							break;
						}
				}
			}
	   }


  if (ERROR_LED_PIN >= 0) 
	  {
		digitalWrite(ERROR_LED_PIN, LOW);
	  }

  if (c == 'c') 
	  {
		binaryToCsv();
	  }
  else if (c == 'd') 
	  {
		dumpData();
	  }
  else if (c == 'e') 
	  {    
		checkOverrun();
	  }
  else if (c == 'r') 
	  {
		logData1();
	  }
  else 
	  {
		Serial.println(F("Invalid entry"));
	  }

}
void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
  myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}

//------------------------------------------------------------------------------
void setup(void) 
{
  if (ERROR_LED_PIN >= 0) 
  {
	pinMode(ERROR_LED_PIN, OUTPUT);
  }
  Serial.begin(9600);
  
  // Read the first sample pin to init the ADC.
  analogRead(PIN_LIST[0]);
  
  Serial.print(F("FreeRam: "));
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
	myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Настройка звукового генератора  
	AD9850.reset();                    //reset module
	delay(1000);
	AD9850.powerDown();                //set signal output to LOW
	AD9850.set_frequency(0,0,1000);    //set power=UP, phase=0, 1kHz frequency 

	Wire.begin();
	if (!RTC.begin())
	{
	//Serial.println("RTC failed");
	while(1);
	};
	// set date time callback function
	SdFile::dateTimeCallback(dateTime); 
	Draw_menu_ADC1();
}
//------------------------------------------------------------------------------
void loop()
{
	menu_ADC();
}
