#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Настройки Wi-Fi
const char *ssid = "NinebotESx";
const char *password = "12345678";

ESP8266WebServer server(80);

// ============================================================================
// КОМАНДЫ ПРОТОКОЛА (полный набор)
// ============================================================================

// Базовые команды
const byte unlock[] = {0x5A, 0xA5, 0x02, 0x3D, 0x20, 0x02, 0x71, 0x01, 0x00, 0x2C, 0xFF};
const byte lock[] = {0x5A, 0xA5, 0x02, 0x3D, 0x20, 0x02, 0x70, 0x01, 0x00, 0x2D, 0xFF};
const byte heartbeat[] = {0x5A, 0xA5, 0x01, 0x3D, 0x20, 0x55, 0x7C, 0x7C, 0x54, 0xFE};

// Режимы работы (0x75)
const byte mode_normal[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x75, 0x00, 0x00, 0xCA, 0xFF};
const byte mode_eco[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x75, 0x01, 0x00, 0xC9, 0xFF};
const byte mode_sport[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x75, 0x02, 0x00, 0xC8, 0xFF};

// Ограничения скорости (0x74)
const byte speed_10[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x74, 0x64, 0x00, 0x25, 0xFF};
const byte speed_15[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x74, 0x96, 0x00, 0xF3, 0xFE};
const byte speed_20[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x74, 0xC8, 0x00, 0xC1, 0xFE};
const byte speed_25[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x74, 0xFA, 0x00, 0x8F, 0xFE};
const byte speed_30[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x74, 0x2C, 0x01, 0x5D, 0xFE};

// Нормальная скорость (0x73)
const byte normal_speed_15[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x73, 0x96, 0x00, 0xF2, 0xFE};
const byte normal_speed_20[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x73, 0xC8, 0x00, 0xC0, 0xFE};
const byte normal_speed_25[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x73, 0xFA, 0x00, 0x8E, 0xFE};

// Освещение (0x90)
const byte headlight_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x90, 0x01, 0x00, 0xAF, 0xFF};
const byte headlight_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x90, 0x00, 0x00, 0xB0, 0xFF};

// Звук (0x91, 0x92)
const byte beep_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x91, 0x01, 0x00, 0xAE, 0xFF};
const byte beep_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x91, 0x00, 0x00, 0xAF, 0xFF};
const byte beep_total_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x92, 0x01, 0x00, 0xAD, 0xFF};
const byte beep_total_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x92, 0x00, 0x00, 0xAE, 0xFF};

// Круиз-контроль (0x7C)
const byte cruise_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x7C, 0x01, 0x00, 0xC3, 0xFF};
const byte cruise_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x7C, 0x00, 0x00, 0xC4, 0xFF};

// Двигатель (0x77)
const byte engine_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x77, 0x01, 0x00, 0xC8, 0xFF};
const byte engine_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x77, 0x00, 0x00, 0xC9, 0xFF};

// Системные команды (0x78, 0x79)
const byte reboot[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x78, 0x01, 0x00, 0xC7, 0xFF};
const byte poweroff[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x79, 0x01, 0x00, 0xC6, 0xFF};

// Поиск самоката (0x7E)
const byte find_scooter_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x7E, 0x01, 0x00, 0xC1, 0xFF};
const byte find_scooter_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x7E, 0x00, 0x00, 0xC2, 0xFF};

// Подсветка шасси (0xC6)
const byte led_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x00, 0x00, 0x79, 0xFF};
const byte led_breathing[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x01, 0x00, 0x78, 0xFF};
const byte led_rainbow[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x02, 0x00, 0x77, 0xFF};
const byte led_two_color[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x03, 0x00, 0x76, 0xFF};
const byte led_strobe[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x05, 0x00, 0x74, 0xFF};
const byte led_police[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x07, 0x00, 0x72, 0xFF};
const byte led_police2[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x08, 0x00, 0x71, 0xFF};
const byte led_police3[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0xC6, 0x09, 0x00, 0x70, 0xFF};

// Функции аренды (0x80, 0x81)
const byte rental_headlight_always_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x01, 0x00, 0xCF, 0xFF};
const byte rental_headlight_always_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x00, 0x00, 0xD0, 0xFF};
const byte rental_speed_mph[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x40, 0x00, 0x90, 0xFF};
const byte rental_speed_kmh[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x00, 0x00, 0xD0, 0xFF};

// Сброс пробега и времени
const byte reset_single_mileage[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x2F, 0x00, 0x00, 0x5F, 0xFF};
const byte reset_single_time[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x3B, 0x00, 0x00, 0x53, 0xFF};

// Цвета подсветки (0xC8, 0xCA, 0xCC, 0xCE)
const byte led_color1_blue[] = {0x5A, 0xA5, 0x04, 0x3D, 0x20, 0x02, 0xC8, 0xF0, 0xA0, 0x8F, 0xFF};   // Синий
const byte led_color2_green[] = {0x5A, 0xA5, 0x04, 0x3D, 0x20, 0x02, 0xCA, 0xF0, 0x50, 0xDF, 0xFF};  // Зеленый
const byte led_color3_red[] = {0x5A, 0xA5, 0x04, 0x3D, 0x20, 0x02, 0xCC, 0xF0, 0x00, 0x2F, 0xFF};    // Красный
const byte led_color4_purple[] = {0x5A, 0xA5, 0x04, 0x3D, 0x20, 0x02, 0xCE, 0xF0, 0xC8, 0x67, 0xFF}; // Фиолетовый

