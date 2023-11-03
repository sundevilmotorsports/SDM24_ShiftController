#include <Servo.h>

Servo ESC;     // create servo object to control the ESC
//Motor Speed Values for clockwise, counter clockwise, and neutral
#define C_SPEED 80
#define CC_SPEED 100
#define NEUTRAL_SPEED 90
#define MAX_SHIFT_TIME 500  // ms

//Inputs
#define C_INPUT_PIN 2
#define CC_INPUT_PIN 3

#define CLOCKWISE 2
#define COUNTER_CLOCKWISE 3
#define NEUTRAL_RET 4

bool timerRunning = false;
bool shiftTimedOut = false;
unsigned long shiftStartMillis = 0;

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

  int req = getECUInput();

  // if we have recieved a shift signal
  if (req == CLOCKWISE || req == COUNTER_CLOCKWISE) {

    // check if we're already shifting
    if (timerRunning) {

      // check if the maximum shift time has been exceeded
      if (millis() + MAX_SHIFT_TIME > shiftStartMillis) {

        shiftTimedOut = true;

      }

    } else {  // if we're not already shifting

      // start the timer
      shiftStartMillis = millis();
      timerRunning = true;

      if (req == CLOCKWISE && !shiftTimedOut) {
        ESC_Shift(CLOCKWISE);
      } else if (req == COUNTER_CLOCKWISE && !shiftTimedOut) {
        ESC_Shift(COUNTER_CLOCKWISE);
      } else {
        ESC_Shift(0);
      }

    }

  } else {  // if we have not recieved a shift signal

    ESC_Shift(0);
    timerRunning = false;
    
  }



}

int getECUInput() {

  //Read inputs
  int stateC = digitalRead(C_INPUT_PIN);
  int stateCC = digitalRead(CC_INPUT_PIN);

  Serial.print(stateC);
  Serial.println(stateCC);

  //check for conflicting signals
  if( (stateC == HIGH) && (stateCC == HIGH) ) {
    Serial.println("ERROR: CONFLICTING INPUTS");
    return -1;
  }

  //check for Clockwise input
  if (stateC == HIGH && stateCC == LOW) { return CLOCKWISE; }               //clockwise rotate
  else if (stateCC == HIGH && stateC == LOW) { return COUNTER_CLOCKWISE; }  //counter-clockwise rotate
  else { return NEUTRAL_RET; }
}

void ESC_Shift(int command){

    if (command > 0) {

      if(command == CLOCKWISE) {

        Serial.println("CLOCKWISE SHIFT");
        ESC.write(C_SPEED);

      } else if(command == COUNTER_CLOCKWISE) {

        Serial.println("COUNTER-CLOCKWISE SHIFT");
        ESC.write(CC_SPEED);

      }

    } else {

      ESC.write(NEUTRAL_SPEED);

    }
}