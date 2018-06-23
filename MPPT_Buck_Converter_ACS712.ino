/* Simple MPPT solar charge controller for 18V solar panels
   Sparkfun Pro Micro 5V, 16MHz
   ACS712 current sensor on the panel side
   Voltage dividers for voltage measurement on panel and output side
   N-channel mosfet in GND line, freewheel diode, inductor
   Working frequency 31.5kHz
   WARNING! This device is not intended to drive 5V USB devices directly!
   Always use a regulated 5V USB adapter on the output! Otherwise, voltage glichtes may damage your USB device!
   WARNING! This controller is COMMON POSITIVE!
*/

const float codeVersion = 1.2; // Software revision

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
float inputPowerPrevious;
float outputVoltage;
float outputVoltagePrevious;
boolean mpptMode;

// ACS712 current sensor calibration variables
const float acs712VoltsPerAmp = 0.185; // 0.185 for 5A version, 100 for 20A, 66 for 30A
const int acs712Offset = 508; // Zero offset is usually 512 = 1/2 of ADC range


float pwm; // float required for better regulation!
boolean trackingDownwards; // true = downwards
float vcc = 4.5; // Init value only. Will be read automatically later on

// Configuration variables
float minVccVoltage = 4.6; // 4.6
float minPanelVoltage = 12.0; // 12.0
float targetPanelVoltage = 13.0; // 13.0 controlled by MPPT (between min. and max.)
float maxPanelVoltage = 16.0; // 16.0
float targetOutputVoltage = 6.0; // 6.0 (a bit above 5V USB voltage for diode drop in USB module!)
int trackingIncrement = 5; // MPPT tracking PWM steps 5

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
  delay(1000);
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
int averageA() { // Input amps
  static long raw[6];

  raw[5] = raw[4];
  raw[4] = raw[3];
  raw[3] = raw[2];
  raw[2] = raw[1];
  raw[1] = raw[0];
  raw[0] = analogRead(ISENSE_IN);
  int average = (raw[0] + raw[1] + raw[2] + raw[3] + raw[4] + raw[5]) / 6;
  return average;
}

int averageV() { // Input volts
  static long raw[6];

  raw[5] = raw[4];
  raw[4] = raw[3];
  raw[3] = raw[2];
  raw[2] = raw[1];
  raw[1] = raw[0];
  raw[0] = inputVoltage;
  int average = (raw[0] + raw[1] + raw[2] + raw[3] + raw[4] + raw[5]) / 6;
  return average;
}

// Main sensor read function
void readSensors() {
  inputVoltage = analogRead(VSENSE_IN) * vcc / 93; // 1023 = vcc * 110 / 10 = 1023 / 55 = 18.6
  //inputCurrent = analogRead(ISENSE_IN);
  inputCurrent = (averageA() - acs712Offset) * vcc / acs712VoltsPerAmp / 1024;
  //if (inputCurrent < 0) inputCurrent = 0;

  inputPower = averageV() * inputCurrent;
  //inputPower = inputVoltage * inputCurrent;


  outputVoltage = inputVoltage - (analogRead(VSENSE_OUT) * vcc / 92); // 1023 = vcc * 110 / 10 = 1023 / 55 = 18.6
}

//
// =======================================================================================================
// LOCKOUT SUB FUNCTION
// =======================================================================================================
//

void lockout() {
  pwm = 0; // Switch output off
#ifdef DEBUG
  Serial.print("Lockout! ");
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
      - MPPT tracker
  */

  // Read current voltages
  readSensors();

  // Voltage controllers ---------------------------------------------------------------------------------

  // If output voltage is too high or not enough wower to do the MPPT calculations: control target = output voltage! ---
  //if (outputVoltage > (targetOutputVoltage - 0.1) || (inputPower < 0.2) || inputVoltage >= maxPanelVoltage) {
  if (outputVoltage > (targetOutputVoltage - 0.3) || (inputPower < 0.2)) {
    pwm += targetOutputVoltage - outputVoltage; // simple p (differential) controller
    if (inputPower < 0.2) pwm /= 2;
    mpptMode = false;
  }

  // else: control target = MPPT ---
  else {
    mpptMode = true;

    // MPPT (max. output voltage) tracking (upwards / downwards is related to PWM value!)
    static unsigned long lastMppt;
    if (millis() - lastMppt >= 1000) { // Every 1000ms
      lastMppt = millis();

      if (trackingDownwards) pwm -= trackingIncrement;
      else pwm += trackingIncrement;

      Serial.print("MPPT ");
      Serial.println(trackingDownwards);

      // Wrong tracking direction (less power than previously), so change it!
      if (inputPower < inputPowerPrevious) {
        trackingDownwards = !trackingDownwards;

        // Tracking limits
        if (pwm <= 10) trackingDownwards = false;
        if (inputVoltage <= minPanelVoltage) trackingDownwards = true;
        if (inputVoltage >= maxPanelVoltage) trackingDownwards = false;
      }

      //outputVoltagePrevious = outputVoltage; // Store previous voltage for next comparison
      inputPowerPrevious = inputPower;
    }
  }

  // Protection
  if (outputVoltage > (targetOutputVoltage + 1.0)) lockout(); // Output overvoltage protection

  // Write PWM output
  pwm = constrain(pwm, 0, 200 );
#ifdef DEBUG
  serialPrint();
#endif
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
    //Flickering is indicating, that the panels deliver more energy as can be used
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

  static unsigned long lastPrint;
  if (millis() - lastPrint >= 500) { // Every 200ms
    lastPrint = millis();
    Serial.print("Panel Target V: ");
    Serial.print(targetPanelVoltage);
    Serial.print("\t V: ");
    Serial.print(inputVoltage);
    Serial.print("\t A: ");
    Serial.print(inputCurrent);
    Serial.print("\t W: ");
    Serial.print(inputPower);
    Serial.print("\t Prev. Watts: ");
    Serial.print(inputPowerPrevious);

    Serial.print("\t Target: ");
    Serial.print(targetOutputVoltage);
    Serial.print("\t Output: ");
    Serial.print(outputVoltage);
    Serial.print("\t PWM: ");
    Serial.print(pwm);
    //Serial.print("\t down : ");
    //Serial.print(trackingDownwards);
    Serial.print("\t vcc: ");
    Serial.println(vcc);
  }
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
