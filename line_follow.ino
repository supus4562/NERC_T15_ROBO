/*
 * ============================================================================
 *  NERC 2026 — Master Competition Line Following & Route Navigation
 *  Single-File Integrated Code (Arduino Mega 2560)
 * ============================================================================
 *
 *  DIAGNOSTIC SERIAL COMMANDS (115200 baud):
 *    's' = Start run
 *    'x' = Emergency stop (pulls enables LOW)
 *    'd' = Toggle debug print
 * ============================================================================
 */

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include <Arduino.h>

// LEFT side (FL + BL in parallel) — Timer3
#define LEFT_LPWM       2       // Timer3B — reverse
#define LEFT_RPWM       3       // Timer3A — forward

// RIGHT side (FR + BR in parallel) — Timer4
#define RIGHT_LPWM      6       // Timer4A — reverse
#define RIGHT_RPWM      7       // Timer4B — forward

// BTS7960 Driver Enable Pins
#define USE_ARDUINO_ENABLE_PINS  true

#if USE_ARDUINO_ENABLE_PINS
  #define LEFT_R_EN       22      // R_EN for left pair (D2_R_EN)
  #define LEFT_L_EN       23      // L_EN for left pair (D2_L_EN)
  #define RIGHT_L_EN      24      // L_EN for right pair (D1_L_EN)
  #define RIGHT_R_EN      25      // R_EN for right pair (D1_R_EN)
#endif

// ============================================================================
//  IR SENSORS — 5x Digital (active LOW = line detected)
// ============================================================================
#define IR_S1           49      // Far left  — junction detect (FL / IR1)
#define IR_S2           50      // Left      — PID (L / IR2)
#define IR_S3           51      // Center    — on-line (CTR / IR3)
#define IR_S4           52      // Right     — PID (R / IR4)
#define IR_S5           53      // Far right — junction detect (FR / IR5)

#define NUM_SENSORS     5

const uint8_t IR_PINS[NUM_SENSORS] = { IR_S1, IR_S2, IR_S3, IR_S4, IR_S5 };
const int SENSOR_WEIGHT[NUM_SENSORS] = { -2, -1, 0, 1, 2 };

// ============================================================================
//  RASPBERRY PI — Serial1 (hardware UART)
// ============================================================================
#define PI_SERIAL       Serial1
#define PI_BAUD         115200

// ============================================================================
//  SPEED & MOTION SETTINGS
// ============================================================================
#define BASE_SPEED            220     // Cruising speed (MAX SPEED)
#define MAX_SPEED             255     // Absolute max limit
#define MIN_SPEED             0       // No reverse during normal line follow

// High-Precision Two-Stage Turn Settings (calibrated by user to 700ms)
#define FAST_TURN_SPEED       200     // High power for quick pivot stage
#define SLOW_TURN_SPEED       80      // Low power for precise line-finding stage
#define TURN_FAST_DURATION_MS 600     // Pivot time for Stage 1 (calibrated perfect 90)

// Active Braking Settings
#define BRAKE_SPEED           220     // Reverse motor drive power
#define BRAKE_DURATION_MS     85      // Pulse duration in milliseconds

// ============================================================================
//  PID CONSTANTS (Tuned for MAX SPEED 255)
// ============================================================================
#define KP                    100.0    // Proportional response
#define KI                    0.0     // Integral response
#define KD                    40.0    // Derivative response (prevents high-speed wobble)

// ============================================================================
//  JUNCTION SETTINGS
// ============================================================================
#define JUNCTION_THRESHOLD    3       // Min sensors active to trigger junction
#define JUNCTION_DEBOUNCE     450     // ms — debounce window (ignores double counting)

