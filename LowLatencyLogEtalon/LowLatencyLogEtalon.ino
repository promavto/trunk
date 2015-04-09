/**
 * This program logs data to a binary file.  Functions are included
 * to convert the binary file to a CSV text file.
 *
 * Samples are logged at regular intervals.  The maximum logging rate
 * depends on the quality of your SD card and the time required to
 * read sensor data.  This example has been tested at 500 Hz with
 * good SD card on an Uno.  4000 HZ is possible on a Due.
 *
 * If your SD card has a long write latency, it may be necessary to use
 * slower sample rates.  Using a Mega Arduino helps overcome latency
 * problems since 13 512 byte buffers will be used.
 *
 * Data is written to the file using a SD multiple block write command.
 */

#define __SAM3X8E__


#include <SdFat.h>
#include <SdFatUtil.h>
#include <AH_AD9850.h>
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include "Wire.h"
#include <rtc_clock.h>



//------------------------------------------------------------------------------
// User data functions.  Modify these functions for your data items.
#include "UserDataType.h"  // Edit this include file to change data_t.
//******************************* Настройки монитора и Touch панели *******************************

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// Настройка монитора

//UTFT          myGLCD(ITDB32S,38,39,40,41);   // Mega
UTFT          myGLCD(ITDB32S,25,26,27,28);     // DUE

UTouch        myTouch(6,5,4,3,2);

// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

boolean default_colors = true;
uint8_t menu_redraw_required = 0;

//----------------------Конец  Настройки дисплея --------------------------------
//Настройка звукового генератора
#define CLK     8  // Назначение выводов генератора сигналов
#define FQUP    9  // Назначение выводов генератора сигналов
#define BitData 10 // Назначение выводов генератора сигналов
#define RESET   11 // Назначение выводов генератора сигналов
AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// настройка звукового генератора

int Set_ADC = 12;


// ADC_MR is 0x400C0004
#define ADC_MR * (volatile unsigned int *) (0x400C0004) /*adc mode word*/
#define ADC_CR * (volatile unsigned int *) (0x400C0000) /*write a 2 to start convertion*/
#define ADC_ISR * (volatile unsigned int *) (0x400C0030) /*status reg -- bit 24 is data ready*/
#define ADC_ISR_DRDY 0x01000000
#define ADC_START 2
#define ADC_LCDR * (volatile unsigned int *) (0x400C0020) /*last converted low 12 bits*/
#define ADC_DATA 0x00000FFF 
uint32_t ulChannel;
static int _readResolution = 12;
uint32_t analogInPin = A0;
//------------------------------------------------------------------------------
// User data functions.  Modify these functions for your data items.
#include "UserDataType.h"  // Edit this include file to change data_t.

// Acquire a data record.
void acquireData(data_t* data) 
{
//  data->time = micros();

	 ADC_CR = ADC_START ;
  for (int i = 0; i < ADC_DIM; i++) 
  {
	ulChannel = g_APinDescription[analogInPin].ulADCChannelNumber ;
    adc_enable_channel( ADC, (adc_channel_num_t)ulChannel );   
    while (!(ADC_ISR & ADC_ISR_DRDY));
    // Read the value
    data->adc[i] = ADC_LCDR & ADC_DATA ;
    // start next
    ADC_CR = ADC_START ;
	//data->adc[i] = analogRead(i);
  }
}

// Print a data record.
void printData(Print* pr, data_t* data) 
{
//  pr->print(data->time);
  for (int i = 0; i < ADC_DIM; i++) 
  {
	pr->write(',');  
	pr->print(data->adc[i]);
  }
  pr->println();
}

// Print data header.
void printHeader(Print* pr) {
  pr->print(F("time"));
  for (int i = 0; i < ADC_DIM; i++) {
	pr->print(F(",adc"));
	pr->print(i);
  }
  pr->println();
}
//==============================================================================
// Start of configuration constants.
//==============================================================================
//Interval between data records in microseconds.
const uint32_t LOG_INTERVAL_USEC = 150;
//------------------------------------------------------------------------------
// Pin definitions.
//
// SD chip select pin.
const uint8_t SD_CS_PIN = 53;
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 13;
//------------------------------------------------------------------------------
// File definitions.
//
// Maximum file size in blocks.
// The program creates a contiguous file with FILE_BLOCK_COUNT 512 byte blocks.
// This file is flash erased using special SD commands.  The file will be
// truncated if logging is stopped early.
const uint32_t FILE_BLOCK_COUNT = 256000;