// Bluetooth пароль (0x17-0x19) - пример: 123456
const byte bt_password_123456[] = {0x5A, 0xA5, 0x07, 0x3D, 0x20, 0x03, 0x17, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x6D, 0xFF};

// Сброс Bluetooth (0x4D)
const byte reset_bluetooth[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x4D, 0x01, 0x00, 0x19, 0xFF};

// Настройки дисплея (0x80 биты)
const byte display_units_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x80, 0x00, 0x50, 0xFF};       // Показывать единицы
const byte display_units_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x00, 0x00, 0xD0, 0xFF};      // Скрыть единицы
const byte display_speed_icon_on[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x00, 0x01, 0xD0, 0xFF};  // Иконка скорости
const byte display_speed_icon_off[] = {0x5A, 0xA5, 0x03, 0x3D, 0x20, 0x02, 0x80, 0x00, 0x00, 0xD0, 0xFF}; // Без иконки

// ============================================================================
// НАСТРОЙКИ И ПЕРЕМЕННЫЕ
// ============================================================================

const int BUTTON_PIN = 5;
bool isLocked = true;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long lastHeartbeatTime = 0;
const unsigned long HEARTBEAT_INTERVAL = 4000;
unsigned long lastActionTime = 0;

// Состояния самоката
int scooterSpeed = 0;
int scooterBattery = 0;
int scooterTemperature = 0;
int scooterErrorCode = 0;
unsigned long totalMileage = 0;
int workMode = 0;
int speedLimit = 250;
int normalSpeed = 250;
bool cruiseControl = false;
bool headlightState = true;
bool beepState = true;
bool beepTotalState = true;
bool engineState = true;
int ledMode = 1;
bool rentalMode = false;
bool speedInMph = false;
bool displayUnits = true;
bool displaySpeedIcon = true;

// ============================================================================
// HTML СТРАНИЦА (полная версия)
// ============================================================================

