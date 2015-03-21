/*




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

#define  ledPin12  12                               // ���������� ����������� �� �����
boolean ledStatus = false;
int end_measure = 0;

//��������� ��������� ����������
#define CLK     8  // ���������� ������� ���������� ��������
#define FQUP    9  // ���������� ������� ���������� ��������
#define BitData 10 // ���������� ������� ���������� ��������
#define RESET   11 // ���������� ������� ���������� ��������
AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// ��������� ��������� ����������

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
//******************************* ��������� �������� � Touch ������ *******************************

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// ��������� ��������

UTFT          myGLCD(ITDB32S,38,39,40,41);

UTouch        myTouch(6,5,4,3,2);

// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

boolean default_colors = true;
uint8_t menu_redraw_required = 0;

//----------------------�����  ��������� ������� --------------------------------

//**************************  ���� ������� ***************************************

int clockCenterX=119;
int clockCenterY=119;
int oldsec=0;
char* str[] = {"MON","TUE","WED","THU","FRI","SAT","SUN"};
char* str_mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

//-----------------------------------------------------------------------------------------------

//*********************** ���������� ��� �������� ����������**********************************
int x, y, z;
char stCurrent[20]     ="";                // ���������� �������� ��������� ������ 
//char stCurrent1[20];                       // ���������� �������� ��������� ������ 
int stCurrentLen       = 0;                // ���������� �������� ����� ��������� ������ 
int stCurrentLen1      = 0;                // ���������� ���������� �������� ����� ��������� ������  
//int stCurrentLen_user  = 0;                // ����������  �������� ����� ��������� ������ ������ ������������
//int stCurrentLen_telef = 0;                // ����������  �������� ����� ��������� ������ ������ ������������
//int stCurrentLen_admin = 0;                // ����������  �������� ����� ��������� ������ ������ ��������������
char stLast[20]        ="";                // ������ � ��������� ������ ������.
//char stLast1[20]       ="";                // ������ � ��������� ������ ������.
int ret                = 0;                // ������� ���������� ��������

//-----------------------------------------------------------------------------------------------------

//******************���������� ���������� ��� �������� � ����� ���� (������)****************************

 int but1, but2, but3, but4, but5, but6, but7, but8, but9, but10, butX, butY, but_m1, but_m2, but_m3, but_m4, but_m5, pressed_button;
// int kbut1, kbut2, kbut3, kbut4, kbut5, kbut6, kbut7, kbut8, kbut9, kbut0, kbut_save,kbut_clear, kbut_exit;
// int kbutA, kbutB, kbutC, kbutD, kbutE, kbutF;
 int m2 = 1; // ���������� ������ ����


//=====================================================================================
  //***************** ���������� ���������� ��� �������� �������*****************************************************

char  txt_menu1_1[]          = "PE\x81\x86""CTPATOP";                                                       // "�����������"
char  txt_menu1_2[]          = "CAMO\x89\x86""CE\x8C";                                                      // "���������"
char  txt_menu1_3[]          = "PE\x81\x86""CT.+ CAMO\x89.";                                                // "������. + �����."
char  txt_menu1_4[]          = "\x89O\x82K\x88\x94\x8D""EH\x86""E \x89K";                                   // "����������� ��"
char  txt_menu2_1[]          = "MENU 2-1";//"\x86H\x8BO C\x8D""ET\x8D\x86KOB";                              // ���� ���������
char  txt_menu2_2[]          = "MENU 2-2";//"\x86H\x8BO N \xA3o\xA0\xAC\x9C.";                              //
char  txt_menu2_3[]          = "MENU 2-3";//"Setup XBee";                                                   //
char  txt_menu2_4[]          = "MENU 2-4";//"menu2_4";                                                      //
char  txt_menu3_1[]          = "MENU 3-1";//"CTEPET\x92 \x8B""A\x87\x89\x91";                               //
char  txt_menu3_2[]          = "MENU 3-2";//"\x8A""c\xA4.N ""\xA4""e\xA0""e\xA5o\xA2""a";                   // ���. � ��������
char  txt_menu3_3[]          = "MENU 3-3";//"\x8A""c\xA4.Level Gaz";                                        //
char  txt_menu3_4[]          = "MENU 3-4";//"\x8A""c\xA4.Level Temp";                                       //
char  txt_menu4_1[]          = "MENU 4-1";//"C\x96poc \x99""a""\xA2\xA2\xABx";                              // ����� ������
char  txt_menu4_2[]          = "MENU 4-2";//"\x8A""c\xA4.N \xA3o\xA0\xAC\x9C.";                             // ���. � �����
char  txt_menu4_3[]          = "MENU 4-3";//"\x89""apo\xA0\xAC \xA3o\xA0\xAC\x9C.";                         // ������ �����.
char  txt_menu4_4[]          = "MENU 4-4";//"\x89""apo\xA0\xAC a\x99\xA1\x9D\xA2.";                         // ������ �����.
char  txt_menu5_1[]          = "MENU 5-1";//"\x86H\x8BO ZigBee";                                            // ���� ZigBee
char  txt_menu5_2[]          = "MENU 5-2";//"Set Adr Coord H";                                              //
char  txt_menu5_3[]          = "MENU 5-3";//"Set Adr Coord L";                                              // 
char  txt_menu5_4[]          = "MENU 5-4";//"Set Adr Network";                                              // 
char  txt_info1[]            = "B""\x97""o""\x99"" ""\x99""a""\xA2\xA2\xAB""x";                             // ���� ������
char  txt_info2[]            = "\x86\xA2\xA5op\xA1""a""\xA6\x9D\xAF";                                       // ����������
char  txt_info3[]            = "Hac\xA4po\x9E\x9F""a c\x9D""c\xA4""e\xA1\xAB";                              // ��������� �������
char  txt_info4[]            = "\x8A""c\xA4""a\xA2o\x97\x9F\x9D c\x9D""c\xA4""e\xA1\xAB";                   // 
char  txt_info5[]            = "\x86\xA2\xA5op\xA1""a""\xA6\x9D\xAF ZigBee";                                // ���������� ZigBee
char  txt_return[]           = "\x85""a\x97""ep\xA8\xA2\xA4\xAC \xA3poc\xA1o\xA4p";            // ��������� ��������
char  txt_sys_menu1[]        = "Record data";                                      //
char  txt_sys_menu2[]        = "Convert to CSV";                                             //
char  txt_sys_menu3[]        = "Dump to Serial";                                  //
char  txt_sys_menu4[]        = "Error details";                                  //


//******************* ADC ***********************************
  uint32_t bn = 1;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;

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
#define RECORD_EIGHT_BITS 0
//------------------------------------------------------------------------------
// Pin definitions.
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 13;

// SD chip select pin.
const uint8_t SD_CS_PIN = 53;
//------------------------------------------------------------------------------
// Buffer definitions.����������� ������.
//
// The logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional 
// buffers.  QUEUE_DIM must be a power of two larger than
//(BUFFER_BLOCK_COUNT + 1).
// ����������� ����� ������������ ����� SdFat ���� BUFFER_BLOCK_COUNT ��������������
// ������. QUEUE_DIM ������ ���� �������� ������ ������, ���
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
bool isrBufNeeded = true;

// overrun count
uint16_t isrOver = 0;

// ADC configuration for each pin.
uint8_t adcmux[PIN_COUNT];
uint8_t adcsra[PIN_COUNT];
uint8_t adcsrb[PIN_COUNT];
uint8_t adcindex = 1;

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

  if (isrBufNeeded && emptyHead == emptyTail) {
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
  // Check for buffer needed. ���������� ��������� ������
  if (isrBufNeeded) 
  {   
	// Remove buffer from empty queue. ������� ����� �� ������� �������.
	isrBuf = emptyQueue[emptyTail];
	emptyTail = queueNext(emptyTail);
	isrBuf->count = 0;
	isrBuf->overrun = isrOver;
	isrBufNeeded = false;    
  }
  // Store ADC data. ������ ������ ���.
  isrBuf->data[isrBuf->count++] = d;

  // Check for buffer full.
  if (isrBuf->count >= PIN_COUNT*SAMPLES_PER_BLOCK) 
  {
	// Put buffer isrIn full queue.  
	uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.
	fullQueue[tmp] = (block_t*)isrBuf;
	fullHead = queueNext(tmp);
   
	// Set buffer needed and clear overruns.
	isrBufNeeded = true;
	isrOver = 0;
  }
}
//------------------------------------------------------------------------------
// timer1 interrupt to clear OCF1B
ISR(TIMER1_COMPB_vect) 
{
  // Make sure ADC ISR responded to timer event. ��������� � ���, ADC ISR ������� �� ������� �������.
  if (timerFlag) timerError = true;
  timerFlag = true;
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) error_P(PSTR(msg))
//------------------------------------------------------------------------------
void error_P(const char* msg) {
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
void adcInit(metadata_t* meta) {
  uint8_t adps;  // prescaler bits for ADCSRA 
  uint32_t ticks = F_CPU*SAMPLE_INTERVAL + 0.5;  // Sample interval cpu cycles.

  if (ADC_REF & ~((1 << REFS0) | (1 << REFS1))) {
    error("Invalid ADC reference");
  }
#ifdef ADC_PRESCALER
  if (ADC_PRESCALER > 7 || ADC_PRESCALER < 2) {
    error("Invalid ADC prescaler");
  }
  adps = ADC_PRESCALER;
#else  // ADC_PRESCALER
  // Allow extra cpu cycles to change ADC settings if more than one pin.
  int32_t adcCycles = (ticks - ISR_TIMER0)/PIN_COUNT;
                      - (PIN_COUNT > 1 ? ISR_SETUP_ADC : 0);
                      
  for (adps = 7; adps > 0; adps--) {
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
  for (int i = 0; i < meta->pinCount; i++) {
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
//------------------------------------------------------------------------------
// enable ADC and timer1 interrupts
void adcStart() {
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
void binaryToCsv() {
  uint8_t lastPct = 0;
  block_t buf;
  metadata_t* pm;
  uint32_t t0 = millis();
  char csvName[13];
  StdioStream csvStream;
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Info: ",CENTER, 5);//
	  //myGLCD.print("File: ",LEFT, 25);//
	  //myGLCD.setColor(VGA_YELLOW);
	  //myGLCD.print(binName,100 , 25);// 
	//  myGLCD.setColor(255, 255, 255);
	//  myGLCD.print("Max block wr:",LEFT, 45);//
  
  if (!binFile.isOpen()) 
  {

	Serial.println(F("No current binary file"));
	return;
  }
  binFile.rewind();
  if (!binFile.read(&buf , 512) == 512) error("Read metadata failed");
  // Create a new CSV file.
  strcpy(csvName, binName);
  strcpy_P(&csvName[BASE_NAME_SIZE + 3], PSTR("CSV"));

  if (!csvStream.fopen(csvName, "w")) 
  {
	error("open csvStream failed");  
  }
  Serial.println();
  Serial.print(F("Writing: "));
  Serial.print(csvName);
  Serial.println(F(" - type any character to stop"));

	  myGLCD.print("Writing:",LEFT, 35);//
	  myGLCD.setColor(VGA_YELLOW);
	  myGLCD.print(csvName,125 , 35);// 
	  myGLCD.setColor(255, 255, 255);

  pm = (metadata_t*)&buf;
  csvStream.print(F("Interval,"));
  float intervalMicros = 1.0e6*pm->sampleInterval/(float)pm->cpuFrequency;
  csvStream.print(intervalMicros, 4);
  csvStream.println(F(",usec"));
  for (uint8_t i = 0; i < pm->pinCount; i++) 
  {
	if (i) csvStream.putc(',');
	csvStream.print(F("pin"));
	csvStream.print(pm->pinNumber[i]);
  }
  csvStream.println(); 
  uint32_t tPct = millis();
  while (!Serial.available() && binFile.read(&buf, 512) == 512) 
  {
	uint16_t i;
	if (buf.count == 0) break;
	if (buf.overrun) {
	  csvStream.print(F("OVERRUN,"));
	  csvStream.println(buf.overrun);     
	}
	for (uint16_t j = 0; j < buf.count; j += PIN_COUNT) {
	  for (uint16_t i = 0; i < PIN_COUNT; i++) {
		if (i) csvStream.putc(',');
		csvStream.print(buf.data[i + j]);     
	  }
	  csvStream.println();
	}
	if ((millis() - tPct) > 1000) {
	  uint8_t pct = binFile.curPosition()/(binFile.fileSize()/100);
	  if (pct != lastPct) {
		tPct = millis();
		lastPct = pct;
		Serial.print(pct, DEC);
		Serial.println('%');
		 myGLCD.printNumI(pct, 5, 75);// 
		 myGLCD.print("%",40, 75);//

	  }
	}
	if (Serial.available()) break;
  }
  csvStream.fclose();  
  Serial.print(F("Done: "));
  Serial.print(0.001*(millis() - t0));
  Serial.println(F(" Seconds"));
  myGLCD.print("Done: ",5, 110);//
  myGLCD.printNumF((0.001*(millis() - t0)),2, 85, 110);// 
//  myGLCD.printNumI((0.001*(millis() - t0)), 85, 110);// 
  myGLCD.print("Seconds",210, 110);//
  myGLCD.print("ESC->PUSH Display",LEFT, 180);
   while (true)
		   {
			  if (myTouch.dataAvailable())
			   {
				  myTouch.read();
				  int  x=myTouch.getX();
				  int  y=myTouch.getY();

				  if ((y>=2) && (y<=240))  // Upper row
				   {
					if ((x>=2) && (x<=319))  // �����
					  {
						 return;
					  }
				   }
			   }
		   }	

}
//------------------------------------------------------------------------------
// read data file and check for overruns
void checkOverrun() 
{
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
void dumpData() {
  block_t buf;
  if (!binFile.isOpen()) {
	Serial.println(F("No current binary file"));
	return;
  }
  binFile.rewind();
  if (binFile.read(&buf , 512) != 512) {
	error("Read metadata failed");
  }
  Serial.println();
  Serial.println(F("Type any character to stop"));
  delay(1000);
  while (!Serial.available() && binFile.read(&buf , 512) == 512) {
	if (buf.count == 0) break;
	if (buf.overrun) {
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
}
//------------------------------------------------------------------------------
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;
void logData() {
  uint32_t bgnBlock, endBlock;
  
  // Allocate extra buffer space.
  block_t block[BUFFER_BLOCK_COUNT];
  
  Serial.println();
  
  // Initialize ADC and timer1.
  adcInit((metadata_t*) &block[0]);
  
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
	if (!sd.remove(TMP_FILE_NAME)) {
	  error("Can't remove tmp file");
	}
  }
  // Create new file.
  Serial.println(F("Creating new file"));
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
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) {
	emptyQueue[emptyHead] = &block[i];
	emptyHead = queueNext(emptyHead);
  }
  // Give SD time to prepare for big write.
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

  // Start logging interrupts.
  adcStart();
  while (1) {
	if (fullHead != fullTail) {
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
	if (timerError) {
	  error("Missed timer event - rate too high");
	}
	if (Serial.available()) {
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
}
void logData1()
{
	uint32_t bgnBlock, endBlock;

	// Allocate extra buffer space.
	block_t block[BUFFER_BLOCK_COUNT];
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("START",CENTER, 40);

	Serial.println();
	// Initialize ADC and timer1.


	adcInit((metadata_t*) &block[0]);
	// Find unused file name.
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
		//	myGLCD.print("Deleting tmp file",LEFT, 135);//
		Serial.println(F("Deleting tmp file"));
		if (!sd.remove(TMP_FILE_NAME)) 
			{
				error("Can't remove tmp file");
			}
	}
	// Create new file.
	//  myGLCD.print("Creating new file",LEFT, 155);//"�����"
	Serial.println(F("Creating new file"));
	binFile.close();
	if (!binFile.createContiguous(sd.vwd(),
	TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT))
		{
			error("createContiguous failed");
		}
	// Get the address of the file on the SD.
	if (!binFile.contiguousRange(&bgnBlock, &endBlock)) 
		{
			error("contiguousRange failed");
		}
	Serial.flush();
	// Use SdFat's internal buffer.
	uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
	if (cache == 0) error("cacheClear failed"); 
 
	// Flash erase all data in the file.
	//  myGLCD.print("Erasing all data",LEFT, 175);//"�����"
	Serial.println(F("Erasing all data"));
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
	delay(1000);
	myGLCD.print("Stop->PUSH Display",LEFT, 120);
	Serial.println(F("Logging - type any character to stop"));
	// Wait for Serial Idle.
	Serial.flush();
	delay(10);
	bn = 1;
	// uint32_t t0 = millis();
	t0 = millis();
	t1 = t0;
	/*uint32_t t1 = t0;*/
	overruns = 0;
	count = 0;
	maxLatency = 0;

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
		//if (myTouch.dataAvailable())
		  if (Serial.available()) 
			{
				// Stop ISR calls.
				adcStop();
				if (isrBuf != 0 && isrBuf->count >= PIN_COUNT) 
					{
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
	if (!sd.card()->writeStop()) 
		{
			error("writeStop failed");
		}
	// Truncate file if recording stopped early.
	myGLCD.clrScr();

	if (bn != FILE_BLOCK_COUNT) 
		{    
			//	myGLCD.print("Truncating file",LEFT, 135);//
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
	// delay(1000);
	//   myGLCD.clrScr();
	//   myGLCD.print("Info: ",CENTER, 5);//
		//myGLCD.print("File: ",LEFT, 25);//
		//myGLCD.setColor(VGA_YELLOW);
		//myGLCD.print(binName,100 , 25);// 
	//  myGLCD.setColor(255, 255, 255);
	//  myGLCD.print("Max block wr:",LEFT, 45);//
	//  myGLCD.setColor(VGA_YELLOW);
	//  myGLCD.printNumI(maxLatency, 210, 45);// 
		//myGLCD.setColor(255, 255, 255);
		//myGLCD.print("Rec time sec:",LEFT, 65);//
		//myGLCD.setColor(VGA_YELLOW);
		//myGLCD.printNumF((0.001*(t1 - t0)),2, 210, 65);// 
		//myGLCD.setColor(255, 255, 255);
	//  myGLCD.print("Sample count:",LEFT, 85);//
	//  myGLCD.setColor(VGA_YELLOW);
	//  myGLCD.printNumI(count/PIN_COUNT, 210, 85);// 
		//myGLCD.setColor(255, 255, 255);
		//myGLCD.print("Samples/sec:",LEFT, 105);//
		//myGLCD.setColor(VGA_YELLOW);
		//myGLCD.printNumF((1000.0/PIN_COUNT)*count/(t1-t0),2, 210, 105);// 
		//myGLCD.setColor(255, 255, 255);
		//myGLCD.print("Overruns: ",LEFT, 125);//
		//myGLCD.setColor(VGA_YELLOW);
		//myGLCD.printNumI(overruns, 230, 125);// 
		//myGLCD.setColor(255, 255, 255);
		//delay(100);
			//while (true)
			//  {
			//  if (myTouch.dataAvailable())
			//   {
			//	  myTouch.read();
			//	  int  x=myTouch.getX();
			//	  int  y=myTouch.getY();

			//	  if ((y>=2) && (y<=240))  // Upper row
			//	   {
			//		if ((x>=2) && (x<=319))  // �����
			//		  {
			//			 return;
			//		  }
			//	   }
			//   }
			//  }		
  
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
//+++++++++++++++++++++++++++++ ����� AnalogBinLogger +++++++++++++++++++++++++++++

//=======================================================================================================

void dateTime(uint16_t* date, uint16_t* time) // ��������� ������ ������� � ���� �����
{
  DateTime now = RTC.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

//************************** ���������� ���� ****************************************************
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
	  if (((y>=200) && (y<=239)) && ((x>=260) && (x<=319))) //��������� �����
	  {
		myGLCD.setColor (255, 0, 0);
		myGLCD.drawRoundRect(260, 200, 319, 239);
		setClock();
	  }

	  if (((y>=140) && (y<=180)) && ((x>=260) && (x<=319))) //�������
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
  butX = myButtons.addButton( 279, 199,  40,  40, "W", BUTTON_SYMBOL); // ������ ���� 
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
void swichMenu() // ������ ���� � ������� "txt....."
	
{
	
	 m2=1;                                                    // ���������� ������ �������� ����
	 while(1) 
	   {
		 myButtons.setTextFont(BigFont);                      // ���������� ������� ����� ������  

			if (myTouch.dataAvailable() == true)              // ��������� ������� ������
			  {
				pressed_button = myButtons.checkButtons();    // ���� ������ - ��������� ��� ������
					 if (pressed_button==butX)                // ������ ����� ����
						  {  
							 AnalogClock();
							 myGLCD.clrScr();
							 myButtons.drawButtons();         // ������������ ������
							 print_up();                      // ������������ ������� ������
						  }
		 
					 if (pressed_button==but_m1)              // ������ 1 �������� ����
						  {
							  myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_BLUE); // ������� ��� ����
							  myButtons.drawButtons();        // ������������ ������
							  default_colors=true;
							  m2=1;                                                // ���������� ������ �������� ����
							  myButtons.relabelButton(but1, txt_menu1_1, m2 == 1);
							  myButtons.relabelButton(but2, txt_menu1_2, m2 == 1);
							  myButtons.relabelButton(but3, txt_menu1_3, m2 == 1);
							  myButtons.relabelButton(but4, txt_menu1_4, m2 == 1);
							  myGLCD.setColor(0, 255, 0);
							  myGLCD.setBackColor(0, 0, 0);
							  myGLCD.print("                      ", CENTER, 0); 
							  myGLCD.print(txt_info1, CENTER, 0);            // "���� ������"
		
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
							  myGLCD.print(txt_info2, CENTER, 0);            // ����������
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
							  myGLCD.print(txt_info3, CENTER, 0);            // ����������
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
	
				   //*****************  ���� �1  **************

				   if (pressed_button==but1 && m2 == 1)
					   {
						//	 start_ADC();
						//	 test_menu();
						     logData();
							 myGLCD.clrScr();
							 myButtons.drawButtons();;
							 print_up();
					   }
	  
				   if (pressed_button==but2 && m2 == 1)
					   {
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					   }
	  
				   if (pressed_button==but3 && m2 == 1)
					   {
						//   pass_test_start();  // ���������� �������� ����������
						//   klav123();          // ������� ���������� � ����������
						//if (ret == 1)        // ���� "�������" - ���������
						//	 {
						//		goto bailout31;  // ������� �� ��������� ���������� ������ ����
						//	 }
						//else                 // ����� ��������� ����� ����
						//	 {
						//		pass_test();     // ��������� ������
						//	 }
						//if ( ( pass1 == 1)||( pass2 == 1) || ( pass3 == 1)) // ���� ����� - ��������� ����� ����
						//	 {
						//		myGLCD.clrScr();   // �������� �����
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//	   colwater_save_start(); // ���� ����� - ��������� ����� ����
						//	 }
						//else  // ������ �� ������ - �������� � ���������
						//	 {
						//		txt_pass_no_all();
						//	 }

							bailout31: // ������������ ������ ����
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					   }
				   if (pressed_button==but4 && m2 == 1)
					   {
						//	pass_test_start();  // ���������� �������� ����������
						//	klav123();          // ������� ���������� � ����������
						//if (ret == 1)        // ���� "�������" - ���������
						//	 {
						//		goto bailout41;  // ������� �� ��������� ���������� ������ ����
						//	 }
						//else                 // ����� ��������� ����� ����
						//	 {
						//		pass_test();     // ��������� ������
						//	 }
						//if ( ( pass1 == 1)||( pass2 == 1) || ( pass3 == 1)) // ���� ����� - ��������� ����� ����
						//	 {
						//		myGLCD.clrScr();   // �������� �����
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		hotwater_save_start(); // ���� ����� - ��������� ����� ����
						//	 }
						//else  // ������ �� ������ - �������� � ���������
						//	 {
						//		txt_pass_no_all();
						//	 }

							bailout41: // ������������ ������ ����
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					   }

				 //*****************  ���� �2  **************


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
		
				//*****************  ���� �3  **************
				   if (pressed_button==but1 && m2 == 3) // ������ ����� ���� 3
					  {
						//	pass_test_start();  // ���������� �������� ����������
						//	klav123();          // ������� ���������� � ����������
						//if (ret == 1)        // ���� "�������" - ���������
						//	{
						//	   goto bailout13;  // ������� �� ��������� ���������� ������ ����
						//	}
						//else                 // ����� ��������� ����� ����
						//   {
						//		pass_test();     // ��������� ������
						//   }
						//if (  ( pass1 == 1) || ( pass2 == 1) || ( pass3 == 1)) // ���� ����� - ��������� ����� ����
						//   {
						//		myGLCD.clrScr();   // �������� �����
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		eeprom_clear == 0;
						//		system_clear_start(); // ���� ����� - ��������� ����� ����
						//   }
						//else  // ������ �� ������ - �������� � ���������
						//   {
						//		txt_pass_no_all();
						//   }

							 bailout13: // ������������ ������ ����
							 myGLCD.clrScr();
							 myButtons.drawButtons();
							 print_up();
					  }

			 //--------------------------------------------------------------
				   if (pressed_button==but2 && m2 == 3)  // ������ ����� ���� 3
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

				   if (pressed_button==but3 && m2 == 3)  // ������ ����� ���� 3
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
				   if (pressed_button==but4 && m2 == 3) // ��������� ����� ���� 3
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

				   //*****************  ���� �4  **************

				   if (pressed_button==but1 && m2 == 4) // ����� ������
					  {
						//	pass_test_start();  // ���������� �������� ����������
						//	klav123();          // ������� ���������� � ����������
						//if (ret == 1)        // ���� "�������" - ���������
						//	{
						//	   goto bailout14;  // ������� �� ��������� ���������� ������ ����
						//	}
				  ////   else                 // ����� ��������� ����� ����
					 //  //   {
						//	   pass_test();     // ��������� ������
					 //  //   }
						//if ( ( pass2 == 1) || ( pass3 == 1)) // ���� ����� - ��������� ����� ����
						//	{
						//		myGLCD.clrScr();   // �������� �����
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		eeprom_clear = 1; // ��������� ������� ����������
						//		system_clear_start(); // ���� ����� - ��������� ����� ����
						//	}
						//else  // ������ �� ������ - �������� � ���������
						//	{
						//		txt_pass_no_all();
						//	}

							bailout14: // ������������ ������ ����
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

				   if (pressed_button==but3 && m2 == 4) // ���� ������ ������������
					  {
						//int  stCurrentLen_pass_user = i2c_eeprom_read_byte( deviceaddress,adr_pass_user-2);  //������� ����� ������  �� ������
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
				   if (pressed_button==but4 && m2 == 4) // ����� ������ ��������������
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
					//*****************  ���� �5  **************

				   if (pressed_button==but1 && m2 == 5) // ����� ������
					  {
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					  }
				   if (pressed_button==but2 && m2 == 5)
					  {
						//	pass_test_start();  // ���������� �������� ����������
						//	klav123();          // ������� ���������� � ����������
						//if (ret == 1)        // ���� "�������" - ���������
						//	{
						//	   goto bailout25;  // ������� �� ��������� ���������� ������ ����
						//	}
						//else                 // ����� ��������� ����� ����
						//   {
						//	   pass_test();     // ��������� ������
						//   }
						//if ( ( pass2 == 1) || ( pass3 == 1)) // ���� ����� - ��������� ����� ����
						//   {
						//	  myGLCD.clrScr();   // �������� �����
						//	  myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//	  delay (500);
						//	  ZigBee_SetH(); // ���� ����� - ��������� ����� ����
						//	  reset_klav();
						//   }
						//else  // ������ �� ������ - �������� � ���������
						//   {
						//	  txt_pass_no_all();
						//   }

						//bailout25:
						//	myButtons.drawButtons();
						//	print_up();
					  }

				   if (pressed_button==but3 && m2 == 5) // ���� ������ ������������
					  {
						//  pass_test_start();  // ���������� �������� ����������
						//  klav123();          // ������� ���������� � ����������
						if (ret == 1)        // ���� "�������" - ���������
						   {
							  goto bailout35;  // ������� �� ��������� ���������� ������ ����
						   }
						else                 // ����� ��������� ����� ����
						   {
						//	   pass_test();     // ��������� ������
						   }
						//if ( ( pass2 == 1) || ( pass3 == 1)) // ���� ����� - ��������� ����� ����
						//   {
						//	  myGLCD.clrScr();   // �������� �����
						//	  myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//	  delay (500);
						//	  ZigBee_SetL(); // ���� ����� - ��������� ����� ����
						//	  reset_klav();
						//	}
						//else  // ������ �� ������ - �������� � ���������
						//	{
						//	  txt_pass_no_all();
						//	}

						bailout35:
							myButtons.drawButtons();
							print_up();
					  }

				   if (pressed_button==but4 && m2 == 5) // ����� ������ ��������������
					  {
				   
						//	pass_test_start();  // ���������� �������� ����������
						//	klav123();          // ������� ���������� � ����������
						if (ret == 1)        // ���� "�������" - ���������
							{
							   goto bailout45;  // ������� �� ��������� ���������� ������ ����
							}
						else                 // ����� ��������� ����� ����
							{
						//	   pass_test();     // ��������� ������
							}
						//if ( ( pass2 == 1) || ( pass3 == 1)) // ���� ����� - ��������� ����� ����
						//	{
						//		myGLCD.clrScr();   // �������� �����
						//		myGLCD.print(txt_pass_ok, RIGHT, 208); 
						//		delay (500);
						//		ZigBee_Set_Network();
						//		reset_klav();
						//	}
						//else  // ������ �� ������ - �������� � ���������
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
void print_up()                             // ������ ������� ������� ��� ����
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
void klav123() // ���� ������ � �������� ����������
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
						myGLCD.print("\x80\x8A\x8B\x8B""EP \x89\x8A""CTO\x87!", CENTER, 192);//"������ ������!"
						delay(500);
						myGLCD.print("                ", CENTER, 192);
						delay(500);
						myGLCD.print("\x80\x8A\x8B\x8B""EP \x89\x8A""CTO\x87!", CENTER, 192);//"������ ������!"
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
  {   // ����� ������ "������������!"
	myGLCD.setColor(255, 0, 0);
	myGLCD.print("\x89""EPE""\x89O\x88HEH\x86""E!", CENTER, 224);// ������������!
	delay(500);
	myGLCD.print("              ", CENTER, 224);
	delay(500);
	myGLCD.print("\x89""EPE""\x89O\x88HEH\x86""E!", CENTER, 224);// ������������!
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
		butX = myButtons.addButton(279, 199,  40,  40, "W", BUTTON_SYMBOL); // ������ ���� 
		but_m1 = myButtons.addButton(  10, 199, 45,  40, "1");
		but_m2 = myButtons.addButton(  61, 199, 45,  40, "2");
		but_m3 = myButtons.addButton(  112, 199, 45,  40, "3");
		but_m4 = myButtons.addButton(  163, 199, 45,  40, "4");
		but_m5 = myButtons.addButton(  214, 199, 45,  40, "5");

}
//++++++++++++++++++++++++++ ����� ���� ������� ++++++++++++++++++++++++++++++++++++




void start_ADC()
{
		 // discard any input
	  while (Serial.read() >= 0) {} // ������� ��� ������� �� ������

	//  Draw_menu_ADC1();
	  char c;



		while (Serial.read() >= 0) {} // �������� ������ ������
	  //Serial.println();
	  //Serial.println(F("type:"));
	  //Serial.println(F("c - convert file to CSV")); 
	  //Serial.println(F("d - dump data to Serial"));  
	  //Serial.println(F("e - overrun error details"));
	  //Serial.println(F("r - record ADC data"));

	  while(!Serial.available()) {} //  ������� ����� �������

	  c = tolower(Serial.read());
	  if (ERROR_LED_PIN >= 0) 
		  {
			digitalWrite(ERROR_LED_PIN, LOW);
		  }
	  // Read any extra Serial data.  �������� ��� ���������� �� �������
	  do {
		delay(10);
	  } while (Serial.read() >= 0);
  
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
			logData();
		  } 
	  else 
		  {
			Serial.println(F("Invalid entry"));
		  }













/*
   while (true)
   {
		delay(10);
		if (myTouch.dataAvailable())
		 {
			  myTouch.read();
			  x=myTouch.getX();
			  y=myTouch.getY();

				if ((x>=30) && (x<=290))       // Upper row
				  {
					if ((y>=20) && (y<=60))    // Button: 1
					  {
						  waitForIt(30, 20, 290, 60);
						  Serial.println("r");
						  logData();
					   //   Info_measureADC();
						 // c = "r";
					  }
					if ((y>=70) && (y<=110))   // Button: 1
					  {
						  waitForIt(30, 70, 290, 110);
						Serial.println("c");
						binaryToCsv();
						 // c = "c";
					  }
					if ((y>=120) && (y<=160))  // Button: 1
					  {
						waitForIt(30, 120, 290, 160);
						Serial.println("d");
						dumpData();
						  // c = "d";
					  }
					if ((y>=170) && (y<=220))  // Button: 1
					  {
						 waitForIt(30, 170, 290, 210);
						  Serial.println("e");
						 // c = "e";
							//int end_measure = 1;
							Serial.print(F("end_measure - "));
							Serial.println(end_measure);
							return;
					  }
				}

		   }
	  }





   */

  /*




 do
 {
  while(!myTouch.dataAvailable()) {}

  if (ERROR_LED_PIN >= 0) 
  {
	digitalWrite(ERROR_LED_PIN, LOW);
  }

   do {
		delay(10);
		if (myTouch.dataAvailable())
		 {
			  myTouch.read();
			  x=myTouch.getX();
			  y=myTouch.getY();
	  
			if ((x>=30) && (x<=290))       // Upper row
			  {
				if ((y>=20) && (y<=60))    // Button: 1
				  {
					  myGLCD.setColor(255, 0, 0);
					  myGLCD.drawRoundRect (30, 20, 290,60);
					  c = 'r';
				  }
				if ((y>=70) && (y<=110))   // Button: 1
				  {
					  myGLCD.setColor(255, 0, 0);
					  myGLCD.drawRoundRect (30, 20+50, 290,60+50);
					  c = 'c';
				  }
				if ((y>=120) && (y<=160))  // Button: 1
				  {
					  myGLCD.setColor(255, 0, 0);
					  myGLCD.drawRoundRect (30, 20+100, 290,60+100);
					   c = 'd';
				  }
				if ((y>=170) && (y<=220))  // Button: 1
				  {
					  myGLCD.setColor(255, 0, 0);
					  myGLCD.drawRoundRect (30, 20+150, 290,60+150);
					  c = 'e';
				  }
			}
		}
	  } while (myTouch.dataAvailable());

	if (c == 'c') 
		{
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+50, 290,60+50);
			//binaryToCsv();
		}
	else if (c == 'd') 
		{
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+100, 290,60+100);
			//dumpData();
		}
	else if (c == 'e') 
		{    
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+150, 290,60+150);
			//checkOverrun();
			//return;
			int end_measure = 1;
			Serial.print(F("end_measure - "));
			Serial.println(end_measure);
			return;
		}
	else if (c == 'r') 
		{
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20, 290,60);
		//	logData();
		//	Info_measureADC();
		} 
	} while (1);

 */
}


void test_menu()
{
	
	//Draw_menu_ADC1();
 do {

	 start_ADC();

 } while (end_measure ==1);

   end_measure = 0;

}


void Draw_menu_ADC1()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 255);
	for (x=0; x<4; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (30, 20+(50*x), 290,60+(50*x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+(50*x), 290,60+(50*x));
		}
	myGLCD.print("Record ADC data", CENTER, 30);     // "Record ADC data"
	myGLCD.print("Convert to CSV", CENTER, 80);      // "Convert to CSV"
	myGLCD.print("Data to Serial", CENTER, 130);     // "Data to Serial"
	myGLCD.print("Error details", CENTER, 180);      // "Error details"
}

void Info_measureADC()
{
	 ////  Serial.flush();
  //    delayMicroseconds(10000);
	  myGLCD.print("Info: ",CENTER, 5);//
	  myGLCD.print("File: ",LEFT, 25);//
	  myGLCD.setColor(VGA_YELLOW);
	  myGLCD.print(binName,100 , 25);// 
	  //myGLCD.setColor(255, 255, 255);
	  //myGLCD.print("Max block wr:",LEFT, 45);//
	  //myGLCD.setColor(VGA_YELLOW);
	  //myGLCD.printNumI(maxLatency, 210, 45);// 
	  myGLCD.setColor(255, 255, 255);
	  myGLCD.print("Rec time sec:",LEFT, 65);//
	  myGLCD.setColor(VGA_YELLOW);
	  myGLCD.printNumF((0.001*(t1 - t0)),2, 210, 65);// 
	  //myGLCD.setColor(255, 255, 255);
	  //myGLCD.print("Sample count:",LEFT, 85);//
	  //myGLCD.setColor(VGA_YELLOW);
	  //myGLCD.printNumI(count/PIN_COUNT, 210, 85);// 
	  //myGLCD.setColor(255, 255, 255);
	  //myGLCD.print("Samples/sec:",LEFT, 105);//
	  //myGLCD.setColor(VGA_YELLOW);
	  //myGLCD.printNumF((1000.0/PIN_COUNT)*count/(t1-t0),2, 210, 105);// 
	  //myGLCD.setColor(255, 255, 255);
	  //myGLCD.print("Overruns: ",LEFT, 125);//
	  //myGLCD.setColor(VGA_YELLOW);
	  //myGLCD.printNumI(overruns, 230, 125);// 
	  //myGLCD.setColor(255, 255, 255);
	  myGLCD.print("ESC->PUSH Display",LEFT, 180);
		 while (true)
		   {
			  if (myTouch.dataAvailable())
			   {
				  myTouch.read();
				  int  x=myTouch.getX();
				  int  y=myTouch.getY();

				  if ((y>=2) && (y<=240))  // Upper row
				   {
					if ((x>=2) && (x<=319))  // �����
					  {
						 return;
					  }
				   }
			   }
		   }		
}


//------------------------------------------------------------------------------
void setup(void) 
{
  if (ERROR_LED_PIN >= 0)
  {
	pinMode(ERROR_LED_PIN, OUTPUT);
  }
 // Serial.begin(9600);
   Serial.begin(115200);
  // Read the first sample pin to init the ADC.
  analogRead(PIN_LIST[0]);
  
  Serial.print(F("FreeRam: "));
  Serial.println(FreeRam());

  // initialize file system.
  if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) 
  {
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
	// ��������� ��������� ����������  
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

}
void loop(void)
{
	//draw_Glav_Menu();
	//swichMenu();
   start_ADC();
//	logData();

}