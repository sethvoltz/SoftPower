/*
   Soft Power

   On the Raspberry Pi, add the following line to /boot/config.txt and hook RASPBERRY_SHUTDOWN_PIN
   to Raspberry Pi physical pin 7 (GPIO 4):
     dtoverlay=gpio-shutdown,gpio_pin=4
 */

#include <FadeLed.h>
#include <Button.h>
#include <Fsm.h>


// =--------------------------------------------------------------------------------= Constants =--=

#define HOLD_PIN                      0       // Output to hold power on, pull low to power down
#define RASPBERRY_SENSE_PIN           1       // Sense pin to detect if the Pi is powered down
#define RASPBERRY_SHUTDOWN_PIN        2       // dtoverlay pin for triggered power down
#define BUTTON_PIN                    3       // Sense pin for power button, also does soft power up
#define LED_PIN                       4       // LED pin inside the power button
#define SOFT_HALT_TIME_MS             3000    // Long press ms for triggering soft powering down
#define HARD_HALT_TIME_MS             10000   // Long press ms for triggering hard power off
#define STATE_CHANGE_TIME_MS          1000    // LED fade duration for state change to off or on
#define SLOW_PULSE_TIME_MS            500     // LED fade duration for slow pulse animation
#define FAST_PULSE_TIME_MS            100     // LED fade duration for fast pulse animation

#define TRANSITION_POWER_DOWN         1
#define TRANSITION_SAFE_HALT          2
#define TRANSITION_UNSAFE_HALT        3


// =------------------------------------------------------------------------------------= Enums =--=

enum ledStates {
  OFF,
  ON,
  SLOW,
  FAST
};


// =-----------------------------------------------------------------------------------= States =--=

State stateNormal(&onNormalEnter, NULL, NULL);
State statePowerDown(&onPowerDownEnter, &onPowerDownRun, NULL);
State stateHalt(&onHaltEnter, NULL, NULL);
Fsm stateMachine(&stateNormal);


// =----------------------------------------------------------------------------------= Globals =--=

Button senseButton(BUTTON_PIN);
FadeLed statusLed(LED_PIN);
ledStates ledState = OFF;


// =--------------------------------------------------------------------------= State Callbacks =--=

void onNormalEnter() {
  Serial.println("State: Normal - Enter");
  // Pull the two control pins high to keep main power and the Raspberry Pi on
  digitalWrite(HOLD_PIN, HIGH);
  digitalWrite(RASPBERRY_SHUTDOWN_PIN, HIGH);
  ledState = ON;
}

void onPowerDownEnter() {
  Serial.println("State: Power Down - Enter");
  // Pull the Raspberry Pi control pin low to trigger the GPIO overlay on it's end
  digitalWrite(RASPBERRY_SHUTDOWN_PIN, LOW);
  ledState = SLOW;
}

void onPowerDownRun() {
  if (!digitalRead(RASPBERRY_SENSE_PIN)) {
    Serial.println("Raspberry Pi sense detected! Powering down");
    stateMachine.trigger(TRANSITION_SAFE_HALT);
  }
}

void onHaltEnter() {
  Serial.println("State: Halt - Enter");
  digitalWrite(HOLD_PIN, LOW);
}

void onTransNormalPowerDown() {
  Serial.println("Transition: Normal -> Power Down");
}

void onTransPowerDownHaltSafe() {
  Serial.println("Transition: Power Down -> Halt (Safe)");
}

void onTransPowerDownHaltUnsafe() {
  Serial.println("Transition: Power Down -> Halt (Unsafe)");
  ledState = FAST;
}


// =----------------------------------------------------------------------------= Setup and Run =--=

void setup() {
  Serial.begin(115200);
  delay(100);

  holdSetup();
  buttonSetup();
  ledSetup();
  stateSetup();
}

void loop() {
  buttonLoop();
  ledLoop();
  stateLoop();
}


// =------------------------------------------------------------------= Setup and Run Functions =--=

void holdSetup() {
  pinMode(HOLD_PIN, OUTPUT);
  pinMode(RASPBERRY_SENSE_PIN, INPUT_PULLDOWN);
  pinMode(RASPBERRY_SHUTDOWN_PIN, OUTPUT);
}

// State Setup & Run

void stateSetup() {
  stateMachine.add_transition(
    &stateNormal, &statePowerDown,
    TRANSITION_POWER_DOWN,
    &onTransNormalPowerDown);

  stateMachine.add_transition(
    &statePowerDown, &stateHalt,
    TRANSITION_SAFE_HALT,
    &onTransPowerDownHaltSafe);

  stateMachine.add_transition(
    &statePowerDown, &stateHalt,
    TRANSITION_UNSAFE_HALT,
    &onTransPowerDownHaltUnsafe);
}

void stateLoop() {
  stateMachine.run_machine();
}

// LED Setup and Run

void ledSetup() {
  pinMode(LED_PIN, OUTPUT);
  statusLed.setTime(SLOW_PULSE_TIME_MS, true);
  statusLed.on();
}

void ledLoop() {
  FadeLed::update();

  if (ledState == OFF || ledState == ON) {
    statusLed.setTime(STATE_CHANGE_TIME_MS);

    if (ledState == OFF) {
      statusLed.off();
    } else {
      statusLed.on();
    }
  } else {
    if (statusLed.done()) {
      statusLed.setTime(ledState == SLOW ? SLOW_PULSE_TIME_MS : FAST_PULSE_TIME_MS, true);

      if (statusLed.get()) {
        statusLed.off();
      } else {
        statusLed.on();
      }
    }
  }
}

// Button Setup and Run

void buttonSetup() {
  senseButton.begin();
}

void buttonLoop() {
  static unsigned long buttonDownTime = millis();
  static unsigned long buttonIsPressed = false;

  if (senseButton.pressed()) {
      Serial.println("Button Down");
      buttonIsPressed = true;
      buttonDownTime = millis();
  }

  if (senseButton.released()) {
    Serial.println("Button Up");
    buttonIsPressed = false;
  }

  if (buttonIsPressed) {
    unsigned long downTimeDiff = millis() - buttonDownTime;
    if (downTimeDiff > SOFT_HALT_TIME_MS) {
      stateMachine.trigger(TRANSITION_POWER_DOWN);
    }

    if (downTimeDiff > HARD_HALT_TIME_MS) {
      stateMachine.trigger(TRANSITION_UNSAFE_HALT);
    }
  }
}
