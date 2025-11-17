#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <vector>

// Настройки Wi-Fi
const char* ssid = "NinebotESx";
const char* password = "12345678";

ESP8266WebServer server(80);

// ============================================================================
// СТРУКТУРЫ И КОНСТАНТЫ
// ============================================================================

struct NinebotCommand {
    std::vector<uint8_t> data;
    String description;
};

// Команды
#define CMD_CMAP_RD        0x01
#define CMD_CMAP_WR        0x02
#define CMD_CMAP_WR_NR     0x03
#define CMD_CMAP_ACK_RD    0x04
#define CMD_CMAP_ACK_WR    0x05
#define CMD_HEARTBEAT      0x55

// Регистры
#define INDEX_ERROR_CODE   0x1B
#define INDEX_BOOL_STATUS  0x1D
#define INDEX_WORK_MODE    0x1F
#define INDEX_BATTERY      0x22
#define INDEX_SPEED        0x26
#define INDEX_MILEAGE_L    0x29
#define INDEX_MILEAGE_H    0x2A
#define INDEX_BODY_TEMP    0x3E
#define INDEX_LOCK         0x70
#define INDEX_UNLOCK       0x71
#define INDEX_NORMAL_SPEED 0x73
#define INDEX_SPEED_LIMIT  0x74
#define INDEX_WORK_MODE_CTL 0x75
#define INDEX_ENGINE       0x77
#define INDEX_REBOOT       0x78
#define INDEX_POWER_OFF    0x79
#define INDEX_CRUISE       0x7C
#define INDEX_FUN_BOOL_1   0x80
#define INDEX_HEADLIGHT    0x90
#define INDEX_BEEP_TOTAL   0x92

// ============================================================================
// ПЕРЕМЕННЫЕ
// ============================================================================

const int BUTTON_PIN = 5;
bool isLocked = true;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long lastHeartbeatTime = 0;
const unsigned long HEARTBEAT_INTERVAL = 4000;

// Данные самоката
int scooterSpeed = 0;
int scooterBattery = 0;
int scooterTemperature = 0;
int scooterErrorCode = 0;
unsigned long totalMileage = 0;
int workMode = 0;
int speedLimit = 0;
bool cruiseControl = false;
bool headlightState = true;
bool beepState = true;
bool engineState = true;