// ============================================================================
//  INIT & MOTOR HELPERS
// ============================================================================
inline void initPins() {
    // PWM outputs
    pinMode(LEFT_RPWM, OUTPUT);  pinMode(LEFT_LPWM, OUTPUT);
    pinMode(RIGHT_RPWM, OUTPUT); pinMode(RIGHT_LPWM, OUTPUT);

#if USE_ARDUINO_ENABLE_PINS
    // Enable outputs
    pinMode(LEFT_R_EN, OUTPUT);  pinMode(LEFT_L_EN, OUTPUT);
    pinMode(RIGHT_R_EN, OUTPUT); pinMode(RIGHT_L_EN, OUTPUT);

    // Enable drivers
    digitalWrite(LEFT_R_EN, HIGH);  digitalWrite(LEFT_L_EN, HIGH);
    digitalWrite(RIGHT_R_EN, HIGH); digitalWrite(RIGHT_L_EN, HIGH);
#endif

    // IR inputs
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        pinMode(IR_PINS[i], INPUT);
    }

    // Motors off
    analogWrite(LEFT_RPWM, 0);  analogWrite(LEFT_LPWM, 0);
    analogWrite(RIGHT_RPWM, 0); analogWrite(RIGHT_LPWM, 0);
}

inline void setLeft(int spd) {
    spd = constrain(spd, -MAX_SPEED, MAX_SPEED);
    spd = -spd; // Physically reversed polarity correction
    if (spd > 0)      { analogWrite(LEFT_RPWM, spd); analogWrite(LEFT_LPWM, 0);    }
    else if (spd < 0) { analogWrite(LEFT_RPWM, 0);   analogWrite(LEFT_LPWM, -spd);  }
    else              { analogWrite(LEFT_RPWM, 0);   analogWrite(LEFT_LPWM, 0);     }
}

inline void setRight(int spd) {
    spd = constrain(spd, -MAX_SPEED, MAX_SPEED);
    if (spd > 0)      { analogWrite(RIGHT_RPWM, spd); analogWrite(RIGHT_LPWM, 0);   }
    else if (spd < 0) { analogWrite(RIGHT_RPWM, 0);   analogWrite(RIGHT_LPWM, -spd); }
    else              { analogWrite(RIGHT_RPWM, 0);   analogWrite(RIGHT_LPWM, 0);    }
}

inline void drive(int leftSpd, int rightSpd) {
    setLeft(leftSpd);
    setRight(rightSpd);
}

inline void stopAll() { setLeft(0); setRight(0); }

inline void emergencyStop() {
#if USE_ARDUINO_ENABLE_PINS
    digitalWrite(LEFT_R_EN, LOW);  digitalWrite(LEFT_L_EN, LOW);
    digitalWrite(RIGHT_R_EN, LOW); digitalWrite(RIGHT_L_EN, LOW);
#endif
    stopAll();
}

inline void enableMotors() {
#if USE_ARDUINO_ENABLE_PINS
    digitalWrite(LEFT_R_EN, HIGH);  digitalWrite(LEFT_L_EN, HIGH);
    digitalWrite(RIGHT_R_EN, HIGH); digitalWrite(RIGHT_L_EN, HIGH);
#endif
}

#endif // PIN_CONFIG_H

// ============================================================================
//  ROUTE DESIGN & ACTION PLAN
//  13 junctions total:
//    - Go 4 junctions, turn RIGHT.
//    - Go 3 junctions from that turn, turn RIGHT.
//    - Go straight for 4 junctions, turn LEFT.
//    - Go 2 more junctions and STOP.
// ============================================================================
enum Action {
    GO_STRAIGHT,
    TURN_LEFT_90,
    TURN_RIGHT_90,
    STOP
};

const char* ACTION_NAMES[] = {
    "GO_STRAIGHT",
    "TURN_LEFT_90",
    "TURN_RIGHT_90",
    "STOP"
};

