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
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Ninebot ES Controller</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: #333;
      line-height: 1.6;
      min-height: 100vh;
      padding: 20px;
    }

    .container {
      max-width: 1200px;
      margin: 0 auto;
    }

    .header {
      text-align: center;
      margin-bottom: 30px;
      color: white;
    }

    .header h1 {
      font-size: 2.5rem;
      margin-bottom: 10px;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
    }

    .header p {
      font-size: 1.1rem;
      opacity: 0.9;
    }

    .dashboard {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
      margin-bottom: 30px;
    }

    .card {
      background: white;
      border-radius: 15px;
      padding: 25px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.2);
      transition: transform 0.3s ease, box-shadow 0.3s ease;
    }

    .card:hover {
      transform: translateY(-5px);
      box-shadow: 0 15px 40px rgba(0,0,0,0.3);
    }

    .status-card {
      text-align: center;
      background: linear-gradient(135deg, #ff6b6b, #ee5a24);
      color: white;
    }

    .status-card.unlocked {
      background: linear-gradient(135deg, #00b894, #00a085);
    }

    .status-icon {
      font-size: 3rem;
      margin-bottom: 15px;
    }

    .status-text {
      font-size: 1.5rem;
      font-weight: bold;
      margin-bottom: 10px;
    }

    .data-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
      gap: 15px;
      margin-top: 20px;
    }

    .data-item {
      text-align: center;
      padding: 15px;
      background: rgba(255,255,255,0.1);
      border-radius: 10px;
    }

    .data-value {
      font-size: 1.8rem;
      font-weight: bold;
      color: white;
    }

    .data-label {
      font-size: 0.9rem;
      opacity: 0.8;
      color: white;
    }

    .control-section {
      margin-bottom: 25px;
    }

    .section-title {
      color: white;
      margin-bottom: 15px;
      font-size: 1.3rem;
      border-left: 4px solid #ff6b6b;
      padding-left: 15px;
    }

    .button-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 12px;
    }

    .btn {
      padding: 15px 20px;
      border: none;
      border-radius: 10px;
      font-size: 1rem;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s ease;
      text-align: center;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }

    .btn:active {
      transform: scale(0.95);
    }

    .btn-primary {
      background: linear-gradient(135deg, #74b9ff, #0984e3);
      color: white;
    }

    .btn-success {
      background: linear-gradient(135deg, #00b894, #00a085);
      color: white;
    }

    .btn-danger {
      background: linear-gradient(135deg, #ff7675, #d63031);
      color: white;
    }

    .btn-warning {
      background: linear-gradient(135deg, #fdcb6e, #e17055);
      color: white;
    }

    .btn-info {
      background: linear-gradient(135deg, #a29bfe, #6c5ce7);
      color: white;
    }

    .btn-toggle {
      background: linear-gradient(135deg, #dfe6e9, #b2bec3);
      color: #2d3436;
    }

    .btn-active {
      background: linear-gradient(135deg, #00cec9, #00b894);
      color: white;
    }

    .toggle-group {
      display: flex;
      background: rgba(255,255,255,0.1);
      border-radius: 10px;
      padding: 5px;
      gap: 5px;
    }

    .toggle-btn {
      flex: 1;
      padding: 12px;
      border: none;
      border-radius: 8px;
      background: transparent;
      color: white;
      cursor: pointer;
      transition: all 0.3s ease;
    }

    .toggle-btn.active {
      background: #ff6b6b;
    }

    .notification {
      position: fixed;
      top: 20px;
      right: 20px;
      padding: 15px 25px;
      background: #00b894;
      color: white;
      border-radius: 10px;
      box-shadow: 0 5px 15px rgba(0,0,0,0.2);
      transform: translateX(400px);
      transition: transform 0.3s ease;
      z-index: 1000;
    }

    .notification.show {
      transform: translateX(0);
    }

    .notification.error {
      background: #d63031;
    }

    /* Адаптивность */
    @media (max-width: 768px) {
      body {
        padding: 10px;
      }

      .header h1 {
        font-size: 2rem;
      }

      .card {
        padding: 20px;
      }

      .button-grid {
        grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
      }

      .btn {
        padding: 12px 15px;
        font-size: 0.9rem;
      }

      .data-value {
        font-size: 1.5rem;
      }
    }

    @media (max-width: 480px) {
      .dashboard {
        grid-template-columns: 1fr;
      }

      .button-grid {
        grid-template-columns: 1fr;
      }

      .data-grid {
        grid-template-columns: repeat(2, 1fr);
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <!-- Шапка -->
    <div class="header">
      <h1>🚀 Ninebot ES Controller</h1>
      <p>Управление вашим самокатом через веб-интерфейс</p>
    </div>

    <!-- Основная панель -->
    <div class="dashboard">
      <!-- Карточка статуса -->
      <div class="card status-card" id="statusCard">
        <div class="status-icon">🔒</div>
        <div class="status-text" id="statusText">ЗАБЛОКИРОВАН</div>
        <div class="status-subtext" id="statusSubtext">Самокат заблокирован</div>
        
        <div class="data-grid">
          <div class="data-item">
            <div class="data-value" id="batteryValue">0%</div>
            <div class="data-label">Батарея</div>
          </div>
          <div class="data-item">
            <div class="data-value" id="speedValue">0</div>
            <div class="data-label">км/ч</div>
          </div>
          <div class="data-item">
            <div class="data-value" id="tempValue">0°</div>
            <div class="data-label">Температура</div>
          </div>
          <div class="data-item">
            <div class="data-value" id="mileageValue">0</div>
            <div class="data-label">км</div>
          </div>
        </div>
      </div>

      <!-- Быстрое управление -->
      <div class="card">
        <h3>⚡ Быстрое управление</h3>
        <div class="button-grid" style="margin-top: 20px;">
          <button class="btn btn-success" onclick="sendCommand('unlock')">
            <span>🔓</span> Разблокировать
          </button>
          <button class="btn btn-danger" onclick="sendCommand('lock')">
            <span>🔒</span> Заблокировать
          </button>
          <button class="btn btn-primary" onclick="toggleDataRefresh()" id="refreshBtn">
            <span>🔄</span> Автообновление
          </button>
        </div>
      </div>
    </div>

    <!-- Управление режимами -->
    <div class="control-section">
      <div class="section-title">🎛️ Режимы работы</div>
      <div class="button-grid">
        <button class="btn btn-info" onclick="sendCommand('mode_normal')">
          <span>🚶</span> NORMAL
        </button>
        <button class="btn btn-info" onclick="sendCommand('mode_eco')">
          <span>🌿</span> ECO
        </button>
        <button class="btn btn-info" onclick="sendCommand('mode_sport')">
          <span>🏎️</span> SPORT
        </button>
      </div>
    </div>

    <!-- Ограничение скорости -->
    <div class="control-section">
      <div class="section-title">📏 Ограничение скорости</div>
      <div class="button-grid">
        <button class="btn btn-warning" onclick="sendCommand('speed_15')">15 км/ч</button>
        <button class="btn btn-warning" onclick="sendCommand('speed_20')">20 км/ч</button>
        <button class="btn btn-warning" onclick="sendCommand('speed_25')">25 км/ч</button>
        <button class="btn btn-warning" onclick="sendCommand('speed_30')">30 км/ч</button>
      </div>
    </div>

    <!-- Дополнительные функции -->
    <div class="control-section">
      <div class="section-title">🔧 Дополнительные функции</div>
      <div class="button-grid">
        <button class="btn btn-toggle" onclick="sendCommand('headlight_toggle')" id="headlightBtn">
          <span>💡</span> Фары ВКЛ
        </button>
        <button class="btn btn-toggle" onclick="sendCommand('beep_toggle')" id="beepBtn">
          <span>🔊</span> Звук ВКЛ
        </button>
        <button class="btn btn-toggle" onclick="sendCommand('cruise_toggle')" id="cruiseBtn">
          <span>⏱️</span> Круиз ОТКЛ
        </button>
        <button class="btn btn-toggle" onclick="sendCommand('engine_on')" id="engineBtn">
          <span>⚡</span> Двигатель ВКЛ
        </button>
      </div>
    </div>
  </div>

  <!-- Уведомления -->
  <div class="notification" id="notification"></div>

  <script>
    let autoRefreshInterval = null;
    let currentStatus = true;

    // Инициализация
    document.addEventListener('DOMContentLoaded', function() {
      loadStatus();
      startAutoRefresh();
    });

    // Функции статуса
    function updateStatus(isLocked) {
      const statusCard = document.getElementById('statusCard');
      const statusText = document.getElementById('statusText');
      const statusSubtext = document.getElementById('statusSubtext');
      
      currentStatus = isLocked;
      
      if (isLocked) {
        statusCard.className = 'card status-card';
        statusText.innerHTML = '🔒 ЗАБЛОКИРОВАН';
        statusSubtext.innerHTML = 'Самокат заблокирован';
      } else {
        statusCard.className = 'card status-card unlocked';
        statusText.innerHTML = '🔓 РАЗБЛОКИРОВАН';
        statusSubtext.innerHTML = 'Самокат готов к работе';
      }
    }

    // Загрузка данных
    async function loadStatus() {
      try {
        const [statusRes, dataRes] = await Promise.all([
          fetch('/status'),
          fetch('/data')
        ]);
        
        const statusData = await statusRes.json();
        const scooterData = await dataRes.json();
        
        if (statusData.success) {
          updateStatus(statusData.isLocked);
        }
        
        if (scooterData.success) {
          updateScooterData(scooterData);
        }
      } catch (error) {
        showNotification('Ошибка загрузки данных', true);
      }
    }

    // Обновление данных самоката
    function updateScooterData(data) {
      document.getElementById('batteryValue').textContent = data.battery + '%';
      document.getElementById('speedValue').textContent = data.speed;
      document.getElementById('tempValue').textContent = data.temperature + '°';
      document.getElementById('mileageValue').textContent = data.mileage;
      
      // Обновляем состояния кнопок
      updateButtonState('headlightBtn', '💡 Фары', data.headlightState);
      updateButtonState('beepBtn', '🔊 Звук', data.beepState);
      updateButtonState('cruiseBtn', '⏱️ Круиз', data.cruiseControl);
      updateButtonState('engineBtn', '⚡ Двигатель', data.engineState);
    }

    // Обновление состояния кнопок
    function updateButtonState(btnId, prefix, state) {
      const btn = document.getElementById(btnId);
      btn.innerHTML = `<span>${prefix.split(' ')[0]}</span> ${prefix.split(' ')[1]} ${state ? 'ВКЛ' : 'ВЫКЛ'}`;
      btn.className = state ? 'btn btn-active' : 'btn btn-toggle';
    }

    // Автообновление
    function startAutoRefresh() {
      autoRefreshInterval = setInterval(loadStatus, 3000);
      document.getElementById('refreshBtn').className = 'btn btn-active';
    }

    function stopAutoRefresh() {
      clearInterval(autoRefreshInterval);
      document.getElementById('refreshBtn').className = 'btn btn-primary';
    }

    function toggleDataRefresh() {
      if (autoRefreshInterval) {
        stopAutoRefresh();
      } else {
        startAutoRefresh();
      }
    }

    // Отправка команд
    async function sendCommand(cmd) {
      try {
        const response = await fetch('/' + cmd);
        const data = await response.json();
        
        if (data.success) {
          showNotification(data.message);
          // Обновляем данные после команды
          setTimeout(loadStatus, 500);
        } else {
          showNotification('Ошибка: ' + data.message, true);
        }
      } catch (error) {
        showNotification('Ошибка соединения', true);
      }
    }

    // Уведомления
    function showNotification(message, isError = false) {
      const notification = document.getElementById('notification');
      notification.textContent = message;
      notification.className = `notification ${isError ? 'error' : ''} show`;
      
      setTimeout(() => {
        notification.className = 'notification';
      }, 3000);
    }

    // Обработка свайпов для мобильных
    let touchStartX = 0;
    let touchEndX = 0;

    document.addEventListener('touchstart', e => {
      touchStartX = e.changedTouches[0].screenX;
    });

    document.addEventListener('touchend', e => {
      touchEndX = e.changedTouches[0].screenX;
      handleSwipe();
    });

    function handleSwipe() {
      const swipeMin = 50;
      if (touchStartX - touchEndX > swipeMin) {
        // Свайп влево - разблокировать
        if (currentStatus) sendCommand('unlock');
      } else if (touchEndX - touchStartX > swipeMin) {
        // Свайп вправо - заблокировать
        if (!currentStatus) sendCommand('lock');
      }
    }
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