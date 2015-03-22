// IMPORTANT: Adafruit_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h FOR SETUP.
#define __SAM3X8E__

#include <SdFat.h>
#include <SdFatUtil.h>
#include <StdioStream.h>
#include "AnalogBinLogger.h"
#include <Adafruit_GFX.h>   
#include <Adafruit_TFTLCD.h> 
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
// uint8_t const ADC_REF = (1 << REFS0);  // Vcc Reference.
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
#define BUF_SIZE 8192
//------------------------------------------------------------------------------
// Buffer definitions.
//
// The logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional 
// buffers.  QUEUE_DIM must be a power of two larger than
//(BUFFER_BLOCK_COUNT + 1).
//
/*
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
*/

const uint8_t BUFFER_BLOCK_COUNT = 28;
const uint8_t QUEUE_DIM = 32;  // Must be a power of two!
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
//Sd2Card sd;

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

//// ADC configuration for each pin.
volatile uint8_t adcmux[PIN_COUNT];
volatile uint8_t adcsra[PIN_COUNT];
volatile uint8_t adcsrb[PIN_COUNT];
volatile uint8_t adcindex = 1;

// Insure no timer events are missed.
volatile bool timerError = false;
volatile bool timerFlag = false;
//------------------------------------------------------------------------------
// ADC done interrupt.

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


ISR(ADC_vect) 
{
	uint8_t d;
  // Read ADC data.
#if RECORD_EIGHT_BITS
 // uint8_t d = ADCH;
#else  // RECORD_EIGHT_BITS
  // This will access ADCL first. 
 // uint16_t d = ADC;
#endif  // RECORD_EIGHT_BITS

  if (isrBufNeeded && emptyHead == emptyTail) {
	// no buffers - count overrun
	if (isrOver < 0XFFFF) isrOver++;
	
	// Avoid missed timer error.
	timerFlag = false;
	return;
  }
  // Start ADC
 /* if (PIN_COUNT > 1) {
	ADMUX = adcmux[adcindex];
	ADCSRB = adcsrb[adcindex];
	ADCSRA = adcsra[adcindex];
	if (adcindex == 0) timerFlag = false;
	adcindex =  adcindex < (PIN_COUNT - 1) ? adcindex + 1 : 0;
  } else {
	timerFlag = false;
  }*/
  // Check for buffer needed.
  if (isrBufNeeded) {   
	// Remove buffer from empty queue.
	isrBuf = emptyQueue[emptyTail];
	emptyTail = queueNext(emptyTail);
	isrBuf->count = 0;
	isrBuf->overrun = isrOver;
	isrBufNeeded = false;    
  }
  // Store ADC data.
  isrBuf->data[isrBuf->count++] = d;

  // Check for buffer full.
  if (isrBuf->count >= PIN_COUNT*SAMPLES_PER_BLOCK) {
	// Put buffer isrIn full queue.  
	uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.
	fullQueue[tmp] = (block_t*)isrBuf;
	fullHead = queueNext(tmp);
   
	// Set buffer needed and clear overruns.
	isrBufNeeded = true;
	isrOver = 0;
  }
}
ISR(TIMER1_COMPB_vect) 
{
  // Make sure ADC ISR responded to timer event.
  if (timerFlag) timerError = true;
  timerFlag = true;
}

void error_P(const char* msg) 
{
  sd.errorPrint_P(msg);
  fatalBlink();
}
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

