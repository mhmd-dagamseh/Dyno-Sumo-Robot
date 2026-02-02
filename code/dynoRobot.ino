// Dyno Sumo Robot

// ====================== SENSOR PINS ======================
byte TRIG_RIGHT = 8;
byte ECHO_RIGHT = 6;
byte TRIG_LEFT = 7;
byte ECHO_LEFT = 5;
byte TRIG_CENTER = 9;
byte ECHO_CENTER = 10;

// ====================== MOTOR PINS (L298N) ======================
byte M1_ENABLE = 3;
byte M1_FORWARD = 2;
byte M1_BACKWARD = 4;

byte M2_ENABLE = 11;
byte M2_FORWARD = 13;
byte M2_BACKWARD = 12;

// ====================== SPEEDS ======================
byte SPEED_NORMAL = 170;
byte SPEED_ATTACK = 240;
byte SPEED_MAX = 255;
byte SPEED_TURN = 190;
byte SPEED_SEARCH = 140;
byte SPEED_EVADE = 220;

// ====================== DISTANCES ======================
byte DIST_DANGER = 10;
byte DIST_CLOSE = 18;
byte DIST_ATTACK = 35;
byte DIST_ALIGN = 55;
byte DIST_FAR = 100;
byte DIST_LOST = 150;

// ====================== TIMINGS ======================
int TIME_ATTACK = 400;
int TIME_EVADE = 250;
int TIME_SEARCH_CHANGE = 1200;
int TIME_COUNTER = 300;

// ====================== FUNCTION PROTOTYPES ======================
void motorLeft(int speed, bool forward);
void motorRight(int speed, bool forward);
void forwardMove(int speed);
void backwardMove(int speed);
void turnRight(int speed);
void turnLeft(int speed);
void pivotRight();
void pivotLeft();
void stopMotors();
float readSensor(int trigPin, int echoPin);
void readAllSensors(float &left, float &center, float &right);
void analyzeOpponent(float left, float center, float right);
void behaviorSearch();
void behaviorAlign();
void behaviorAttack();
void behaviorTactical();
void behaviorEvade();
void behaviorCounter();
void makeDecision(float left, float center, float right);
void testMotors();

// ====================== SYSTEM VARIABLES ======================
unsigned long loopTimer = 0;
unsigned long searchTimer = 0;
unsigned long attackTimer = 0;
unsigned long evadeTimer = 0;
unsigned long lostTimer = 0;

int opponentSide = 0;
int lastOpponentSide = 0;
float opponentDistance = 0;
float opponentSpeed = 0;

int searchPattern = 0;
int attackPattern = 0;
int evadePattern = 0;

bool opponentFound = false;
bool opponentClose = false;
bool opponentVeryClose = false;

// ====================== SETUP ======================
void setup() {
  // MOTOR PINS
  pinMode(M1_ENABLE, OUTPUT);
  pinMode(M1_FORWARD, OUTPUT);
  pinMode(M1_BACKWARD, OUTPUT);
  pinMode(M2_ENABLE, OUTPUT);
  pinMode(M2_FORWARD, OUTPUT);
  pinMode(M2_BACKWARD, OUTPUT);
  
  // SENSOR PINS
  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);
  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);
  pinMode(TRIG_CENTER, OUTPUT);
  pinMode(ECHO_CENTER, INPUT);
  
  // SERIAL
  Serial.begin(9600);
  Serial.println("SUMO ROBOT - FULL AUTOMATION");
  
  // MOTOR TEST
  testMotors();
}

// ====================== MOTOR TEST ======================
void testMotors() {
  forwardMove(150);
  delay(300);
  backwardMove(150);
  delay(300);
  turnRight(180);
  delay(300);
  turnLeft(180);
  delay(300);
  stopMotors();
  delay(500);
}

// ====================== MOTOR CONTROL ======================
void motorLeft(int speed, bool isForward) {
  speed = constrain(speed, 0, 255);
  digitalWrite(M1_FORWARD, isForward ? HIGH : LOW);
  digitalWrite(M1_BACKWARD, isForward ? LOW : HIGH);
  analogWrite(M1_ENABLE, speed);
}

void motorRight(int speed, bool isForward) {
  speed = constrain(speed, 0, 255);
  digitalWrite(M2_FORWARD, isForward ? HIGH : LOW);
  digitalWrite(M2_BACKWARD, isForward ? LOW : HIGH);
  analogWrite(M2_ENABLE, speed);
}

