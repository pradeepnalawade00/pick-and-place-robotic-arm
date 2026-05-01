"""
Pick and Place Robotic Arm — ESP32 Firmware
Author: Pradeep Nalawade
Description: Complete ESP32 Arduino firmware for a 4-DOF pick-and-place
             robotic arm. Provides a Wi-Fi hosted web interface for
             real-time control and position management.

Hardware:
  - ESP32 Dev Module (38-pin)
  - 4x SG90 / MG996R Servo Motors
  - 5V 3A regulated power supply

Libraries required:
  - ESP32Servo  (install via Arduino Library Manager)
"""

// ─────────────────────────────────────────────
//  INCLUDES
// ─────────────────────────────────────────────
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// ─────────────────────────────────────────────
//  CONFIGURATION
// ─────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Serial baud rate
#define BAUD_RATE 115200

// EEPROM size (for saved positions)
#define EEPROM_SIZE 64

// Servo GPIO pins
#define PIN_BASE     13
#define PIN_SHOULDER 12
#define PIN_ELBOW    14
#define PIN_GRIPPER  27

// Servo angle limits [min, max] degrees
#define BASE_MIN      0
#define BASE_MAX    180
#define SHOULDER_MIN 20
#define SHOULDER_MAX 160
#define ELBOW_MIN    10
#define ELBOW_MAX   170
#define GRIPPER_MIN   0   // fully open
#define GRIPPER_MAX  90   // fully closed

// Default home position
#define HOME_BASE     90
#define HOME_SHOULDER 90
#define HOME_ELBOW    90
#define HOME_GRIPPER   0

// Servo movement speed (ms between degree steps)
#define SERVO_SPEED_MS 10

// ─────────────────────────────────────────────
//  GLOBALS
// ─────────────────────────────────────────────
Servo servoBase, servoShoulder, servoElbow, servoGripper;
WebServer server(80);

// Current angles
int angleBase     = HOME_BASE;
int angleShoulder = HOME_SHOULDER;
int angleElbow    = HOME_ELBOW;
int angleGripper  = HOME_GRIPPER;

// Named saved positions (stored in EEPROM)
struct SavedPosition {
  int base;
  int shoulder;
  int elbow;
  int gripper;
};
SavedPosition savedPos = {HOME_BASE, HOME_SHOULDER, HOME_ELBOW, HOME_GRIPPER};

// ─────────────────────────────────────────────
//  SERVO CONTROL
// ─────────────────────────────────────────────

int clampAngle(int angle, int minVal, int maxVal) {
  return max(minVal, min(maxVal, angle));
}

// Smooth servo movement — prevents mechanical shock
void smoothMove(Servo& servo, int& current, int target, int minA, int maxA) {
  target = clampAngle(target, minA, maxA);
  int step = (target > current) ? 1 : -1;
  while (current != target) {
    current += step;
    servo.write(current);
    delay(SERVO_SPEED_MS);
  }
}

void moveAllSmooth(int base, int shoulder, int elbow, int gripper) {
  // Move simultaneously using simple stepping loop
  base     = clampAngle(base,     BASE_MIN,     BASE_MAX);
  shoulder = clampAngle(shoulder, SHOULDER_MIN, SHOULDER_MAX);
  elbow    = clampAngle(elbow,    ELBOW_MIN,    ELBOW_MAX);
  gripper  = clampAngle(gripper,  GRIPPER_MIN,  GRIPPER_MAX);

  int steps = max({abs(base - angleBase), abs(shoulder - angleShoulder),
                   abs(elbow - angleElbow), abs(gripper - angleGripper)});

  if (steps == 0) return;

  for (int i = 1; i <= steps; i++) {
    float t = (float)i / steps;
    int b = angleBase     + (int)(t * (base     - angleBase));
    int s = angleShoulder + (int)(t * (shoulder - angleShoulder));
    int e = angleElbow    + (int)(t * (elbow    - angleElbow));
    int g = angleGripper  + (int)(t * (gripper  - angleGripper));
    servoBase.write(b);
    servoShoulder.write(s);
    servoElbow.write(e);
    servoGripper.write(g);
    delay(SERVO_SPEED_MS);
  }
  angleBase = base; angleShoulder = shoulder;
  angleElbow = elbow; angleGripper = gripper;
}

void goHome() {
  moveAllSmooth(HOME_BASE, HOME_SHOULDER, HOME_ELBOW, HOME_GRIPPER);
  Serial.println("[ARM] Returned to home position");
}

