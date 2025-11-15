#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ====================== WiFi START/STOP ======================
const char* ssid = "ESP32-Robot";
const char* password = "12345678";
unsigned long previousMillis = 0;
unsigned long pauseStartTime = 0; 

WebServer server(80);
bool wifiStart = false;
bool timerStarted = false;
bool isPaused = false;     // To pause the robot
bool needsToAvoid = false; // Flag to run avoidance maneuver

// New variables for scan cycles
int checkCycleCount = 0;
bool humanFoundInSet = false;

// ====================== SAFE DELAY (prevents WiFi drop) ======================
void safeDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    delay(1);
    yield(); // ✅ Keeps WiFi/server responsive
  }
}

// ====================== DARK THEME UI ======================
void handleRoot() {
  String page =
    "<html>"
    "<body style='background:#121212; color:white; text-align:center; font-family:Arial; padding-top:40px;'>"
    "<h1 style='color:#00bfff;'>ESP32 Rescue Robot</h1>"
    "<h3 style='opacity:0.8;'>Wi-Fi Control Panel</h3>"

    "<button onclick=\"location.href='/start'\" "
    "style='padding:20px 50px; font-size:30px; background:#1db954; color:white; border:none; border-radius:12px; margin:20px;'>"
    "START"
    "</button><br>"

    "<button onclick=\"location.href='/continue'\" "
    "style='padding:20px 50px; font-size:30px; background:#ff8c00; color:white; border:none; border-radius:12px; margin:20px;'>"
    "CONTINUE"
    "</button><br>"

    "<button onclick=\"location.href='/stop'\" "
    "style='padding:20px 50px; font-size:30px; background:#e63946; color:white; border:none; border-radius:12px; margin:20px;'>"
    "STOP"
    "</button>"

    "<h2 style='margin-top:40px;'>Status: ";

  if (isPaused)
    page += "<span style='color:#ff8c00;'>PAUSED</span>";
  else if (wifiStart)
    page += "<span style='color:#1db954;'>RUNNING</span>";
  else
    page += "<span style='color:#e63946;'>STOPPED</span>";

  page += "</h2></body></html>";

  server.send(200, "text/html", page);
}

void startRobot() {
  wifiStart = true;
  isPaused = false;
  needsToAvoid = false;
  timerStarted = false;
  checkCycleCount = 0;     // Reset counter
  humanFoundInSet = false; // Reset flag
  handleRoot();
}

void stopRobot() {
  wifiStart = false;
  isPaused = false;
  needsToAvoid = false;
  stopMotors();
  timerStarted = false;
  handleRoot();
}

// Updated function to handle timer resume and cycle reset
void continueRobot() {
  if (isPaused) {
    unsigned long pauseDuration = millis() - pauseStartTime;
    previousMillis += pauseDuration; // Adjust timing after pause
  }
  isPaused = false;
  checkCycleCount = 0;
  humanFoundInSet = false;
  handleRoot();
}

// ====================== SERVO ======================
Servo myservo;
int servo_pin = 18;
int count = 0;

// ====================== ULTRASONIC ======================
#define sound_speed 0.034
const int trig_pin = 4;
const int echo_pin = 2;
float distance;
float duration;
int left_distance;
int right_distance;

const int irPin = 5;
const int irPin1 = 22;

// ====================== MOTORS ======================
const int motor1Pin1 = 27;
const int motor1Pin2 = 26;
const int enable1Pin = 14;
const int motor2Pin1 = 33;
const int motor2Pin2 = 32;
const int enable2Pin = 25;
int check = 0;

const int freq = 60000;
const int pwmChannel0 = 1;
const int pwmChannel1 = 2;
const int resolution = 8;

int dutycycleLeft = 180;
int dutycycleRight = 180;
int dutycycleLeftTurn = 180;
int dutycycleRightTurn = 180;

// ====================== HUMAN DETECTION ======================
const int humanPin = 13;
const int ledPin = 19;

// ====================== TIMING ======================

bool moving = true;
const long moveDuration = 25000;
const long stopDuration = 40000;
// Track when pause starts

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  WiFi.setSleep(false);

  // Servo
  ESP32PWM::allocateTimer(0);
  myservo.setPeriodHertz(50);
  myservo.attach(servo_pin, 500, 2500);
  myservo.write(125);

  // Ultrasonic
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  pinMode(irPin, INPUT);
  pinMode(irPin1, INPUT);

  // Motors
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  pinMode(enable2Pin, OUTPUT);

  ledcAttachChannel(enable1Pin, freq, resolution, pwmChannel0);
  ledcAttachChannel(enable2Pin, freq, resolution, pwmChannel1);

  // Human detection
  pinMode(humanPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // WiFi
  WiFi.softAP(ssid, password);
  WiFi.softAPsetHostname("ESP32-Robot");
  Serial.println("WiFi AP Started!");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/start", startRobot);
  server.on("/stop", stopRobot);
  server.on("/continue", continueRobot);
  server.begin();
}

