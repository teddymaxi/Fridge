#include "rotation.h"
#define NUM_FILT 23
int filt = 0;
// #FIXME
// dir, step, enable, hall, hallZero, encPins{1..8}, num_filt
EFW EFW1(54, 55, 38, A14, A15, 36, 38, 42, 44, 52, 50, 31, 29, 24);
EFW EFW2(54, 55, 38, A14, A15, 36, 38, 42, 44, 52, 50, 31, 29, 24);
//EFW EFW3 создаст класс с дефолтными параметрами описанными на 18+- строках в ротатион 


int done = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  //Тест функции move_to_filt

  if (Serial.available() > 0) {
    filt = Serial.parseInt();
    Serial.print("Filter to move: ");
    Serial.println(filt);
    delay(100);
  }
  
  if(filt < NUM_FILT){
      if(0 != EFW2.cur_filt){
          Serial.println("Returning the 2nd disk to zero");
          EFW2.move_to_filt(0);
      }
	  if (filt != EFW1.cur_filt){
		  EFW1.move_to_filt(filt);
		  Serial.print("Current filter: ");
		  Serial.println(EFW1.cur_filt);
	  }
  }
  else{
      if(0 != EFW1.cur_filt){
          Serial.println("Returning the 1st disk to zero");
          EFW1.move_to_filt(0);
      }
	  if (filt % NUM_FILT != EFW2.cur_filt){
		  EFW2.move_to_filt(filt % NUM_FILT);
		  Serial.print("Current filter: ");
		  Serial.println(EFW2.cur_filt);
	  }
  }
}