#define ROUTE_LENGTH 13
const Action routeActions[ROUTE_LENGTH] = {
    GO_STRAIGHT,            // Junction 1
    GO_STRAIGHT,            // Junction 2
    GO_STRAIGHT,            // Junction 3
    TURN_RIGHT_90,          // Junction 4: 1st Turn (Right)
    
    GO_STRAIGHT,            // Junction 5 (1 after turn)
    GO_STRAIGHT,            // Junction 6 (2 after turn)
    TURN_RIGHT_90,          // Junction 7: 2nd Turn (Right)
    
    GO_STRAIGHT,            // Junction 8 (1 after turn)
    GO_STRAIGHT,            // Junction 9 (2 after turn)
    GO_STRAIGHT,            // Junction 10 (3 after turn)
    TURN_LEFT_90,           // Junction 11: 3rd Turn (Left)
    
    GO_STRAIGHT,            // Junction 12 (1 after turn)
    STOP                    // Junction 13: 2nd after turn (Brake & Stop)
};

// ============================================================================
//  GLOBAL STATE
// ============================================================================
float lastError = 0;
float integral = 0;
unsigned long lastPIDTime = 0;

int currentJunctionIndex = 0;
unsigned long lastJunctionTime = 0;
bool onJunction = false;

// Line lost tracking
unsigned long lineLostTime = 0;
float lastKnownPosition = 0;
bool lineFound = true;

enum RobotState {
    STATE_IDLE,
    STATE_LINE_FOLLOW,
    STATE_JUNCTION_ACTION,
    STATE_STOPPED
};

RobotState currentState = STATE_IDLE;
bool debugMode = false;

// ============================================================================
//  SENSOR READING
// ============================================================================
bool sensorActive[NUM_SENSORS];

int readSensors() {
    int activeCount = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensorActive[i] = (digitalRead(IR_PINS[i]) == HIGH);
        if (sensorActive[i]) activeCount++;
    }
    return activeCount;
}

// ============================================================================
//  POSITION CALCULATION
// ============================================================================
float computePosition() {
    float weightedSum = 0;
    int activeCount = 0;

    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensorActive[i]) {
            weightedSum += SENSOR_WEIGHT[i];
            activeCount++;
        }
    }

    if (activeCount == 0) {
        lineFound = false;
        if (lineLostTime == 0) lineLostTime = millis();
        return lastKnownPosition;
    }

    lineFound = true;
    lineLostTime = 0;

    float position = weightedSum / (float)activeCount;
    lastKnownPosition = position;
    return position;
}

// ============================================================================
//  PID COMPUTE
// ============================================================================
float pidCompute(float error) {
    unsigned long now = millis();
    float dt = (now - lastPIDTime) / 1000.0;
    if (dt <= 0) dt = 0.005;
    lastPIDTime = now;

    float P = KP * error;

    integral += error * dt;
    integral = constrain(integral, -50, 50);
    float I = KI * integral;

    float D = KD * (error - lastError) / dt;
    lastError = error;

    return P + I + D;
}

// ============================================================================
//  HIGH-PRECISION ACTIVE BRAKE
// ============================================================================
void activeBrake() {
    Serial.println(F("[BRAKE] Firing active reverse brake pulse..."));
    
    setLeft(-BRAKE_SPEED);
    setRight(-BRAKE_SPEED);
    delay(BRAKE_DURATION_MS);
    
    stopAll();
    #if USE_ARDUINO_ENABLE_PINS
        digitalWrite(LEFT_R_EN, LOW);  digitalWrite(LEFT_L_EN, LOW);
        digitalWrite(RIGHT_R_EN, LOW); digitalWrite(RIGHT_L_EN, LOW);
    #endif
    
    Serial.println(F("[BRAKE] Brakes locked."));
}

