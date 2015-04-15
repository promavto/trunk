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

#define __SAM3X8E__

#include <SdFat.h>
#include <SdFatUtil.h>
#include <StdioStream.h>
#include "AnalogBinLogger.h"
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <DueTimer.h>
#include <AH_AD9850.h>

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

//Настройка звукового генератора
#define CLK     8  // Назначение выводов генератора сигналов
#define FQUP    9  // Назначение выводов генератора сигналов
#define BitData 10 // Назначение выводов генератора сигналов
#define RESET   11 // Назначение выводов генератора сигналов
AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// настройка звукового генератора

// ADC speed one channel 480,000 samples/sec (no enable per read)
//           one channel 288,000 samples/sec (enable per read)

#define ADC_MR * (volatile unsigned int *) (0x400C0004) /*adc mode word*/
#define ADC_CR * (volatile unsigned int *) (0x400C0000) /*write a 2 to start convertion*/
#define ADC_ISR * (volatile unsigned int *) (0x400C0030) /*status reg -- bit 24 is data ready*/
#define ADC_ISR_DRDY 0x01000000

#define ADC_START 2
#define ADC_LCDR * (volatile unsigned int *) (0x400C0020) /*last converted low 12 bits*/
#define ADC_DATA 0x00000FFF 
#define ADC_STARTUP_FAST 12


#define ADC_CHER * (volatile unsigned int *) (0x400C0010) /*ADC Channel Enable Register  Только запись*/
#define ADC_CHSR * (volatile unsigned int *) (0x400C0018) /*ADC Channel Status Register  Только чтение */
#define ADC_CDR0 * (volatile unsigned int *) (0x400C0050) /*ADC Channel Только чтение */
#define ADC_ISR_EOC0 0x00000001


uint32_t ulChannel;

//------------------------------------------------------------------------------
// Analog pin number list for a sample. 
int Channel_0 = 1;
int Channel_1 = 0;
int Channel_2 = 0;
int Channel_3 = 0;
int Channel_x = 0;
int count_pin = 0;
int set_strob = 20;

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

const uint8_t BUFFER_BLOCK_COUNT = 12;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 16;  // Must be a power of two!

//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if a reset or power failure occurs.
#define TMP_FILE_NAME "TMP_LOG.BIN"

// Size of file base name.  Must not be larger than six.
//Размер базовой части имени файла. Должно быть не больше, чем шесть.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;


//==============================================================================
SdFat sd;

SdBaseFile binFile;

char binName[13] = FILE_BASE_NAME "00.BIN";

size_t SAMPLES_PER_BLOCK ;//= DATA_DIM16/PIN_COUNT; // 254 разделить на количество входов
typedef block16_t block_t;

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

// Insure no timer events are missed.
volatile bool timerError = false;
volatile bool timerFlag = false;
//------------------------------------------------------------------------------
bool ledOn = false;
void firstHandler()
{
	ADC_CHER = Channel_x;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
	ADC_CR = ADC_START ;                 // Запустить преобразование

   while (!(ADC_ISR & ADC_ISR_DRDY));

  if (isrBufNeeded && emptyHead == emptyTail) 
	  {
		// no buffers - count overrun 
		if (isrOver < 0XFFFF) isrOver++;
	
		// Avoid missed timer error. Избежать пропущенных ошибку таймера.
		timerFlag = false;
		return;
	  }

 // Check for buffer needed.  Проверьте буфера, необходимого.
  if (isrBufNeeded) 
	  {   
		// Remove buffer from empty queue. Удалить буфер из пустого очереди.
		isrBuf = emptyQueue[emptyTail];
		emptyTail = queueNext(emptyTail);
		isrBuf->count = 0;            // Счнтчик в 0
		isrBuf->overrun = isrOver;    // 
		isrBufNeeded = false;    
	  }
  // Store ADC data.

		if (Channel_0 == 1 ) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[7];
		if (Channel_1 == 1 ) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[6];
		if (Channel_2 == 1 ) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[5];
		if (Channel_3 == 1 ) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[4];
 
  if (isrBuf->count >= count_pin*SAMPLES_PER_BLOCK)  //  SAMPLES_PER_BLOCK -количество памяти выделенное на  один вход 
								 // SAMPLES_PER_BLOCK = DATA_DIM16/PIN_COUNT; // 254 разделить на количество входов
	  {
		// Put buffer isrIn full queue.  Положите буфер isrIn полной очереди.
		uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.
		fullQueue[tmp] = (block_t*)isrBuf;
		fullHead = queueNext(tmp);
		// Set buffer needed and clear overruns.
		isrBufNeeded = true;
		isrOver = 0;
	  }
}