// ====================== LOOP ======================
void loop() {
  server.handleClient();

  if (!wifiStart) {
    stopMotors();
    timerStarted = false;
    isPaused = false;
    needsToAvoid = false;
    return;
  }

  if (isPaused) {
    stopMotors();
    return;
  }

  if (needsToAvoid) {
    performAvoidanceManeuver();
    needsToAvoid = false;
    moving = true;
    return;
  }

  unsigned long currentMillis = millis();
  if (!timerStarted) {
    previousMillis = currentMillis;
    timerStarted = true;
    moving = true;
  }

  if (moving) {
    if ((currentMillis - previousMillis) < moveDuration) {
      digitalWrite(ledPin, LOW);
      obstacleAvoidance();
    } else {
      stopMotors();
      previousMillis = currentMillis;
      moving = false;
    }
  } else {
    if ((currentMillis - previousMillis) < stopDuration) {
      humanDetection();
    } else {
      centerServo();
      previousMillis = currentMillis;
      moving = true;

      if (checkCycleCount >= 4) {
        if (!humanFoundInSet) {
          pauseStartTime = millis();
          isPaused = true;
        } else {
          checkCycleCount = 0;
          humanFoundInSet = false;
        }
      }
    }
  }
}

// ====================== OBSTACLE AVOIDANCE ======================
void obstacleAvoidance() {
  int sensorValue = digitalRead(irPin);
  int sensorValue1 = digitalRead(irPin1);
  getDistance();

  if ((distance < 55) || (sensorValue == 1 || sensorValue1 == 1)) {
    stopMotors();
    safeDelay(300);
    backward();
    safeDelay(600);
    stopMotors();
    safeDelay(300);

    safeDelay(10000);
    int humanDetected = digitalRead(humanPin);

    if (humanDetected == HIGH) {
      Serial.println("HUMAN DETECTED! Pausing.");
      for (int i = 0; i < 9; i++) {
        digitalWrite(ledPin, HIGH);
        safeDelay(400);
        digitalWrite(ledPin, LOW);
        safeDelay(400);
      }
      pauseStartTime = millis();
      isPaused = true;
      needsToAvoid = true;
      return;
    } else {
      Serial.println("Just an obstacle. Avoiding immediately.");
      needsToAvoid = true;
      return;
    }
  }

  moveForward();
}

// ====================== AVOIDANCE MANEUVER ======================
void performAvoidanceManeuver() {
  check_servo(180);
  left_distance = getDistance();
  check_servo(30);
  right_distance = getDistance();
  centerServo();

  if (right_distance < left_distance)
    turnLeft();
  else
    turnRight();

  safeDelay(500);
}

// ====================== MOTOR CONTROL ======================
void moveForward() {
  ledcWrite(enable1Pin, dutycycleLeft);
  ledcWrite(enable2Pin, dutycycleRight);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}

void backward() {
  ledcWrite(enable1Pin, dutycycleLeft);
  ledcWrite(enable2Pin, dutycycleRight);
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

void turnLeft() {
  ledcWrite(enable1Pin, dutycycleLeftTurn);
  ledcWrite(enable2Pin, dutycycleRightTurn);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

void turnRight() {
  ledcWrite(enable1Pin, dutycycleLeftTurn);
  ledcWrite(enable2Pin, dutycycleRightTurn);
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}

void stopMotors() {
  ledcWrite(enable1Pin, 0);
  ledcWrite(enable2Pin, 0);
}

// ====================== SENSOR & SERVO ======================
long getDistance() {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  duration = pulseIn(echo_pin, HIGH);
  distance = (duration * sound_speed) / 2;

  Serial.print("distance(cm): ");
  Serial.println(distance);

  safeDelay(200);
  return distance;
}

void check_servo(int pos) {
  if (pos == 30) {
    for (int i = 180; i >= 50; i--) {
      myservo.write(i);
      safeDelay(15);
    }
  } else if (pos == 180) {
    for (int i = 125; i <= 180; i++) {
      myservo.write(i);
      safeDelay(15);
    }
  }
}

void centerServo() {
  for (int i = 50; i <= 125; i++) {
    myservo.write(i);
    safeDelay(20);
  }
}

// ====================== HUMAN DETECTION (SCAN MODE) ======================
void humanDetection() {
  stopMotors();

  if (count == 0) {
    check_servo(180);
    count = 1;
    safeDelay(12000);

    int humanDetected = digitalRead(humanPin);
    if (humanDetected == HIGH) {
      Serial.println("Human detected!");
      humanFoundInSet = true;
      for (int i = 0; i < 10; i++) {
        digitalWrite(ledPin, HIGH);
        safeDelay(400);
        digitalWrite(ledPin, LOW);
        safeDelay(400);
      }
    }
  } else if (count == 1) {
    check_servo(30);
    count = 0;
    safeDelay(12000);

    int humanDetected = digitalRead(humanPin);
    if (humanDetected == HIGH) {
      Serial.println("Human detected!");
      humanFoundInSet = true;
      for (int i = 0; i < 10; i++) {
        digitalWrite(ledPin, HIGH);
        safeDelay(400);
        digitalWrite(ledPin, LOW);
        safeDelay(400);
      }
    }

    checkCycleCount++;
    Serial.print("Scan Cycle Complete: ");
    Serial.println(checkCycleCount);
  }
}
