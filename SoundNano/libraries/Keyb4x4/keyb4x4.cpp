// keyb4x4.cpp
#include "keyb4x4.h"
#include "Arduino.h"

//here we fill array of keyboard values. You can modify it if necessary.
const char Keyb4x4::KeyTable44[][4]={{'1','2','3','A'},{'4','5','6','B'},{'7','8','9','C'},{'*','0','#','D'}};

Keyb4x4::Keyb4x4(int pin1, int pin2, int pin3, int pin4, int pin5, int pin6, int pin7, int pin8, int NumberConsequentRepeats, int MicrosToAskOnePin, int UnknownValuePresumedAs)
{
  this->cols[0] = pin1;        // pins keyboard connected to
  this->cols[1] = pin2;        // pins keyboard connected to
  this->cols[2] = pin3;        // pins keyboard connected to
  this->cols[3] = pin4;        // pins keyboard connected to
  this->rows[0] = pin5;        // pins keyboard connected to
  this->rows[1] = pin6;        // pins keyboard connected to
  this->rows[2] = pin7;        // pins keyboard connected to
  this->rows[3] = pin8;        // pins keyboard connected to
  this->NumberConsequentRepeats=NumberConsequentRepeats;
  this->MicrosToAskOnePin=MicrosToAskOnePin;
  this->UnknownValuePresumedAs=UnknownValuePresumedAs;
  this->UsedBuffer=-1;
  KeyPressed=false;
}


void Keyb4x4::begin(boolean UseInternalPullingResistors)
{
    // setup the pins on the microcontroller:
  for (int i=0; i<4; i++) {
    pinMode(cols[i], OUTPUT); // to send voltage,
    pinMode(rows[i], INPUT);  // and to read results.
    //prepare OUTPUT pins:
    digitalWrite(cols[i], LOW);
   
    //prepare INPUT pins:
    if (UseInternalPullingResistors) {
        digitalWrite(rows[i], HIGH); // Here we enable internal pulling resistors between INPUT pins to ground. On my Arduino Mega they don't work at all :-(.
    }
    else {
        digitalWrite(rows[i], LOW);  // and here disable them: it's preferable if you already have external pulling resistors in your scheme.
    }
  } //cycle end
}

char Keyb4x4::ask(void)
{
//  char r='-';
  for (int i=0; i<4; i++) {
    for (int j=0; j<4;j++)      digitalWrite(cols[j], LOW);      //set OUTPUT pin's to zero. Don't remove or you'll get false positives
    digitalWrite(cols[i], HIGH); //set +5V to pin from column and
    for (int j=0; j<4;j++){      //check is it visible on any row?
      if (digitalRead(rows[j])==HIGH) {  // OK, if we got circuit so take several repeats to ensure it is not false positive because of induction, statics or other
        long counter=1;    // we already have 1 answer
        unsigned long TimeToStop=micros()+MicrosToAskOnePin;  //don't gonna fall to infifnite cycle if pin "free-float"
        while ((counter<NumberConsequentRepeats) && (micros()<TimeToStop)) {  //the investigation
          if (digitalRead(rows[j])==HIGH) {
            counter++;
          }
          else{  // negativ answer?
            counter=0;  //begin collect from zero!
          }
        }
        //here investigation over and we check it's result
        if (counter>=NumberConsequentRepeats) { //Yes! col[i] and row[j] connected! пришёл к успеху
          return KeyTable44[3-j][3-i];
        }
        else{ //Unknown result
          if (UnknownValuePresumedAs==HIGH) { //считать гуся карасём
            return KeyTable44[3-j][3-i];
          }
          else if (UnknownValuePresumedAs==LOW) {}//do nothing  не свезло, не пофартило
        }
      } //end of first If(DigitalRead...) statement 
    }//end of rows cycle
  }//end columns cycle
  return '\0';
}

void Keyb4x4::process(void)
{
  char t;
  t=ask();
  if (CurrentPressedKey==t) {return;} //key is still pressed from previous run
  else if  ((t=='\0') && (CurrentPressedKey!='\0')) { // key pressed now released - add it to buffer ранее нажатая кнопка отпущена - добавляем
    UsedBuffer++;
    buffer[UsedBuffer]=CurrentPressedKey;
    CurrentPressedKey='\0';
    if (UsedBuffer>255) UsedBuffer=-1; //BTW if overflow - just start from beginning.
    KeyPressed=true;
    return;
  }
  else if ((t!='\0') && (CurrentPressedKey=='\0')) { // key just pressed right now
    CurrentPressedKey=t; //и подождём while it releases
    return;
  }
  else if ((t!='\0') && (CurrentPressedKey!='\0')) { //other key pressed while first don't released. It's bad. It's undefinite situation because of matrix keyb structure. Don't do so!
    //do nothing как-нибудь так!
  }
}

char Keyb4x4::get(void)
{
  char t='\0';
  if (UsedBuffer>=0) {
    t=buffer[0];
    for (int i=0; i<UsedBuffer;i++) buffer[i]=buffer[i+1];
    UsedBuffer--;
    if (UsedBuffer<0) KeyPressed=false;
  }
  return t;
}

void Keyb4x4::SetParameteres(int NumberConsequentRepeats=0, int MicrosToAskOnePin=0, int UnknownValuePresumedAs=LOW)
{
  if (NumberConsequentRepeats!=0) this->NumberConsequentRepeats=NumberConsequentRepeats;
  if (MicrosToAskOnePin!=0) this->MicrosToAskOnePin=MicrosToAskOnePin;
  if (UnknownValuePresumedAs!=0) this->UnknownValuePresumedAs=UnknownValuePresumedAs;
}

int Keyb4x4::version(void)
{
  return 3;
}