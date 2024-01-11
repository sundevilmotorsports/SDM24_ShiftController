#include <Servo.h>
// must be 2 or 3 for interrupts! on Nano
#define DOWN_INPUT_PIN          2
#define UP_INPUT_PIN            3
// timeouts
#define TIMEOUT_ERROR           500  // ms
#define TIMEOUT_NEUTRAL_PULSE   10    // ms
// thresholds
#define THRESH_NEUTRAL_PULSE    20
// neutral settings
#define N_SHIFT_TIME            50    // ms
#define NEUTRAL_POWER           50    // %
// internal states
#define S_IDLE                  0
#define S_PULSE_MONITOR         1
#define S_SHIFTING_NEUTRAL      2
#define S_SHIFTING_UP           3
#define S_SHIFTING_DOWN         4
#define S_LOCKOUT               5
// Errors
#define E_UPSHIFT               0
#define E_DOWNSHIFT             1
//ESC constants
#define C_SPEED 0         // 0% PWM duty cycle
#define CC_SPEED 180      // 100% PWM duty cycle
#define NEUTRAL_SPEED 90  // 50% PWM duty cycle
//Shift Time Gets defined by the pulse width?
#define MIN_PWM 1000      //ms
#define MAX_PWM 2000      //ms
//Outputs
#define ESC_OUTPUT_PIN 9
#define COUNTER_CLOCKWISE 3
#define NEUTRAL_RET 4

// wire monitors
volatile bool downshiftRose   =   false;
volatile bool downshiftFell   =   false;
volatile bool upshiftFell     =   false;
volatile bool upshiftRose     =   false;

unsigned int controllerState  =   S_IDLE;
//neutral PWM detection variables
unsigned long pulseMonitorLastTriggered = 0;
unsigned int neutralPulseCounter = 0;
//TODO Need to define what the actual neutral state pulse looks like need to scope signal from ECU
//error variables
unsigned long errorTimeoutStartingMillis = millis();
unsigned int whichLineErrored;

//Servo vars
Servo ESC;     // create servo object to control the ESC
//Clockwise rotation -> downshift
//Counter-clockwise -> upshift
//TODO verify with actual motor setup directions are right

void setup() {

  Serial.begin(115200);

  //setup inputs
  pinMode(UP_INPUT_PIN, INPUT);
  pinMode(DOWN_INPUT_PIN, INPUT);
  //outputs
  ESC.attach(ESC_OUTPUT_PIN, 1000, 2000); // (pin, min pulse width, max pulse width in microseconds)
  enableInterrupts();
  //Set initial ESC speed(position)
  ESC.write(NEUTRAL_SPEED);
}

