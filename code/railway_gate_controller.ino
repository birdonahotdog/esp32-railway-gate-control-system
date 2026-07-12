#include <ESP32Servo.h>

// =====================================================
// SENSOR POLARITY CONFIG (from your measured readings)
// =====================================================
#define INVERT_VSETC   0
#define INVERT_VSETAC  0

#define INVERT_IRSB    1
#define INVERT_IRRG    1
#define INVERT_IRBS    1
#define INVERT_IRGR    1

#define INVERT_IR5     1
#define INVERT_IR6     1

#define DEBOUNCE_MS 50

// Buzzer beep pattern timing (bip-bip-bip)
#define BUZZER_ON_MS  150
#define BUZZER_OFF_MS 150

// Obstacle gate hold time
#define OBSTACLE_DURATION_MS 3000

// =====================================================
// PIN MAPPING
// =====================================================

const int Vsetc  = 34;
const int Vsetac = 36;

const int IRsb = 13;
const int IRrg = 32;
const int IRbs = 18;
const int IRgr = 19;

const int IR5 = 22;
const int IR6 = 21;

const int ServoLeft  = 23;
const int ServoRight = 25;
const int Buzzer     = 26;

// =====================================================
// SERVO OBJECTS
// =====================================================

Servo servoLeft;
Servo servoRight;

const int GATE_OPEN_ANGLE   = 90;
const int GATE_CLOSED_ANGLE = 0;

// =====================================================
// SENSOR CURRENT / PREVIOUS (DEBOUNCED, LOGICAL) STATES
// =====================================================

int curVsetc, curVsetac;
int curIRsb, curIRrg, curIRbs, curIRgr;
int curIR5, curIR6;

int prevVsetc = LOW, prevVsetac = LOW;
int prevIRsb = LOW, prevIRrg = LOW, prevIRbs = LOW, prevIRgr = LOW;
int prevIR5 = LOW, prevIR6 = LOW;

int rawVsetc = LOW, rawVsetac = LOW;
int rawIRsb = LOW, rawIRrg = LOW, rawIRbs = LOW, rawIRgr = LOW;
int rawIR5 = LOW, rawIR6 = LOW;

unsigned long tVsetc = 0, tVsetac = 0;
unsigned long tIRsb = 0, tIRrg = 0, tIRbs = 0, tIRgr = 0;
unsigned long tIR5 = 0, tIR6 = 0;

// =====================================================
// TRAIN COUNTERS
// =====================================================

int activeCW  = 0;
int activeACW = 0;

// =====================================================
// SET AUTHENTICATION COUNTERS (consumed immediately after use)
// =====================================================

int cwSetAuth  = 0; // Vsetc  -> IRsb (SET)
int acwSetAuth = 0; // Vsetac -> IRbs (SET)

// RESET no longer requires vibration authentication (Vrc/Vrac removed).
// IRrg (CW) and IRgr (ACW) trigger RESET directly on rising edge.

// =====================================================
// BUZZER STATE (non-blocking bip-bip-bip pattern)
// =====================================================

bool buzzerActive = false; // true while a train sequence wants the buzzer beeping
bool ir5Active     = false;
bool ir6Active     = false;
unsigned long ir5StartTime = 0;
unsigned long ir6StartTime = 0;

bool buzzerPinState = false;
unsigned long buzzerLastToggle = 0;

// =====================================================
// GATE STATE TRACKING
// =====================================================

bool leftGateOpen  = true;
bool rightGateOpen = true;

// =====================================================
// GATE CONTROL HELPERS
// =====================================================

void openLeftGate() {
  if (!leftGateOpen) {
    servoLeft.write(GATE_OPEN_ANGLE);
    leftGateOpen = true;
  }
}

void closeLeftGate() {
  if (leftGateOpen) {
    servoLeft.write(GATE_CLOSED_ANGLE);
    leftGateOpen = false;
  }
}

void openRightGate() {
  if (!rightGateOpen) {
    servoRight.write(GATE_OPEN_ANGLE);
    rightGateOpen = true;
  }
}

void closeRightGate() {
  if (rightGateOpen) {
    servoRight.write(GATE_CLOSED_ANGLE);
    rightGateOpen = false;
  }
}

void openBothGates()  { openLeftGate();  openRightGate(); }
void closeBothGates() { closeLeftGate(); closeRightGate(); }

// Called right after a RESET decrements a counter. If no trains
// remain active in either direction, open gates (buzzer is already
// stopped at gate-close time, this is just a safety net).
void openGatesIfAllClear() {
  if ((activeCW + activeACW) == 0) {
    openBothGates();
    buzzerActive = false;
  }
}