void secondHandler()
{
	Serial.println("[ - ] Second Handler!");
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
	if (ERROR_LED_PIN >= 0) {
	  digitalWrite(ERROR_LED_PIN, HIGH);
	  delay(200);
	  digitalWrite(ERROR_LED_PIN, LOW);
	  delay(200);
	}
  }
}
//==============================================================================

// initialize ADC and timer1
void adcInit(metadata_t* meta) 
{

	  meta->pinCount = count_pin;
	  meta->recordEightBits = RECORD_EIGHT_BITS;
  
		 int i = 0;
		if (Channel_0 == 1 )
			{
				meta->pinNumber[i] = 0;
				i++;
			}
		if (Channel_1 == 1 )
			{
				meta->pinNumber[i] = 1;
				i++;
			}
		
		if (Channel_2 == 1 ) 
			{
				meta->pinNumber[i] = 2;
				i++;
			}

		if (Channel_3 == 1 ) 
			{
			   meta->pinNumber[i] = 3;
			}

	  // Sample interval in CPU clock ticks.
//	  meta->sampleInterval = ticks;
	  meta->cpuFrequency = F_CPU;
	  float sampleRate = (float)meta->cpuFrequency/meta->sampleInterval;
	  Serial.print(F("Sample pins:"));
	  for (int i = 0; i < meta->pinCount; i++) 
	  {
		Serial.print(' ');
		Serial.print(meta->pinNumber[i], DEC);
	  }
 
	  Serial.println(); 
	  Serial.print(F("ADC bits: 12 "));
	  Serial.print(F("ADC interval usec: "));
	  Serial.println(set_strob);
	  //Serial.print(F("Sample Rate: "));
	  //Serial.println(sampleRate);  
	  //Serial.print(F("Sample interval usec: "));
	  //Serial.println(1000000.0/sampleRate, 4); 
}

//------------------------------------------------------------------------------
// Convert binary file to CSV file.