void loop() {

  //Serial.println(controllerState);
  //Act for each state
  if (controllerState == S_IDLE) {
    //Turn off ESC
    ESC.write(NEUTRAL_SPEED);
    // reset wire monitor vars
    downshiftFell = false;
    upshiftFell = false;
    // check wire variables
    if (downshiftRose) { controllerState = S_SHIFTING_DOWN; } 
    else if (upshiftRose) { controllerState = S_PULSE_MONITOR; }

  } else if (controllerState == S_PULSE_MONITOR) {

    disableDownInterrupt();
    pulseMonitorLastTriggered = millis();

    upshiftRose = false;
    //poll for PWM from ECU
    while ((pulseMonitorLastTriggered + TIMEOUT_NEUTRAL_PULSE) > millis()) {

      if (upshiftRose) {

        neutralPulseCounter += 1;
        pulseMonitorLastTriggered = millis();
        upshiftRose = false;

      }

      if (neutralPulseCounter >= THRESH_NEUTRAL_PULSE) {

        neutralPulseCounter = 0;
        controllerState = S_SHIFTING_NEUTRAL;
        break;

      }

    }

    if (neutralPulseCounter < THRESH_NEUTRAL_PULSE) {

      controllerState = S_SHIFTING_UP;

    }

  } else if (controllerState == S_SHIFTING_NEUTRAL) {
    //Neutral Actuation
    // disable interrupts and clear monitors
    disableInterrupts();
    downshiftRose   =   false;
    downshiftFell   =   false;
    upshiftFell     =   false;
    upshiftRose     =   false;

    
    //TODO could add gear position then make sure we are in 1st before
    //doing a neutral shift
    //Shift neutral
    ESC.write(CC_SPEED);
    delay(N_SHIFT_TIME);
    ESC.write(NEUTRAL_SPEED);
    controllerState = S_IDLE;
    enableInterrupts();

  } else if (controllerState == S_SHIFTING_UP) {
    //Upshift actuation
    disableDownInterrupt();//turn off downshift interrupts
    errorTimeoutStartingMillis = millis();    // record when we started the action

    //Shift UP
    ESC.write(CC_SPEED); //TURN ON OUTPUT
    // loop until the timeout has expired
    while ((errorTimeoutStartingMillis + TIMEOUT_ERROR) >= millis()) {
      // if we see the line has gone down, break out of the loop
      if (upshiftFell) {
        //TURN OFF OUTPUT
        ESC.write(NEUTRAL_SPEED);
        // return to idle
        controllerState = S_IDLE;
        break;
      }
    }

    // if the loop broke because the timer expired
    if ((errorTimeoutStartingMillis + TIMEOUT_ERROR) <= millis()) {
      //TURN OFF OUTPUT immediately
      ESC.write(NEUTRAL_SPEED);
      controllerState = S_LOCKOUT;    // go to the lockout state
      whichLineErrored = E_UPSHIFT;   // record that it was the upshift line that caused the error
    } else {
      enableDownInterrupt();//reenable Down shifting inputs
    }
    //Reset Globals to defaults
    downshiftRose   =   false;
    downshiftFell   =   false;
    upshiftFell     =   false;
    upshiftRose     =   false;

  } else if (controllerState == S_SHIFTING_DOWN) {
    //Downshift logic
    disableUpInterrupt();//turn off upshift interrupts
    errorTimeoutStartingMillis = millis();    // record when we started the action

    //SHIFT DOWN
    ESC.write(C_SPEED);//TURN ON OUTPUT
    // loop until the timeout has not expired
    while ((errorTimeoutStartingMillis + TIMEOUT_ERROR) >= millis()) {
      // if we see the line has gone down, break out of the loop
      if (downshiftFell) {
        //TURN OFF OUTPUT
        ESC.write(NEUTRAL_SPEED);
        // return to idle
        controllerState = S_IDLE;
        break;
      }
    }

    // if the loop broke because the timer expired
    if ((errorTimeoutStartingMillis + TIMEOUT_ERROR) <= millis()) {
      ESC.write(NEUTRAL_SPEED);       //Turn off
      controllerState = S_LOCKOUT;    // go to the lockout state
      whichLineErrored = E_DOWNSHIFT;   // record that it was the downshift line that caused the error
    } else {
      enableUpInterrupt();
    }
    // clear the line monitors
    downshiftRose   =   false;
    downshiftFell   =   false;
    upshiftFell     =   false;
    upshiftRose     =   false;

  } else if (controllerState == S_LOCKOUT) {
    Serial.println("IN LOCKOUT");
    // Set to Neutral
    ESC.write(NEUTRAL_SPEED);
    // hold until the line releases
    while (true) {
      if (whichLineErrored == E_DOWNSHIFT) {
        if (downshiftFell) {
          break;
        }
      } else if (whichLineErrored == E_UPSHIFT) {
        if (upshiftFell) {
          break;
        }
      }
    }
    // clear line monitors
    downshiftRose   =   false;
    downshiftFell   =   false;
    upshiftFell     =   false;
    upshiftRose     =   false;
    //Re-enable normal operation inputs
    enableInterrupts();
    controllerState = S_IDLE;
    Serial.println("OUT LOCKOUT");
  } else { controllerState = S_IDLE; }
}

void ISRDown() {
  disableInterrupts();
  if (digitalRead(DOWN_INPUT_PIN)) {
    downshiftRose = true;
    Serial.println("DOWNSHIFT - RISE");
  } else {
    downshiftFell = true;
    Serial.println("DOWNSHIFT - FALL");
  }
  enableInterrupts();
}

void ISRUp() {
  disableInterrupts();
  if (digitalRead(UP_INPUT_PIN)) {
    upshiftRose = true;
    Serial.println("UPSHIFT - RISE");
  } else {
    upshiftFell = true;
    Serial.println("DOWNSHIFT - FALL");
  }
  enableInterrupts();
}
//All Interrupt Methods below
void disableUpInterrupt() {
  detachInterrupt(digitalPinToInterrupt(UP_INPUT_PIN));
}

void disableDownInterrupt() {
  detachInterrupt(digitalPinToInterrupt(DOWN_INPUT_PIN));
}

void enableUpInterrupt() {
  attachInterrupt(digitalPinToInterrupt(UP_INPUT_PIN), ISRUp, CHANGE);
}

void enableDownInterrupt() {
  attachInterrupt(digitalPinToInterrupt(DOWN_INPUT_PIN), ISRDown, CHANGE);
}

void disableInterrupts() {
  disableDownInterrupt();
  disableUpInterrupt();
}

void enableInterrupts() {
  enableDownInterrupt();
  enableUpInterrupt();
}
