#include <WiFi.h>
#include <WebServer.h>

//WIFI 
const char* ssid = "WAREHOUSE_ROBOT";
const char* password = "12345678";
WebServer server(80);
// IR 
#define IR_LEFT   35
#define IR_RIGHT  34

//ultrasonic
#define TRIG 18
#define ECHO 19
#define RACK_DISTANCE 12   // cm

//MOTORS 
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25


int targetRack = 0;
int rackCount = 0;
bool rackLock = false;

unsigned long waitStartTime = 0;
bool waitingDone = false;


enum RobotState {
  FORWARD_RUN,
  WAIT_AT_RACK,
  RETURN_RUN,
  STOPPED
};

RobotState state = STOPPED;


void setup() {
  Serial.begin(9600);

  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopRobot();

  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  server.on("/rack1", [](){ startMission(1); server.send(200,"text/plain","Rack 1"); });
  server.on("/rack2", [](){ startMission(2); server.send(200,"text/plain","Rack 2"); });
  server.on("/rack3", [](){ startMission(3); server.send(200,"text/plain","Rack 3"); });
  server.on("/stop",  [](){ stopRobot(); state = STOPPED; server.send(200,"text/plain","Stopped"); });

  server.begin();
}

//LOOP 
void loop() {
  server.handleClient();
  if (state == STOPPED) return;

  int L = digitalRead(IR_LEFT);
  int R = digitalRead(IR_RIGHT);
  int d = getDistance();

  Serial.print("L=");
  Serial.print(L);
  Serial.print(" R=");
  Serial.print(R);
  Serial.print(" D=");
  Serial.print(d);
  Serial.print(" Rack=");
  Serial.print(rackCount);
  Serial.print(" State=");
  Serial.println(state);

  /* ===== LINE FOLLOW ===== */
  if (state == FORWARD_RUN || state == RETURN_RUN) {
    if (L == 1 && R == 1) forward();
    else if (L == 1 && R == 0) turnLeft();
    else if (L == 0 && R == 1) turnRight();
    else stopRobot();
  }

  /* ===== RACK DETECTION ===== */
  if (d > 0 && d < RACK_DISTANCE) {
    if (!rackLock) {
      rackLock = true;

      if (state == FORWARD_RUN) rackCount++;
      else if (state == RETURN_RUN) rackCount--;

      Serial.print("RACK UPDATED → ");
      Serial.println(rackCount);
    }
  } else if (d > RACK_DISTANCE + 5) {
    rackLock = false;
  }

  // TARGET RACK REACHED
  if (state == FORWARD_RUN && rackCount == targetRack) {
    stopRobot();

    if (targetRack == 2) {
      waitStartTime = millis();
      waitingDone = false;
      state = WAIT_AT_RACK;
      Serial.println("WAITING 10 SECONDS AT RACK 2");
    } else {
      rotate180();
      state = RETURN_RUN;
    }
  }

  /* ===== WAIT AT RACK 2 ===== */
  if (state == WAIT_AT_RACK) {
    stopRobot();

    if (!waitingDone && millis() - waitStartTime >= 10000) {
      waitingDone = true;
      rotate180();
      state = RETURN_RUN;
      Serial.println("WAIT COMPLETE – RETURNING HOME");
    }
  }

  /* ===== HOME REACHED ===== */
  if (state == RETURN_RUN && rackCount == 0) {
    stopRobot();
    state = STOPPED;
    Serial.println("HOME REACHED – MISSION COMPLETE");
  }

  delay(40);
}

/* ================= FUNCTIONS ================= */

void startMission(int rack) {
  targetRack = rack;
  rackCount = 0;
  rackLock = false;
  waitingDone = false;
  state = FORWARD_RUN;

  Serial.print("MISSION STARTED → RACK ");
  Serial.println(targetRack);
}

int getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

/* ===== MOTION ===== */

void rotate180() {
  Serial.println("TURNING 180 DEGREE");

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  delay(700);   // calibrate once
  stopRobot();
}

void forward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stopRobot() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
