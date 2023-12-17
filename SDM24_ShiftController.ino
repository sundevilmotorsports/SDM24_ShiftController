// must be 2 or 3 for interrupts!
#define DOWN_INPUT_PIN          2
#define UP_INPUT_PIN            3

// timeouts
#define TIMEOUT_ERROR           1000  // ms
#define TIMEOUT_NEUTRAL_PULSE   10    // ms

// thresholds
#define THRESH_NEUTRAL_PULSE    20

// neutral settings
#define NEUTRAL_TIME            90    // ms
#define NEUTRAL_POWER           50    // %

// internal states
#define S_IDLE                  0
#define S_PULSE_MONITOR         1
#define S_SHIFTING_NEUTRAL      2
#define S_SHIFTING_UP           3
#define S_SHIFTING_DOWN         4
#define S_LOCKOUT               5

// errors
#define E_UPSHIFT               0
#define E_DOWNSHIFT             1

// wire monitors
volatile bool downshiftRose   =   false;
volatile bool downshiftFell   =   false;
volatile bool upshiftFell     =   false;
volatile bool upshiftRose     =   false;

unsigned int controllerState  =   S_IDLE;

unsigned long pulseMonitorLastTriggered = 0;
unsigned int neutralPulseCounter = 0;

unsigned long errorTimeoutStartingMillis = millis();
unsigned int whichLineErrored;

void setup() {

  pinMode(UP_INPUT_PIN, INPUT);
  pinMode(DOWN_INPUT_PIN, INPUT);

  enableInterrupts();

}

void loop() {

  if (controllerState == S_IDLE) {

    // reset wire monitor vars
    downshiftFell = false;
    upshiftFell = false;

    // check wire variables
    if (downshiftRose) { controllerState = S_SHIFTING_DOWN; } 
    else if (upshiftRose) { controllerState = S_PULSE_MONITOR; }

    // TODO: kill output

  } else if (controllerState == S_PULSE_MONITOR) {

    disableDownInterrupt();
    pulseMonitorLastTriggered = millis();

    upshiftRose = false;

    while (pulseMonitorLastTriggered + TIMEOUT_NEUTRAL_PULSE > millis()) {

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

    // disable interrupts and clear monitors
    disableInterrupts();
    downshiftRose   =   false;
    downshiftFell   =   false;
    upshiftFell     =   false;
    upshiftRose     =   false;

    // set power for specified time

    enableInterrupts();
    controllerState = S_IDLE;

  } else if (controllerState == S_SHIFTING_UP) {

    disableDownInterrupt();

    errorTimeoutStartingMillis = millis();    // record when we started the action

    // TODO: set power

    // loop until the timeout has not expired
    while (errorTimeoutStartingMillis + TIMEOUT_ERROR > millis()) {

      // if we see the line has gone down, break out of the loop
      if (upshiftFell) {

        // return to idle
        controllerState = S_IDLE;
        break;

      }

    }

    // if the loop broke because the timer expired
    if (errorTimeoutStartingMillis + TIMEOUT_ERROR < millis()) {

      controllerState = S_LOCKOUT;    // go to the lockout state
      whichLineErrored = E_UPSHIFT;   // record that it was the upshift line that caused the error

    } else {

      enableDownInterrupt();

    }

    downshiftRose   =   false;
    downshiftFell   =   false;
    upshiftFell     =   false;
    upshiftRose     =   false;


  } else if (controllerState == S_SHIFTING_DOWN) {

    errorTimeoutStartingMillis = millis();    // record when we started the action

    // TODO: set power

    // loop until the timeout has not expired
    while (errorTimeoutStartingMillis + TIMEOUT_ERROR > millis()) {

      // if we see the line has gone down, break out of the loop
      if (downshiftFell) {

        // return to idle
        controllerState = S_IDLE;
        break;

      }

    }

    // if the loop broke because the timer expired
    if (errorTimeoutStartingMillis + TIMEOUT_ERROR < millis()) {

      controllerState = S_LOCKOUT;    // go to the lockout state
      whichLineErrored = E_DOWNSHIFT;   // record that it was the downshift line that caused the error

    }

    // clear the line monitors
    downshiftRose   =   false;
    downshiftFell   =   false;
    upshiftFell     =   false;
    upshiftRose     =   false;


  } else if (controllerState == S_LOCKOUT) {

    // TODO: kill power

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

    enableInterrupts();
    controllerState = S_IDLE;

  } else { controllerState = S_IDLE; }

  

}

void ISRDown() {

  disableInterrupts();

  if (digitalRead(DOWN_INPUT_PIN)) {
    downshiftRose = true;
  } else {
    downshiftFell = true;
  }

  enableInterrupts();
  
}

void ISRUp() {

  disableInterrupts();

  if (digitalRead(UP_INPUT_PIN)) {
    upshiftRose = true;
  } else {
    upshiftFell = true;
  }

  enableInterrupts();
  
}

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
