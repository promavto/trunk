#include <keyb4x4.h>
#include <Arduino.h>
// подключаем библиотеку для работы с LCD
#include <LiquidCrystal.h>

  //создаем объект клавиатуры
  Keyb4x4 myKeyb (40,41,42,43,44,45,46,47);
  // сосздаем объект LCD, указывая контакты данных
  LiquidCrystal lcd(48, 49, 50, 51, 52, 53);

int line=0;
int column=0;
unsigned long t=0;
 
void setup()
{
  // указываем размерность экрана и начинаем работать
  lcd.begin(20, 4);
  lcd.clear();

  lcd.print("Keyb input below:");
  lcd.setCursor(0, 1);

//иниц. клав. 
  myKeyb.begin(true);
}
 
void loop()
{
  char k='\0';
  //вспомогательное - засечка времени
t=micros(); //мне надо быстро, мне ещё много чего крутить в realtime
  //Опрос клавиатуры
myKeyb.process();
  //вспомогательное - засечка времени
  t=micros()-t;

if (myKeyb.KeyPressed) {
    k=myKeyb.get();

  lcd.setCursor(column, 1);
  lcd.print(k);
  column++;
  if (column>19) column=0;
  lcd.setCursor(0, 2);
  lcd.print("Run time, mcs: ");
  lcd.print(t);
  }
}