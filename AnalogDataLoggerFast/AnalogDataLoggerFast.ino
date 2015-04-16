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
#include "Wire.h"
#include <rtc_clock.h>



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

boolean default_colors = true;
uint8_t menu_redraw_required = 0;


//----------------------Конец  Настройки дисплея --------------------------------





//**************************  Меню прибора ***************************************

const int clockCenterX=119;
const int clockCenterY=119;
int oldsec=0;
char* str[] = {"MON","TUE","WED","THU","FRI","SAT","SUN"};
char* str_mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

RTC_clock rtc_clock(XTAL);

char* daynames[]={"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};


//-----------------------------------------------------------------------------------------------
//uint8_t sec = 0;       //Initialization time
//uint8_t min = 0;
//uint8_t hour = 0;
//uint8_t dow = 1;
//uint8_t date = 1;
//uint8_t mon = 1;
//uint16_t year = 14;
unsigned long timeF;
int flag_time = 0;

int hh,mm,ss,dow,dd,mon,yyyy;



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

 int x_kn = 30;  // Смещение кнопок по Х
int dgvh;
const int hpos = 95; //set 0v on horizontal  grid
int port = 0;
int x_osc,y_osc;
int Input = 0;
int Old_Input = 0;
int Sample[254];
int OldSample[254];
unsigned long LongFile = 0;
float StartSample = 0; 
float EndSample = 0;
//float V_koeff = 0.0488;
float koeff_h = 7.759;
int MaxAnalog = 0;
unsigned long SrednAnalog = 0;
unsigned long SrednCount = 0;
bool Set_x = false;
int Set_ADC = 10;
int MinAnalog = 500;
int mode = 0;

int mode1 = 0;             //Переключение чувствительности
int dTime = 1;
int tmode = 0;
int Trigger = 0;
int SampleSize = 0;
float SampleTime = 0;





 //***************** Назначение переменных для хранения текстов*****************************************************

char  txt_menu1_1[]          = "PE\x81\x86""CTPATOP";                                                       // "РЕГИСТРАТОР"
char  txt_menu1_2[]          = "CAMO\x89\x86""CE\x8C";                                                      // "САМОПИСЕЦ"
char  txt_menu1_3[]          = "PE\x81\x86""CT.+ CAMO\x89.";                                                // "РЕГИСТ. + САМОП."
char  txt_menu1_4[]          = "PA\x80OTA c SD";                                                            // "РАБОТА с SD"
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
char  txt_return[]           = "\x85""a\x97""ep\xA8\xA2\xA4\xAC \xA3poc\xA1o\xA4p";                         // Завершить просмотр

char  txt_ADC_menu1[]        = "Record data";                                                               //
char  txt_ADC_menu2[]        = "Convert to CSV";                                                            //
char  txt_ADC_menu3[]        = "Dump to Serial";                                                            //
char  txt_ADC_menu4[]        = "EXIT";                                                                      //

char  txt_osc_menu1[]        = "Oscilloscope";                                                              //
char  txt_osc_menu2[]        = "Oscill_Time";                                                                    //
char  txt_osc_menu3[]        = "Menu 3";                                                                    //
char  txt_osc_menu4[]        = "EXIT";           

char  txt_SD_menu1[]         = "View File";                                                                //
char  txt_SD_menu2[]         = "Info SD";                                                                  //
char  txt_SD_menu3[]         = "Format SD";                                                                //
char  txt_SD_menu4[]         = "EXIT";           

char  txt_info6[]             = "Info: ";                                //Info: 
char  txt_info7[]             = "Writing:"; 
char  txt_info8[]             = "%"; 
char  txt_info9[]             = "Done: "; 
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
  rtc_clock.get_time(&hh,&mm,&ss);
  rtc_clock.get_date(&dow,&dd,&mon,&yyyy);

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(yyyy, mon, dd);

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hh, mm, ss);
}



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
//#define ADC_CDR0 * (volatile unsigned int *) (0x400C0050) /*ADC Channel Только чтение */
//#define ADC_ISR_EOC0 0x00000001


uint32_t ulChannel;

//------------------------------------------------------------------------------
// Analog pin number list for a sample. 
int Channel_0 = 1;
int Channel_1 = 0;
int Channel_2 = 0;
int Channel_3 = 0;
int Channel_x = 0;
int count_pin = 0;
int set_strob = 100;

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
uint32_t const ERASE_SIZE = 262144L;
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
//+++++++++++++++++++++++ SD info ++++++++++++++++++++++++++
Sd2Card card;
SdVolume vol;

// serial output steam
ArduinoOutStream cout(Serial);


// global for card size
uint32_t cardSize;

// global for card erase size
uint32_t eraseSize;

//++++++++++++++++++ SD Format ++++++++++++++++++++++++++++++++++
const uint8_t spiSpeed = SPI_HALF_SPEED;
uint32_t cardSizeBlocks;
uint16_t cardCapacityMB;

// cache for SD block
cache_t cache;

// MBR information
uint8_t partType;
uint32_t relSector;
uint32_t partSize;

// Fake disk geometry
uint8_t numberOfHeads;
uint8_t sectorsPerTrack;

// FAT parameters
uint16_t reservedSectors;
uint8_t sectorsPerCluster;
uint32_t fatStart;
uint32_t fatSize;
uint32_t dataStart;

// constants for file system structure
uint16_t const BU16 = 128;
uint16_t const BU32 = 8192;

//  strings needed in file system structures
char noName[] = "NO NAME    ";
char fat16str[] = "FAT16   ";
char fat32str[] = "FAT32   ";
//------------------------------------------------------------------------------
#define sdError(msg) sdError_P(PSTR(msg))