void binaryToCsv() 
{
  uint8_t lastPct = 0;
  block_t buf;
  metadata_t* pm;
  uint32_t t0 = millis();
  char csvName[13];
  StdioStream csvStream;
  
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
  pm = (metadata_t*)&buf;
  csvStream.print(F("Interval,"));
   Serial.println(F("Interval "));
//  float intervalMicros = 1.0e6*pm->sampleInterval/(float)pm->cpuFrequency;
  float intervalMicros = 100;
  csvStream.print(intervalMicros, 4);
  csvStream.println(F(",usec"));
   Serial.println(F("Head 0 "));
 /* for (uint8_t i = 0; i < pm->pinCount; i++) 
  {
	if (i) csvStream.putc(',');
	csvStream.print(F("pin"));
	csvStream.print(pm->pinNumber[i]);
  }*/
  Serial.println(F("Head 1 "));
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
	for (uint16_t j = 0; j < buf.count; j += count_pin) 
	{
	  for (uint16_t i = 0; i < count_pin; i++) 
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
	  }
	}
	if (Serial.available()) break;
  }
  csvStream.fclose();  
  Serial.print(F("Done: "));
  Serial.print(0.001*(millis() - t0));
  Serial.println(F(" Seconds"));
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
	if (buf.overrun) 
	{
	  if (!headerPrinted) 
	  {
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
  Serial.println(F("Type any character to stop"));
  delay(1000);
  while (!Serial.available() && binFile.read(&buf , 512) == 512) 
  {
	if (buf.count == 0) break;
	if (buf.overrun) 
	{
	  Serial.print(F("OVERRUN,"));
	  Serial.println(buf.overrun);
	}
	for (uint16_t i = 0; i < buf.count; i++) 
	{
	  Serial.print(buf.data[i], DEC);
	  if ((i+1)%count_pin) 
	  {
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
void logData() 
{
  uint32_t bgnBlock, endBlock;
  
  // Allocate extra buffer space.
  block_t block[BUFFER_BLOCK_COUNT];
  
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
		Serial.println(F("Deleting tmp file"));
		if (!sd.remove(TMP_FILE_NAME)) 
			{
			  error("Can't remove tmp file");
			}
	  }
  // Create new file.
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
  // Use SdFat's internal buffer.
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0) error("cacheClear failed"); 
 
  // Flash erase all data in the file.
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
  // Write metadata. Написать метаданные.
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
  
  // Put rest of buffers in the empty queue. Поместите остальные буферов в пустую очередь.
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) 
	  {
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

   Timer3.start(set_strob);

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
			  if (usec > maxLatency) maxLatency = usec; // Максимальное время записи блока в SD
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

			   Timer3.stop();
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
			 Timer3.stop();
		  if (isrBuf != 0 && isrBuf->count >= count_pin) 
		  {
			// Truncate to last complete sample.
			isrBuf->count = count_pin*(isrBuf->count/count_pin);
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
  Serial.println(count/count_pin);
  Serial.print(F("Samples/sec: "));
  Serial.println((1000.0/count_pin)*count/(t1-t0));
  Serial.print(F("Overruns: "));
  Serial.println(overruns);
  Serial.println(F("Done"));
}

void chench_analog()
{

		   Channel_x = 0;
		   count_pin = 0;
	 
		if (Channel_0 == 1 )
			{
				Channel_x|=0x80;
			//	PIN_LIST[count_pin] = 0;
				count_pin++;
			}
		if (Channel_1 == 1 )
			{
				Channel_x|=0x40;
			//	PIN_LIST[count_pin] = 1;
				count_pin++;
			}
		
		if (Channel_2 == 1 ) 
			{
				Channel_x|=0x20;
			//	PIN_LIST[count_pin] = 2;
				count_pin++;
			}

		if (Channel_3 == 1 ) 
			{
				Channel_x|=0x10;
			//	PIN_LIST[count_pin] = 3;
				count_pin++;
			}

		 SAMPLES_PER_BLOCK = DATA_DIM16/count_pin;
}
//------------------------------------------------------------------------------
void setup(void) 
{
  if (ERROR_LED_PIN >= 0)
  {
	pinMode(ERROR_LED_PIN, OUTPUT);
  }
  Serial.begin(115200);
 
  
  Serial.print(F("FreeRam: "));
  Serial.println(FreeRam());

  // initialize file system.
  if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) 
  {
	sd.initErrorPrint();
	fatalBlink();
  }

   ADC_MR |= 0x00000100 ; // ADC full speed

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
	AD9850.set_frequency(0,0,1000);    //set power=UP, phase=0, 1kHz frequency 

	Channel_0 = 0;
	Channel_1 = 1;
	Channel_2 = 1;
	Channel_3 = 0;

	chench_analog();

  //adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
	Timer3.attachInterrupt(firstHandler); // Every 500ms
	Timer4.attachInterrupt(secondHandler).setFrequency(1);
	//  	Timer3.attachInterrupt(firstHandler).start(500000); // Every 500ms
	//Timer4.attachInterrupt(secondHandler).setFrequency(1).start();

}
//------------------------------------------------------------------------------
void loop(void) 
{
  // discard any input

  while (Serial.read() >= 0) {}
  Serial.println();
  Serial.println(F("type:"));
  Serial.println(F("c - convert file to CSV")); 
  Serial.println(F("d - dump data to Serial"));  
  Serial.println(F("e - overrun error details"));
  Serial.println(F("r - record ADC data"));
//  Timer3.start(100);
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
