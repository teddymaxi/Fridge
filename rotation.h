#ifndef EFW_H
#define EFW_H

#define ACE128_ARDUINO_PINS 1
#include <AccelStepper.h>
#include "ACE128.h"
#include "ACE128map12345678.h" 

class EFW{
    public:
        //Пины двигателя
        int dirPin =  23;
        int stepPin =  27;
		// >>> enable its output to turn on the engine 
		int step_enable_pin = 38;

        //Пины энкодера
        uint8_t pin1 = 36;//test commit
        uint8_t pin2 = 38;
        uint8_t pin3 = 42;
        uint8_t pin4 = 44;
        uint8_t pin5 = 52;
        uint8_t pin6 = 50;
        uint8_t pin7 = 31;
        uint8_t pin8 = 29;

        //Пин датчика Холла
        int hallPin = A14;
        //Пин датчика Холла для нулевой позиции
        int hallPinZero = A15;

        //Максимально допустимое ускорение
        int max_acc = -100;

        //Текущая позиция и фильтр
        int cur_pos = 0;
        int cur_filt = 0;

        //Количество фильтров
        int num_filt = 24;


        //1 - т.к. используем драйвер
        int motorInterfaceType = 1;
        // Объект двигателя
        AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

        //Объект энкодера. 8 цифр - соотвествие пинов энкодера с пинами ардуино (например, пин 1 энкодера соответсвует пину pin1 арудино)
        ACE128 myACE = ACE128 (pin1, pin2, pin3,  pin4, pin5,  pin6, pin7, pin8, (uint8_t*)encoderMap_12345678, 0);

        EFW (int dirPin, int stepPin, int hallPin, int hallPinZero,
        uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, uint8_t pin5, uint8_t pin6, uint8_t pin7, uint8_t pin8,
        int num_filt) ;

        //Определить направление вращения для калибровки    
        int define_dir(int pin);

        //Вычисляет количество позиций между start и finish по часовой стрелке
        int clockwise_shift(int start, int finish);
        //Вычисляет количество позиций между start и finish против часовой стрелки
        int anticlockwise_shift(int start, int finish);

        //Уточнить положение с помощью магнитов и датчика Холла
        void adjust(int pin);

        //Передвинуться в заданную позицию
        void move_to_filt(int target_filt);

};

EFW::EFW (int dirPin, int stepPin, int step_enable_pin, int hallPin, int hallPinZero,
 uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, uint8_t pin5, uint8_t pin6, uint8_t pin7, uint8_t pin8, int num_filt) {
    
    this->dirPin = dirPin;
    this->stepPin = stepPin;
	this->step_enable_pin = step_enable_pin;
    this->hallPin = hallPin;
    this->hallPinZero = hallPinZero;

    this->pin1 = pin1;
    this->pin2 = pin2;
    this->pin3 = pin3;
    this->pin4 = pin4;
    this->pin5 = pin5;
    this->pin6 = pin6;
    this->pin7 = pin7;
    this->pin8 = pin8;
    this->num_filt = num_filt;
	
	myACE = ACE128 (pin1, pin2, pin3,  pin4, pin5,  pin6, pin7, pin8, (uint8_t*)encoderMap_12345678, 0);
	stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
	pinMode(step_enable_pin, OUTPUT);
	digitalWrite(step_enable_pin, LOW);

    myACE.begin();
}

