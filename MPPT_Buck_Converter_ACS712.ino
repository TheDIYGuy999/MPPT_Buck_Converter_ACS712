/* Simple MPPT solar charge controller for 18V solar panels
   Sparkfun Pro Micro 5V, 16MHz
   ACS712 current sensor on the panel side
   Voltage dividers for voltage measurement on panel and output side
   N-channel mosfet in GND line, freewheel diode, inductor
   Supplied by the panel voltage, can't drain your battery
   Working frequency 31.5kHz
   WARNING! This device is not intended to drive 5V USB devices directly!
   Always use a regulated 5V USB adapter on the output! Otherwise, voltage glichtes may damage your USB device!
   WARNING! This controller is COMMON POSITIVE!
*/

const float codeVersion = 1.3; // Software revision

//
// =======================================================================================================
// BUILD OPTIONS (comment out unneeded options)
// =======================================================================================================
//

#define DEBUG // if not commented out, Serial.print() is active! For debugging only!!

//
// =======================================================================================================
// INCLUDE LIRBARIES
// =======================================================================================================
//

// Tabs (header files in sketch directory)
#include "readVCC.h"

// Libraries
#include <statusLED.h> // TheDIYGuy999 library: https://github.com/TheDIYGuy999/statusLED
#include <PWMFrequency.h> // https://github.com/TheDIYGuy999/PWMFrequency

//
// =======================================================================================================
// PIN ASSIGNMENTS & GLOBAL VARIABLES
// =======================================================================================================
//

// Status LED objects
statusLED LED(true); // true = inversed

// output pins
#define PWM 10
#define ISENSE_IN A2
#define VSENSE_IN A1
#define VSENSE_OUT A0
#define POT A3

// Global variables
float inputVoltage;
float inputCurrent;
float inputPower;
float inputPowerDelta;
float inputPowerPrevious = 9999.0; // Init value above max. panel power!
float outputVoltage;
float outputVoltagePrevious;
boolean mpptMode;

// ACS712 current sensor calibration variables
const float acs712VoltsPerAmp = 0.185; // 0.185 for 5A version, 100 for 20A, 66 for 30A
const int acs712Offset = 508; // Zero offset is usually 512 = 1/2 of ADC range


float pwm; // float required for finer granularity during calculadion in differential equations!
boolean trackingDownwards; // true = downwards
float vcc = 4.5; // Init value only. Will be read automatically later on

// Configuration variables
float minPanelVoltage = 12.0; // 12.0
float targetPanelVoltage = 14.5; // 14.0 (calculated by MPPT algorithm)
float maxPanelVoltage = 17.0; // 16.0
float targetOutputVoltage = 5.5; // 6.0 (a bit above 5V USB voltage for diode drop in USB module!)
float trackingIncrement = 0.5; // MPPT tracking voltage step 0.5V

//
// =======================================================================================================
// MAIN ARDUINO SETUP (1x during startup)
// =======================================================================================================
//

void setup() {

  // serial port setup
#ifdef DEBUG
  Serial.begin(19200);
#endif

  // PWM frequency
  setPWMPrescaler(PWM, 1 );  // 1 = 31.5kHz

  // output setup
  pinMode(PWM, OUTPUT);

  // LED setup
  LED.begin(17); // Onboard LED on pin 17

  // switch off output
  analogWrite(PWM, 0);
}

//
// =======================================================================================================
// READ POTENTIOMETER (inactive, for testing only)
// =======================================================================================================
//
void readPot() {
  //minPanelVoltage = analogRead(POT) / 102.3 + 7.0; // 7 - 17V
}

//
// =======================================================================================================
// READ SENSORS
// =======================================================================================================
//

// Averaging subfunctions
float averageP() { // Input power (running average)
  static float raw[10];

  raw[9] = raw[8];
  raw[8] = raw[7];
  raw[7] = raw[6];
  raw[6] = raw[5];
  raw[5] = raw[4];
  raw[4] = raw[3];
  raw[3] = raw[2];
  raw[2] = raw[1];
  raw[1] = raw[0];
  raw[0] = inputVoltage * inputCurrent;
  float average = (raw[0] + raw[1] + raw[2] + raw[3] + raw[4] + raw[5] + raw[6] + raw[7] + raw[8] + raw[9]) / 10.0;
  return inputVoltage * inputCurrent;//average;
}


// Main sensor read function
void readSensors() {

  inputVoltage = analogRead(VSENSE_IN) * vcc / 93; // 1023 = vcc * 110 / 10 = 1023 / 55 = 18.6

  inputCurrent = (analogRead(ISENSE_IN) - acs712Offset) * vcc / acs712VoltsPerAmp / 1024;

  inputPower = averageP();

  outputVoltage = inputVoltage - (analogRead(VSENSE_OUT) * vcc / 92); // 1023 = vcc * 110 / 10 = 1023 / 55 = 18.6
}

//
// =======================================================================================================
// LOCKOUT SUB FUNCTION
// =======================================================================================================
//

void lockout() {
  analogWrite(PWM, 0); // Switch output off
#ifdef DEBUG
  Serial.print("Over voltage lockout! ");
  Serial.println(outputVoltage);
#endif
}