void adcInit(metadata_t* meta) 
{
  uint8_t adps;  // prescaler bits for ADCSRA 
  uint32_t ticks = F_CPU*SAMPLE_INTERVAL + 0.5;  // Sample interval cpu cycles.
// Исправить
 /* if (ADC_REF & ~((1 << REFS0) | (1 << REFS1))) {
	error("Invalid ADC reference");
  }*/
#ifdef ADC_PRESCALER
  if (ADC_PRESCALER > 7 || ADC_PRESCALER < 2) {
	//error("Invalid ADC prescaler");
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
  if (meta->adcFrequency > (RECORD_EIGHT_BITS ? 2000000 : 1000000)) {
	//error("Sample Rate Too High");
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
	//if (pin >= NUM_ANALOG_INPUTS) error("Invalid Analog pin number");
	meta->pinNumber[i] = pin;
	
   // Set ADC reference and low three bits of analog pin number.   
//	adcmux[i] = (pin & 7) | ADC_REF;
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


uint32_t const ERASE_SIZE = 262144L;
void logData() 
{
  uint32_t bgnBlock, endBlock;
  
  // Allocate extra buffer space.
  block_t block[BUFFER_BLOCK_COUNT];
  
  Serial.println();
  
  // Initialize ADC and timer1.
//  adcInit((metadata_t*) &block[0]);
  
  // Find unused file name.
  if (BASE_NAME_SIZE > 6) {
	//error("FILE_BASE_NAME too long");
  }
  while (sd.exists(binName)) {
	if (binName[BASE_NAME_SIZE + 1] != '9') {
	  binName[BASE_NAME_SIZE + 1]++;
	} else {
	  binName[BASE_NAME_SIZE + 1] = '0';
	  if (binName[BASE_NAME_SIZE] == '9') {
		//error("Can't create file name");
	  }
	  binName[BASE_NAME_SIZE]++;
	}
  }
  // Delete old tmp file.
  if (sd.exists(TMP_FILE_NAME)) {
	Serial.println(F("Deleting tmp file"));
	if (!sd.remove(TMP_FILE_NAME)) {
	//  error("Can't remove tmp file");
	}
  }
  // Create new file.
  Serial.println(F("Creating new file"));
  binFile.close();
  if (!binFile.createContiguous(sd.vwd(),
	TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT)) {
//error("createContiguous failed");
  }
  // Get the address of the file on the SD.
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
//error("contiguousRange failed");
  }
  // Use SdFat's internal buffer.
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
// if (cache == 0) error("cacheClear failed"); 
 
  // Flash erase all data in the file.
  Serial.println(F("Erasing all data"));
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) {
	endErase = bgnErase + ERASE_SIZE;
	if (endErase > endBlock) endErase = endBlock;
	if (!sd.card()->erase(bgnErase, endErase)) {
//  error("erase failed");
	}
	bgnErase = endErase + 1;
  }
  // Start a multiple block write.
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) {
//error("writeBegin failed");
  }
  // Write metadata.
  if (!sd.card()->writeData((uint8_t*)&block[0])) {
//error("Write metadata failed");
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
 // adcStart();
  while (1) {
	if (fullHead != fullTail) {
	  // Get address of block to write.
	  block_t* pBlock = fullQueue[fullTail];
	  
	  // Write block to SD.
	  uint32_t usec = micros();
	  if (!sd.card()->writeData((uint8_t*)pBlock)) {
//	error("write data failed");
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
//	adcStop();
		break;
	  }
	}
	if (timerError) {
	//  error("Missed timer event - rate too high");
	}
	if (Serial.available()) {
	  // Stop ISR calls.
	//  adcStop();
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
//error("writeStop failed");
  }
  // Truncate file if recording stopped early.
  if (bn != FILE_BLOCK_COUNT) {    
	Serial.println(F("Truncating file"));
	if (!binFile.truncate(512L * bn)) {
//  error("Can't truncate file");
	}
  }
  if (!binFile.rename(sd.vwd(), binName)) {
// error("Can't rename file");
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

void Start_Oscilloscope()
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
		//binaryToCsv();
	  }
  else if (c == 'd') 
	  {
		//dumpData();
	  }
  else if (c == 'e') 
	  {    
		//checkOverrun();
	  }
  else if (c == 'r') 
	  {
		//logData1();
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

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
 
	// init the trigger state machine  инициализации триггера
	min_ADC = 100; max_ADC = 100; trig_1 = 50;  trig_2 = 150;
  
	// init the ADC  инициализировать АЦП
	if (analogInPin5 < A0) analogInPin5 += A0;  // 
	ulChannel = g_APinDescription[analogInPin5].ulADCChannelNumber ;
	adc_enable_channel( ADC, (adc_channel_num_t)ulChannel );   
   
	// Read the first sample pin to init the ADC.
	analogRead(PIN_LIST[0]);
  
	Serial.print(F("FreeRam: "));
	Serial.println(FreeRam());


	if (!sd.begin(SD_CS_PIN,  SPI_HALF_SPEED)) 
	{
	sd.initErrorPrint();
	//fatalBlink();
	}


	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("Info: ",CENTER, 5);//
	Draw_menu_ADC1();
	delay(100);

}
//======================================================================

void loop(void) 
{

	menu_ADC();
  // Start_Oscilloscope();
 //Start_ADC();
}