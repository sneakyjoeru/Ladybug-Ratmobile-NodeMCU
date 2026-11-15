/*
  RATMOBILE (home WiFi version) — ESP8266 (NodeMCU) firmware

  - Joins your existing WiFi network (set WIFI_SSID / WIFI_PASS below).
  - Web page at http://ratmobile.local from any device on that network,
    with W A S D buttons and speed sliders. The serial monitor (115200)
    also prints the address your router gave it.
  - If the network can't be reached within 20 seconds, it falls back to its
    own hotspot "Ratmobile" (password ratmobile123890, http://192.168.4.1),
    so the robot is never left without a control page.
  - The physical keys work the whole time, connected or not.
  - Physical key switches A, W, D on the lid work like the keyboard keys.
  - Holding the W switch for over a second and then pressing it again
    reverses the direction; short repeated presses keep going the same way.
  - Three or more keys at once: the robot stops.
  - Firmware can be updated over WiFi at http://ratmobile.local/update
    (anyone on your home network can open the page and that link).

  Board: "NodeMCU 1.0 (ESP-12E Module)" in the Arduino IDE (ESP8266 core).
  No extra libraries needed.

  Wiring (GPIO numbers as printed on the board):
    IO12 -> DRV8833 IN1   left motor forward
    IO14 -> DRV8833 IN2   left motor reverse
    IO4  -> DRV8833 IN3   right motor forward
    IO5  -> DRV8833 IN4   right motor reverse
    IO15 -> DRV8833 EEP   driver enable (sleep when low)
    IO0  -> switch A      left     (to GND when pressed)
    IO2  -> switch D      right    (to GND when pressed)
    IO13 -> switch W      forward  (to GND when pressed)
    RX   -> switch S      back     (optional, see USE_BTN_S)
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <EEPROM.h>

// ---------------------------------------------------------------- settings

// Your home WiFi. 2.4 GHz only: the ESP8266 cannot see 5 GHz networks.
const char* WIFI_SSID = "WiFiNetworkName";
const char* WIFI_PASS = "WiFiNetworkPassword";

// How long to keep trying before giving up on the home network.
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;

// If the home network can't be reached, open this hotspot instead.
// Set FALLBACK_HOTSPOT to false to keep trying the home network forever.
const bool  FALLBACK_HOTSPOT = true;
const char* AP_SSID = "Ratmobile";
const char* AP_PASS = "ratmobile123890";

// The robot answers at http://<HOSTNAME>.local
const char* HOSTNAME = "ratmobile";

const uint8_t PIN_IN1 = 12;   // left forward
const uint8_t PIN_IN2 = 14;   // left reverse
const uint8_t PIN_IN3 = 4;    // right forward
const uint8_t PIN_IN4 = 5;    // right reverse
const uint8_t PIN_EEP = 15;   // DRV8833 enable

const uint8_t PIN_BTN_A = 0;
const uint8_t PIN_BTN_D = 2;
const uint8_t PIN_BTN_W = 13;
const uint8_t PIN_BTN_S = 3;  // RX pin

// Set to true only if the S switch is really wired to RX.
const bool USE_BTN_S = false;

// If a motor turns the wrong way, flip its flag instead of resoldering.
const bool INVERT_LEFT  = false;
const bool INVERT_RIGHT = false;

// If W + A steers right instead of left, set this to true.
const bool SWAP_MOTORS = false;

// When driving and steering together, the inner wheel runs at this share
// of the drive speed (0.0 = inner wheel stops, 1.0 = no steering).
const float ARC_INNER = 0.35f;

// Quick ramp, in PWM steps per 5 ms (max 1023). Used for the web buttons and
// for every slow-down or stop. 40 gives 0 -> full in ~130 ms.
// Lower = gentler on the small 5 V booster.
const int RAMP_STEP = 40;

// Smooth start for the physical keys: time to climb from standstill to full
// power while a key is held. Bigger = gentler. Releasing a key still stops
// quickly.
const unsigned long KEY_ACCEL_MS = 1200;

// Reverse with the physical W switch:
//   - Short presses (press, let go, press again) keep the same direction.
//   - If W was held for a while (W_HOLD_MS or longer) and is then pressed
//     again, the direction flips: forward becomes back, back becomes forward.
//
// W_HOLD_MS: how long a press must last before the next press flips.
// W_MIN_GAP_MS: a pause shorter than this is a wobble inside one push, not
//   a release; the push simply continues.
// W_RESET_MS: after a pause longer than this, W means forward again.
//   0 = never reset.
const unsigned long W_HOLD_MS    = 1000;
const unsigned long W_MIN_GAP_MS = 200;
const unsigned long W_RESET_MS   = 3000;

// A switch must hold its state this long before it counts (contact bounce
// would otherwise look like a double press).
const unsigned long DEBOUNCE_MS = 25;

// The smooth start begins at this power (0..1023) instead of zero, because
// the motors don't turn at all below it and the ramp would feel like a delay.
const int KEY_ACCEL_START = 250;

// The web page must keep sending commands; if it goes quiet for this long
// (lost connection, closed tab), the robot stops.
const unsigned long WEB_TIMEOUT_MS = 600;

// WiFi transmit power in dBm (0..20.5). Lower power draws less current,
// which helps the small booster, but shortens the range to your router.
// Raise it towards 20 if the robot drops off the network in far rooms.
const float WIFI_TX_POWER = 15.0f;

const int PWM_MAX = 1023;

// ---------------------------------------------------------------- state

enum { KEY_W = 1, KEY_A = 2, KEY_S = 4, KEY_D = 8 };

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;
DNSServer dns;

uint8_t driveSpeed = 70;   // percent
uint8_t turnSpeed  = 55;   // percent

enum NetState { NET_CONNECTING, NET_HOME, NET_HOTSPOT };
NetState netState = NET_CONNECTING;
unsigned long connectStartMs = 0;

uint8_t webKeys = 0;
unsigned long lastWebMs = 0;

int curLeft = 0, curRight = 0;   // current motor output, -1023..1023
unsigned long lastRampMs = 0;

// ---------------------------------------------------------------- web page

const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Ratmobile</title>
<style>
  :root{color-scheme:dark}
  *{box-sizing:border-box}
  body{margin:0;font-family:system-ui,sans-serif;background:#14161b;color:#e8eaf0;
       display:flex;flex-direction:column;align-items:center;gap:22px;padding:22px 16px;
       -webkit-user-select:none;user-select:none;-webkit-touch-callout:none}
  h1{margin:0;font-size:22px;letter-spacing:.08em}
  #st{font-size:13px;color:#8b93a5}
  #st.bad{color:#ff6b6b}
  .pad{display:grid;grid-template-columns:repeat(3,92px);grid-template-rows:repeat(2,92px);gap:12px}
  .k{border:2px solid #3a4152;border-radius:16px;background:#1f232c;color:#e8eaf0;
     font-size:30px;font-weight:700;touch-action:none;cursor:pointer}
  .k small{display:block;font-size:11px;font-weight:400;color:#8b93a5;margin-top:2px}
  .k.on{background:#3d7dff;border-color:#3d7dff}
  .k.on small{color:#e8eaf0}
  #w{grid-column:2;grid-row:1}
  #a{grid-column:1;grid-row:2} #s{grid-column:2;grid-row:2} #d{grid-column:3;grid-row:2}
  .set{width:100%;max-width:340px;display:flex;flex-direction:column;gap:14px}
  label{display:flex;justify-content:space-between;font-size:15px}
  input[type=range]{width:100%;height:34px;accent-color:#3d7dff}
  a{color:#8b93a5;font-size:12px}
</style></head><body>
<h1>RATMOBILE</h1>
<div id="st">connecting…</div>
<div class="pad">
  <button class="k" id="w">W<small>forward</small></button>
  <button class="k" id="a">A<small>left</small></button>
  <button class="k" id="s">S<small>back</small></button>
  <button class="k" id="d">D<small>right</small></button>
</div>
<div class="set">
  <div><label>Drive speed <span id="sv">–</span></label>
       <input type="range" id="sp" min="20" max="100" step="5"></div>
  <div><label>Turn speed <span id="tv">–</span></label>
       <input type="range" id="tn" min="20" max="100" step="5"></div>
</div>
<a href="/update">firmware update</a>
<script>
const B={w:1,a:2,s:4,d:8}, ALIAS={arrowup:'w',arrowleft:'a',arrowdown:'s',arrowright:'d'};
const $=id=>document.getElementById(id);
let mask=0,busy=false,dirty=false;
function st(ok){const e=$('st');e.textContent=ok?'connected':'no connection';e.className=ok?'':'bad';}
function push(){
  if(busy){dirty=true;return;}
  busy=true;dirty=false;
  fetch('/k?m='+mask,{cache:'no-store'}).then(()=>st(true)).catch(()=>st(false))
    .finally(()=>{busy=false;if(dirty)push();});
}
function key(k,on){
  const b=B[k];if(!b)return;
  const m=on?(mask|b):(mask&~b);
  if(m===mask)return;
  mask=m;$(k).classList.toggle('on',on);push();
}
function releaseAll(){for(const k in B)key(k,false);}
for(const k in B){
  const e=$(k);
  e.addEventListener('pointerdown',ev=>{ev.preventDefault();try{e.setPointerCapture(ev.pointerId);}catch(_){ }key(k,true);});
  for(const n of ['pointerup','pointercancel','lostpointercapture'])e.addEventListener(n,()=>key(k,false));
  e.addEventListener('contextmenu',ev=>ev.preventDefault());
}
function kb(ev,on){
  if(ev.repeat)return;
  let k=ev.key.toLowerCase();k=ALIAS[k]||k;
  if(B[k]){ev.preventDefault();key(k,on);}
}
addEventListener('keydown',ev=>kb(ev,true));
addEventListener('keyup',ev=>kb(ev,false));
addEventListener('blur',releaseAll);
document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseAll();});
setInterval(()=>{if(mask)push();},200);

function show(){$('sv').textContent=$('sp').value+'%';$('tv').textContent=$('tn').value+'%';}
function save(){fetch('/set?speed='+$('sp').value+'&turn='+$('tn').value,{cache:'no-store'}).then(()=>st(true)).catch(()=>st(false));}
for(const id of ['sp','tn']){$(id).addEventListener('input',show);$(id).addEventListener('change',save);}
fetch('/state',{cache:'no-store'}).then(r=>r.json()).then(j=>{$('sp').value=j.speed;$('tn').value=j.turn;show();st(true);}).catch(()=>st(false));
</script></body></html>)HTML";

// ---------------------------------------------------------------- settings storage

void loadSettings() {
  EEPROM.begin(8);
  if (EEPROM.read(0) == 0xA5) {
    driveSpeed = constrain(EEPROM.read(1), 20, 100);
    turnSpeed  = constrain(EEPROM.read(2), 20, 100);
  }
}

void saveSettings() {
  EEPROM.write(0, 0xA5);
  EEPROM.write(1, driveSpeed);
  EEPROM.write(2, turnSpeed);
  EEPROM.commit();
}

// ---------------------------------------------------------------- motors

// value: -1023 (full reverse) .. 1023 (full forward)
void writeMotor(uint8_t pinFwd, uint8_t pinRev, int value, bool invert) {
  if (invert) value = -value;
  if (value >= 0) {
    analogWrite(pinRev, 0);
    analogWrite(pinFwd, value);
  } else {
    analogWrite(pinFwd, 0);
    analogWrite(pinRev, -value);
  }
}

void applyMotors() {
  int l = curLeft, r = curRight;
  if (SWAP_MOTORS) { int t = l; l = r; r = t; }
  writeMotor(PIN_IN1, PIN_IN2, l, INVERT_LEFT);
  writeMotor(PIN_IN3, PIN_IN4, r, INVERT_RIGHT);
}

// Move one motor's output one tick closer to its target.
// Speeding up uses stepUp; slowing down, stopping and reversing use the
// quick RAMP_STEP. startAt lets a slow ramp skip the dead zone near zero.
int approach(int current, int target, int stepUp, int startAt) {
  if (current == target) return current;

  bool sameDirection = (current >= 0 && target >= 0) || (current <= 0 && target <= 0);
  bool speedingUp = sameDirection && abs(target) > abs(current);

  if (!speedingUp) {
    // Slow down towards the target, but never jump across zero in one go.
    int goal = sameDirection ? target : 0;
    if (current > goal) return max(current - RAMP_STEP, goal);
    return min(current + RAMP_STEP, goal);
  }

  int dir = (target > 0) ? 1 : -1;
  int mag = abs(current);
  if (mag < startAt) mag = startAt; else mag += stepUp;
  if (mag > abs(target)) mag = abs(target);
  return dir * mag;
}

// ---------------------------------------------------------------- input

uint8_t readButtonsRaw() {
  uint8_t k = 0;
  if (digitalRead(PIN_BTN_W) == LOW) k |= KEY_W;
  if (digitalRead(PIN_BTN_A) == LOW) k |= KEY_A;
  if (digitalRead(PIN_BTN_D) == LOW) k |= KEY_D;
  if (USE_BTN_S && digitalRead(PIN_BTN_S) == LOW) k |= KEY_S;
  return k;
}

// Debounced switches, with the double press on W turned into "back".
// Call once per tick.
uint8_t readButtons(unsigned long now) {
  static uint8_t lastRaw = 0, stable = 0;
  static unsigned long changedMs = 0;

  uint8_t raw = readButtonsRaw();
  if (raw != lastRaw) {
    lastRaw = raw;
    changedMs = now;
  } else if (now - changedMs >= DEBOUNCE_MS) {
    stable = raw;
  }

  static bool wWasDown = false, wEverPressed = false, wReverse = false;
  static unsigned long wDownMs = 0, wUpMs = 0;

  bool wDown = stable & KEY_W;
  if (wDown && !wWasDown) {                       // W just pressed
    unsigned long gap = now - wUpMs;
    if (!wEverPressed) {
      wReverse = false;                           // very first press: forward
      wDownMs = now;
    } else if (gap < W_MIN_GAP_MS) {
      // a wobble: same push, same direction, hold time keeps counting
    } else {
      bool lastWasLong = (wUpMs - wDownMs) >= W_HOLD_MS;
      if (W_RESET_MS != 0 && gap > W_RESET_MS) wReverse = false;   // long pause: forward
      else if (lastWasLong) wReverse = !wReverse;                  // held, then pressed again: flip
      // otherwise a short press: keep the direction
      wDownMs = now;
    }
    wEverPressed = true;
  } else if (!wDown && wWasDown) {                // W just released
    wUpMs = now;
  }
  wWasDown = wDown;

  uint8_t keys = stable;
  if (wDown && wReverse) keys = (keys & ~KEY_W) | KEY_S;
  return keys;
}

// Turn the pressed keys into a target speed for each motor.
void keysToTargets(uint8_t keys, int &left, int &right) {
  int fwd  = ((keys & KEY_W) ? 1 : 0) - ((keys & KEY_S) ? 1 : 0);
  int turn = ((keys & KEY_D) ? 1 : 0) - ((keys & KEY_A) ? 1 : 0);

  int drive = (int)driveSpeed * PWM_MAX / 100;
  int spin  = (int)turnSpeed  * PWM_MAX / 100;

  if (fwd != 0) {
    left = right = fwd * drive;
    if (turn > 0) right = (int)(right * ARC_INNER);   // steer right
    if (turn < 0) left  = (int)(left  * ARC_INNER);   // steer left
  } else if (turn != 0) {
    left  =  turn * spin;                             // spin on the spot
    right = -turn * spin;
  } else {
    left = right = 0;
  }
}

// ---------------------------------------------------------------- web handlers

void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

void handleKeys() {
  webKeys = (uint8_t)(server.arg("m").toInt() & 0x0F);
  lastWebMs = millis();
  server.send(200, "text/plain", "ok");
}

void handleSet() {
  if (server.hasArg("speed")) driveSpeed = constrain(server.arg("speed").toInt(), 20, 100);
  if (server.hasArg("turn"))  turnSpeed  = constrain(server.arg("turn").toInt(), 20, 100);
  saveSettings();
  server.send(200, "text/plain", "ok");
}

void handleState() {
  String j = String("{\"speed\":") + driveSpeed + ",\"turn\":" + turnSpeed + "}";
  server.send(200, "application/json", j);
}

void handleNotFound() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// ---------------------------------------------------------------- network

void startHomeWifi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.setOutputPower(WIFI_TX_POWER);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  connectStartMs = millis();
  netState = NET_CONNECTING;
  Serial.print(F("Connecting to "));
  Serial.println(WIFI_SSID);
}

void startHotspot() {
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.setOutputPower(WIFI_TX_POWER);
  WiFi.softAP(AP_SSID, AP_PASS);
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  dns.setErrorReplyCode(DNSReplyCode::NonExistentDomain);
  dns.start(53, String(HOSTNAME) + ".local", WiFi.softAPIP());
  netState = NET_HOTSPOT;
  Serial.print(F("Home WiFi not reachable. Hotspot \""));
  Serial.print(AP_SSID);
  Serial.print(F("\" is up, open http://"));
  Serial.println(WiFi.softAPIP());
}

// Call often. Never blocks, so the keys and motors keep working while
// the WiFi is still connecting.
void serviceNetwork(unsigned long now) {
  switch (netState) {
    case NET_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        MDNS.begin(HOSTNAME);
        MDNS.addService("http", "tcp", 80);
        netState = NET_HOME;
        Serial.print(F("Connected. Open http://"));
        Serial.print(HOSTNAME);
        Serial.print(F(".local or http://"));
        Serial.println(WiFi.localIP());
      } else if (FALLBACK_HOTSPOT && now - connectStartMs > WIFI_CONNECT_TIMEOUT_MS) {
        startHotspot();
      }
      break;
    case NET_HOME:
      MDNS.update();        // reconnects after a dropout are automatic
      break;
    case NET_HOTSPOT:
      dns.processNextRequest();
      MDNS.update();
      break;
  }
}

// ---------------------------------------------------------------- setup / loop

void setup() {
  // Motors off and driver asleep before anything else.
  pinMode(PIN_EEP, OUTPUT);
  digitalWrite(PIN_EEP, LOW);
  for (uint8_t p : {PIN_IN1, PIN_IN2, PIN_IN3, PIN_IN4}) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }

  // TX only, so the RX pin stays free for the optional S switch.
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();
  Serial.println(F("Ratmobile starting"));

  pinMode(PIN_BTN_W, INPUT_PULLUP);
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_D, INPUT_PULLUP);
  if (USE_BTN_S) pinMode(PIN_BTN_S, INPUT_PULLUP);

  analogWriteRange(PWM_MAX);
  analogWriteFreq(1000);

  loadSettings();

  startHomeWifi();

  server.on("/", handleRoot);
  server.on("/k", handleKeys);
  server.on("/set", handleSet);
  server.on("/state", handleState);
  server.onNotFound(handleNotFound);
  updater.setup(&server);          // firmware upload page at /update
  server.begin();

  digitalWrite(PIN_EEP, HIGH);     // wake the motor driver
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  serviceNetwork(now);

  // Stop if the web page has gone quiet.
  if (webKeys && now - lastWebMs > WEB_TIMEOUT_MS) webKeys = 0;

  if (now - lastRampMs >= 5) {
    lastRampMs = now;

    uint8_t buttons = readButtons(now);

    // Three or more keys at once (lid and web together): do nothing.
    uint8_t keys = buttons | webKeys;
    if (__builtin_popcount(keys) >= 3) keys = 0;

    int targetLeft, targetRight;
    keysToTargets(keys, targetLeft, targetRight);

    // Physical keys get the smooth start; the web buttons stay snappy.
    int stepUp = RAMP_STEP, startAt = 0;
    if (buttons) {
      stepUp  = max(1, (int)((long)PWM_MAX * 5 / (long)KEY_ACCEL_MS));
      startAt = KEY_ACCEL_START;
    }

    int nl = approach(curLeft, targetLeft, stepUp, startAt);
    int nr = approach(curRight, targetRight, stepUp, startAt);
    if (nl != curLeft || nr != curRight) {
      curLeft = nl;
      curRight = nr;
      applyMotors();
    }
  }
}