// =====================================================
// EDGE DETECTION HELPER
// =====================================================

bool risingEdge(int current, int previous) {
  return (current == HIGH && previous == LOW);
}

// =====================================================
// DEBOUNCE HELPER
// =====================================================

int readDebounced(int pin, bool invert, int &rawState, unsigned long &lastChangeTime, int lastStableState) {
  int rawRead = digitalRead(pin);
  int logical = invert ? !rawRead : rawRead;

  if (logical != rawState) {
    rawState = logical;
    lastChangeTime = millis();
  }

  if ((millis() - lastChangeTime) >= DEBOUNCE_MS) {
    return rawState;
  }

  return lastStableState;
}

// =====================================================
// NON-BLOCKING BUZZER PATTERN UPDATE
// =====================================================
// Sounds bip-bip-bip whenever buzzerActive OR ir5Active OR ir6Active
// is true. Silent otherwise. Never blocks the loop.

void updateBuzzerPattern() {
  bool shouldBeep = buzzerActive || ir5Active || ir6Active;

  if (!shouldBeep) {
    if (buzzerPinState) {
      buzzerPinState = false;
      digitalWrite(Buzzer, LOW);
    }
    return;
  }

  unsigned long now = millis();
  unsigned long interval = buzzerPinState ? BUZZER_ON_MS : BUZZER_OFF_MS;

  if (now - buzzerLastToggle >= interval) {
    buzzerPinState = !buzzerPinState;
    buzzerLastToggle = now;
    digitalWrite(Buzzer, buzzerPinState ? HIGH : LOW);
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);

  pinMode(Vsetc,  INPUT);
  pinMode(Vsetac, INPUT);

  pinMode(IRsb, INPUT);
  pinMode(IRrg, INPUT);
  pinMode(IRbs, INPUT);
  pinMode(IRgr, INPUT);

  pinMode(IR5, INPUT);
  pinMode(IR6, INPUT);

  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);

  servoLeft.attach(ServoLeft);
  servoRight.attach(ServoRight);

  servoLeft.write(GATE_OPEN_ANGLE);
  servoRight.write(GATE_OPEN_ANGLE);
  leftGateOpen  = true;
  rightGateOpen = true;

  activeCW  = 0;
  activeACW = 0;

  cwSetAuth  = 0;
  acwSetAuth = 0;

  buzzerActive = false;
  ir5Active = false;
  ir6Active = false;

  rawVsetc  = INVERT_VSETC  ? !digitalRead(Vsetc)  : digitalRead(Vsetc);
  rawVsetac = INVERT_VSETAC ? !digitalRead(Vsetac) : digitalRead(Vsetac);

  rawIRsb = INVERT_IRSB ? !digitalRead(IRsb) : digitalRead(IRsb);
  rawIRrg = INVERT_IRRG ? !digitalRead(IRrg) : digitalRead(IRrg);
  rawIRbs = INVERT_IRBS ? !digitalRead(IRbs) : digitalRead(IRbs);
  rawIRgr = INVERT_IRGR ? !digitalRead(IRgr) : digitalRead(IRgr);

  rawIR5 = INVERT_IR5 ? !digitalRead(IR5) : digitalRead(IR5);
  rawIR6 = INVERT_IR6 ? !digitalRead(IR6) : digitalRead(IR6);

  prevVsetc  = rawVsetc;
  prevVsetac = rawVsetac;

  prevIRsb = rawIRsb;
  prevIRrg = rawIRrg;
  prevIRbs = rawIRbs;
  prevIRgr = rawIRgr;

  prevIR5 = rawIR5;
  prevIR6 = rawIR6;

  unsigned long now = millis();
  tVsetc = tVsetac = now;
  tIRsb = tIRrg = tIRbs = tIRgr = now;
  tIR5 = tIR6 = now;

  Serial.println("System ready. Vrc/Vrac removed - reset now triggers directly on IRrg/IRgr.");
}

// =====================================================
// READ SENSORS
// =====================================================

void readSensors() {
  curVsetc  = readDebounced(Vsetc,  INVERT_VSETC,  rawVsetc,  tVsetc,  prevVsetc);
  curVsetac = readDebounced(Vsetac, INVERT_VSETAC, rawVsetac, tVsetac, prevVsetac);

  curIRsb = readDebounced(IRsb, INVERT_IRSB, rawIRsb, tIRsb, prevIRsb);
  curIRrg = readDebounced(IRrg, INVERT_IRRG, rawIRrg, tIRrg, prevIRrg);
  curIRbs = readDebounced(IRbs, INVERT_IRBS, rawIRbs, tIRbs, prevIRbs);
  curIRgr = readDebounced(IRgr, INVERT_IRGR, rawIRgr, tIRgr, prevIRgr);

  curIR5 = readDebounced(IR5, INVERT_IR5, rawIR5, tIR5, prevIR5);
  curIR6 = readDebounced(IR6, INVERT_IR6, rawIR6, tIR6, prevIR6);
}