// ─────────────────────────────────────────────
//  EEPROM — SAVED POSITIONS
// ─────────────────────────────────────────────

void savePosition() {
  savedPos = {angleBase, angleShoulder, angleElbow, angleGripper};
  EEPROM.put(0, savedPos);
  EEPROM.commit();
  Serial.printf("[EEPROM] Position saved: B=%d S=%d E=%d G=%d\n",
                savedPos.base, savedPos.shoulder, savedPos.elbow, savedPos.gripper);
}

void loadAndGoSaved() {
  EEPROM.get(0, savedPos);
  moveAllSmooth(savedPos.base, savedPos.shoulder, savedPos.elbow, savedPos.gripper);
  Serial.println("[ARM] Moved to saved position");
}

// ─────────────────────────────────────────────
//  WEB INTERFACE (HTML served from ESP32)
// ─────────────────────────────────────────────

const char* HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Robotic Arm Control</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', sans-serif; background: #0d1117; color: #e6edf3; min-height: 100vh; padding: 20px; }
  h1 { text-align: center; color: #a78bfa; margin-bottom: 24px; font-size: 1.6rem; }
  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; max-width: 700px; margin: 0 auto; }
  .card { background: #161b22; border: 1px solid #30363d; border-radius: 12px; padding: 18px; }
  .card h3 { color: #58a6ff; margin-bottom: 14px; font-size: 0.9rem; text-transform: uppercase; letter-spacing: 0.05em; }
  label { display: flex; justify-content: space-between; font-size: 0.85rem; color: #8b949e; margin-bottom: 6px; }
  input[type=range] { width: 100%; accent-color: #a78bfa; margin-bottom: 16px; cursor: pointer; }
  .btn-row { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 8px; }
  button { flex: 1; padding: 10px 14px; border: none; border-radius: 8px; font-weight: 600;
           font-size: 0.85rem; cursor: pointer; transition: opacity 0.2s; }
  button:hover { opacity: 0.85; }
  .btn-primary { background: #a78bfa; color: #fff; }
  .btn-danger  { background: #f85149; color: #fff; }
  .btn-success { background: #3fb950; color: #fff; }
  .btn-neutral { background: #30363d; color: #e6edf3; }
  .status { text-align: center; margin-top: 16px; padding: 10px; border-radius: 8px;
            background: #161b22; border: 1px solid #30363d; font-size: 0.8rem; color: #8b949e; }
  .angles { font-family: monospace; font-size: 0.9rem; color: #39d353; text-align: center; }
</style>
</head>
<body>
<h1>🤖 Robotic Arm Control Panel</h1>
<div class="grid">
  <div class="card" style="grid-column: 1/-1">
    <h3>Joint Angles</h3>
    <label>Base <span id="baseVal">90°</span></label>
    <input type="range" id="base" min="0" max="180" value="90" oninput="updateLabel('base','baseVal')">
    <label>Shoulder <span id="shoulderVal">90°</span></label>
    <input type="range" id="shoulder" min="20" max="160" value="90" oninput="updateLabel('shoulder','shoulderVal')">
    <label>Elbow <span id="elbowVal">90°</span></label>
    <input type="range" id="elbow" min="10" max="170" value="90" oninput="updateLabel('elbow','elbowVal')">
    <label>Gripper <span id="gripperVal">0° (Open)</span></label>
    <input type="range" id="gripper" min="0" max="90" value="0" oninput="updateLabel('gripper','gripperVal')">
    <div class="btn-row">
      <button class="btn-primary" onclick="sendCommand()">▶ Move Arm</button>
      <button class="btn-neutral" onclick="goHome()">🏠 Home</button>
      <button class="btn-success" onclick="savePos()">💾 Save Position</button>
      <button class="btn-danger"  onclick="loadPos()">📂 Load Saved</button>
    </div>
  </div>
</div>
<div class="status">
  <div class="angles" id="currentAngles">Current: B=90° | S=90° | E=90° | G=0°</div>
</div>
<script>
function updateLabel(sliderId, labelId) {
  const v = document.getElementById(sliderId).value;
  document.getElementById(labelId).textContent = sliderId === 'gripper'
    ? `${v}° (${v==0?'Open':v==90?'Closed':'Partial'})` : `${v}°`;
}
async function sendCommand() {
  const b = document.getElementById('base').value;
  const s = document.getElementById('shoulder').value;
  const e = document.getElementById('elbow').value;
  const g = document.getElementById('gripper').value;
  const res = await fetch(`/move?base=${b}&shoulder=${s}&elbow=${e}&gripper=${g}`);
  const json = await res.json();
  document.getElementById('currentAngles').textContent =
    `Current: B=${json.base}° | S=${json.shoulder}° | E=${json.elbow}° | G=${json.gripper}°`;
}
async function goHome() {
  await fetch('/home');
  ['base','shoulder','elbow'].forEach(id => {
    document.getElementById(id).value = 90;
    updateLabel(id, id+'Val');
  });
  document.getElementById('gripper').value = 0;
  updateLabel('gripper','gripperVal');
  document.getElementById('currentAngles').textContent = 'Current: B=90° | S=90° | E=90° | G=0°';
}
async function savePos() { await fetch('/save'); alert('Position saved to EEPROM!'); }
async function loadPos() {
  const res = await fetch('/load');
  const json = await res.json();
  ['base','shoulder','elbow','gripper'].forEach(k => {
    document.getElementById(k).value = json[k];
    updateLabel(k, k+'Val');
  });
  document.getElementById('currentAngles').textContent =
    `Current: B=${json.base}° | S=${json.shoulder}° | E=${json.elbow}° | G=${json.gripper}°`;
}
</script>
</body>
</html>
)rawliteral";

// ─────────────────────────────────────────────
//  HTTP ROUTE HANDLERS
// ─────────────────────────────────────────────

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleMove() {
  int b = server.hasArg("base")     ? server.arg("base").toInt()     : angleBase;
  int s = server.hasArg("shoulder") ? server.arg("shoulder").toInt() : angleShoulder;
  int e = server.hasArg("elbow")    ? server.arg("elbow").toInt()    : angleElbow;
  int g = server.hasArg("gripper")  ? server.arg("gripper").toInt()  : angleGripper;

  moveAllSmooth(b, s, e, g);

  StaticJsonDocument<128> doc;
  doc["base"] = angleBase; doc["shoulder"] = angleShoulder;
  doc["elbow"] = angleElbow; doc["gripper"] = angleGripper;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

  Serial.printf("[HTTP] Move → B=%d S=%d E=%d G=%d\n", angleBase, angleShoulder, angleElbow, angleGripper);
}

void handleHome() {
  goHome();
  server.send(200, "text/plain", "OK");
}

void handleSave() {
  savePosition();
  server.send(200, "text/plain", "Saved");
}

void handleLoad() {
  loadAndGoSaved();
  StaticJsonDocument<128> doc;
  doc["base"] = angleBase; doc["shoulder"] = angleShoulder;
  doc["elbow"] = angleElbow; doc["gripper"] = angleGripper;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStatus() {
  StaticJsonDocument<256> doc;
  doc["base"] = angleBase; doc["shoulder"] = angleShoulder;
  doc["elbow"] = angleElbow; doc["gripper"] = angleGripper;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────

void setup() {
  Serial.begin(BAUD_RATE);
  delay(500);
  Serial.println("\n[BOOT] Pick & Place Robotic Arm — Pradeep Nalawade");

  // EEPROM init
  EEPROM.begin(EEPROM_SIZE);

  // Attach servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servoBase.setPeriodHertz(50);
  servoShoulder.setPeriodHertz(50);
  servoElbow.setPeriodHertz(50);
  servoGripper.setPeriodHertz(50);
  servoBase.attach(PIN_BASE,         500, 2400);
  servoShoulder.attach(PIN_SHOULDER, 500, 2400);
  servoElbow.attach(PIN_ELBOW,       500, 2400);
  servoGripper.attach(PIN_GRIPPER,   500, 2400);

  // Move to home
  goHome();

  // Connect to Wi-Fi
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500); Serial.print("."); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] Failed — starting AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("RoboArm_AP", "robotic123");
    Serial.printf("[WIFI] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }

  // Register routes
  server.on("/",       handleRoot);
  server.on("/move",   handleMove);
  server.on("/home",   handleHome);
  server.on("/save",   handleSave);
  server.on("/load",   handleLoad);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("[HTTP] Web server started");
  Serial.println("[READY] Open your browser to control the arm");
}

// ─────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────

void loop() {
  server.handleClient();
  delay(2);
}