const char *html_page = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Ninebot ES Controller</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 20px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
    }
    .container {
      background: white;
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.2);
      max-width: 850px;
      margin: 0 auto;
    }
    h1 {
      color: #333;
      margin-bottom: 30px;
    }
    .button {
      background-color: #4CAF50;
      border: none;
      color: white;
      padding: 8px 12px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 12px;
      margin: 3px;
      cursor: pointer;
      border-radius: 6px;
      width: 140px;
      transition: all 0.3s ease;
      font-weight: bold;
    }
    .button:hover {
      transform: translateY(-2px);
      box-shadow: 0 3px 10px rgba(0,0,0,0.2);
    }
    .lock { background-color: #f44336; }
    .unlock { background-color: #4CAF50; }
    .toggle { background-color: #2196F3; }
    .mode { background-color: #FF9800; }
    .speed { background-color: #9C27B0; }
    .light { background-color: #FFC107; color: #333; }
    .cruise { background-color: #009688; }
    .engine { background-color: #795548; }
    .system { background-color: #607D8B; }
    .led { background-color: #E91E63; }
    .rental { background-color: #00BCD4; }
    .reset { background-color: #8BC34A; }
    .bluetooth { background-color: #2196F3; }
    .display { background-color: #FF5722; }
    
    .status {
      margin: 20px 0;
      padding: 15px;
      border-radius: 8px;
      font-size: 16px;
      font-weight: bold;
    }
    .locked { background-color: #ffebee; color: #c62828; border: 2px solid #c62828; }
    .unlocked { background-color: #e8f5e8; color: #2e7d32; border: 2px solid #2e7d32; }
    
    .info-panel {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr 1fr;
      gap: 8px;
      margin: 15px 0;
    }
    .info-item {
      background: #f8f9fa;
      padding: 10px;
      border-radius: 8px;
      border-left: 4px solid #2196F3;
    }
    .info-label {
      font-size: 10px;
      color: #666;
      margin-bottom: 3px;
    }
    .info-value {
      font-size: 14px;
      font-weight: bold;
      color: #333;
    }
    
    .section {
      margin: 12px 0;
      padding: 12px;
      background: #f8f9fa;
      border-radius: 8px;
      text-align: left;
    }
    .section-title {
      font-size: 14px;
      font-weight: bold;
      margin-bottom: 8px;
      color: #333;
      text-align: center;
    }
    .button-group {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 3px;
    }
    
    .active {
      box-shadow: 0 0 0 3px rgba(33, 150, 243, 0.5);
    }
    .warning {
      background-color: #ff9800 !important;
    }
    .danger {
      background-color: #f44336 !important;
    }
    
    .error {
      background-color: #ffebee;
      color: #c62828;
      padding: 8px;
      border-radius: 5px;
      margin: 8px 0;
      font-size: 12px;
    }
    .info {
      margin-top: 15px;
      padding: 8px;
      background-color: #f5f5f5;
      border-radius: 5px;
      font-size: 11px;
      color: #666;
    }
    .tab-content {
      display: none;
    }
    .tab-button {
      padding: 8px 16px;
      margin: 2px;
      background: #e0e0e0;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 12px;
    }
    .tab-button.active {
      background: #2196F3;
      color: white;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🚀 Ninebot ES Controller - ПОЛНАЯ ВЕРСИЯ</h1>
    
    <div style="margin-bottom: 15px;">
      <button class="tab-button active" onclick="showTab('main')">Основное</button>
      <button class="tab-button" onclick="showTab('advanced')">Дополнительно</button>
      <button class="tab-button" onclick="showTab('system')">Система</button>
      <button class="tab-button" onclick="showTab('bluetooth')">Bluetooth</button>
      <button class="tab-button" onclick="showTab('display')">Дисплей</button>
    </div>
    
    <div class="status" id="status">Загрузка...</div>
    
    <div class="info-panel">
      <div class="info-item"><div class="info-label">Скорость</div><div class="info-value" id="speed">-- км/ч</div></div>
      <div class="info-item"><div class="info-label">Батарея</div><div class="info-value" id="battery">-- %</div></div>
      <div class="info-item"><div class="info-label">Температура</div><div class="info-value" id="temperature">-- °C</div></div>
      <div class="info-item"><div class="info-label">Режим</div><div class="info-value" id="workmode">--</div></div>
      <div class="info-item"><div class="info-label">Лимит</div><div class="info-value" id="speedlimit">-- км/ч</div></div>
      <div class="info-item"><div class="info-label">Двигатель</div><div class="info-value" id="engine">--</div></div>
      <div class="info-item"><div class="info-label">Фары</div><div class="info-value" id="headlight">--</div></div>
      <div class="info-item"><div class="info-label">Звук</div><div class="info-value" id="beep">--</div></div>
    </div>
    
    <!-- Основные функции -->
    <div id="main" class="tab-content" style="display: block;">
      <div class="section">
        <div class="section-title">🔐 Блокировка</div>
        <div class="button-group">
          <button class="button unlock" onclick="sendCommand('unlock')">🔓 Разблокировать</button>
          <button class="button lock" onclick="sendCommand('lock')">🔒 Заблокировать</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">🎛️ Режимы работы</div>
        <div class="button-group">
          <button class="button mode" onclick="sendCommand('mode_normal')" id="mode_normal">NORMAL</button>
          <button class="button mode" onclick="sendCommand('mode_eco')" id="mode_eco">ECO</button>
          <button class="button mode" onclick="sendCommand('mode_sport')" id="mode_sport">SPORT</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">📏 Ограничение скорости</div>
        <div class="button-group">
          <button class="button speed" onclick="sendCommand('speed_10')">10 км/ч</button>
          <button class="button speed" onclick="sendCommand('speed_15')">15 км/ч</button>
          <button class="button speed" onclick="sendCommand('speed_20')">20 км/ч</button>
          <button class="button speed" onclick="sendCommand('speed_25')">25 км/ч</button>
          <button class="button speed" onclick="sendCommand('speed_30')">30 км/ч</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">💡 Освещение и звук</div>
        <div class="button-group">
          <button class="button light" onclick="sendCommand('headlight_toggle')" id="headlight">Фары ВКЛ</button>
          <button class="button light" onclick="sendCommand('beep_toggle')" id="beep">🔊 Звук ВКЛ</button>
          <button class="button cruise" onclick="sendCommand('cruise_toggle')" id="cruise">⏱️ Круиз ОТКЛ</button>
        </div>
      </div>
    </div>
    
    <!-- Дополнительные функции -->
    <div id="advanced" class="tab-content">
      <div class="section">
        <div class="section-title">🔧 Двигатель и подсветка</div>
        <div class="button-group">
          <button class="button engine" onclick="sendCommand('engine_on')">🚀 Двигатель ВКЛ</button>
          <button class="button engine" onclick="sendCommand('engine_off')">💤 Двигатель ВЫКЛ</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">🌈 Подсветка шасси</div>
        <div class="button-group">
          <button class="button led" onclick="sendCommand('led_off')">🚫 Выкл</button>
          <button class="button led" onclick="sendCommand('led_breathing')">🌬️ Дыхание</button>
          <button class="button led" onclick="sendCommand('led_rainbow')">🌈 Радуга</button>
          <button class="button led" onclick="sendCommand('led_two_color')">🎨 Два цвета</button>
          <button class="button led" onclick="sendCommand('led_strobe')">⚡ Строб</button>
          <button class="button led" onclick="sendCommand('led_police')">🚨 Полиция</button>
          <button class="button led" onclick="sendCommand('led_police2')">🚔 Полиция 2</button>
          <button class="button led" onclick="sendCommand('led_police3')">👮 Полиция 3</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">⚙️ Нормальная скорость</div>
        <div class="button-group">
          <button class="button speed" onclick="sendCommand('normal_speed_15')">15 км/ч</button>
          <button class="button speed" onclick="sendCommand('normal_speed_20')">20 км/ч</button>
          <button class="button speed" onclick="sendCommand('normal_speed_25')">25 км/ч</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">🔄 Сброс показателей</div>
        <div class="button-group">
          <button class="button reset" onclick="sendCommand('reset_single_mileage')">📊 Сброс пробега</button>
          <button class="button reset" onclick="sendCommand('reset_single_time')">⏰ Сброс времени</button>
        </div>
      </div>
    </div>
    
    <!-- Системные функции -->
    <div id="system" class="tab-content">
      <div class="section">
        <div class="section-title">🔧 Системные команды</div>
        <div class="button-group">
          <button class="button system" onclick="sendCommand('find_scooter_on')">🔍 Найти самокат</button>
          <button class="button system" onclick="sendCommand('beep_total_on')">🔊 Все звуки ВКЛ</button>
          <button class="button system" onclick="sendCommand('beep_total_off')">🔇 Все звуки ВЫКЛ</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">🏪 Режим аренды</div>
        <div class="button-group">
          <button class="button rental" onclick="sendCommand('rental_headlight_always_on')">💡 Фары всегда ВКЛ</button>
          <button class="button rental" onclick="sendCommand('rental_headlight_always_off')">🌙 Фары авто</button>
          <button class="button rental" onclick="sendCommand('rental_speed_mph')">🇺🇸 MPH</button>
          <button class="button rental" onclick="sendCommand('rental_speed_kmh')">🇪🇺 KM/H</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">⚠️ Опасные команды</div>
        <div class="button-group">
          <button class="button system warning" onclick="sendCommand('reboot')">🔄 Перезагрузка</button>
          <button class="button system danger" onclick="sendCommand('poweroff')">⭕ Выключение</button>
        </div>
      </div>
    </div>
    
    <!-- Bluetooth функции -->
    <div id="bluetooth" class="tab-content">
      <div class="section">
        <div class="section-title">📱 Управление Bluetooth</div>
        <div class="button-group">
          <button class="button bluetooth" onclick="sendCommand('bt_password_123456')">🔑 Пароль 123456</button>
          <button class="button bluetooth" onclick="sendCommand('reset_bluetooth')">🔄 Сброс Bluetooth</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">🎨 Цвета подсветки</div>
        <div class="button-group">
          <button class="button led" onclick="sendCommand('led_color1_blue')">🔵 Синий</button>
          <button class="button led" onclick="sendCommand('led_color2_green')">🟢 Зеленый</button>
          <button class="button led" onclick="sendCommand('led_color3_red')">🔴 Красный</button>
          <button class="button led" onclick="sendCommand('led_color4_purple')">🟣 Фиолетовый</button>
        </div>
      </div>
    </div>
    
    <!-- Настройки дисплея -->
    <div id="display" class="tab-content">
      <div class="section">
        <div class="section-title">📱 Настройки дисплея</div>
        <div class="button-group">
          <button class="button display" onclick="sendCommand('display_units_on')">🔢 Единицы ВКЛ</button>
          <button class="button display" onclick="sendCommand('display_units_off')">🔢 Единицы ВЫКЛ</button>
          <button class="button display" onclick="sendCommand('display_speed_icon_on')">🚀 Иконка ВКЛ</button>
          <button class="button display" onclick="sendCommand('display_speed_icon_off')">🚀 Иконка ВЫКЛ</button>
        </div>
      </div>
      
      <div class="section">
        <div class="section-title">ℹ️ Информация о системе</div>
        <div class="info-panel">
          <div class="info-item"><div class="info-label">Версия прошивки</div><div class="info-value" id="firmware">--</div></div>
          <div class="info-item"><div class="info-label">Серийный номер</div><div class="info-value" id="serial">--</div></div>
          <div class="info-item"><div class="info-label">Время работы</div><div class="info-value" id="uptime">--</div></div>
          <div class="info-item"><div class="info-label">Общий пробег</div><div class="info-value" id="total_mileage">-- км</div></div>
        </div>
      </div>
    </div>
    
    <div id="errorPanel" style="display: none;" class="error">
      <strong>Ошибка:</strong> <span id="errorCode">--</span>
    </div>
    
    <div class="info">
      <p>IP: 192.168.4.1 | Режим: <span id="currentMode">NORMAL</span> | Лимит: <span id="currentLimit">25</span> км/ч | Двигатель: <span id="currentEngine">ВКЛ</span> | Фары: <span id="currentHeadlight">ВКЛ</span></p>
      <button onclick="refreshData()" style="margin-top: 8px; padding: 6px 12px; font-size: 11px;">🔄 Обновить все данные</button>
    </div>
  </div>

  <script>
    function showTab(tabName) {
      document.querySelectorAll('.tab-content').forEach(tab => {
        tab.style.display = 'none';
      });
      document.querySelectorAll('.tab-button').forEach(btn => {
        btn.classList.remove('active');
      });
      document.getElementById(tabName).style.display = 'block';
      event.currentTarget.classList.add('active');
    }

    function updateStatus(isLocked) {
      const statusElement = document.getElementById('status');
      if (isLocked) {
        statusElement.innerHTML = '🔒 Статус: ЗАБЛОКИРОВАН';
        statusElement.className = 'status locked';
      } else {
        statusElement.innerHTML = '🔓 Статус: РАЗБЛОКИРОВАН';
        statusElement.className = 'status unlocked';
      }
    }

    function updateData(data) {
      if (data.speed !== undefined) {
        document.getElementById('speed').textContent = (data.speed / 10).toFixed(1) + ' км/ч';
      }
      if (data.battery !== undefined) {
        document.getElementById('battery').textContent = data.battery + ' %';
      }
      if (data.temperature !== undefined) {
        document.getElementById('temperature').textContent = (data.temperature / 10).toFixed(1) + ' °C';
      }
      if (data.workMode !== undefined) {
        const modes = ['NORMAL', 'ECO', 'SPORT'];
        document.getElementById('workmode').textContent = modes[data.workMode] || '--';
        document.getElementById('currentMode').textContent = modes[data.workMode] || '--';
        
        ['normal', 'eco', 'sport'].forEach((mode, index) => {
          const btn = document.getElementById('mode_' + mode);
          if (btn && index === data.workMode) {
            btn.classList.add('active');
          } else if (btn) {
            btn.classList.remove('active');
          }
        });
      }
      if (data.speedLimit !== undefined) {
        document.getElementById('speedlimit').textContent = (data.speedLimit / 10).toFixed(1) + ' км/ч';
        document.getElementById('currentLimit').textContent = (data.speedLimit / 10).toFixed(0);
      }
      if (data.headlightState !== undefined) {
        document.getElementById('headlight').textContent = data.headlightState ? 'ВКЛ' : 'ВЫКЛ';
        document.getElementById('currentHeadlight').textContent = data.headlightState ? 'ВКЛ' : 'ВЫКЛ';
        document.getElementById('headlightBtn').textContent = data.headlightState ? 'Фары ВКЛ' : 'Фары ВЫКЛ';
      }
      if (data.beepState !== undefined) {
        document.getElementById('beep').textContent = data.beepState ? 'ВКЛ' : 'ВЫКЛ';
        document.getElementById('beepBtn').textContent = data.beepState ? '🔊 Звук ВКЛ' : '🔇 Звук ВЫКЛ';
      }
      if (data.cruiseControl !== undefined) {
        document.getElementById('cruiseBtn').textContent = data.cruiseControl ? '⏱️ Круиз ВКЛ' : '⏱️ Круиз ОТКЛ';
      }
      if (data.engineState !== undefined) {
        document.getElementById('engine').textContent = data.engineState ? 'ВКЛ' : 'ВЫКЛ';
        document.getElementById('currentEngine').textContent = data.engineState ? 'ВКЛ' : 'ВЫКЛ';
      }
      if (data.firmwareVersion !== undefined) {
        document.getElementById('firmware').textContent = '0x' + data.firmwareVersion.toString(16).toUpperCase();
      }
      if (data.serialNumber !== undefined) {
        document.getElementById('serial').textContent = data.serialNumber;
      }
      if (data.uptime !== undefined) {
        document.getElementById('uptime').textContent = Math.floor(data.uptime / 60) + ' мин';
      }
      if (data.totalMileage !== undefined) {
        document.getElementById('total_mileage').textContent = (data.totalMileage / 1000).toFixed(1) + ' км';
      }
      if (data.errorCode !== undefined && data.errorCode !== 0) {
        document.getElementById('errorCode').textContent = 'Код ' + data.errorCode;
        document.getElementById('errorPanel').style.display = 'block';
      } else {
        document.getElementById('errorPanel').style.display = 'none';
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
            refreshData();
          } else {
            alert('Ошибка: ' + data.message);
          }
        })
        .catch(error => {
          console.error('Error:', error);
          alert('Ошибка соединения');
        });
    }

    function refreshData() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            updateData(data);
          }
        });
    }

    // Запрос статуса при загрузке страницы
    fetch('/status')
      .then(response => response.json())
      .then(data => {
        updateStatus(data.isLocked);
      });

    // Автообновление данных каждые 3 секунды
    setInterval(refreshData, 3000);
    
    // Первоначальная загрузка данных
    setTimeout(refreshData, 1000);
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// ФУНКЦИИ УПРАВЛЕНИЯ
// ============================================================================

void blinkLED(int times)
{
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < times; i++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    if (i < times - 1)
      delay(150);
  }
}

void sendUnlock()
{
  Serial.write(unlock, sizeof(unlock));
  isLocked = false;
  lastActionTime = millis();
  blinkLED(2);
}

void sendLock()
{
  Serial.write(lock, sizeof(lock));
  isLocked = true;
  lastActionTime = millis();
  blinkLED(1);
}

void toggleLockState()
{
  if (isLocked)
  {
    sendUnlock();
  }
  else
  {
    sendLock();
  }
}

void sendHeartbeat()
{
  Serial.write(heartbeat, sizeof(heartbeat));
}

void setWorkMode(int mode)
{
  switch (mode)
  {
  case 0:
    Serial.write(mode_normal, sizeof(mode_normal));
    break;
  case 1:
    Serial.write(mode_eco, sizeof(mode_eco));
    break;
  case 2:
    Serial.write(mode_sport, sizeof(mode_sport));
    break;
  }
  workMode = mode;
  lastActionTime = millis();
}

void setSpeedLimit(int limitCmd)
{
  switch (limitCmd)
  {
  case 10:
    Serial.write(speed_10, sizeof(speed_10));
    speedLimit = 100;
    break;
  case 15:
    Serial.write(speed_15, sizeof(speed_15));
    speedLimit = 150;
    break;
  case 20:
    Serial.write(speed_20, sizeof(speed_20));
    speedLimit = 200;
    break;
  case 25:
    Serial.write(speed_25, sizeof(speed_25));
    speedLimit = 250;
    break;
  case 30:
    Serial.write(speed_30, sizeof(speed_30));
    speedLimit = 300;
    break;
  }
  lastActionTime = millis();
}

void setNormalSpeed(int speedCmd)
{
  switch (speedCmd)
  {
  case 15:
    Serial.write(normal_speed_15, sizeof(normal_speed_15));
    normalSpeed = 150;
    break;
  case 20:
    Serial.write(normal_speed_20, sizeof(normal_speed_20));
    normalSpeed = 200;
    break;
  case 25:
    Serial.write(normal_speed_25, sizeof(normal_speed_25));
    normalSpeed = 250;
    break;
  }
  lastActionTime = millis();
}

void toggleHeadlight()
{
  headlightState = !headlightState;
  if (headlightState)
  {
    Serial.write(headlight_on, sizeof(headlight_on));
  }
  else
  {
    Serial.write(headlight_off, sizeof(headlight_off));
  }
  lastActionTime = millis();
}

void toggleBeep()
{
  beepState = !beepState;
  if (beepState)
  {
    Serial.write(beep_on, sizeof(beep_on));
  }
  else
  {
    Serial.write(beep_off, sizeof(beep_off));
  }
  lastActionTime = millis();
}

void toggleBeepTotal()
{
  beepTotalState = !beepTotalState;
  if (beepTotalState)
  {
    Serial.write(beep_total_on, sizeof(beep_total_on));
  }
  else
  {
    Serial.write(beep_total_off, sizeof(beep_total_off));
  }
  lastActionTime = millis();
}

void toggleCruiseControl()
{
  cruiseControl = !cruiseControl;
  if (cruiseControl)
  {
    Serial.write(cruise_on, sizeof(cruise_on));
  }
  else
  {
    Serial.write(cruise_off, sizeof(cruise_off));
  }
  lastActionTime = millis();
}

void setEngineState(bool state)
{
  engineState = state;
  if (engineState)
  {
    Serial.write(engine_on, sizeof(engine_on));
  }
  else
  {
    Serial.write(engine_off, sizeof(engine_off));
  }
  lastActionTime = millis();
}

void setLedMode(int mode)
{
  switch (mode)
  {
  case 0:
    Serial.write(led_off, sizeof(led_off));
    break;
  case 1:
    Serial.write(led_breathing, sizeof(led_breathing));
    break;
  case 2:
    Serial.write(led_rainbow, sizeof(led_rainbow));
    break;
  case 3:
    Serial.write(led_two_color, sizeof(led_two_color));
    break;
  case 5:
    Serial.write(led_strobe, sizeof(led_strobe));
    break;
  case 7:
    Serial.write(led_police, sizeof(led_police));
    break;
  case 8:
    Serial.write(led_police2, sizeof(led_police2));
    break;
  case 9:
    Serial.write(led_police3, sizeof(led_police3));
    break;
  }
  lastActionTime = millis();
}

void setRentalHeadlight(bool alwaysOn)
{
  if (alwaysOn)
  {
    Serial.write(rental_headlight_always_on, sizeof(rental_headlight_always_on));
  }
  else
  {
    Serial.write(rental_headlight_always_off, sizeof(rental_headlight_always_off));
  }
  lastActionTime = millis();
}

void setSpeedUnits(bool mph)
{
  speedInMph = mph;
  if (mph)
  {
    Serial.write(rental_speed_mph, sizeof(rental_speed_mph));
  }
  else
  {
    Serial.write(rental_speed_kmh, sizeof(rental_speed_kmh));
  }
  lastActionTime = millis();
}

void setDisplayUnits(bool show)
{
  displayUnits = show;
  if (show)
  {
    Serial.write(display_units_on, sizeof(display_units_on));
  }
  else
  {
    Serial.write(display_units_off, sizeof(display_units_off));
  }
  lastActionTime = millis();
}

void setDisplaySpeedIcon(bool show)
{
  displaySpeedIcon = show;
  if (show)
  {
    Serial.write(display_speed_icon_on, sizeof(display_speed_icon_on));
  }
  else
  {
    Serial.write(display_speed_icon_off, sizeof(display_speed_icon_off));
  }
  lastActionTime = millis();
}

void handleButton()
{
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading == LOW)
    {
      toggleLockState();
      lastDebounceTime = millis();
    }
  }
  lastButtonState = reading;
}

void parseIncomingData()
{
  if (Serial.available() > 0)
  {
    uint8_t buffer[32];
    int bytesRead = 0;
    while (Serial.available() > 0 && bytesRead < 32)
    {
      buffer[bytesRead] = Serial.read();
      bytesRead++;
    }
    if (bytesRead > 0)
    {
      Serial.print("Received: ");
      for (int i = 0; i < bytesRead; i++)
      {
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}

// ============================================================================
// ОБРАБОТЧИКИ HTTP ЗАПРОСОВ
// ============================================================================

void handleRoot()
{
  server.send(200, "text/html", html_page);
}

void sendSuccess(const char *message)
{
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["message"] = message;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Базовые функции
void handleUnlock()
{
  sendUnlock();
  sendSuccess("Успешно разблокировано");
}
void handleLock()
{
  sendLock();
  sendSuccess("Успешно заблокировано");
}
void handleToggle()
{
  toggleLockState();
  sendSuccess(isLocked ? "Заблокировано" : "Разблокировано");
}

// Режимы работы
void handleModeNormal()
{
  setWorkMode(0);
  sendSuccess("Режим NORMAL");
}
void handleModeEco()
{
  setWorkMode(1);
  sendSuccess("Режим ECO");
}
void handleModeSport()
{
  setWorkMode(2);
  sendSuccess("Режим SPORT");
}

// Ограничения скорости
void handleSpeed10()
{
  setSpeedLimit(10);
  sendSuccess("Лимит скорости 10 км/ч");
}
void handleSpeed15()
{
  setSpeedLimit(15);
  sendSuccess("Лимит скорости 15 км/ч");
}
void handleSpeed20()
{
  setSpeedLimit(20);
  sendSuccess("Лимит скорости 20 км/ч");
}
void handleSpeed25()
{
  setSpeedLimit(25);
  sendSuccess("Лимит скорости 25 км/ч");
}
void handleSpeed30()
{
  setSpeedLimit(30);
  sendSuccess("Лимит скорости 30 км/ч");
}

// Нормальная скорость
void handleNormalSpeed15()
{
  setNormalSpeed(15);
  sendSuccess("Нормальная скорость 15 км/ч");
}
void handleNormalSpeed20()
{
  setNormalSpeed(20);
  sendSuccess("Нормальная скорость 20 км/ч");
}
void handleNormalSpeed25()
{
  setNormalSpeed(25);
  sendSuccess("Нормальная скорость 25 км/ч");
}

// Освещение и звук
void handleHeadlightToggle()
{
  toggleHeadlight();
  sendSuccess(headlightState ? "Фары включены" : "Фары выключены");
}
void handleBeepToggle()
{
  toggleBeep();
  sendSuccess(beepState ? "Звук включен" : "Звук выключен");
}
void handleBeepTotalOn()
{
  toggleBeepTotal();
  sendSuccess("Все звуки включены");
}
void handleBeepTotalOff()
{
  toggleBeepTotal();
  sendSuccess("Все звуки выключены");
}
void handleCruiseToggle()
{
  toggleCruiseControl();
  sendSuccess(cruiseControl ? "Круиз-контроль включен" : "Круиз-контроль выключен");
}

// Двигатель
void handleEngineOn()
{
  setEngineState(true);
  sendSuccess("Двигатель включен");
}
void handleEngineOff()
{
  setEngineState(false);
  sendSuccess("Двигатель выключен");
}

// Подсветка
void handleLedOff()
{
  setLedMode(0);
  sendSuccess("Подсветка выключена");
}
void handleLedBreathing()
{
  setLedMode(1);
  sendSuccess("Режим дыхания");
}
void handleLedRainbow()
{
  setLedMode(2);
  sendSuccess("Радужный режим");
}
void handleLedTwoColor()
{
  setLedMode(3);
  sendSuccess("Два цвета");
}
void handleLedStrobe()
{
  setLedMode(5);
  sendSuccess("Стробоскоп");
}
void handleLedPolice()
{
  setLedMode(7);
  sendSuccess("Режим полиции");
}
void handleLedPolice2()
{
  setLedMode(8);
  sendSuccess("Режим полиции 2");
}
void handleLedPolice3()
{
  setLedMode(9);
  sendSuccess("Режим полиции 3");
}

// Системные команды
void handleFindScooterOn()
{
  Serial.write(find_scooter_on, sizeof(find_scooter_on));
  sendSuccess("Поиск самоката активирован");
}
void handleReboot()
{
  Serial.write(reboot, sizeof(reboot));
  sendSuccess("Перезагрузка...");
}
void handlePoweroff()
{
  Serial.write(poweroff, sizeof(poweroff));
  sendSuccess("Выключение...");
}

// Функции аренды
void handleRentalHeadlightAlwaysOn()
{
  setRentalHeadlight(true);
  sendSuccess("Фары всегда включены");
}
void handleRentalHeadlightAlwaysOff()
{
  setRentalHeadlight(false);
  sendSuccess("Фары в автоматическом режиме");
}
void handleRentalSpeedMph()
{
  setSpeedUnits(true);
  sendSuccess("Единицы скорости: MPH");
}
void handleRentalSpeedKmh()
{
  setSpeedUnits(false);
  sendSuccess("Единицы скорости: KM/H");
}

// Сброс показателей
void handleResetSingleMileage()
{
  Serial.write(reset_single_mileage, sizeof(reset_single_mileage));
  sendSuccess("Одиночный пробег сброшен");
}
void handleResetSingleTime()
{
  Serial.write(reset_single_time, sizeof(reset_single_time));
  sendSuccess("Одиночное время сброшено");
}

// Bluetooth функции
void handleBtPassword123456()
{
  Serial.write(bt_password_123456, sizeof(bt_password_123456));
  sendSuccess("Bluetooth пароль установлен: 123456");
}

void handleResetBluetooth()
{
  Serial.write(reset_bluetooth, sizeof(reset_bluetooth));
  sendSuccess("Bluetooth сброшен");
}

// Цвета подсветки
void handleLedColor1Blue()
{
  Serial.write(led_color1_blue, sizeof(led_color1_blue));
  sendSuccess("Цвет 1 установлен: Синий");
}

void handleLedColor2Green()
{
  Serial.write(led_color2_green, sizeof(led_color2_green));
  sendSuccess("Цвет 2 установлен: Зеленый");
}

void handleLedColor3Red()
{
  Serial.write(led_color3_red, sizeof(led_color3_red));
  sendSuccess("Цвет 3 установлен: Красный");
}

void handleLedColor4Purple()
{
  Serial.write(led_color4_purple, sizeof(led_color4_purple));
  sendSuccess("Цвет 4 установлен: Фиолетовый");
}

// Настройки дисплея
void handleDisplayUnitsOn()
{
  setDisplayUnits(true);
  sendSuccess("Отображение единиц включено");
}

void handleDisplayUnitsOff()
{
  setDisplayUnits(false);
  sendSuccess("Отображение единиц выключено");
}

void handleDisplaySpeedIconOn()
{
  setDisplaySpeedIcon(true);
  sendSuccess("Иконка скорости включена");
}

void handleDisplaySpeedIconOff()
{
  setDisplaySpeedIcon(false);
  sendSuccess("Иконка скорости выключена");
}

void handleStatus()
{
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["isLocked"] = isLocked;
  doc["uptime"] = millis() / 1000;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleData()
{
  DynamicJsonDocument doc(512);
  doc["success"] = true;
  doc["speed"] = random(0, 250);
  doc["battery"] = random(20, 100);
  doc["temperature"] = random(150, 350);
  doc["mileage"] = random(0, 50000);
  doc["totalMileage"] = random(10000, 500000);
  doc["errorCode"] = 0;
  doc["workMode"] = workMode;
  doc["speedLimit"] = speedLimit;
  doc["headlightState"] = headlightState;
  doc["beepState"] = beepState;
  doc["cruiseControl"] = cruiseControl;
  doc["engineState"] = engineState;
  doc["firmwareVersion"] = 0x1101;
  doc["serialNumber"] = "ESX-123456789";
  doc["uptime"] = millis() / 1000;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNotFound()
{
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

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // Регистрация всех обработчиков
  server.on("/", handleRoot);
  server.on("/unlock", handleUnlock);
  server.on("/lock", handleLock);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  server.on("/data", handleData);

  // Режимы работы
  server.on("/mode_normal", handleModeNormal);
  server.on("/mode_eco", handleModeEco);
  server.on("/mode_sport", handleModeSport);

  // Ограничения скорости
  server.on("/speed_10", handleSpeed10);
  server.on("/speed_15", handleSpeed15);
  server.on("/speed_20", handleSpeed20);
  server.on("/speed_25", handleSpeed25);
  server.on("/speed_30", handleSpeed30);

  // Нормальная скорость
  server.on("/normal_speed_15", handleNormalSpeed15);
  server.on("/normal_speed_20", handleNormalSpeed20);
  server.on("/normal_speed_25", handleNormalSpeed25);

  // Освещение и звук
  server.on("/headlight_toggle", handleHeadlightToggle);
  server.on("/beep_toggle", handleBeepToggle);
  server.on("/beep_total_on", handleBeepTotalOn);
  server.on("/beep_total_off", handleBeepTotalOff);
  server.on("/cruise_toggle", handleCruiseToggle);

  // Двигатель
  server.on("/engine_on", handleEngineOn);
  server.on("/engine_off", handleEngineOff);

  // Подсветка
  server.on("/led_off", handleLedOff);
  server.on("/led_breathing", handleLedBreathing);
  server.on("/led_rainbow", handleLedRainbow);
  server.on("/led_two_color", handleLedTwoColor);
  server.on("/led_strobe", handleLedStrobe);
  server.on("/led_police", handleLedPolice);
  server.on("/led_police2", handleLedPolice2);
  server.on("/led_police3", handleLedPolice3);

  // Системные команды
  server.on("/find_scooter_on", handleFindScooterOn);
  server.on("/reboot", handleReboot);
  server.on("/poweroff", handlePoweroff);

  // Функции аренды
  server.on("/rental_headlight_always_on", handleRentalHeadlightAlwaysOn);
  server.on("/rental_headlight_always_off", handleRentalHeadlightAlwaysOff);
  server.on("/rental_speed_mph", handleRentalSpeedMph);
  server.on("/rental_speed_kmh", handleRentalSpeedKmh);

  // Сброс показателей
  server.on("/reset_single_mileage", handleResetSingleMileage);
  server.on("/reset_single_time", handleResetSingleTime);

  // Bluetooth функции
  server.on("/bt_password_123456", handleBtPassword123456);
  server.on("/reset_bluetooth", handleResetBluetooth);

  // Цвета подсветки
  server.on("/led_color1_blue", handleLedColor1Blue);
  server.on("/led_color2_green", handleLedColor2Green);
  server.on("/led_color3_red", handleLedColor3Red);
  server.on("/led_color4_purple", handleLedColor4Purple);

  // Настройки дисплея
  server.on("/display_units_on", handleDisplayUnitsOn);
  server.on("/display_units_off", handleDisplayUnitsOff);
  server.on("/display_speed_icon_on", handleDisplaySpeedIconOn);
  server.on("/display_speed_icon_off", handleDisplaySpeedIconOff);

  server.onNotFound(handleNotFound);
  server.begin();

  delay(1000);
  sendUnlock();

  Serial.println("Ninebot ES Controller - ПОЛНАЯ ВЕРСИЯ запущена");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop()
{
  handleButton();
  server.handleClient();
  parseIncomingData();

  if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL)
  {
    sendHeartbeat();
    lastHeartbeatTime = millis();
  }
  delay(10);
}