// HTML страница
const char* html_page = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Ninebot ES Controller</title>
  <style>
    body { font-family: Arial; margin: 20px; }
    .button { 
      background: #4CAF50; color: white; border: none; padding: 10px 15px; 
      margin: 5px; border-radius: 5px; cursor: pointer; 
    }
    .lock { background: #f44336; }
    .unlock { background: #4CAF50; }
    .status { padding: 10px; margin: 10px 0; border-radius: 5px; }
    .locked { background: #ffebee; color: #c62828; border: 1px solid #c62828; }
    .unlocked { background: #e8f5e8; color: #2e7d32; border: 1px solid #2e7d32; }
  </style>
</head>
<body>
  <h1>🚀 Ninebot ES Controller</h1>
  
  <div class="status" id="status">Загрузка...</div>
  
  <div>
    <button class="button unlock" onclick="sendCommand('unlock')">🔓 Разблокировать</button>
    <button class="button lock" onclick="sendCommand('lock')">🔒 Заблокировать</button>
  </div>
  
  <div>
    <h3>Режимы работы:</h3>
    <button class="button" onclick="sendCommand('mode_normal')">NORMAL</button>
    <button class="button" onclick="sendCommand('mode_eco')">ECO</button>
    <button class="button" onclick="sendCommand('mode_sport')">SPORT</button>
  </div>
  
  <div>
    <h3>Ограничение скорости:</h3>
    <button class="button" onclick="sendCommand('speed_15')">15 км/ч</button>
    <button class="button" onclick="sendCommand('speed_20')">20 км/ч</button>
    <button class="button" onclick="sendCommand('speed_25')">25 км/ч</button>
    <button class="button" onclick="sendCommand('speed_30')">30 км/ч</button>
  </div>
  
  <div>
    <h3>Управление:</h3>
    <button class="button" onclick="sendCommand('headlight_toggle')" id="headlightBtn">Фары ВКЛ</button>
    <button class="button" onclick="sendCommand('beep_toggle')" id="beepBtn">🔊 Звук ВКЛ</button>
    <button class="button" onclick="sendCommand('cruise_toggle')" id="cruiseBtn">⏱️ Круиз ОТКЛ</button>
  </div>

  <script>
    function updateStatus(isLocked) {
      const status = document.getElementById('status');
      if (isLocked) {
        status.innerHTML = '🔒 Статус: ЗАБЛОКИРОВАН';
        status.className = 'status locked';
      } else {
        status.innerHTML = '🔓 Статус: РАЗБЛОКИРОВАН';
        status.className = 'status unlocked';
      }
    }

    function sendCommand(cmd) {
      fetch('/' + cmd)
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            if (data.isLocked !== undefined) {
              updateStatus(data.isLocked);
            }
            alert(data.message);
          } else {
            alert('Ошибка: ' + data.message);
          }
        });
    }

    // Запрос статуса при загрузке
    fetch('/status')
      .then(response => response.json())
      .then(data => {
        updateStatus(data.isLocked);
      });
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// ФУНКЦИИ ПРОТОКОЛА
// ============================================================================

uint16_t calculateChecksum(uint8_t* data, int len) {
    uint16_t checksum = 0;
    for (int i = 0; i < len; i++) {
        checksum += data[i];
    }
    return ~checksum;
}

NinebotCommand createCommand(uint8_t commandType, uint8_t dataIndex, uint16_t dataValue = 0, const String& desc = "") {
    NinebotCommand cmd;
    cmd.description = desc;
    
    uint8_t dataLength = 0;
    switch (commandType) {
        case CMD_CMAP_RD: dataLength = 1; break;
        case CMD_CMAP_WR: 
        case CMD_CMAP_WR_NR: dataLength = 2; break;
        case CMD_HEARTBEAT: dataLength = 1; break;
        default: dataLength = 2;
    }
    
    int totalLength = 9 + dataLength;
    cmd.data.resize(totalLength);
    
    cmd.data[0] = 0x5A;
    cmd.data[1] = 0xA5;
    cmd.data[2] = dataLength;
    cmd.data[3] = 0x3D;
    cmd.data[4] = 0x20;
    cmd.data[5] = commandType;
    cmd.data[6] = dataIndex;
    
    if (dataLength > 0) {
        if (dataLength >= 2) {
            cmd.data[7] = dataValue & 0xFF;
            cmd.data[8] = (dataValue >> 8) & 0xFF;
        } else {
            cmd.data[7] = dataValue & 0xFF;
        }
    }
    
    uint16_t crc = calculateChecksum(cmd.data.data() + 2, dataLength + 4);
    cmd.data[dataLength + 7] = crc & 0xFF;
    cmd.data[dataLength + 8] = (crc >> 8) & 0xFF;
    
    return cmd;
}

// ============================================================================
// ФУНКЦИИ УПРАВЛЕНИЯ
// ============================================================================

void blinkLED(int times) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN, HIGH);
        if (i < times - 1) delay(150);
    }
}

void sendUnlock() {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_UNLOCK, 0x0001, "Разблокировка");
    Serial.write(cmd.data.data(), cmd.data.size());
    isLocked = false;
    blinkLED(2);
}

void sendLock() {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_LOCK, 0x0001, "Блокировка");
    Serial.write(cmd.data.data(), cmd.data.size());
    isLocked = true;
    blinkLED(1);
}

void toggleLockState() {
    if (isLocked) {
        sendUnlock();
    } else {
        sendLock();
    }
}

void sendHeartbeat() {
    NinebotCommand cmd = createCommand(CMD_HEARTBEAT, INDEX_CRUISE, 0x007C, "Heartbeat");
    Serial.write(cmd.data.data(), cmd.data.size());
}

void setWorkMode(uint8_t mode) {
    if (mode > 2) return;
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_WORK_MODE_CTL, mode, "Режим работы");
    Serial.write(cmd.data.data(), cmd.data.size());
    workMode = mode;
}

void setSpeedLimit(uint16_t limit) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_SPEED_LIMIT, limit, "Лимит скорости");
    Serial.write(cmd.data.data(), cmd.data.size());
    speedLimit = limit;
}

void setHeadlight(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_HEADLIGHT, enabled ? 0x0001 : 0x0000, "Фары");
    Serial.write(cmd.data.data(), cmd.data.size());
    headlightState = enabled;
}

void setBeep(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_BEEP_TOTAL, enabled ? 0x0001 : 0x0000, "Звук");
    Serial.write(cmd.data.data(), cmd.data.size());
    beepState = enabled;
}

void setCruiseControl(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_CRUISE, enabled ? 0x0001 : 0x0000, "Круиз-контроль");
    Serial.write(cmd.data.data(), cmd.data.size());
    cruiseControl = enabled;
}

void setEngineState(bool state) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_ENGINE, state ? 0x0001 : 0x0000, "Двигатель");
    Serial.write(cmd.data.data(), cmd.data.size());
    engineState = state;
}