void EFW::adjust(int pin){
    stepper.setMaxSpeed(1000.0); 

    //Определяем направление вращения
    int dir = define_dir(pin);
    stepper.setSpeed(25*dir);

    Serial.println("Begining adjustment");
    Serial.println(dir);
    
    int old_pos = stepper.currentPosition();
    int new_pos = stepper.currentPosition();   
    int new_val= analogRead(pin);
    int old_val = analogRead(pin);

    //!!!!! Следить за показаниями датчика! Около магнита они могут быть как больше, так и меньше, в зависимости от этого надо менять знак c <=
    // на >=. Аналогично в функции define_dir

    while (new_val <= old_val){ //Пока не достигли максимума
        stepper.runSpeed();
        new_pos = stepper.currentPosition();
        if (abs(old_pos - new_pos) >= 3){//Считываем значение с датчика Холла не каждый шаг, а с некоторым интервалом - чтобы не упираться в предел чувствительности датчика
                                         //(В данном случае - считываем значение каждые 3 шага)
            old_val = new_val;          
            new_val = analogRead(pin);
            old_pos = new_pos;
            Serial.print(old_val);
            Serial.print(" ");
            Serial.println(new_val);
        }
    }
    //Откатываемся чуть-чуть назад, потому что в предыдущем цикле мы проскочили максимум
    stepper.move(-dir*3);
    stepper.setSpeed(-25*dir);
    while(stepper.distanceToGo() != 0){
        stepper.runSpeed();
    }
    Serial.println("Final refinments");
    old_val = new_val;          
    new_val = analogRead(pin);
    Serial.print(old_val);
    Serial.print(" ");
    Serial.println(new_val);

    Serial.println("Finishing adjustment");


}

int EFW::define_dir(int pin){
    stepper.setMaxSpeed(1000.0);
    int old_val;
    int new_val;
    old_val = analogRead(pin);
    //Сдвигаемся на 8 шагов, сравниваем измеренное значение с тем, что было в начале, и на основе этого выбираем напарвление
    stepper.setCurrentPosition(0);
    stepper.setSpeed(100);
    while(stepper.currentPosition() != 8)
    {
        stepper.runSpeed();
    }
    new_val = analogRead(pin);

    Serial.println("Calibration done");
    Serial.println(old_val);
    Serial.println(new_val);

    //!!!!! Следить за показаниями датчика! Около магнита они могут быть как больше, так и меньше, в зависимости от этого надо менять знак c <=
    // на >=. Аналогично в функции adjust
    if (new_val <= old_val){ 
        return 1;
    }else{
        return -1;
    }
}


int EFW::anticlockwise_shift(int start, int finish){
    if (finish > start){
        return 127 - finish + start + 1;    
    }
    return abs(finish - start) ;
    
}

int EFW::clockwise_shift(int start, int finish){
    if (finish >= start){
        return finish - start ;           
    }
    return 127 - start + finish + 1;
    
}


void EFW::move_to_filt(int target_filt){
    int target_pos = 127 * target_filt/num_filt;
    stepper.setMaxSpeed(1000.0);   
    cur_pos = myACE.upos();   //upos - unsigned position, т.е. число от 0 до 127
    
    int init_pos = cur_pos;
    int poses_to_move = 0;
    int shift;
    int dir;

    //выбираем оптимальное направление и вычисляем количество позиций энкодера, на которое надо сдвинуться
    poses_to_move = min(clockwise_shift(cur_pos, target_pos), anticlockwise_shift(cur_pos, target_pos));
    dir = (poses_to_move == clockwise_shift(cur_pos, target_pos))? 1: -1;

    while (cur_pos != target_pos) { //Двигаемся пока энкодер не скажет, что уже достигли нужной позиции
        //Считываем, насколько уже сдвинулись
        if (dir == 1){
            shift = clockwise_shift(init_pos, cur_pos);
        } else{
            shift = anticlockwise_shift(init_pos, cur_pos);
        }

        //Выставляем нужную скорость
        if (shift <= poses_to_move/2){
            stepper.setSpeed(dir*max_acc*(shift+1));    
        } else{
            stepper.setSpeed(dir*max_acc*(poses_to_move - shift +1 ));     
        }

        stepper.runSpeed();
        cur_pos= myACE.upos(); 
    }
    if (cur_pos != 0){
    cur_filt = cur_pos*num_filt/127 + 1;
    } else{
        cur_filt = 0;
    }
    
}

#endif