// ============================================================================
//  TWO-STAGE HIGH-PRECISION 90° TURNS
// ============================================================================
void turnLeft90() {
    Serial.println(F("[TURN] Left 90° (Two-Stage)"));
    drive(BASE_SPEED,BASE_SPEED);
    delay(100);
    // Stage 1: Fast Pivot
    drive(-FAST_TURN_SPEED, FAST_TURN_SPEED);
    delay(TURN_FAST_DURATION_MS);

    // Stage 2: Slow Precision search
    drive(-SLOW_TURN_SPEED, SLOW_TURN_SPEED);
    unsigned long timeout = millis() + 1200;
    while (millis() < timeout) {
        if (digitalRead(IR_PINS[2]) == HIGH) { // Center sensor sees line
            break;
        }
    }

    // Brief reverse tap to arrest turn rotation
    drive(FAST_TURN_SPEED, -FAST_TURN_SPEED);
    delay(35);
    stopAll();
    delay(60);

    lastError = 0;
    integral = 0;
    lastPIDTime = millis();
}

void turnRight90() {
    Serial.println(F("[TURN] Right 90° (Two-Stage)"));
    drive(BASE_SPEED,BASE_SPEED);
    delay(100);
    // Stage 1: Fast Pivot
    drive(FAST_TURN_SPEED, -FAST_TURN_SPEED);
    delay(TURN_FAST_DURATION_MS);

    // Stage 2: Slow Precision search
    drive(SLOW_TURN_SPEED, -SLOW_TURN_SPEED);
    unsigned long timeout = millis() + 1200;
    while (millis() < timeout) {
        if (digitalRead(IR_PINS[2]) == HIGH) { // Center sensor sees line
            break;
        }
    }

    // Brief reverse tap to arrest turn rotation
    drive(-FAST_TURN_SPEED, FAST_TURN_SPEED);
    delay(35);
    stopAll();
    delay(60);

    lastError = 0;
    integral = 0;
    lastPIDTime = millis();
}

// ============================================================================
//  JUNCTION ACTIONS EXECUTION
// ============================================================================
void handleJunctionAction() {
    currentJunctionIndex++;
    Serial.print(F("[ROUTE] Junction "));
    Serial.print(currentJunctionIndex);
    Serial.print(F(" / "));
    Serial.println(ROUTE_LENGTH);

    Action action = STOP;
    if (currentJunctionIndex <= ROUTE_LENGTH) {
        action = routeActions[currentJunctionIndex - 1];
    } else {
        Serial.println(F("[WARNING] Extra junction detected! Defaulting to STOP."));
    }

    Serial.print(F("[ACTION] Executing: "));
    Serial.println(ACTION_NAMES[action]);

    switch (action) {
        case GO_STRAIGHT:
            // Drive straight past the cross-line
            drive(BASE_SPEED, BASE_SPEED);
            delay(150); // Clear the 3cm thick tape completely
            lastError = 0;
            integral = 0;
            lastPIDTime = millis();
            break;

        case TURN_LEFT_90:
            // Push forward slightly to center pivot on junction
            drive(BASE_SPEED, BASE_SPEED);
            delay(120); 
            stopAll();
            delay(40);
            turnLeft90();
            break;

        case TURN_RIGHT_90:
            // Push forward slightly to center pivot on junction
            drive(BASE_SPEED, BASE_SPEED);
            delay(120);
            stopAll();
            delay(40);
            turnRight90();
            break;

        case STOP:
            drive(BASE_SPEED, BASE_SPEED);
            activeBrake();
            currentState = STATE_STOPPED;
            break;
    }
}

