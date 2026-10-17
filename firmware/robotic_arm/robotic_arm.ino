/*
 * Bluetooth-Controlled Robotic Arm (ESP32)
 * Engineering Design and Tools, Lewis University, Spring 2026
 * Author: Juan Jose Castro (team project)
 *
 * Controls a 3-servo pantograph-style arm over Classic Bluetooth serial
 * (device name "Gripper") or USB serial.
 *
 * Commands:
 *   W = Up        S = Down
 *   A = Forward   D = Backward
 *   O = Open gripper   C = Close gripper
 *   H = Return to home position
 *
 * Libraries: ESP32Servo, BluetoothSerial (ESP32 Arduino core)
 */

#include <ESP32Servo.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

// ---------- Pin assignments ----------
#define LEFT_SERVO_PIN    13
#define RIGHT_SERVO_PIN   12
#define GRIPPER_SERVO_PIN 14

// ---------- Home positions (degrees) ----------
#define ORIGIN_LEFT     90
#define ORIGIN_RIGHT    90
#define ORIGIN_GRIPPER  30

// ---------- Angle limits (found through testing) ----------
#define LEFT_MIN    20
#define LEFT_MAX    160
#define RIGHT_MIN   20
#define RIGHT_MAX   160
#define GRIPPER_MIN 20
#define GRIPPER_MAX 70

// ---------- Motion settings ----------
#define ARM_STEP_SIZE      100
#define GRIPPER_STEP_SIZE  100
#define SERVO_DELAY_MS     15   // delay between 1-degree steps

Servo leftServo;
Servo rightServo;
Servo gripperServo;

int leftPos    = ORIGIN_LEFT;
int rightPos   = ORIGIN_RIGHT;
int gripperPos = ORIGIN_GRIPPER;

// Keep a target angle inside its allowed range
int clampValue(int value, int minVal, int maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

// Send a message to both USB serial and Bluetooth
void printBoth(String message) {
  Serial.println(message);
  SerialBT.println(message);
}

void printPositions() {
  String message = "Positions -> L: " + String(leftPos) +
                   " R: " + String(rightPos) +
                   " G: " + String(gripperPos);

  Serial.println(message);
  SerialBT.println(message);
}

// Move both arm servos together, 1 degree at a time, so the
// pantograph linkage moves smoothly
void moveArmDual(int targetLeft, int targetRight) {
  while (leftPos != targetLeft || rightPos != targetRight) {
    unsigned long start = millis();

    if (leftPos < targetLeft) {
      leftPos++;
    } else if (leftPos > targetLeft) {
      leftPos--;
    }

    if (rightPos < targetRight) {
      rightPos++;
    } else if (rightPos > targetRight) {
      rightPos--;
    }

    leftServo.write(leftPos);
    rightServo.write(rightPos);

    yield();

    while (millis() - start < SERVO_DELAY_MS) {
      yield();
    }
  }
}

// Move a single servo smoothly, 1 degree at a time
void moveSingleSmooth(Servo &servo, int &currentPos, int targetPos) {
  while (currentPos != targetPos) {
    unsigned long start = millis();

    if (targetPos > currentPos) {
      currentPos++;
    } else {
      currentPos--;
    }

    servo.write(currentPos);

    yield();

    while (millis() - start < SERVO_DELAY_MS) {
      yield();
    }
  }
}

void backToOrigin() {
  printBoth("Resetting to origin...");

  moveArmDual(ORIGIN_LEFT, ORIGIN_RIGHT);
  moveSingleSmooth(gripperServo, gripperPos, ORIGIN_GRIPPER);

  printPositions();
}

void handleCommand(char instruction) {
  instruction = toupper(instruction);

  if (instruction == '\n' || instruction == '\r' || instruction == ' ') {
    return;
  }

  Serial.print("Command: ");
  Serial.println(instruction);

  SerialBT.print("Command: ");
  SerialBT.println(instruction);

  switch (instruction) {
    case 'W':
      printBoth("Moving arm up...");
      moveArmDual(
        clampValue(leftPos + ARM_STEP_SIZE, LEFT_MIN, LEFT_MAX),
        clampValue(rightPos - ARM_STEP_SIZE, RIGHT_MIN, RIGHT_MAX)
      );
      break;

    case 'S':
      printBoth("Moving arm down...");
      moveArmDual(
        clampValue(leftPos - ARM_STEP_SIZE, LEFT_MIN, LEFT_MAX),
        clampValue(rightPos + ARM_STEP_SIZE, RIGHT_MIN, RIGHT_MAX)
      );
      break;

    case 'A':
      printBoth("Moving arm forward...");
      moveArmDual(
        clampValue(leftPos - ARM_STEP_SIZE, LEFT_MIN, LEFT_MAX),
        clampValue(rightPos - ARM_STEP_SIZE, RIGHT_MIN, RIGHT_MAX)
      );
      break;

    case 'D':
      printBoth("Moving arm backward...");
      moveArmDual(
        clampValue(leftPos + ARM_STEP_SIZE, LEFT_MIN, LEFT_MAX),
        clampValue(rightPos + ARM_STEP_SIZE, RIGHT_MIN, RIGHT_MAX)
      );
      break;

    case 'O':
      printBoth("Opening gripper...");
      moveSingleSmooth(
        gripperServo,
        gripperPos,
        clampValue(gripperPos - GRIPPER_STEP_SIZE, GRIPPER_MIN, GRIPPER_MAX)
      );
      break;

    case 'C':
      printBoth("Closing gripper...");
      moveSingleSmooth(
        gripperServo,
        gripperPos,
        clampValue(gripperPos + GRIPPER_STEP_SIZE, GRIPPER_MIN, GRIPPER_MAX)
      );
      break;

    case 'H':
      backToOrigin();
      break;

    default:
      printBoth("Unknown command.");
      break;
  }

  printPositions();
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Gripper");

  leftServo.setPeriodHertz(50);
  rightServo.setPeriodHertz(50);
  gripperServo.setPeriodHertz(50);

  leftServo.attach(LEFT_SERVO_PIN, 500, 2400);
  rightServo.attach(RIGHT_SERVO_PIN, 500, 2400);
  gripperServo.attach(GRIPPER_SERVO_PIN, 500, 2400);

  delay(500);

  leftServo.write(leftPos);
  rightServo.write(rightPos);
  gripperServo.write(gripperPos);

  delay(500);

  printBoth("Bluetooth ready. Device: Gripper");
  printBoth("Commands:");
  printBoth("W = Up");
  printBoth("S = Down");
  printBoth("A = Forward");
  printBoth("D = Backward");
  printBoth("O = Open gripper");
  printBoth("C = Close gripper");
  printBoth("H = Reset");

  printPositions();
}

void loop() {
  while (SerialBT.available()) {
    char command = SerialBT.read();
    handleCommand(command);
  }

  while (Serial.available()) {
    char command = Serial.read();
    handleCommand(command);
  }
}