void forwardMove(int speed = SPEED_NORMAL) {
  motorLeft(speed, true);
  motorRight(speed, true);
}

void backwardMove(int speed = SPEED_NORMAL) {
  motorLeft(speed, false);
  motorRight(speed, false);
}

void turnRight(int speed = SPEED_TURN) {
  motorLeft(speed, true);
  motorRight(speed, false);
}

void turnLeft(int speed = SPEED_TURN) {
  motorLeft(speed, false);
  motorRight(speed, true);
}

void pivotRight() {
  motorLeft(SPEED_TURN + 30, true);
  motorRight(SPEED_TURN + 30, true);
  delay(60);
  motorLeft(SPEED_TURN + 30, true);
  motorRight(SPEED_TURN + 30, false);
}

void pivotLeft() {
  motorLeft(SPEED_TURN + 30, false);
  motorRight(SPEED_TURN + 30, false);
  delay(60);
  motorLeft(SPEED_TURN + 30, false);
  motorRight(SPEED_TURN + 30, true);
}

void stopMotors() {
  analogWrite(M1_ENABLE, 0);
  analogWrite(M2_ENABLE, 0);
}

// ====================== SENSOR FUNCTIONS ======================
float readSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 12000);
  
  if (duration == 0) {
    return 200.0;
  }
  
  float distance = duration * 0.0343 / 2.0;
  
  if (distance < 2.0 || distance > 200.0) {
    return 200.0;
  }
  
  return distance;
}

void readAllSensors(float &left, float &center, float &right) {
  right = readSensor(TRIG_RIGHT, ECHO_RIGHT);
  delayMicroseconds(100);
  left = readSensor(TRIG_LEFT, ECHO_LEFT);
  delayMicroseconds(100);
  center = readSensor(TRIG_CENTER, ECHO_CENTER);
}

// ====================== OPPONENT ANALYSIS ======================
void analyzeOpponent(float left, float center, float right) {
  static float prevDistance = 0;
  static unsigned long prevTime = 0;
  
  float minDist = 200.0;
  int side = 0;
  
  if (center < minDist && center < DIST_LOST) {
    minDist = center;
    side = 0;
  }
  if (left < minDist && left < DIST_LOST) {
    minDist = left;
    side = -1;
  }
  if (right < minDist && right < DIST_LOST) {
    minDist = right;
    side = 1;
  }
  
  opponentDistance = minDist;
  opponentSide = side;
  
  opponentFound = (minDist < DIST_LOST);
  opponentClose = (minDist < DIST_ATTACK);
  opponentVeryClose = (minDist < DIST_CLOSE);
  
  if (side == 0 && prevTime > 0 && opponentDistance < DIST_FAR) {
    unsigned long currentTime = millis();
    float timeDiff = (currentTime - prevTime) / 1000.0;
    
    if (timeDiff > 0.05) {
      opponentSpeed = (opponentDistance - prevDistance) / timeDiff;
    }
    
    prevDistance = opponentDistance;
    prevTime = currentTime;
  }
  
  if (opponentFound) {
    lastOpponentSide = opponentSide;
    lostTimer = millis();
  }
}

// ====================== BEHAVIORS ======================

// 1. SEARCH
void behaviorSearch() {
  unsigned long currentTime = millis();
  
  if (currentTime - searchTimer > TIME_SEARCH_CHANGE) {
    searchPattern = (searchPattern + 1) % 5;
    searchTimer = currentTime;
  }
  
  switch(searchPattern) {
    case 0:
      turnRight(SPEED_SEARCH);
      break;
      
    case 1:
      turnLeft(SPEED_SEARCH);
      break;
      
    case 2:
      forwardMove(SPEED_SEARCH - 20);
      delay(350);
      break;
      
    case 3:
      stopMotors();
      delay(150);
      break;
      
    case 4:
      turnRight(SPEED_SEARCH + 20);
      delay(600);
      break;
  }
}

// 2. ALIGN
void behaviorAlign() {
  float diff = 0;
  
  float left = readSensor(TRIG_LEFT, ECHO_LEFT);
  float right = readSensor(TRIG_RIGHT, ECHO_RIGHT);
  
  diff = left - right;
  
  if (opponentSide == 1 || diff > 12) {
    turnRight(SPEED_TURN + 30);
    delay(100);
  } 
  else if (opponentSide == -1 || diff < -12) {
    turnLeft(SPEED_TURN + 30);
    delay(100);
  }
  else if (diff > 5) {
    turnRight(SPEED_TURN);
    delay(60);
  }
  else if (diff < -5) {
    turnLeft(SPEED_TURN);
    delay(60);
  }
  else {
    forwardMove(SPEED_ATTACK - 20);
    delay(120);
  }
}

