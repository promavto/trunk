#include <Arduino.h>

/* Как пользоваться классом:
0) Создайте экземпляр класса. Сообщите конструктору номера пинов, к которым подключена ваша клавиатура, в порядке увеличения номера на разъеме клавы. Там есть нумерация, есличо.
2) в setup() выполните begin()
3) в loop() вставьте process() Он выполняется за 230-250 микросекунд (не милли-, а микро) + MicrosToAskOnePin (440-450 мкс всего) иногда
...
Используйте get() чтобы извлечь нажатый юзером PROFIT в виде char'a.

*/

// keyb4h4.h

class Keyb4x4 {
  public:
    // constructor: give it number of pins connected to keyboard; three last values have reasonable defaults and usually should be omitted.
    // Pin numbering is printed on keyb's jack.
  Keyb4x4(int pin1, int pin2, int pin3, int pin4, int pin5, int pin6, int pin7, int pin8,  int NumberConsequentRepeats=7, int MicrosToAskOnePin=200, int UnknownValuePresumedAs=LOW);
 
//Run it in Setup() 
  //Keyb4x4::begin() Initiates pins, run this function in Setup
  void begin (boolean UseInternalPullingResistors=false); //default=false because they don't work ha-ha-ha.
 
// Run it in loop()
  //Keyb4x4::process() Checks keyboard every loop() cycle, adding keys to buffer. Only _pressed and then released_ keys are added!
  void process();
 
 
  //Keyb4x4::ask()  Basic function. Quickly asks keyboard and returns: char '\0' if nothing pressed or char of KeyTable set if button is pressed.
  char ask(void);  //should be used not itself but in some wrap function like process()
 
  //Main function - use it to...
  //Keyb4x4::get().    ...get one char from buffer. Or '\0' if it is empty.
  char get(void);
 
  boolean KeyPressed;  //true if user have pressed something...
 
  void SetParameteres(int NumberConsequentRepeats, int MicrosToAskOnePin, int UnknownValuePresumedAs); //if you need to change it from defaults. To change just one param, omit  others - they will be ignored
  int version(void);  //для понтов
  private:
  int cols[4]; //here I store numbers of pins connected to one side of keyb matrix (let's call its "columns")
  int rows[4]; //here I store numbers of pins connected to other side of keyb matrix (let's call its "rows")
  long NumberConsequentRepeats;  // Function Ask will DigitalRead a pin form rows[] array until get "NumberConsequentRepeats" of the same consequent values, OR
  unsigned long MicrosToAskOnePin;        // until "MicrosToAskOnePin" time will pass. In this case it will ensure the pin is Unknown and
  int UnknownValuePresumedAs;                // presume it is HIGH or LOW (default) according to UnknownValuePresumedAs variable
  boolean UseInternalPullingResistors;      // use only external 10 kOm resistors and you'll get no problems.
  static const char KeyTable44[4][4]; //key values, see initialisation below
  char buffer[256]; //keyboard buffer for process() and get()
  int UsedBuffer; // pointer to last used place in buffer.
  char CurrentPressedKey; // for process()
};