void toggleHeadlight() {
    headlightState = !headlightState;
    setHeadlight(headlightState);
}

void toggleBeep() {
    beepState = !beepState;
    setBeep(beepState);
}

void toggleCruiseControl() {
    cruiseControl = !cruiseControl;
    setCruiseControl(cruiseControl);
}

void handleButton() {
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading == LOW) {
            toggleLockState();
            lastDebounceTime = millis();
        }
    }
    lastButtonState = reading;
}

// ============================================================================
// HTTP ОБРАБОТЧИКИ
// ============================================================================

void handleRoot() {
    server.send(200, "text/html", html_page);
}

void sendSuccess(const char* message) {
    DynamicJsonDocument doc(200);
    doc["success"] = true;
    doc["message"] = message;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleUnlock() { sendUnlock(); sendSuccess("Успешно разблокировано"); }
void handleLock() { sendLock(); sendSuccess("Успешно заблокировано"); }
void handleToggle() { toggleLockState(); sendSuccess(isLocked ? "Заблокировано" : "Разблокировано"); }

void handleModeNormal() { setWorkMode(0); sendSuccess("Режим NORMAL"); }
void handleModeEco() { setWorkMode(1); sendSuccess("Режим ECO"); }
void handleModeSport() { setWorkMode(2); sendSuccess("Режим SPORT"); }

void handleSpeed15() { setSpeedLimit(150); sendSuccess("Лимит скорости 15 км/ч"); }
void handleSpeed20() { setSpeedLimit(200); sendSuccess("Лимит скорости 20 км/ч"); }
void handleSpeed25() { setSpeedLimit(250); sendSuccess("Лимит скорости 25 км/ч"); }
void handleSpeed30() { setSpeedLimit(300); sendSuccess("Лимит скорости 30 км/ч"); }

void handleHeadlightToggle() { toggleHeadlight(); sendSuccess(headlightState ? "Фары включены" : "Фары выключены"); }
void handleBeepToggle() { toggleBeep(); sendSuccess(beepState ? "Звук включен" : "Звук выключен"); }
void handleCruiseToggle() { toggleCruiseControl(); sendSuccess(cruiseControl ? "Круиз-контроль включен" : "Круиз-контроль выключен"); }

void handleEngineOn() { setEngineState(true); sendSuccess("Двигатель включен"); }
void handleEngineOff() { setEngineState(false); sendSuccess("Двигатель выключен"); }

void handleStatus() {
    DynamicJsonDocument doc(200);
    doc["success"] = true;
    doc["isLocked"] = isLocked;
    doc["uptime"] = millis() / 1000;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleData() {
    DynamicJsonDocument doc(512);
    doc["success"] = true;
    doc["speed"] = scooterSpeed;
    doc["battery"] = scooterBattery;
    doc["temperature"] = scooterTemperature;
    doc["mileage"] = totalMileage;
    doc["errorCode"] = scooterErrorCode;
    doc["workMode"] = workMode;
    doc["speedLimit"] = speedLimit;
    doc["headlightState"] = headlightState;
    doc["beepState"] = beepState;
    doc["cruiseControl"] = cruiseControl;
    doc["engineState"] = engineState;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleNotFound() {
    DynamicJsonDocument doc(200);
    doc["success"] = false;
    doc["message"] = "Страница не найдена";
    String response;
    serializeJson(doc, response);
    server.send(404, "application/json", response);
}

// ============================================================================
// SETUP И LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    // Регистрация обработчиков
    server.on("/", handleRoot);
    server.on("/unlock", handleUnlock);
    server.on("/lock", handleLock);
    server.on("/toggle", handleToggle);
    server.on("/status", handleStatus);
    server.on("/data", handleData);

    server.on("/mode_normal", handleModeNormal);
    server.on("/mode_eco", handleModeEco);
    server.on("/mode_sport", handleModeSport);

    server.on("/speed_15", handleSpeed15);
    server.on("/speed_20", handleSpeed20);
    server.on("/speed_25", handleSpeed25);
    server.on("/speed_30", handleSpeed30);

    server.on("/headlight_toggle", handleHeadlightToggle);
    server.on("/beep_toggle", handleBeepToggle);
    server.on("/cruise_toggle", handleCruiseToggle);

    server.on("/engine_on", handleEngineOn);
    server.on("/engine_off", handleEngineOff);

    server.onNotFound(handleNotFound);
    server.begin();

    delay(1000);
    sendUnlock();

    Serial.println("Ninebot ES Controller запущен");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}

void loop() {
    handleButton();
    server.handleClient();

    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        lastHeartbeatTime = millis();
    }
    
    delay(10);
}