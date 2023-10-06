#include <Servo.h>

Servo ESC;     // create servo object to control the ESC
//Motor Speed Values for clockwise, counter clockwise, and neutral
#define C_SPEED 80
#define CC_SPEED 100
#define NEUTRAL_SPEED 90
#define SHIFT_TIME 250

//Inputs
#define C_INPUT_PIN 2
#define CC_INPUT_PIN 3

#define CLOCKWISE 2
#define COUNTER_CLOCKWISE 3
#define NEUTRAL_RET 4

void setup() {

  Serial.begin(9600);

  pinMode(C_INPUT_PIN, INPUT);
  pinMode(CC_INPUT_PIN, INPUT);

  // Attach the ESC on pin 9
  ESC.attach(9,1000,2000); // (pin, min pulse width, max pulse width in microseconds) 

  // calibration procedure
  ESC.write(NEUTRAL_SPEED);
  delay(2000);
  ESC.write(180);
  delay(500);
  ESC.write(0);
  delay(50);
  ESC.write(NEUTRAL_SPEED);

}

void loop() {

  int req = GetButtonInput();
  ESC_Shift(req);

}

int GetButtonInput() {

  //Read inputs
  int buttonStateC = digitalRead(C_INPUT_PIN);
  int buttonStateCC = digitalRead(CC_INPUT_PIN);

  delay(50);//software debounce

  Serial.print(buttonStateC);
  Serial.println(buttonStateCC);

  //check for conflicting signals
  if( (buttonStateC == HIGH) && (buttonStateCC == HIGH) ) {
    Serial.println("ERROR: CONFLICTING INPUTS");
    delay(10);
    return -1;
  }

  //check for Clockwise input
  if ( (buttonStateC == HIGH) && (buttonStateCC == LOW)) {return CLOCKWISE;}//clockwise rotate
  else if ( (buttonStateCC == HIGH) && (buttonStateC == LOW)) {return COUNTER_CLOCKWISE;}//counter-clockwise rotate
  else {return NEUTRAL_RET;}
}

void ESC_Shift(int command){

    if (command > 0) {
      if(command == CLOCKWISE) {
          Serial.println("CLOCKWISE SHIFT");
          ESC.write(C_SPEED);
          delay(SHIFT_TIME);
          ESC.write(NEUTRAL_SPEED);
        }
      if(command == COUNTER_CLOCKWISE) {
          Serial.println("COUNTER-CLOCKWISE SHIFT");
          ESC.write(CC_SPEED);
          delay(SHIFT_TIME);
          ESC.write(NEUTRAL_SPEED);
        }
       else{ESC.write(NEUTRAL_SPEED);}
       }

}
