//#ifndef EFW_H
//#define EFW_H

#define ACE128_ARDUINO_PINS 1
#include <AccelStepper.h>
#include "ACE128.h" //" to use local library 
#include "ACE128map12345678.h" //encoder map 

//fixing 
class FW{

    //orchestrating class to do it all  

    // dir, step, enable, hall, hallZero, encPins{1..8}, num_filt
    //EFW EFW1(54, 55, 38, A14, A15, 36, 38, 42, 44, 52, 50, 31, 29, 24);


    //EFW::EFW (int dirPin, int stepPin, int step_enable_pin, int hallPin, int hallPinZero,
    // uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, uint8_t pin5, uint8_t pin6, uint8_t pin7, uint8_t pin8, int num_filt)

    //NOTE no need to define input class variables only method inputs

 
    // TODO move to main arduino file 
    Serial.println("[INFO]_FW_Initialization");
    // TODO if else exception style
        
    private:
        //pins for each FW
        //motor pins 
        //motor speed and accel
        //hall sensor pins 
        //Encoder pins     

        //Пины двигателя
        int dirPin;
        int stepPin;
		// >>> enable its output to turn on the engine 
		int step_enable_pin;

        //Пины энкодера
        uint8_t pin1;
        uint8_t pin2;
        uint8_t pin3;
        uint8_t pin4;
        uint8_t pin5;
        uint8_t pin6;
        uint8_t pin7;
        uint8_t pin8;

        //Пин датчика Холла
        int hallPin;
        //Пин датчика Холла для нулевой позиции
        int hallPinZero;

        //Максимально допустимое ускорение
        int max_acc = -100;

        //Текущая позиция и фильтр
        int cur_pos = 0;
        int cur_filt = 0;
        //TODO 
        //потенциальная дыра нужен фикс: мы считаем что при включение фильтры находятся в 0 позиции
        //однако если  это не так то столкновения не избежать.

        //Количество фильтров
        int num_filt = 22;
        //TODO уточнить кол-во фильтров на одном колесе, по памяти Эверьян Говорил что суммарно их 44=> на каждом колесе 22

        //1 - т.к. используем драйвер
        int motorInterfaceType = 1;    
}

class Calibration
{
    Serial.println("[INFO]_Calibration");
    //class for initial calibration on startup 
    //Calibration filterChange 2 main classe callable in orchestrator
    private:

    public:
    //method to discover positions of all filter on according FW
    void Calibration::adjust(){
        //TODO speed adjust parameter
    stepper.setMaxSpeed(1000.0); 

    //Определяем направление вращения
    int dir = define_dir(pin);
    stepper.setSpeed(25*dir);

    Serial.println("[INFO]_Begining adjustment");
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
    Serial.println("[INFO]_Final refinments");
    old_val = new_val;          
    new_val = analogRead(pin);
    Serial.print(old_val);
    Serial.print(" ");
    Serial.println(new_val);

    Serial.println("[INFO]_Finishing adjustment");


}


}

class filterChange{
    Serial.println("[INFO]_filterChange");
    //инициализация энкодера и мотора
    myACE = ACE128 (epin1, epin2, epin3,  epin4, epin5,  epin6, epin7, epin8, (uint8_t*)encoderMap_12345678, 0);
	stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
	pinMode(step_enable_pin, OUTPUT);
	digitalWrite(step_enable_pin, LOW);

    myACE.begin();
    //class for changing to X filter
    //Calibration filterChange 2 main classe callable in orchestrator
    private:

    public:

    //functions to choose the shortest route fo FilterChange
int filterChange::anticlockwise_shift(int start, int finish){
    if (finish > start){
        return 127 - finish + start + 1;    
    }
    return abs(finish - start) ;
    
}

int filterChange::clockwise_shift(int start, int finish){
    if (finish >= start){
        return finish - start ;           
    }
    return 127 - start + finish + 1;
    
}

    void filterChange::move_to_filt(int target_filt){
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
}

}

 class Sensor
 {
     // sensors (hall and encoder) controlling class 
 private:
     //since we got 2 it requires to store incoming pins for X encoder in private variables 
    int Sensor::Hall(int HallPin){
        a = get()
        Serial.println("[INFO]_Hall");
    //function to read sensor data 
    }
    int Sensor::Encoder(){
        Serial.println("[INFO]_Encoder");
    }
}



class MotorController
{
    Serial.println("[INFO]_MotorController");
    //мы же можем узнать направление мотора при калибровке ручной, есть ли смысл его поределять ? 
    //т.к. оно зависит от установки 
        // class to handle Motor movement for x amount of steps 
        // or moving to set position
        //and adjusting it according to sensor data 
    private:
    // reqs a function to correct motor movement with help of encoder and hall sensor 
    int MotorController::MoveTo(){
        Serial.println("[INFO]_MoveTo");
    }
    int MotorController::MoveFor(){
        Serial.println("[INFO]_MoveFor");
    }
}