// ============================================================================
//  LINE FOLLOW LOOP
// ============================================================================
void lineFollow() {
    int activeCount = readSensors();

    // Junction Detection (3 or more sensors active)
    if (activeCount >= JUNCTION_THRESHOLD) {
        if (!onJunction && (millis() - lastJunctionTime > JUNCTION_DEBOUNCE)) {
            onJunction = true;
            lastJunctionTime = millis();
            currentState = STATE_JUNCTION_ACTION;
            return;
        }
    } else {
        // RESET PROTECTION LOCKOUT:
        // Do not allow onJunction to reset to false until:
        // 1. We see clear floor (activeCount <= 1)
        // 2. We have physically moved away from the junction (at least 300ms has elapsed since detection)
        // This eliminates double-counting on the 3cm thick tape.
        if (activeCount <= 1 && (millis() - lastJunctionTime > 300)) {
            onJunction = false;
        }
    }

    // Compute PID Correction
    float position = computePosition();
    float correction = pidCompute(position);

    // Recovery if line is lost
    if (!lineFound) {
        unsigned long lostDuration = millis() - lineLostTime;
        if (lostDuration > 1200) {
            Serial.println(F("[ERROR] Line lost for 1.2s — stopping"));
            activeBrake();
            currentState = STATE_STOPPED;
            return;
        }
        correction *= 1.4;
    }

    // Apply motor speeds
    int leftSpeed = BASE_SPEED + (int)correction;
    int rightSpeed = BASE_SPEED - (int)correction;

    // Clamp speed limits
    leftSpeed = constrain(leftSpeed, MIN_SPEED, MAX_SPEED);
    rightSpeed = constrain(rightSpeed, MIN_SPEED, MAX_SPEED);

    drive(leftSpeed, rightSpeed);
}

// ============================================================================
//  SERIAL USER COMMANDS
// ============================================================================
void handleSerialCommand() {
    if (!Serial.available()) return;

    char cmd = Serial.read();

    switch (cmd) {
        case 's':
        case 'S':
            Serial.println(F("\n>>> STARTING COMPETITION RUN <<<"));
            currentJunctionIndex = 0;
            lastError = 0;
            integral = 0;
            onJunction = false;
            lastJunctionTime = 0;
            lastPIDTime = millis();
            
            enableMotors();
            currentState = STATE_LINE_FOLLOW;
            break;

        case 'x':
        case 'X':
            emergencyStop();
            currentState = STATE_IDLE;
            Serial.println(F("\n>>> EMERGENCY STOPPED <<<"));
            break;

        case 'd':
        case 'D':
            debugMode = !debugMode;
            Serial.print(F("Debug mode: "));
            Serial.println(debugMode ? F("ON") : F("OFF"));
            break;
    }
}

// ============================================================================
//  DEBUG FEEDBACK
// ============================================================================
unsigned long lastDebugPrint = 0;

void printDebug() {
    if (!debugMode) return;
    if (millis() - lastDebugPrint < 150) return;
    lastDebugPrint = millis();

    Serial.print(F("  "));
    for (int i = 0; i < NUM_SENSORS; i++) {
        Serial.print(sensorActive[i] ? F("██") : F(" ."));
    }

    float pos = computePosition();
    Serial.print(F(" | Pos:"));
    Serial.print(pos, 1);
    Serial.print(F(" | JuncIndex:"));
    Serial.print(currentJunctionIndex);
    if (!lineFound) Serial.print(F(" [LOST]"));
    Serial.println();
}

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println(F("\n"));
    Serial.println(F("=============================================="));
    Serial.println(F("   NERC 2026 — Master Line Following System"));
    Serial.println(F("   Speed: MAX (255) | Two-Stage Turn & Active Brakes"));
    Serial.println(F("=============================================="));

    initPins();
    Serial.println(F("  Hardware initialized."));
    Serial.println(F("  Left polarity correction: ACTIVE."));
    Serial.println(F("  Place robot on line, then send 's' to start."));
    Serial.println(F("==============================================\n"));

    currentState = STATE_IDLE;
}

// ============================================================================
//  LOOP
// ============================================================================
void loop() {
    handleSerialCommand();

    switch (currentState) {
        case STATE_IDLE:
            break;

        case STATE_LINE_FOLLOW:
            lineFollow();
            printDebug();
            break;

        case STATE_JUNCTION_ACTION:
            handleJunctionAction();
            if (currentState != STATE_STOPPED) {
                currentState = STATE_LINE_FOLLOW;
            }
            break;

        case STATE_STOPPED:
            break;
    }
}
