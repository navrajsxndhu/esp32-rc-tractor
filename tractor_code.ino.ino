#include <PS4Controller.h>
#include <ESP32Servo.h>

// Light pin definitions
#define HEADLIGHT1_PIN 26
#define HEADLIGHT2_PIN 27
#define REARLIGHT1_PIN 33
#define REARLIGHT2_PIN 25

// Servo pins
#define MOVEMENT_SERVO_PIN 23
#define SERVO1_PIN 14
#define SERVO3_PIN 2
#define SERVO4_PIN 32

#define VOLTAGE_READ_PIN 34

Servo movementServo;
Servo servo1;
Servo servo3;
Servo servo4;

int movementServoAngle = 60;
int servo1Angle = 90;
int servo3Angle = 150;
int servo4Angle = 0;

bool headlightState = false;
bool blinkMode = false;
bool rearLightManual = false;
bool rearLightState = false;

unsigned long lastBlinkTime = 0;
const int blinkInterval = 200;

void setup() {
  Serial.begin(115200);

  pinMode(HEADLIGHT1_PIN, OUTPUT);
  pinMode(HEADLIGHT2_PIN, OUTPUT);
  pinMode(REARLIGHT1_PIN, OUTPUT);
  pinMode(REARLIGHT2_PIN, OUTPUT);
  pinMode(VOLTAGE_READ_PIN, INPUT);

  movementServo.attach(MOVEMENT_SERVO_PIN, 500, 2500);
  servo1.attach(SERVO1_PIN, 500, 2500);
  servo3.attach(SERVO3_PIN, 500, 2500);
  servo4.attach(SERVO4_PIN, 500, 2500);

  movementServo.write(movementServoAngle);
  servo1.write(servo1Angle);
  servo3.write(servo3Angle);
  servo4.write(servo4Angle);

  if (!PS4.begin("1c:69:20:94:23:ca")) {
    Serial.println("PS4 connection failed");
    while (1);
  }

  Serial.println("Waiting for PS4 controller...");
}

void loop() {
  if (PS4.isConnected()) {
    int batteryLevel = PS4.Battery();
    if (batteryLevel <= 1) PS4.setLed(255, 0, 0);
    else if (batteryLevel == 2) PS4.setLed(255, 128, 0);
    else if (batteryLevel == 3) PS4.setLed(255, 255, 0);
    else if (batteryLevel == 4) PS4.setLed(0, 255, 0);
    else PS4.setLed(0, 0, 255);

    static bool prevSquare = false;
    static bool prevCircle = false;
    static bool prevL1 = false;
    static bool prevR1 = false;

    bool squarePressed = PS4.Square();
    bool circlePressed = PS4.Circle();
    bool l1Pressed = PS4.L1();
    bool r1Pressed = PS4.R1();
    bool upPressed = PS4.Up();
    bool downPressed = PS4.Down();
    bool trianglePressed = PS4.Triangle();
    bool crossPressed = PS4.Cross();

    // Headlight toggle (OFF → ON → BLINK)
    if (squarePressed && !prevSquare) {
      if (!headlightState) {
        digitalWrite(HEADLIGHT1_PIN, HIGH);
        digitalWrite(HEADLIGHT2_PIN, HIGH);
        headlightState = true;
        blinkMode = false;
      } else if (!blinkMode) {
        blinkMode = true;
      } else {
        digitalWrite(HEADLIGHT1_PIN, LOW);
        digitalWrite(HEADLIGHT2_PIN, LOW);
        headlightState = false;
        blinkMode = false;
      }
    }
    prevSquare = squarePressed;

    // Manual rear light toggle
    if (circlePressed && !prevCircle) {
      rearLightManual = !rearLightManual;
      rearLightState = rearLightManual;
      digitalWrite(REARLIGHT1_PIN, rearLightState);
      digitalWrite(REARLIGHT2_PIN, rearLightState);
    }
    prevCircle = circlePressed;

    // Headlight blinking mode
    if (blinkMode) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastBlinkTime >= blinkInterval) {
        lastBlinkTime = currentMillis;
        bool state = digitalRead(HEADLIGHT1_PIN);
        digitalWrite(HEADLIGHT1_PIN, !state);
        digitalWrite(HEADLIGHT2_PIN, !state);
      }
    }

    // Movement with left stick Y
    int leftY = PS4.LStickY();
    int rightX = PS4.RStickX();

    movementServoAngle = map(leftY, -128, 127, 0, 120);
    movementServoAngle = constrain(movementServoAngle, 0, 120);
    movementServo.write(movementServoAngle);

    // Auto rear light ON in reverse
    if (movementServoAngle < 30) {
      rearLightState = true;
      digitalWrite(REARLIGHT1_PIN, HIGH);
      digitalWrite(REARLIGHT2_PIN, HIGH);
    } else if (!rearLightManual) {
      rearLightState = false;
      digitalWrite(REARLIGHT1_PIN, LOW);
      digitalWrite(REARLIGHT2_PIN, LOW);
    }

    // Steering with right stick X
    servo1Angle = map(rightX, -128, 127, 180, 0);
    servo1Angle = constrain(servo1Angle, 0, 180);
    servo1.write(servo1Angle);

    // Servo 3: NORMAL (UP = up, DOWN = down)
    if (upPressed && servo3Angle < 180) servo3Angle++;
    if (downPressed && servo3Angle > 0) servo3Angle--;
    servo3Angle = constrain(servo3Angle, 0, 180);
    servo3.write(servo3Angle);

    // Servo 4: NORMAL (TRIANGLE = up, CROSS = down)
    if (trianglePressed && servo4Angle < 120) servo4Angle++;
    if (crossPressed && servo4Angle > 0) servo4Angle--;
    servo4Angle = constrain(servo4Angle, 0, 120);
    servo4.write(servo4Angle);

    // Individual headlight control
    if (l1Pressed && !prevL1)
      digitalWrite(HEADLIGHT1_PIN, !digitalRead(HEADLIGHT1_PIN));
    if (r1Pressed && !prevR1)
      digitalWrite(HEADLIGHT2_PIN, !digitalRead(HEADLIGHT2_PIN));
    prevL1 = l1Pressed;
    prevR1 = r1Pressed;

    // Battery voltage reading
    int rawADC = analogRead(VOLTAGE_READ_PIN);
    float voltage = rawADC * (3.3 / 4095.0);
    float batteryVoltage = voltage * 2.0;

    // Debug info
    Serial.print("Voltage: "); Serial.print(batteryVoltage); Serial.print("V | ");
    Serial.print("Steering: "); Serial.print(servo1Angle);
    Serial.print(" | Servo3: "); Serial.print(servo3Angle);
    Serial.print(" | Servo4: "); Serial.println(servo4Angle);

    delay(10);
  } else {
    Serial.println("PS4 Controller Disconnected...");
    delay(100);
  }
}