// log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "DATA"
//------------------------------------------------------------------------------
// Buffer definitions.
//
// The logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional 
// buffers.
//
#ifndef RAMEND
// Assume ARM. Use total of nine 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 8;
//
#elif RAMEND < 0X8FF
#error Too little SRAM
//
#elif RAMEND < 0X10FF
// Use total of two 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 1;
//
#elif RAMEND < 0X20FF
// Use total of five 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 4;
//
#else  // RAMEND
// Use total of 13 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 12;
#endif  // RAMEND
//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if a reset or power failure occurs.
#define TMP_FILE_NAME "TMP_LOG.BIN"

// Size of file base name.  Must not be larger than six.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

SdFat sd;

SdBaseFile binFile;

char binName[13] = FILE_BASE_NAME "00.BIN";

// Number of data records in a block.
const uint16_t DATA_DIM = (512 - 4)/sizeof(data_t);

//Compute fill so block size is 512 bytes.  FILL_DIM may be zero.
const uint16_t FILL_DIM = 512 - 4 - DATA_DIM*sizeof(data_t);

struct block_t 
{
  uint16_t count;
  uint16_t overrun;
  data_t data[DATA_DIM];
  uint8_t fill[FILL_DIM];
};

const uint8_t QUEUE_DIM = BUFFER_BLOCK_COUNT + 2;

block_t* emptyQueue[QUEUE_DIM];
uint8_t emptyHead;
uint8_t emptyTail;

block_t* fullQueue[QUEUE_DIM];
uint8_t fullHead;
uint8_t fullTail;