// =====================================================
// CLOCKWISE STATE MACHINE
// =====================================================
// Vsetc -> auth IRsb(SET) -> IRbs(BUZZER ON, only if CW active)
// -> IRgr(CLOSE GATES + BUZZER OFF, only if CW active) -> IRrg(RESET, direct)

void updateClockwiseStateMachine() {
  if (risingEdge(curVsetc, prevVsetc)) {
    cwSetAuth++;
    Serial.println("[CW] Vsetc auth++");
  }

  if (risingEdge(curIRsb, prevIRsb)) {
    if (cwSetAuth > 0) {
      cwSetAuth--;
      activeCW++;
      Serial.printf("[CW] SET fired. activeCW=%d\n", activeCW);
    }
  }

  if (risingEdge(curIRbs, prevIRbs)) {
    if (activeCW > 0) {
      buzzerActive = true;
      Serial.println("[CW] BUZZER ON");
    }
  }

  if (risingEdge(curIRgr, prevIRgr)) {
    if (activeCW > 0) {
      closeBothGates();
      buzzerActive = false;
      Serial.println("[CW] GATES CLOSED, buzzer OFF");
    }
  }

  if (risingEdge(curIRrg, prevIRrg)) {
    if (activeCW > 0) {
      activeCW--;
      Serial.printf("[CW] RESET fired. activeCW=%d\n", activeCW);
      openGatesIfAllClear();
    }
  }
}

// =====================================================
// ANTI-CLOCKWISE STATE MACHINE
// =====================================================
// Vsetac -> auth IRbs(SET) -> IRsb(BUZZER ON, only if ACW active)
// -> IRrg(CLOSE GATES, only if ACW active) -> IRgr(RESET, direct)

void updateAntiClockwiseStateMachine() {
  if (risingEdge(curVsetac, prevVsetac)) {
    acwSetAuth++;
    Serial.println("[ACW] Vsetac auth++");
  }

  if (risingEdge(curIRbs, prevIRbs)) {
    if (acwSetAuth > 0) {
      acwSetAuth--;
      activeACW++;
      Serial.printf("[ACW] SET fired. activeACW=%d\n", activeACW);
    }
  }

  if (risingEdge(curIRsb, prevIRsb)) {
    if (activeACW > 0) {
      buzzerActive = true;
      Serial.println("[ACW] BUZZER ON");
    }
  }

  if (risingEdge(curIRrg, prevIRrg)) {
    if (activeACW > 0) {
      closeBothGates();
      buzzerActive = false;
      Serial.println("[ACW] GATES CLOSED, buzzer OFF");
    }
  }

  if (risingEdge(curIRgr, prevIRgr)) {
    if (activeACW > 0) {
      activeACW--;
      Serial.printf("[ACW] RESET fired. activeACW=%d\n", activeACW);
      openGatesIfAllClear();
    }
  }
}

// =====================================================
// OBSTACLE DETECTION (non-blocking)
// =====================================================

void obstacleDetection() {
  unsigned long now = millis();

  if (risingEdge(curIR5, prevIR5) && !ir5Active) {
    ir5Active = true;
    ir5StartTime = now;
    openLeftGate();
  }

  if (ir5Active && (now - ir5StartTime >= OBSTACLE_DURATION_MS)) {
    closeLeftGate();
    ir5Active = false;
  }

  if (risingEdge(curIR6, prevIR6) && !ir6Active) {
    ir6Active = true;
    ir6StartTime = now;
    openRightGate();
  }

  if (ir6Active && (now - ir6StartTime >= OBSTACLE_DURATION_MS)) {
    closeRightGate();
    ir6Active = false;
  }
}

// =====================================================
// SAVE PREVIOUS SENSOR STATES
// =====================================================

void savePreviousSensorStates() {
  prevVsetc  = curVsetc;
  prevVsetac = curVsetac;

  prevIRsb = curIRsb;
  prevIRrg = curIRrg;
  prevIRbs = curIRbs;
  prevIRgr = curIRgr;

  prevIR5 = curIR5;
  prevIR6 = curIR6;
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  readSensors();
  updateClockwiseStateMachine();
  updateAntiClockwiseStateMachine();
  obstacleDetection();
  updateBuzzerPattern();
  savePreviousSensorStates();
}