void sdError_P(const char* str) {
  cout << pstr("error: ");
  cout << pgm(str) << endl;
  if (card.errorCode()) {
	cout << pstr("SD error: ") << hex << int(card.errorCode());
	cout << ',' << int(card.errorData()) << dec << endl;
  }
  while (1);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if DEBUG_PRINT
void debugPrint() {
  cout << pstr("FreeRam: ") << FreeRam() << endl;
  cout << pstr("partStart: ") << relSector << endl;
  cout << pstr("partSize: ") << partSize << endl;
  cout << pstr("reserved: ") << reservedSectors << endl;
  cout << pstr("fatStart: ") << fatStart << endl;
  cout << pstr("fatSize: ") << fatSize << endl;
  cout << pstr("dataStart: ") << dataStart << endl;
  cout << pstr("clusterCount: ");
  cout << ((relSector + partSize - dataStart)/sectorsPerCluster) << endl;
  cout << endl;
  cout << pstr("Heads: ") << int(numberOfHeads) << endl;
  cout << pstr("Sectors: ") << int(sectorsPerTrack) << endl;
  cout << pstr("Cylinders: ");
  cout << cardSizeBlocks/(numberOfHeads*sectorsPerTrack) << endl;
}
#endif  // DEBUG_PRINT
//------------------------------------------------------------------------------
// write cached block to the card
uint8_t writeCache(uint32_t lbn) 
{
  return card.writeBlock(lbn, cache.data);
}
//------------------------------------------------------------------------------
// initialize appropriate sizes for SD capacity
void initSizes() {
  if (cardCapacityMB <= 6) {
	sdError("Card is too small.");
  } else if (cardCapacityMB <= 16) {
	sectorsPerCluster = 2;
  } else if (cardCapacityMB <= 32) {
	sectorsPerCluster = 4;
  } else if (cardCapacityMB <= 64) {
	sectorsPerCluster = 8;
  } else if (cardCapacityMB <= 128) {
	sectorsPerCluster = 16;
  } else if (cardCapacityMB <= 1024) {
	sectorsPerCluster = 32;
  } else if (cardCapacityMB <= 32768) {
	sectorsPerCluster = 64;
  } else {
	// SDXC cards
	sectorsPerCluster = 128;
  }

  cout << pstr("Blocks/Cluster: ") << int(sectorsPerCluster) << endl;
  // set fake disk geometry
  sectorsPerTrack = cardCapacityMB <= 256 ? 32 : 63;

  if (cardCapacityMB <= 16) {
	numberOfHeads = 2;
  } else if (cardCapacityMB <= 32) {
	numberOfHeads = 4;
  } else if (cardCapacityMB <= 128) {
	numberOfHeads = 8;
  } else if (cardCapacityMB <= 504) {
	numberOfHeads = 16;
  } else if (cardCapacityMB <= 1008) {
	numberOfHeads = 32;
  } else if (cardCapacityMB <= 2016) {
	numberOfHeads = 64;
  } else if (cardCapacityMB <= 4032) {
	numberOfHeads = 128;
  } else {
	numberOfHeads = 255;
  }
}
//------------------------------------------------------------------------------
// zero cache and optionally set the sector signature
void clearCache(uint8_t addSig) {
  memset(&cache, 0, sizeof(cache));
  if (addSig) {
	cache.mbr.mbrSig0 = BOOTSIG0;
	cache.mbr.mbrSig1 = BOOTSIG1;
  }
}
//------------------------------------------------------------------------------
// zero FAT and root dir area on SD
void clearFatDir(uint32_t bgn, uint32_t count) {
  clearCache(false);
  if (!card.writeStart(bgn, count)) {
	sdError("Clear FAT/DIR writeStart failed");
  }
  for (uint32_t i = 0; i < count; i++) {
	if ((i & 0XFF) == 0) cout << '.';
	if (!card.writeData(cache.data)) {
	  sdError("Clear FAT/DIR writeData failed");
	}
  }
  if (!card.writeStop()) {
	sdError("Clear FAT/DIR writeStop failed");
  }
  cout << endl;
}
//------------------------------------------------------------------------------
// return cylinder number for a logical block number
uint16_t lbnToCylinder(uint32_t lbn) {
  return lbn / (numberOfHeads * sectorsPerTrack);
}
//------------------------------------------------------------------------------
// return head number for a logical block number
uint8_t lbnToHead(uint32_t lbn) {
  return (lbn % (numberOfHeads * sectorsPerTrack)) / sectorsPerTrack;
}
//------------------------------------------------------------------------------
// return sector number for a logical block number
uint8_t lbnToSector(uint32_t lbn) {
  return (lbn % sectorsPerTrack) + 1;
}
//------------------------------------------------------------------------------
// format and write the Master Boot Record
void writeMbr() 
{
  clearCache(true);
  part_t* p = cache.mbr.part;
  p->boot = 0;
  uint16_t c = lbnToCylinder(relSector);
  if (c > 1023) sdError("MBR CHS");
  p->beginCylinderHigh = c >> 8;
  p->beginCylinderLow = c & 0XFF;
  p->beginHead = lbnToHead(relSector);
  p->beginSector = lbnToSector(relSector);
  p->type = partType;
  uint32_t endLbn = relSector + partSize - 1;
  c = lbnToCylinder(endLbn);
  if (c <= 1023) {
	p->endCylinderHigh = c >> 8;
	p->endCylinderLow = c & 0XFF;
	p->endHead = lbnToHead(endLbn);
	p->endSector = lbnToSector(endLbn);
  } else {
	// Too big flag, c = 1023, h = 254, s = 63
	p->endCylinderHigh = 3;
	p->endCylinderLow = 255;
	p->endHead = 254;
	p->endSector = 63;
  }
  p->firstSector = relSector;
  p->totalSectors = partSize;
  if (!writeCache(0)) sdError("write MBR");
}
//------------------------------------------------------------------------------
// generate serial number from card size and micros since boot
uint32_t volSerialNumber() 
{
  return (cardSizeBlocks << 8) + micros();
}
//------------------------------------------------------------------------------
// format the SD as FAT16
void makeFat16() 
{
  uint32_t nc;
  for (dataStart = 2 * BU16;; dataStart += BU16) 
  {
	nc = (cardSizeBlocks - dataStart)/sectorsPerCluster;
	fatSize = (nc + 2 + 255)/256;
	uint32_t r = BU16 + 1 + 2 * fatSize + 32;
	if (dataStart < r) continue;
	relSector = dataStart - r + BU16;
	break;
  }
  // check valid cluster count for FAT16 volume
  if (nc < 4085 || nc >= 65525) sdError("Bad cluster count");
  reservedSectors = 1;
  fatStart = relSector + reservedSectors;
  partSize = nc * sectorsPerCluster + 2 * fatSize + reservedSectors + 32;
  if (partSize < 32680) {
	partType = 0X01;
  } else if (partSize < 65536) {
	partType = 0X04;
  } else {
	partType = 0X06;
  }
  // write MBR
  writeMbr();
  clearCache(true);
  fat_boot_t* pb = &cache.fbs;
  pb->jump[0] = 0XEB;
  pb->jump[1] = 0X00;
  pb->jump[2] = 0X90;
  for (uint8_t i = 0; i < sizeof(pb->oemId); i++) {
	pb->oemId[i] = ' ';
  }
  pb->bytesPerSector = 512;
  pb->sectorsPerCluster = sectorsPerCluster;
  pb->reservedSectorCount = reservedSectors;
  pb->fatCount = 2;
  pb->rootDirEntryCount = 512;
  pb->mediaType = 0XF8;
  pb->sectorsPerFat16 = fatSize;
  pb->sectorsPerTrack = sectorsPerTrack;
  pb->headCount = numberOfHeads;
  pb->hidddenSectors = relSector;
  pb->totalSectors32 = partSize;
  pb->driveNumber = 0X80;
  pb->bootSignature = EXTENDED_BOOT_SIG;
  pb->volumeSerialNumber = volSerialNumber();
  memcpy(pb->volumeLabel, noName, sizeof(pb->volumeLabel));
  memcpy(pb->fileSystemType, fat16str, sizeof(pb->fileSystemType));
  // write partition boot sector
  if (!writeCache(relSector)) {
	sdError("FAT16 write PBS failed");
  }
  // clear FAT and root directory
  clearFatDir(fatStart, dataStart - fatStart);
  clearCache(false);
  cache.fat16[0] = 0XFFF8;
  cache.fat16[1] = 0XFFFF;
  // write first block of FAT and backup for reserved clusters
  if (!writeCache(fatStart)
	|| !writeCache(fatStart + fatSize)) {
	sdError("FAT16 reserve failed");
  }
}
//------------------------------------------------------------------------------
// format the SD as FAT32
void makeFat32() 
{
  uint32_t nc;
  relSector = BU32;
  for (dataStart = 2 * BU32;; dataStart += BU32) {
	nc = (cardSizeBlocks - dataStart)/sectorsPerCluster;
	fatSize = (nc + 2 + 127)/128;
	uint32_t r = relSector + 9 + 2 * fatSize;
	if (dataStart >= r) break;
  }
  // error if too few clusters in FAT32 volume
  if (nc < 65525) sdError("Bad cluster count");
  reservedSectors = dataStart - relSector - 2 * fatSize;
  fatStart = relSector + reservedSectors;
  partSize = nc * sectorsPerCluster + dataStart - relSector;
  // type depends on address of end sector
  // max CHS has lbn = 16450560 = 1024*255*63
  if ((relSector + partSize) <= 16450560) {
	// FAT32
	partType = 0X0B;
  } else {
	// FAT32 with INT 13
	partType = 0X0C;
  }
  writeMbr();
  clearCache(true);

  fat32_boot_t* pb = &cache.fbs32;
  pb->jump[0] = 0XEB;
  pb->jump[1] = 0X00;
  pb->jump[2] = 0X90;
  for (uint8_t i = 0; i < sizeof(pb->oemId); i++) {
	pb->oemId[i] = ' ';
  }
  pb->bytesPerSector = 512;
  pb->sectorsPerCluster = sectorsPerCluster;
  pb->reservedSectorCount = reservedSectors;
  pb->fatCount = 2;
  pb->mediaType = 0XF8;
  pb->sectorsPerTrack = sectorsPerTrack;
  pb->headCount = numberOfHeads;
  pb->hidddenSectors = relSector;
  pb->totalSectors32 = partSize;
  pb->sectorsPerFat32 = fatSize;
  pb->fat32RootCluster = 2;
  pb->fat32FSInfo = 1;
  pb->fat32BackBootBlock = 6;
  pb->driveNumber = 0X80;
  pb->bootSignature = EXTENDED_BOOT_SIG;
  pb->volumeSerialNumber = volSerialNumber();
  memcpy(pb->volumeLabel, noName, sizeof(pb->volumeLabel));
  memcpy(pb->fileSystemType, fat32str, sizeof(pb->fileSystemType));
  // write partition boot sector and backup
  if (!writeCache(relSector)
	|| !writeCache(relSector + 6)) {
	sdError("FAT32 write PBS failed");
  }
  clearCache(true);
  // write extra boot area and backup
  if (!writeCache(relSector + 2)
	|| !writeCache(relSector + 8)) {
	sdError("FAT32 PBS ext failed");
  }
  fat32_fsinfo_t* pf = &cache.fsinfo;
  pf->leadSignature = FSINFO_LEAD_SIG;
  pf->structSignature = FSINFO_STRUCT_SIG;
  pf->freeCount = 0XFFFFFFFF;
  pf->nextFree = 0XFFFFFFFF;
  // write FSINFO sector and backup
  if (!writeCache(relSector + 1)
	|| !writeCache(relSector + 7)) {
	sdError("FAT32 FSINFO failed");
  }
  clearFatDir(fatStart, 2 * fatSize + sectorsPerCluster);
  clearCache(false);
  cache.fat32[0] = 0x0FFFFFF8;
  cache.fat32[1] = 0x0FFFFFFF;
  cache.fat32[2] = 0x0FFFFFFF;
  // write first block of FAT and backup for reserved clusters
  if (!writeCache(fatStart)
	|| !writeCache(fatStart + fatSize)) {
	sdError("FAT32 reserve failed");
  }
}
//------------------------------------------------------------------------------
// flash erase all data
//uint32_t const ERASE_SIZE = 262144L;
void eraseCard() 
{
  cout << endl << pstr("Erasing\n");
  uint32_t firstBlock = 0;
  uint32_t lastBlock;
  uint16_t n = 0;

  do {
	lastBlock = firstBlock + ERASE_SIZE - 1;
	if (lastBlock >= cardSizeBlocks) lastBlock = cardSizeBlocks - 1;
	if (!card.erase(firstBlock, lastBlock)) sdError("erase failed");
	cout << '.';
	if ((n++)%32 == 31) cout << endl;
	firstBlock += ERASE_SIZE;
  } while (firstBlock < cardSizeBlocks);
  cout << endl;

  if (!card.readBlock(0, cache.data)) sdError("readBlock");
  cout << hex << showbase << setfill('0') << internal;
  cout << pstr("All data set to ") << setw(4) << int(cache.data[0]) << endl;
  cout << dec << noshowbase << setfill(' ') << right;
  cout << pstr("Erase done\n");
}
//------------------------------------------------------------------------------
void formatCard() 
{
  cout << endl;
  cout << pstr("Formatting\n");
  initSizes();
  if (card.type() != SD_CARD_TYPE_SDHC) {
	cout << pstr("FAT16\n");
	makeFat16();
  } else {
	cout << pstr("FAT32\n");
	makeFat32();
  }
#if DEBUG_PRINT
  debugPrint();
#endif  // DEBUG_PRINT
  cout << pstr("Format done\n");
}

//------------------------------------------------------------------------------
// store error strings in flash
#define sdErrorMsg(msg) sdErrorMsg_P(PSTR(msg));
void sdErrorMsg_P(const char* str) 
{
  cout << pgm(str) << endl;
  if (card.errorCode()) {
	cout << pstr("SD errorCode: ");
	cout << hex << int(card.errorCode()) << endl;
	cout << pstr("SD errorData: ");
	cout << int(card.errorData()) << dec << endl;
  }
}
//------------------------------------------------------------------------------
uint8_t cidDmp() 
{
  cid_t cid;
  if (!card.readCID(&cid)) 
  {
	sdErrorMsg("readCID failed");
	return false;
  }
  cout << pstr("\nManufacturer ID: ");
  cout << hex << int(cid.mid) << dec << endl;
  cout << pstr("OEM ID: ") << cid.oid[0] << cid.oid[1] << endl;
  cout << pstr("Product: ");
  for (uint8_t i = 0; i < 5; i++) 
  {
	cout << cid.pnm[i];
  }
  cout << pstr("\nVersion: ");
  cout << int(cid.prv_n) << '.' << int(cid.prv_m) << endl;
  cout << pstr("Serial number: ") << hex << cid.psn << dec << endl;
  cout << pstr("Manufacturing date: ");
  cout << int(cid.mdt_month) << '/';
  cout << (2000 + cid.mdt_year_low + 10 * cid.mdt_year_high) << endl;
  cout << endl;
  return true;
}
//------------------------------------------------------------------------------
uint8_t csdDmp() {
  csd_t csd;
  uint8_t eraseSingleBlock;
  if (!card.readCSD(&csd)) {
	sdErrorMsg("readCSD failed");
	return false;
  }
  if (csd.v1.csd_ver == 0) {
	eraseSingleBlock = csd.v1.erase_blk_en;
	eraseSize = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
  } else if (csd.v2.csd_ver == 1) {
	eraseSingleBlock = csd.v2.erase_blk_en;
	eraseSize = (csd.v2.sector_size_high << 1) | csd.v2.sector_size_low;
  } else {
	cout << pstr("csd version error\n");
	return false;
  }
  eraseSize++;
  cout << pstr("cardSize: ") << 0.000512*cardSize;
  cout << pstr(" MB (MB = 1,000,000 bytes)\n");

  cout << pstr("flashEraseSize: ") << int(eraseSize) << pstr(" blocks\n");
  cout << pstr("eraseSingleBlock: ");
  if (eraseSingleBlock) {
	cout << pstr("true\n");
  } else {
	cout << pstr("false\n");
  }
  return true;
}
//------------------------------------------------------------------------------
// print partition table
uint8_t partDmp() {
  cache_t *p = vol.cacheClear();
  if (!p) {
	sdErrorMsg("cacheClear failed");
	return false;
  }
  if (!card.readBlock(0, p->data)) {
	  sdErrorMsg("read MBR failed");
	  return false;
  }
  for (uint8_t ip = 1; ip < 5; ip++) {
	part_t *pt = &p->mbr.part[ip - 1];
	if ((pt->boot & 0X7F) != 0 || pt->firstSector > cardSize) {
	  cout << pstr("\nNo MBR. Assuming Super Floppy format.\n");
	  return true;
	}
  }
  cout << pstr("\nSD Partition Table\n");
  cout << pstr("part,boot,type,start,length\n");
  for (uint8_t ip = 1; ip < 5; ip++) {
	part_t *pt = &p->mbr.part[ip - 1];
	cout << int(ip) << ',' << hex << int(pt->boot) << ',' << int(pt->type);
	cout << dec << ',' << pt->firstSector <<',' << pt->totalSectors << endl;
  }
  return true;
}
//------------------------------------------------------------------------------
void volDmp() {
  cout << pstr("\nVolume is FAT") << int(vol.fatType()) << endl;
  cout << pstr("blocksPerCluster: ") << int(vol.blocksPerCluster()) << endl;
  cout << pstr("clusterCount: ") << vol.clusterCount() << endl;
  uint32_t volFree = vol.freeClusterCount();
  cout << pstr("freeClusters: ") <<  volFree << endl;
  float fs = 0.000512*volFree*vol.blocksPerCluster();
  cout << pstr("freeSpace: ") << fs << pstr(" MB (MB = 1,000,000 bytes)\n");
  cout << pstr("fatStartBlock: ") << vol.fatStartBlock() << endl;
  cout << pstr("fatCount: ") << int(vol.fatCount()) << endl;
  cout << pstr("blocksPerFat: ") << vol.blocksPerFat() << endl;
  cout << pstr("rootDirStart: ") << vol.rootDirStart() << endl;
  cout << pstr("dataStartBlock: ") << vol.dataStartBlock() << endl;
  if (vol.dataStartBlock() % eraseSize) {
	cout << pstr("Data area is not aligned on flash erase boundaries!\n");
	cout << pstr("Download and use formatter from www.sdcard.org/consumer!\n");
  }
}

//------------------------------------------------------------------------------

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
	  Serial.println(F("ADC bits: 12 "));
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
  float intervalMicros = set_strob;
  csvStream.print(intervalMicros, 4);
  csvStream.println(F(",usec"));
 //  Serial.println(F("Head 0 "));
  for (uint8_t i = 0; i < pm->pinCount; i++) 
  {
	if (i) csvStream.putc(',');
	csvStream.print(F("pin"));
	csvStream.print(pm->pinNumber[i]);
  }
//  Serial.println(F("Head 1 "));
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
//uint32_t const ERASE_SIZE = 262144L;
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
		myGLCD.print(txt_info13,LEFT, 135);              //
		if (!sd.remove(TMP_FILE_NAME)) 
			{
			  error("Can't remove tmp file");
			}
	  }
  // Create new file.
  Serial.println(F("Creating new file"));
  myGLCD.print(txt_info27,LEFT, 155);//
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
  myGLCD.print(txt_info14,LEFT, 175);  //Erasing all data
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
  myGLCD.setColor(VGA_LIME);
  myGLCD.print(txt_info15, CENTER, 200);
  // Wait for Serial Idle.
  Serial.flush();
  delay(10);
  uint32_t bn = 1;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;
  myGLCD.setColor(255,150,50);
  myGLCD.print("                    ", CENTER, 135);
  myGLCD.print(txt_info28, CENTER, 135);
  // Start logging interrupts.
  overruns = 0;
   Timer3.start(set_strob);

  while (1) 
	  {
		if (fullHead != fullTail) 
			{
			  // Get address of block to write.  Получить адрес блока, чтобы написать
			  block_t* pBlock = fullQueue[fullTail];
	  
			  // Write block to SD.  Написать блок SD
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
		if (myTouch.dataAvailable()) 
			{
				myGLCD.setColor(VGA_YELLOW);
				myGLCD.print("                    ", CENTER, 135);
				myGLCD.print("Stop record", CENTER, 135);
				myGLCD.setColor(255, 255, 255);
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
			  	while (myTouch.dataAvailable()){}
				delay(1000);
				break;
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
  //Serial.print(F("File renamed: "));
  //Serial.println(binName);
  //Serial.print(F("Max block write usec: "));
  //Serial.println(maxLatency);
  //Serial.print(F("Record time sec: "));
  //Serial.println(0.001*(t1 - t0), 3);
  //Serial.print(F("Sample count: "));
  //Serial.println(count/count_pin);
  //Serial.print(F("Samples/sec: "));
  //Serial.println((1000.0/count_pin)*count/(t1-t0));
  //Serial.print(F("Overruns: "));
  //Serial.println(overruns);
  //Serial.println(F("Done"));
  //delay(100);
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
	myGLCD.printNumI(count, RIGHT, 85);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info20,LEFT, 105);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumF((1000.0)*count/(t1-t0),2, RIGHT, 105);// 
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

void chench_analog()
{
	//Подготовка номера аналогового сигнала, количества каналов и кода настройки АЦП
		   Channel_x = 0;
		   count_pin = 0;
	 
		if (Channel_0 == 1 )
			{
				Channel_x|=0x80;
				count_pin++;
			}
		if (Channel_1 == 1 )
			{
				Channel_x|=0x40;
				count_pin++;
			}
		
		if (Channel_2 == 1 ) 
			{
				Channel_x|=0x20;
				count_pin++;
			}

		if (Channel_3 == 1 ) 
			{
				Channel_x|=0x10;
				count_pin++;
			}

		 SAMPLES_PER_BLOCK = DATA_DIM16/count_pin;
}

//************************** Аналоговые часы ************************************
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

  rtc_clock.get_time(&hh,&mm,&ss);
  rtc_clock.get_date(&dow,&dd,&mon,&yyyy);

 // clock_read();
  drawMin(mm);
  drawHour(hh, mm);
  drawSec(ss);
  oldsec=ss;

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
	rtc_clock.get_time(&hh,&mm,&ss);
	rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
	myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.print(str[dow-1], 256, 8);
  if (dd<10)
	myGLCD.printNumI(dd, 272, 28);
  else
	myGLCD.printNumI(dd, 264, 28);

  myGLCD.print(str_mon[mon-1], 256, 48);
  myGLCD.printNumI(yyyy, 248, 65);
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
   rtc_clock.get_time(&hh,&mm,&ss);
   rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
  //clock_read();
  
  while (true)
  {

	if (oldsec!=ss)
	{
	  if ((ss==0) and (mm==0) and (hh==0))
	  {
		clearDate();
		printDate();
	  }
	  if (ss==0)
	  {
		drawMin(mm);
		drawHour(hh, mm);
	  }
	  drawSec(ss);
	  oldsec=ss;
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
		setClockRTC();
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
	  rtc_clock.get_time(&hh,&mm,&ss);
	  rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
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
							bailout31: // Восстановить пункты меню
							myGLCD.clrScr();
							myButtons.drawButtons();
							print_up();
					   }
				   if (pressed_button==but4 && m2 == 1)
					   {
							Draw_menu_SD();
							menu_SD();
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
//++++++++++++++++++++++++++ Конец меню прибора ++++++++++++++++++++++++
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
							oscilloscope();
							Draw_menu_Osc();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							myGLCD.clrScr();
							oscilloscope_time();
							Draw_menu_Osc();
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
							myGLCD.clrScr();
							DrawGrid();
							buttons();
						//	dumpData_Osc();
							Draw_menu_Osc();
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
void trigger()
{
	do
	 {
	   Input = analogRead(port);   //
	  // Old_Input = Input;
	 }  while (Input<Trigger); 
	 do
	 {
		Input = analogRead(port); //
	 }  while (Input>Trigger);        //Input < Trigger
}
void print_set()
{
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255,255);
	myGLCD.print("Delay", 260, 5);
	myGLCD.print("     ", 270, 20);
	myGLCD.printNumI(dTime, 270, 20);
	myGLCD.print("Trig.", 265, 50);
	myGLCD.print("    ", 265, 65);
	if (tmode == 0)myGLCD.print(" 0% ", 268, 65);
	if (tmode == 1)myGLCD.print("50%", 266, 65);
	if (tmode == 2)myGLCD.print("100%", 266, 65);

	SampleTime =( EndSample/1000-StartSample/1000);
	myGLCD.print("mSec.", 260, 140);
	myGLCD.print("      ", 260, 160);
	myGLCD.printNumF(SampleTime, 1, 260, 160);
	myGLCD.print("V/del.", 260, 95);
	myGLCD.print("      ", 260, 110);
	if (mode1 == 0)myGLCD.print("1", 275, 110);
	if (mode1 == 1)myGLCD.print("0.5", 268, 110);
	if (mode1 == 2)myGLCD.print("0.2", 268, 110);
	if (mode1 == 3)myGLCD.print("0.1", 268, 110);
}
void print_set1()
{
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255,255);
	myGLCD.print("Delay", 260, 5);
	myGLCD.print("     ", 260, 20);
	if (mode == 0)myGLCD.print("1min", 265, 20);
	if (mode == 1)myGLCD.print("6min", 265, 20);
	if (mode == 2)myGLCD.print("12min", 260, 20);
	if (mode == 3)myGLCD.print("18min", 260, 20);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("0",1, 163);
	if (mode == 0)
	{
		myGLCD.print("10", 35, 163);
		myGLCD.print("20", 75, 163);
		myGLCD.print("30", 115, 163);
		myGLCD.print("40", 155, 163);
		myGLCD.print("50", 195, 163);
		myGLCD.print("60", 233, 163);
	}
	if (mode == 1)
	{
		myGLCD.print("1 ", 38, 163);
		myGLCD.print("2 ", 78, 163);
		myGLCD.print("3 ", 118, 163);
		myGLCD.print("4 ", 158, 163);
		myGLCD.print("5 ", 198, 163);
		myGLCD.print("6 ", 233, 163);
	}
	if (mode == 2)
	{
		myGLCD.print("2 ", 38, 163);
		myGLCD.print("4 ", 78, 163);
		myGLCD.print("6 ", 118, 163);
		myGLCD.print("8 ", 158, 163);
		myGLCD.print("10", 195, 163);
		myGLCD.print("12", 233, 163);
	}

	if (mode == 3)
	{
		myGLCD.print("3 ", 38, 163);
		myGLCD.print("6 ", 78, 163);
		myGLCD.print("9 ", 118, 163);
		myGLCD.print("12 ", 155, 163);
		myGLCD.print("15", 195, 163);
		myGLCD.print("18", 233, 163);
	}

	myGLCD.setBackColor( 0, 0, 255);

	if(Set_x == true)
	{
	   myGLCD.print("V Max", 265, 50);
	   myGLCD.print(" /x  ", 265, 65);
	}
	else
	{
	   myGLCD.print("V Max", 265, 50);
	   myGLCD.print("     ", 265, 65);
	}

	myGLCD.print("V/del.", 260, 95);
	myGLCD.print("      ", 260, 110);
	if (mode1 == 0)myGLCD.print("1", 275, 110);
	if (mode1 == 1)myGLCD.print("0.5", 268, 110);
	if (mode1 == 2)myGLCD.print("0.2", 268, 110);
	if (mode1 == 3)myGLCD.print("0.1", 268, 110);

	myGLCD.print("Port", 265, 140);
	myGLCD.printNumI(port, 275, 156);

	myGLCD.print(" ADC ", 262, 185);
	myGLCD.printNumI(Set_ADC, 272, 202);
}
void oscilloscope()
{
	uint32_t bgnBlock, endBlock;
	block_t block[BUFFER_BLOCK_COUNT];
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	delay(500);
	myGLCD.clrScr();
	buttons();
	int xpos;
	int ypos1;
	int ypos2;
	print_set();

	while(1) 
	{
		 DrawGrid();
		 if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();

				if ((x_osc>=2) && (x_osc<=240))  //  Delay Button
					{
						if ((y_osc>=1) && (y_osc<=160))  // Delay row
						{
							break;
						} 
					}

				myGLCD.setBackColor( 0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.setColor (255, 255,255);
				myGLCD.drawRoundRect (250, 1, 310, 40);
				myGLCD.drawRoundRect (250, 45, 310, 85);
				myGLCD.drawRoundRect (250, 90, 310, 130);
				myGLCD.drawRoundRect (250, 135, 310, 175);
			if ((x_osc>=250) && (x_osc<=310))  //  Delay Button
			  {
				  if ((y_osc>=1) && (y_osc<=40))  // Delay row
				  {
					waitForIt(250, 1, 310, 40);
					mode ++ ;
					if (mode > 10) mode = 0;   
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
					print_set();
				  }
			 if ((y_osc>=45) && (y_osc<=85))  // Trigger  row
				 {
					waitForIt(250, 45, 310, 85);
					tmode ++;
					if (tmode > 2)tmode = 0;
					if (tmode == 0) Trigger = MinAnalog;
					if (tmode == 1) Trigger = MaxAnalog/2;//;
					if (tmode == 2) Trigger = MaxAnalog-10;//20;
					print_set();
				 }
			 if ((y_osc>=90) && (y_osc<=130))  // Port select   row
				 {
					waitForIt(250, 90, 310, 130);
					mode1 ++ ;
					myGLCD.clrScr();
					buttons();
					if (mode1 > 3) mode1 = 0;   
					if (mode1 == 0) koeff_h = 7.759;
					if (mode1 == 1) koeff_h = 3.879;
					if (mode1 == 2) koeff_h = 1.939;
					if (mode1 == 3) koeff_h = 0.969;
					print_set();
				 }
			 if ((y_osc>=135) && (y_osc<=175))  // Port select   row
				 {
					waitForIt(250, 135, 310, 175);
					//break;
				 }
		   }
				
			 if ((y_osc>=205) && (y_osc<=239))  //  Delay Button
					{
						 touch_osc();
					}
		}
	
		 trigger();

		// Записать аналоговый сигнал в блок памяти
		StartSample = micros();
		for( xpos = 0;	xpos < 240; xpos ++) 
			{
				Sample[xpos] = analogRead(port);    //
				MaxAnalog = max(MaxAnalog, Sample[xpos]);
				MinAnalog = min(MinAnalog, Sample[xpos]);
				delayMicroseconds(dTime); //dTime
			}
		EndSample = micros();
		DrawGrid();
  
		// Display the collected analog data from array
		for( int xpos = 0; xpos < 239;	xpos ++)
			{
				// Erase previous display Стереть предыдущий экран
				myGLCD.setColor( 0, 0, 0);
				ypos1 = 255-(OldSample[ xpos + 1]/koeff_h) - hpos; 
				ypos2 = 255-(OldSample[ xpos + 2]/koeff_h) - hpos;

				if(ypos1<0) ypos1 = 0;
				if(ypos2<0) ypos2 = 0;
				if(ypos1>220) ypos1 = 220;
				if(ypos2>220) ypos2 = 220;
				myGLCD.drawLine (xpos + 1, ypos1, xpos + 2, ypos2);
				if (xpos == 0) myGLCD.drawLine (xpos + 1, 1, xpos + 1, 220);
				//Draw the new data
				myGLCD.setColor( 255, 255, 255);
				ypos1 = 255-(Sample[ xpos]/koeff_h) - hpos;
				ypos2 = 255-(Sample[ xpos + 1]/koeff_h)- hpos;

				if(ypos1<0) ypos1 = 0;
				if(ypos2<0) ypos2 = 0;
				if(ypos1>220) ypos1 = 220;
				if(ypos2>220) ypos2 = 220;
				myGLCD.drawLine (xpos, ypos1, xpos + 1, ypos2);
				OldSample[xpos] = Sample[ xpos];
			}
	}
koeff_h = 7.759;
mode1 = 0;
Trigger = 0;
myGLCD.setFont( BigFont);
while (myTouch.dataAvailable()){}
}
void oscilloscope_time()
{
	uint32_t bgnBlock, endBlock;
	block_t block[BUFFER_BLOCK_COUNT];
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	delay(500);
	myGLCD.clrScr();
	buttons1();
	int xpos;
	int ypos1;
	int ypos2;
	int sec_osc = 0;
	int min_osc = 0;
	print_set1();
	uint32_t logTime;
	uint32_t SAMPLE_INTERVAL_MS = 250;
	int32_t diff;
	for( xpos = 0; xpos < 239;	xpos ++)

	{
		OldSample[xpos] = 0;
	}
	DrawGrid1();
	while(1) 
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();

				if ((x_osc>=2) && (x_osc<=240))  //  Delay Button
					{
						if ((y_osc>=1) && (y_osc<=160))  // Delay row
						{
							waitForIt(2, 1, 240, 160);
							break;
						} 
					}

				myGLCD.setBackColor( 0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.setColor (255, 255,255);
				myGLCD.drawRoundRect (250, 1, 310, 40);
				myGLCD.drawRoundRect (250, 45, 310, 85);
				myGLCD.drawRoundRect (250, 90, 310, 130);
				myGLCD.drawRoundRect (250, 135, 310, 175);
				myGLCD.drawRoundRect (250, 180, 310, 220);
			if ((x_osc>=250) && (x_osc<=310))               //  Delay Button
			  {
				  if ((y_osc>=1) && (y_osc<=40))            // Delay row
					  {
						waitForIt(250, 1, 310, 40);
						mode ++ ;
						if (mode > 3) mode = 0;   
						if (mode == 0) SAMPLE_INTERVAL_MS = 250;
						if (mode == 1) SAMPLE_INTERVAL_MS = 1500;
						if (mode == 2) SAMPLE_INTERVAL_MS = 3000;
						if (mode == 3) SAMPLE_INTERVAL_MS = 4500;
						print_set1();
					  }

				if ((y_osc>=45) && (y_osc<=85))  // Trigger  row
					 {
						waitForIt(250, 45, 310, 85);
						if(Set_x == true) 
						{
							 Set_x = false;
						}
						else
						{
							Set_x = true;
						}
						print_set1();
					 }

				if ((y_osc>=90) && (y_osc<=130))  // Port select   row
					{
					waitForIt(250, 90, 310, 130);
					mode1 ++ ;
					if (mode1 > 3) mode1 = 0;   
					if (mode1 == 0) koeff_h = 7.759;
					if (mode1 == 1) koeff_h = 3.879;
					if (mode1 == 2) koeff_h = 1.939;
					if (mode1 == 3) koeff_h = 0.969;
					print_set1();
					}

				if ((y_osc>=135) && (y_osc<=175))  // Port select   row
				 {
					waitForIt(250, 135, 310, 175);
						port ++ ;
					if (port > 5) port = 0;   
						print_set1();
				 }

				if ((y_osc>= 180) && (y_osc<=220))  // Port select   row
				 {
					waitForIt(250, 180, 310, 220);
		
					if(Set_ADC == 12) 
						{
							 Set_ADC = 10;
						}
					else
						{
							Set_ADC = 12;
						}
						analogReadResolution(Set_ADC); 
						print_set1();
				  }

		   }
		}

	logTime = micros();
	StartSample = micros();

	for( xpos = 1; xpos < 240;	xpos ++)

	 {
		 DrawGrid1();
		 if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();

				if ((x_osc>=2) && (x_osc<=240))  //  Delay Button
					{
						if ((y_osc>=1) && (y_osc<=160))  // Delay row
						{
							waitForIt(2, 1, 240, 160);
							myGLCD.print("STOP", CENTER, 200);
							break;
						} 
					}
			}

		 logTime += 1000UL*SAMPLE_INTERVAL_MS;
			   MaxAnalog = 0;
				do
				 {
					 MaxAnalog = max(MaxAnalog, analogRead(port));
					 SrednAnalog += MaxAnalog;
					 SrednCount++;
					 delayMicroseconds(20);                  //
					 diff = micros() - logTime;
					 EndSample = micros();
					 if(EndSample - StartSample > 1000000 )
					 {
						StartSample  =   EndSample ;
						sec_osc++;                          // Подсчет секунд
						if (sec_osc >= 60)
							{
							  sec_osc = 0;
							  min_osc++;                    // Подсчет минут
							}
						Serial.println(analogRead(port));
						myGLCD.setBackColor( 0, 0, 0);
						myGLCD.setFont( BigFont);
						myGLCD.print("Min", 8, 180);
						myGLCD.printNumI(min_osc, 60, 180);
						myGLCD.print("Sec", 120, 180);
						myGLCD.print("   ", 170, 180);
						myGLCD.printNumI(sec_osc, 170, 180);
					 }

				 } while (diff < 0);

				 if(Set_x == true)
					 {
		
						 MaxAnalog =  SrednAnalog / SrednCount;
						 SrednAnalog = 0;
						 SrednCount = 0;
					 }

				if (Set_ADC==12) MaxAnalog = MaxAnalog/4;
				Sample[xpos] = MaxAnalog;
				ypos1 = 255-(OldSample[ xpos -1 ]/koeff_h) - hpos; 
				ypos2 = 255-(Sample[ xpos]/koeff_h)- hpos;

				if(ypos1<0) ypos1 = 0;
				if(ypos2<0) ypos2 = 0;
				if(ypos1>220) ypos1 = 220;
				if(ypos2>220) ypos2 = 220;
				myGLCD.setColor( 255, 255, 255);
				myGLCD.setBackColor( 0, 0, 0);
				myGLCD.drawLine (xpos, ypos1, xpos+1 , ypos2);
				myGLCD.drawLine (xpos, ypos1+1, xpos+1 , ypos2+1);
				OldSample[xpos] = Sample[ xpos];

	} 

	koeff_h = 7.759;
//	mode1 = 0;
	Trigger = 0;
	myGLCD.setFont( BigFont);

	while (!myTouch.dataAvailable()){}
	delay(50);
	while (myTouch.dataAvailable()){}
	delay(50);
}
void buttons()
{
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (250, 1, 310, 40);
	myGLCD.fillRoundRect (250, 45, 310, 85);
	myGLCD.fillRoundRect (250, 90, 310, 130);
	myGLCD.fillRoundRect (250, 135, 310, 175);

	myGLCD.fillRoundRect (5+x_kn, 219, 30+x_kn, 238);
	myGLCD.fillRoundRect (35+x_kn, 219, 60+x_kn, 238);
	myGLCD.fillRoundRect (65+x_kn, 219, 90+x_kn, 238);
	myGLCD.fillRoundRect (95+x_kn, 219, 120+x_kn, 238);
	myGLCD.fillRoundRect (125+x_kn, 219, 150+x_kn, 238);
	myGLCD.fillRoundRect (155+x_kn, 219, 180+x_kn, 238);
	myGLCD.fillRoundRect (185+x_kn, 219, 210+x_kn, 238);
	myGLCD.fillRoundRect (217+x_kn, 206, 244+x_kn, 238);
	myGLCD.fillRoundRect (280, 206, 310, 238);

	myGLCD.setColor(VGA_YELLOW);
	myGLCD.fillRoundRect (1+x_kn, 206, 214+x_kn, 212);       //Желтая полоса
	myGLCD.fillRoundRect (1, 206, 28, 238);                  // Кнопка "<"
	myGLCD.fillRoundRect (217+x_kn, 206, 244+x_kn, 238);     // Кнопка ">"

	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255,255);
	myGLCD.print("1s", 12+x_kn, 222);
	myGLCD.print("5s", 42+x_kn, 222);
	myGLCD.print("10s", 67+x_kn, 222);
	myGLCD.print("1m", 102+x_kn, 222);
	myGLCD.print("5m", 132+x_kn, 222);
	myGLCD.print("10m", 157+x_kn, 222);
	myGLCD.print("15m", 187+x_kn, 222);

	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setFont( BigFont);
	myGLCD.print("<", 8, 214);
	myGLCD.print(">", 253, 214);
	myGLCD.setColor(0, 0, 255);
	//myGLCD.setFont( SmallFont);
}
void buttons1()
{
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (250, 1, 310, 40);
	myGLCD.fillRoundRect (250, 45, 310, 85);
	myGLCD.fillRoundRect (250, 90, 310, 130);
	myGLCD.fillRoundRect (250, 135, 310, 175);
	myGLCD.fillRoundRect (250, 180, 310, 220);
}
void DrawGrid()
{

  myGLCD.setColor( 0, 200, 0);
  for(  dgvh = 0; dgvh < 5; dgvh ++)
  {
	  myGLCD.drawLine( dgvh * 40, 0, dgvh * 40, 160);
	  myGLCD.drawLine(  0, dgvh * 40, 240 ,dgvh * 40);
  }
	myGLCD.drawLine( 200, 0, 200, 160);
	myGLCD.drawLine( 240, 0, 240, 160);
	myGLCD.setColor(255, 255, 255);           // Белая окантовка
	myGLCD.drawRoundRect (250, 1, 310, 40);
	myGLCD.drawRoundRect (250, 45, 310, 85);
	myGLCD.drawRoundRect (250, 90, 310, 130);
	myGLCD.drawRoundRect (250, 135, 310, 175);
	myGLCD.drawRoundRect (5+x_kn, 218, 30+x_kn, 239);
	myGLCD.drawRoundRect (35+x_kn, 218, 60+x_kn, 239);
	myGLCD.drawRoundRect (65+x_kn, 218, 90+x_kn, 239);
	myGLCD.drawRoundRect (95+x_kn, 218, 120+x_kn, 239);
	myGLCD.drawRoundRect (125+x_kn, 218, 150+x_kn, 239);
	myGLCD.drawRoundRect (155+x_kn, 218, 180+x_kn, 239);
	myGLCD.drawRoundRect (185+x_kn, 218, 210+x_kn, 239);
	myGLCD.drawRoundRect (280, 205, 311, 239);

	myGLCD.setColor(VGA_LIME);
	myGLCD.drawRoundRect (1+x_kn, 205, 215+x_kn, 213);
	myGLCD.drawRoundRect (0, 205, 29, 239);
	myGLCD.drawRoundRect (217+x_kn, 205, 245+x_kn, 239);
	myGLCD.setColor(255, 255, 255);           // 

}
void DrawGrid1()
{

 myGLCD.setColor( 0, 200, 0);
  for(  dgvh = 0; dgvh < 5; dgvh ++)
  {
	  myGLCD.drawLine( dgvh * 40, 0, dgvh * 40, 160);
	  myGLCD.drawLine(  0, dgvh * 40, 240 ,dgvh * 40);
  }
	myGLCD.drawLine( 200, 0, 200, 160);
	myGLCD.drawLine( 240, 0, 240, 160);
	myGLCD.setColor(255, 255, 255);           // Белая окантовка
}
void Draw_menu_SD()
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
	myGLCD.print( txt_SD_menu1, CENTER, 30);     // 
	myGLCD.print( txt_SD_menu2, CENTER, 80);      
	myGLCD.print( txt_SD_menu3, CENTER, 130);     
	myGLCD.print( txt_SD_menu4, CENTER, 180);      
}
void menu_SD()
{
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
					//!!		dumpData_Osc();
							Draw_menu_SD();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							SD_info();
							Draw_menu_SD();
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
							myGLCD.clrScr();
							menu_formatSD();
							Draw_menu_SD();
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
void SD_info()
{
 // read any existing Serial data
  while (Serial.read() >= 0) {}

  //// pstr stores strings in flash to save RAM
  //cout << pstr("\ntype any character to start\n");
  //while (Serial.read() <= 0) {}
  delay(400);  // catch Due reset problem
  
  uint32_t t = millis();
  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!card.init(SPI_HALF_SPEED, SD_CS_PIN)) 
  {
	sdErrorMsg("\ncard.init failed");
	return;
  }
  t = millis() - t;
  
  cardSize = card.cardSize();
  if (cardSize == 0) {
	sdErrorMsg("cardSize failed");
	return;
  }
  cout << pstr("\ninit time: ") << t << " ms" << endl;
  cout << pstr("\nCard type: ");
  switch (card.type()) {
	case SD_CARD_TYPE_SD1:
	  cout << pstr("SD1\n");
	  break;

	case SD_CARD_TYPE_SD2:
	  cout << pstr("SD2\n");
	  break;

	case SD_CARD_TYPE_SDHC:
	  if (cardSize < 70000000) 
	  {
		cout << pstr("SDHC\n");
	  } else {
		cout << pstr("SDXC\n");
	  }
	  break;

	default:
	  cout << pstr("Unknown\n");
  }
  if (!cidDmp()) return;
  if (!csdDmp()) return;
  uint32_t ocr;
  if (!card.readOCR(&ocr)) {
	sdErrorMsg("\nreadOCR failed");
	return;    
  }
  cout << pstr("OCR: ") << hex << ocr << dec << endl;
  if (!partDmp()) return;
  if (!vol.init(&card)) {
	sdErrorMsg("\nvol.init failed");
	return;
  }
  volDmp();

}
void SD_format()
{

	// Не применять!, реализовано в menu_formatSD();
	char c;
	// Draw_menu_formatSD();
	// menu_formatSD();
	  // Переделать
  // read any existing Serial data
  while (Serial.read() >= 0) {}
  
  cout << pstr(
	"\n"
	"Options are:\n"
	"E - erase the card and skip formatting.\n"
	"F - erase and then format the card. (recommended)\n"
	"Q - quick format the card without erase.\n"
	"\n"
	"Enter option: ");
	
  while (!Serial.available()) {}
  c = Serial.read();
  cout << c << endl;
  if (!strchr("EFQ", c)) 
  {
	cout << pstr("Quiting, invalid option entered.") << endl;
	return;
  }

  if (!card.init(spiSpeed, SD_CS_PIN)) 
  {
	cout << pstr(
	 "\nSD initialization failure!\n"
	 "Is the SD card inserted correctly?\n"
	 "Is chip select correct at the top of this sketch?\n");
	sdError("card.init failed");
  }



  cardSizeBlocks = card.cardSize();
  if (cardSizeBlocks == 0) sdError("cardSize");
  cardCapacityMB = (cardSizeBlocks + 2047)/2048;

  cout << pstr("Card Size: ") << cardCapacityMB;
  cout << pstr(" MB, (MB = 1,048,576 bytes)") << endl;

  if (c == 'E' || c == 'F') 
  {
	eraseCard();
  }
  if (c == 'F' || c == 'Q')
  {
	formatCard();
  }
}
void Draw_menu_formatSD()
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
	myGLCD.print( "Erase skip format", CENTER, 30);     // 
	myGLCD.print( "Erase and format", CENTER, 80);      
	myGLCD.print( "Quick format", CENTER, 130);     
	myGLCD.print( txt_SD_menu4, CENTER, 180);      
}
void menu_formatSD()
{

	 if (!card.init(spiSpeed, SD_CS_PIN)) 
  {
	cout << pstr(
	 "\nSD initialization failure!\n"
	 "Is the SD card inserted correctly?\n"
	 "Is chip select correct at the top of this sketch?\n");
	sdError("card.init failed");
  }

  cardSizeBlocks = card.cardSize();
  if (cardSizeBlocks == 0) sdError("cardSize");
  cardCapacityMB = (cardSizeBlocks + 2047)/2048;

  cout << pstr("Card Size: ") << cardCapacityMB;
  cout << pstr(" MB, (MB = 1,048,576 bytes)") << endl;

  Draw_menu_formatSD();

		// discard any input
	//while (Serial.read() >= 0) {} // Удалить все символы из буфера

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
							eraseCard();
						Draw_menu_formatSD();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							myGLCD.clrScr();
							eraseCard();
						Draw_menu_formatSD();
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
							myGLCD.clrScr();
							formatCard();
							Draw_menu_formatSD();
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
void touch_osc()  //  Нижнее меню осциллографа
{
	delay(10);
	myTouch.read();
	x_osc=myTouch.getX();
	y_osc=myTouch.getY();

	if ((y_osc>=205) && (y_osc<=239))  //  Delay Button
	{
		if ((x_osc>=5+x_kn) && (x_osc<=30+x_kn))  //  Delay Button
			{
				waitForIt(5+x_kn, 218, 30+x_kn, 239);
				buttons();
				myGLCD.setColor(VGA_LIME);
				myGLCD.setBackColor( VGA_LIME);
				myGLCD.fillRoundRect (5+x_kn, 219, 30+x_kn, 238);
				myGLCD.setColor(0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.print("1s", 12+x_kn, 222);
			}
		if ((x_osc>=35+x_kn) && (x_osc<=60+x_kn))  //  Delay Button
			{
				waitForIt(35+x_kn, 218, 60+x_kn, 239);
				buttons();
				myGLCD.setColor(VGA_LIME);
				myGLCD.setBackColor( VGA_LIME);
				myGLCD.fillRoundRect (35+x_kn, 219, 60+x_kn, 238);
				myGLCD.setColor(0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.print("5s", 42+x_kn, 222);
			}
		if ((x_osc>=65+x_kn) && (x_osc<=90+x_kn))  //  Delay Button
			{
				waitForIt(65+x_kn, 218, 90+x_kn, 239);
				buttons();
				myGLCD.setColor(VGA_LIME);
				myGLCD.setBackColor( VGA_LIME);
				myGLCD.fillRoundRect (65+x_kn, 219, 90+x_kn, 238);
				myGLCD.setColor(0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.print("10s", 67+x_kn, 222);
			}
		if ((x_osc>=95+x_kn) && (x_osc<=120+x_kn))  //  Delay Button
			{
				waitForIt(95+x_kn, 218, 120+x_kn, 239);
				buttons();
				myGLCD.setColor(VGA_LIME);
				myGLCD.setBackColor( VGA_LIME);
				myGLCD.fillRoundRect (95+x_kn, 219, 120+x_kn, 238);
				myGLCD.setColor(0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.print("1m", 102+x_kn, 222);
			}
		if ((x_osc>=125+x_kn) && (x_osc<=150+x_kn))  //  Delay Button
			{
				waitForIt(125+x_kn, 218, 150+x_kn, 239);
				buttons();
				myGLCD.setColor(VGA_LIME);
				myGLCD.setBackColor( VGA_LIME);
				myGLCD.fillRoundRect (125+x_kn, 219, 150+x_kn, 238);
				myGLCD.setColor(0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.print("5m", 132+x_kn, 222);
			}
		if ((x_osc>=155+x_kn) && (x_osc<=180+x_kn))  //  Delay Button
			{
				waitForIt(155+x_kn, 218, 180+x_kn, 239);
				buttons();
				myGLCD.setColor(VGA_LIME);
				myGLCD.setBackColor( VGA_LIME);
				myGLCD.fillRoundRect (155+x_kn, 219, 180+x_kn, 238);
				myGLCD.setColor(0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.print("10m", 157+x_kn, 222);
			}
		if ((x_osc>=185+x_kn) && (x_osc<=210+x_kn))  //  Delay Button
			{
				waitForIt(185+x_kn, 218, 210+x_kn, 239);
				buttons();
				myGLCD.setColor(VGA_LIME);
				myGLCD.setBackColor( VGA_LIME);
				myGLCD.fillRoundRect (185+x_kn, 219, 210+x_kn, 238);
				myGLCD.setColor(0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.print("15m", 187+x_kn, 222);
			}

		if ((x_osc>=0) && (x_osc<=29))  //  Delay Button
			{
				waitForIt(1, 205, 29, 239);

			}

		if ((x_osc>=217+x_kn) && (x_osc<= 245+x_kn))  //  Delay Button
			{
				waitForIt(217+x_kn, 205, 245+x_kn, 239);

			}
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
	myGLCD.print( txt_ADC_menu1, CENTER, 30);       // "Record ADC data"
	myGLCD.print( txt_ADC_menu2, CENTER, 80);       // "Convert to CSV"
	myGLCD.print( txt_ADC_menu3, CENTER, 130);      // "Data to Serial"
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

	Channel_0 = 1;
	Channel_1 = 0;
	Channel_2 = 1;
	Channel_3 = 0;

	chench_analog();

  //adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
	Timer3.attachInterrupt(firstHandler); // Every 500ms
	Timer4.attachInterrupt(secondHandler).setFrequency(1);
	//  	Timer3.attachInterrupt(firstHandler).start(500000); // Every 500ms
	//Timer4.attachInterrupt(secondHandler).setFrequency(1).start();
	rtc_clock.init();
	rtc_clock.set_time(__TIME__);
	rtc_clock.set_date(__DATE__);
	SdFile::dateTimeCallback(dateTime); 
	//++++++++++++++++ SD info ++++++++++++++++++++++++++++++
	
  // use uppercase in hex and use 0X base prefix
  cout << uppercase << showbase << endl;
  set_strob = 100;

  // pstr stores strings in flash to save RAM
  cout << pstr("SdFat version: ") << SD_FAT_VERSION << endl;

	Serial.println(F("Setup Ok!"));



}
//------------------------------------------------------------------------------
void loop(void) 
{
  
	draw_Glav_Menu();
	swichMenu();
	
//	
//	
//	
//	// discard any input
//
//  while (Serial.read() >= 0) {}
//  Serial.println();
//  Serial.println(F("type:"));
//  Serial.println(F("c - convert file to CSV")); 
//  Serial.println(F("d - dump data to Serial"));  
//  Serial.println(F("e - overrun error details"));
//  Serial.println(F("r - record ADC data"));
////  Timer3.start(100);
//  while(!Serial.available()) {}
//  char c = tolower(Serial.read());
//  if (ERROR_LED_PIN >= 0) {
//	digitalWrite(ERROR_LED_PIN, LOW);
//  }
//  // Read any extra Serial data.
//  do {
//	delay(10);
//  } while (Serial.read() >= 0);
//  
//  if (c == 'c') {
//	binaryToCsv();
//  } else if (c == 'd') {
//	dumpData();
//  } else if (c == 'e') {    
//	checkOverrun();
//  } else if (c == 'r') {
//	logData();
//  } else {
//	Serial.println(F("Invalid entry"));
//  }
}