// Advance queue index.
inline uint8_t queueNext(uint8_t ht) {return ht < (QUEUE_DIM - 1) ? ht + 1 : 0;}
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
void fatalBlink() 
{
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
// Convert binary file to CSV file.
void binaryToCsv() {
  uint8_t lastPct = 0;
  block_t block;
  uint32_t t0 = millis();
  uint32_t syncCluster = 0;
  SdFile csvFile;
  char csvName[13];
  
  if (!binFile.isOpen()) {
	Serial.println();
	Serial.println(F("No current binary file"));
	return;
  }
  binFile.rewind();
  // Create a new csvFile.
  strcpy(csvName, binName);
  strcpy_P(&csvName[BASE_NAME_SIZE + 3], PSTR("CSV"));

  if (!csvFile.open(csvName, O_WRITE | O_CREAT | O_TRUNC)) {
	error("open csvFile failed");  
  }
  Serial.println();
  Serial.print(F("Writing: "));
  Serial.print(csvName);
  Serial.println(F(" - type any character to stop"));
  printHeader(&csvFile);
  uint32_t tPct = millis();
  while (!Serial.available() && binFile.read(&block, 512) == 512) {
	uint16_t i;
	if (block.count == 0) break;
	if (block.overrun) {
	  csvFile.print(F("OVERRUN,"));
	  csvFile.println(block.overrun);
	}
	for (i = 0; i < block.count; i++) {
	  printData(&csvFile, &block.data[i]);
	}
	if (csvFile.curCluster() != syncCluster) {
	  csvFile.sync();
	  syncCluster = csvFile.curCluster();
	}
	if ((millis() - tPct) > 1000) {
	  uint8_t pct = binFile.curPosition()/(binFile.fileSize()/100);
	  if (pct != lastPct) {
		tPct = millis();
		lastPct = pct;
		Serial.print(pct, DEC);
		Serial.println('%');
	  }
	}
	if (Serial.available()) break;
  }
  csvFile.close();
  Serial.print(F("Done: "));
  Serial.print(0.001*(millis() - t0));
  Serial.println(F(" Seconds"));
}
//------------------------------------------------------------------------------
// read data file and check for overruns
void checkOverrun() {
  bool headerPrinted = false;
  block_t block;
  uint32_t bgnBlock, endBlock;
  uint32_t bn = 0;
  
  if (!binFile.isOpen()) {
	Serial.println();
	Serial.println(F("No current binary file"));
	return;
  }
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
	error("contiguousRange failed");
  }
  binFile.rewind();
  Serial.println();
  Serial.println(F("Checking overrun errors - type any character to stop"));
  while (binFile.read(&block, 512) == 512) {
	if (block.count == 0) break;
	if (block.overrun) {
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
	  Serial.println(block.overrun);
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
  block_t block;
  if (!binFile.isOpen()) {
	Serial.println();
	Serial.println(F("No current binary file"));
	return;
  }
  binFile.rewind();
  Serial.println();
  Serial.println(F("Type any character to stop"));
  delay(1000);
  printHeader(&Serial);
  while (!Serial.available() && binFile.read(&block , 512) == 512) {
	if (block.count == 0) break;
	if (block.overrun) {
	  Serial.print(F("OVERRUN,"));
	  Serial.println(block.overrun);
	}
	for (uint16_t i = 0; i < block.count; i++) {
	  printData(&Serial, &block.data[i]);
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
  block_t* curBlock = 0;
  Serial.println();
  
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
  Serial.println(F("Logging - type any character to stop"));
  // Wait for Serial Idle.
  Serial.flush();
  delay(10);
  uint32_t bn = 0;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overrun = 0;
  uint32_t overrunTotal = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;
  int32_t diff;
  // Start at a multiple of interval.
  uint32_t logTime = micros()/LOG_INTERVAL_USEC + 1;
  logTime *= LOG_INTERVAL_USEC;
  bool closeFile = false;
  while (1) {
	// Time for next data record.
	logTime += LOG_INTERVAL_USEC;
	if (Serial.available()) closeFile = true;
	
	if (closeFile) {
	   if (curBlock != 0 && curBlock->count >= 0) {
		// Put buffer in full queue.
		fullQueue[fullHead] = curBlock;
		fullHead = queueNext(fullHead);
		curBlock = 0;
	  }   
	} else {
	  if (curBlock == 0 && emptyTail != emptyHead) {
		curBlock = emptyQueue[emptyTail];
		emptyTail = queueNext(emptyTail);
		curBlock->count = 0;
		curBlock->overrun = overrun;
		overrun = 0;
	  }
	  do {
		diff = logTime - micros();
	  } while(diff > 0);
	  if (diff < -10) error("LOG_INTERVAL_USEC too small");
	  if (curBlock == 0) {
		overrun++;
	  } else {
		acquireData(&curBlock->data[curBlock->count++]);
		if (curBlock->count == DATA_DIM) {
		  fullQueue[fullHead] = curBlock;
		  fullHead = queueNext(fullHead);        
		  curBlock = 0;
		}
	  }
	}
	
	if (fullHead == fullTail) {
	  // Exit loop if done.
	  if (closeFile) break;
	} else if (!sd.card()->isBusy()) {
	  // Get address of block to write.
	  block_t* pBlock = fullQueue[fullTail];
	  fullTail = queueNext(fullTail);     
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
		overrunTotal += pBlock->overrun;
		if (ERROR_LED_PIN >= 0) {
		  digitalWrite(ERROR_LED_PIN, HIGH);
		}
	  }
	  // Move block to empty queue.
	  emptyQueue[emptyHead] = pBlock;
	  emptyHead = queueNext(emptyHead);
	  bn++;
	  if (bn == FILE_BLOCK_COUNT) {
		// File full so stop
		break;
	  }
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
  Serial.println(count);
  Serial.print(F("Samples/sec: "));
  Serial.println((1000.0)*count/(t1-t0));
  Serial.print(F("Overruns: "));
  Serial.println(overrunTotal);
  Serial.println(F("Done"));
}
//------------------------------------------------------------------------------
void setup(void) {
  if (ERROR_LED_PIN >= 0) {
	pinMode(ERROR_LED_PIN, OUTPUT);
  }
  Serial.begin(9600);
  
  Serial.print(F("FreeRam: "));
  Serial.println(FreeRam());
  Serial.print(F("Records/block: "));
  Serial.println(DATA_DIM);
  if (sizeof(block_t) != 512) error("Invalid block size");
  // initialize file system.
  if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) {
	sd.initErrorPrint();
	fatalBlink();
  }
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(255, 255, 255);
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
	analogReadResolution(Set_ADC); 
  // Настройка звукового генератора  
	AD9850.reset();                    //reset module
	delay(1000);
	AD9850.powerDown();                //set signal output to LOW
	AD9850.set_frequency(0,0,1000);    //set power=UP, phase=0, 1kHz frequency 

  ADC_MR |= 0x00000100 ; // ADC full speed

 if (analogInPin < A0) analogInPin += A0;
  ulChannel = g_APinDescription[analogInPin].ulADCChannelNumber ;
  adc_enable_channel( ADC, (adc_channel_num_t)ulChannel );   

}
//------------------------------------------------------------------------------
void loop(void) {
  // discard any input
  while (Serial.read() >= 0) {}
  Serial.println();
  Serial.println(F("type:"));
  Serial.println(F("c - convert file to CSV")); 
  Serial.println(F("d - dump data to Serial"));  
  Serial.println(F("e - overrun error details"));
  Serial.println(F("r - record data"));

  while(!Serial.available()) {}
  char c = tolower(Serial.read());
  
  // Discard extra Serial data.
  do {
	delay(10);
  } while (Serial.read() >= 0);
  
  if (ERROR_LED_PIN >= 0) {
	digitalWrite(ERROR_LED_PIN, LOW);
  }
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