// 3. ATTACK
void behaviorAttack() {
  attackPattern = (attackPattern + 1) % 4;
  
  switch(attackPattern) {
    case 0:
      forwardMove(SPEED_ATTACK);
      delay(120);
      break;
      
    case 1:
      forwardMove(SPEED_ATTACK);
      delay(80);
      turnRight(SPEED_TURN + 40);
      delay(50);
      break;
      
    case 2:
      forwardMove(SPEED_ATTACK);
      delay(100);
      backwardMove(SPEED_NORMAL);
      delay(40);
      break;
      
    case 3:
      forwardMove(SPEED_ATTACK);
      delay(90);
      if (lastOpponentSide == 1) {
        turnLeft(SPEED_TURN + 30);
      } else {
        turnRight(SPEED_TURN + 30);
      }
      delay(60);
      break;
  }
  
  attackTimer = millis();
}

// 4. TACTICAL ATTACK
void behaviorTactical() {
  backwardMove(SPEED_EVADE);
  delay(180);
  
  if (lastOpponentSide == 1) {
    pivotLeft();
    delay(120);
  } else if (lastOpponentSide == -1) {
    pivotRight();
    delay(120);
  } else {
    if (millis() % 2 == 0) {
      pivotLeft();
    } else {
      pivotRight();
    }
    delay(100);
  }
  
  forwardMove(SPEED_MAX);
  delay(200);
  
  evadeTimer = millis();
}

// 5. EVADE
void behaviorEvade() {
  evadePattern = (evadePattern + 1) % 3;
  
  switch(evadePattern) {
    case 0:
      if (opponentSide == -1) {
        turnRight(SPEED_EVADE + 20);
      } else {
        turnLeft(SPEED_EVADE + 20);
      }
      delay(120);
      backwardMove(SPEED_EVADE);
      delay(100);
      break;
      
    case 1:
      if (opponentSide == 1) {
        turnLeft(SPEED_EVADE + 20);
      } else {
        turnRight(SPEED_EVADE + 20);
      }
      delay(120);
      backwardMove(SPEED_EVADE);
      delay(100);
      break;
      
    case 2:
      backwardMove(SPEED_MAX);
      delay(160);
      if (millis() % 2 == 0) {
        pivotLeft();
      } else {
        pivotRight();
      }
      delay(140);
      break;
  }
  
  evadeTimer = millis();
}

// 6. COUNTER ATTACK
void behaviorCounter() {
  forwardMove(SPEED_MAX);
  delay(180);
  
  if (lastOpponentSide == 1) {
    turnLeft(SPEED_TURN + 50);
    delay(80);
  } else if (lastOpponentSide == -1) {
    turnRight(SPEED_TURN + 50);
    delay(80);
  }
  
  delay(100);
}

// ====================== DECISION MAKING ======================
void makeDecision(float left, float center, float right) {
  if (center < DIST_DANGER || left < DIST_DANGER || right < DIST_DANGER) {
    behaviorEvade();
    return;
  }
  
  if (center < DIST_CLOSE && opponentSpeed < -80) {
    behaviorTactical();
    return;
  }
  
  if (center > DIST_DANGER && center < DIST_ATTACK) {
    behaviorAttack();
    return;
  }
  
  if ((left < DIST_ALIGN || right < DIST_ALIGN) && center > DIST_ATTACK) {
    behaviorAlign();
    return;
  }
  
  if (millis() - evadeTimer < TIME_COUNTER && millis() - evadeTimer > 100) {
    behaviorCounter();
    return;
  }
  
  behaviorSearch();
}

// ====================== MAIN LOOP ======================
void loop() {
  unsigned long currentTime = millis();
  
  float leftDist, centerDist, rightDist;
  readAllSensors(leftDist, centerDist, rightDist);
  
  analyzeOpponent(leftDist, centerDist, rightDist);
  
  makeDecision(leftDist, centerDist, rightDist);
  
  if (currentTime - loopTimer > 800) {
    Serial.print("L:");
    Serial.print(leftDist);
    Serial.print(" C:");
    Serial.print(centerDist);
    Serial.print(" R:");
    Serial.print(rightDist);
    Serial.print(" S:");
    Serial.print(opponentSide);
    Serial.print(" V:");
    Serial.println(opponentSpeed);
    
    loopTimer = currentTime;
  }
  
  delay(15);
}
