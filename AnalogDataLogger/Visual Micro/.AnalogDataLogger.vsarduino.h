/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Arduino Due (Programming Port), Platform=sam, Package=arduino
*/

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define __SAM3X8E__
#define USB_VID 0x2341
#define USB_PID 0x003e
#define USBCON
#define USB_MANUFACTURER "\"Unknown\""
#define USB_PRODUCT "\"Arduino Due\""
#define ARDUINO 161
#define ARDUINO_MAIN
#define printf iprintf
#define __SAM__
#define __sam__
#define F_CPU 84000000L
#define __cplusplus
#define ARDUINO_ARCH_SAM
#define ARDUINO_SAM_DUE
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__

#define __ICCARM__
#define __ASM
#define __INLINE
#define __GNUC__ 0
#define __ICCARM__
#define __ARMCC_VERSION 400678
#define __attribute__(noinline)

#define prog_void
#define PGM_VOID_P int
            
typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}



#include <arduino.h>
#include <pins_arduino.h> 
#include <variant.h> 
#undef F
#define F(string_literal) ((const PROGMEM char *)(string_literal))
#undef cli
#define cli()
#define pgm_read_byte(address_short)
#define pgm_read_word(address_short)
#define pgm_read_word2(address_short)
#define digitalPinToPort(P)
#define digitalPinToBitMask(P) 
#define digitalPinToTimer(P)
#define analogInPinToBit(P)
#define portOutputRegister(P)
#define portInputRegister(P)
#define portModeRegister(P)

void acquireData(data_t* data);
void printData(Print* pr, data_t* data);
void printHeader(Print* pr);
inline uint8_t queueNext(uint8_t ht);
void error_P(const char* msg);
void fatalBlink();
void binaryToCsv();
void checkOverrun();
void dumpData();
void dumpData_Osc();
void logData();
int bcd2bin(int temp);
void clock_read();
void clock_print_serial();
void drawDisplay();
void drawMark(int h);
void drawSec(int s);
void drawMin(int m);
void drawHour(int h, int m);
void printDate();
void clearDate();
void AnalogClock();
void draw_Glav_Menu();
void swichMenu();
void print_up();
void klav123();
void updateStr(int val);
void waitForIt(int x1, int y1, int x2, int y2);
void reset_klav();
void Draw_menu_Osc();
void menu_Oscilloscope();
void trigger();
void print_set();
void oscilloscope();
void buttons();
void DrawGrid();
void DrawGrid1();
void Draw_menu_SD();
void menu_SD();
void touch_osc();
void Draw_menu_ADC1();
void menu_ADC();
void setup(void);
void loop(void);

#include <AnalogDataLogger.ino>
#include <UserDataType.h>
#endif