//
// =======================================================================================================
// MPPT TRACKING
// =======================================================================================================
//
void mppt() {

  /* MPPT Strategy:
      There are two controllers:
      - Output voltage controller
      - MPPT tracker, if output voltage is below target
  */

  // Read current voltages
  readSensors();

  // Panel undervoltage lockout ---------------------------------------------------------------------------
  while (outputVoltage < 3.0 && inputVoltage < 15.0) {  // 3.0 15.0
    analogWrite(PWM, 0);
    LED.on();
    Serial.println("Panel undervoltage, waiting for more sun...");
    delay(2000);
    checkVcc();
    readSensors();
  }

  // Voltage controllers ---------------------------------------------------------------------------------

  // If output voltage is too high or not enough power to do the MPPT calculations: control target = output voltage! ---
  //if (outputVoltage > (targetOutputVoltage - 0.2) || (inputPower < 0.3)) {
  if (outputVoltage > (targetOutputVoltage - 0.2)) {
    pwm += targetOutputVoltage - outputVoltage; // simple p (differential) controller
    mpptMode = false;
  }

  // else: control target = MPPT ---
  else {
    mpptMode = true;

    // MPPT (max. input power) tracking direction (upwards / downwards is related to panel voltage!)
    static unsigned long lastMppt;
    if (millis() - lastMppt >= 1000) { // Every 1000ms
      lastMppt = millis();

      // Calculate power delta
      inputPowerDelta = inputPower - inputPowerPrevious;

      if (trackingDownwards) targetPanelVoltage -= trackingIncrement;
      else targetPanelVoltage += trackingIncrement;
#ifdef DEBUG
      Serial.print("MPPT ");
      Serial.println(trackingDownwards);
#endif


      // Tracking direction is depending on the panel voltage, if outside limits!
      if (targetPanelVoltage <= minPanelVoltage) {
        targetPanelVoltage = (minPanelVoltage + maxPanelVoltage) / 2;
#ifdef DEBUG
        Serial.println("PV lim. -");
#endif
      }
      else if (targetPanelVoltage >= maxPanelVoltage) {
        targetPanelVoltage = (minPanelVoltage + maxPanelVoltage) / 2;
#ifdef DEBUG
        Serial.println("PV lim. +");
#endif
      }

      else { // if within voltage limits, search for maximum power point!
        // Wrong tracking direction (less power than previously), so change it!
        if (inputPowerDelta < 0.0) { // 0.03A current sensor step * 20V = 0.6W
          trackingDownwards = !trackingDownwards;
        }
      }

      // Store previous power for next comparison
      inputPowerPrevious = inputPower;
    }

    // Calculate deviation
    static unsigned long lastCalc;
    if (millis() - lastCalc >= 50) { // Every 50ms (prevent it from oscillating in low light condidions)
      lastCalc = millis();
      pwm -= targetPanelVoltage - inputVoltage; // simple p (differential) controller
    }
  }

  // Protection ----------------------------------------------------------------------------------------
  if (outputVoltage > (targetOutputVoltage + 1.0)) lockout(); // Output overvoltage protection

  // Write PWM output ----------------------------------------------------------------------------------
  pwm = constrain(pwm, 0, 150 );
  //pwm = analogRead(POT) * 256 / 1024;
  serialPrint();
  analogWrite(PWM, pwm);
}

//
// =======================================================================================================
// LED
// =======================================================================================================
//
void led() {
  if (mpptMode) {
    // Indicate panel voltage: 4 flashes = 14V etc.
    LED.flash(20, 380, 700, inputVoltage); // ON, OFF, PAUSE, PULSES
  }
  else {
    //Flickering is indicating, that the panels could deliver more energy as currently can be used
    LED.flash(30, 100, 0, 0);
  }
}

//
// =======================================================================================================
// CHECK VCC VOLTAGE
// =======================================================================================================
//
void checkVcc() {

  static unsigned long lastVcc;
  if (millis() - lastVcc >= 1000) { // Every 1000ms
    lastVcc = millis();
    vcc = readVcc() / 1000.0;
  }
}

//
// =======================================================================================================
// SERIAL PRINT
// =======================================================================================================
//
void serialPrint() {
#ifdef DEBUG
  static unsigned long lastPrint;
  if (millis() - lastPrint >= 1000) { // Every 1000ms
    lastPrint = millis();

    Serial.print("In T. V: ");
    Serial.print(targetPanelVoltage);
    Serial.print("\t In V: ");
    Serial.print(inputVoltage);
    Serial.print("\t A: ");
    Serial.print(inputCurrent);
    Serial.print("\t W: ");
    Serial.print(inputPower);
    Serial.print("\t Delta Watts: ");
    Serial.print(inputPowerDelta);

    Serial.print("\t Out T. V: ");
    Serial.print(targetOutputVoltage);
    Serial.print("\t Out V: ");
    Serial.print(outputVoltage);
    Serial.print("\t PWM: ");
    Serial.print(pwm);
    //Serial.print("\t down : ");
    //Serial.print(trackingDownwards);
    Serial.print("\t vcc: ");
    Serial.println(vcc);
  }
#endif
}

//
// =======================================================================================================
// MAIN LOOP
// =======================================================================================================
//

void loop() {
  readPot();
  mppt();
  led();
  checkVcc();
}
