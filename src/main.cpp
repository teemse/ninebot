#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Настройки WiFi
const char* ssid = "NinebotES_Controller";
const char* password = "12345678";

// Сервер
ESP8266WebServer server(80);

// Буферы для данных
uint8_t rxBuffer[64];
uint8_t txBuffer[64];

// Структура для хранения данных самоката
struct ScooterData {
  // Основные данные
  int16_t bodyTemp;
  int16_t speed;
  int16_t batteryPercent;
  int16_t battery1Percent;
  int16_t battery2Percent;
  int16_t voltage;
  int32_t totalMileage;
  int16_t singleMileage;
  int16_t errorCode;
  int16_t alarmCode;
  bool isLocked;
  bool isActivated;
  uint16_t booleanState;
  
  // Расширенные данные
  int16_t workMode;
  int16_t workSystem;
  int16_t actualMileage;
  int16_t predictedMileage;
  int16_t bat1Temp;
  int16_t bat2Temp;
  int16_t mosTemp;
  int16_t motorCurrent;
  int16_t averageSpeed;
  int16_t bmsVersion;
  int16_t bleVersion;
  
  // Данные подключения
  bool isConnected;
  unsigned long lastResponseTime;
  uint8_t connectionTimeout;
  unsigned long startupTime;
};

ScooterData scooterData;

// Прототипы функций
uint16_t calculateChecksum(uint8_t* data, int len);
bool sendCommand(uint8_t targetID, uint8_t command, uint8_t index, uint8_t* data, uint8_t dataLen);
bool readMemory(uint8_t index, uint8_t length);
bool writeMemory(uint8_t index, uint8_t* data, uint8_t dataLen, bool withResponse = true);
void handleHeartbeat();
void parseResponse(uint8_t* data, uint8_t length);
void updateScooterData();
void checkConnection();
bool pingScooter();
void forceUpdateData();
void updateExtendedData();

// Веб-страница
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Ninebot ES Controller</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 1000px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .section { margin-bottom: 20px; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        .data-row { display: flex; justify-content: space-between; margin: 5px 0; }
        .button { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        .btn-primary { background: #007bff; color: white; }
        .btn-success { background: #28a745; color: white; }
        .btn-warning { background: #ffc107; color: black; }
        .btn-danger { background: #dc3545; color: white; }
        .btn-info { background: #17a2b8; color: white; }
        .status-on { color: green; font-weight: bold; }
        .status-off { color: red; font-weight: bold; }
        .status-connected { color: green; font-weight: bold; }
        .status-disconnected { color: red; font-weight: bold; animation: blink 1s infinite; }
        .tab { overflow: hidden; border: 1px solid #ccc; background-color: #f1f1f1; border-radius: 5px; }
        .tab button { background-color: inherit; float: left; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; }
        .tab button:hover { background-color: #ddd; }
        .tab button.active { background-color: #ccc; }
        .tabcontent { display: none; padding: 6px 12px; border: 1px solid #ccc; border-top: none; border-radius: 0 0 5px 5px; }
        .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        .checkbox-group { display: flex; flex-wrap: wrap; gap: 10px; margin: 10px 0; }
        .checkbox-item { display: flex; align-items: center; gap: 5px; }
        @keyframes blink {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Ninebot ES Controller</h1>
        
        <div class="tab">
            <button class="tablinks active" onclick="openTab(event, 'Status')">Статус</button>
            <button class="tablinks" onclick="openTab(event, 'Control')">Управление</button>
            <button class="tablinks" onclick="openTab(event, 'Advanced')">Расширенные</button>
            <button class="tablinks" onclick="openTab(event, 'Lights')">Подсветка</button>
        </div>

        <!-- Вкладка Статус -->
        <div id="Status" class="tabcontent" style="display: block;">
            <div class="section">
                <h2>Статус подключения</h2>
                <div class="data-row">
                    <span>Подключение к самокату:</span>
                    <span id="connectionStatus" class="status-disconnected">ПРОВЕРКА...</span>
                </div>
                <div class="data-row">
                    <span>Последний ответ:</span>
                    <span id="lastResponse">--</span>
                </div>
                <button class="button btn-primary" onclick="checkConnection()">Проверить подключение</button>
            </div>
            
            <div class="section">
                <h2>Статус системы</h2>
                <div class="grid-2">
                    <div>
                        <div class="data-row">
                            <span>Скорость:</span>
                            <span id="speed">--</span>
                        </div>
                        <div class="data-row">
                            <span>Температура:</span>
                            <span id="temperature">--</span>
                        </div>
                        <div class="data-row">
                            <span>Батарея:</span>
                            <span id="battery">--</span>
                        </div>
                        <div class="data-row">
                            <span>Батарея 1:</span>
                            <span id="battery1">--</span>
                        </div>
                        <div class="data-row">
                            <span>Батарея 2:</span>
                            <span id="battery2">--</span>
                        </div>
                    </div>
                    <div>
                        <div class="data-row">
                            <span>Пробег (общий):</span>
                            <span id="totalMileage">--</span>
                        </div>
                        <div class="data-row">
                            <span>Пробег (поездка):</span>
                            <span id="singleMileage">--</span>
                        </div>
                        <div class="data-row">
                            <span>Статус блокировки:</span>
                            <span id="lockStatus" class="status-off">Заблокирован</span>
                        </div>
                        <div class="data-row">
                            <span>Режим работы:</span>
                            <span id="workMode">--</span>
                        </div>
                        <div class="data-row">
                            <span>Код ошибки:</span>
                            <span id="errorCode">0</span>
                        </div>
                    </div>
                </div>
                <button class="button btn-primary" onclick="forceUpdateData()">Обновить данные</button>
            </div>
        </div>

        <!-- Вкладка Управление -->
        <div id="Control" class="tabcontent">
            <div class="section">
                <h2>Основное управление</h2>
                <button class="button btn-success" onclick="sendCommand('unlock')">Разблокировать</button>
                <button class="button btn-danger" onclick="sendCommand('lock')">Заблокировать</button>
                <button class="button btn-warning" onclick="sendCommand('reboot')">Перезагрузить</button>
                <button class="button btn-danger" onclick="sendCommand('poweroff')">Выключить</button>
                <button class="button btn-info" onclick="sendCommand('engine')">Двигатель ВКЛ</button>
            </div>

            <div class="section">
                <h2>Режимы работы</h2>
                <button class="button btn-primary" onclick="setMode(0)">NORMAL</button>
                <button class="button btn-primary" onclick="setMode(1)">ECO</button>
                <button class="button btn-primary" onclick="setMode(2)">SPORT</button>
            </div>

            <div class="section">
                <h2>Ограничение скорости</h2>
                <input type="range" id="speedLimit" min="10" max="250" value="150" onchange="updateSpeedLimit(this.value)">
                <span id="speedLimitValue">15.0 km/h</span>
                <button class="button btn-primary" onclick="applySpeedLimit()">Применить</button>
            </div>

            <div class="section">
                <h2>Круиз-контроль</h2>
                <button class="button btn-success" onclick="setCruise(1)">Включить</button>
                <button class="button btn-danger" onclick="setCruise(0)">Выключить</button>
            </div>

            <div class="section">
                <h2>Фары и сигналы</h2>
                <button class="button btn-primary" onclick="controlLight(1)">Фары ВКЛ</button>
                <button class="button btn-primary" onclick="controlLight(0)">Фары ВЫКЛ</button>
                <button class="button btn-warning" onclick="findScooter()">Найти самокат</button>
            </div>
        </div>

        <!-- Вкладка Расширенные -->
        <div id="Advanced" class="tabcontent">
            <div class="section">
                <h2>Расширенные данные</h2>
                <div class="grid-2">
                    <div>
                        <div class="data-row">
                            <span>Темп. батареи 1:</span>
                            <span id="bat1Temp">--</span>
                        </div>
                        <div class="data-row">
                            <span>Темп. батареи 2:</span>
                            <span id="bat2Temp">--</span>
                        </div>
                        <div class="data-row">
                            <span>Темп. MOSFET:</span>
                            <span id="mosTemp">--</span>
                        </div>
                        <div class="data-row">
                            <span>Ток мотора:</span>
                            <span id="motorCurrent">--</span>
                        </div>
                        <div class="data-row">
                            <span>Напряжение:</span>
                            <span id="voltage">--</span>
                        </div>
                    </div>
                    <div>
                        <div class="data-row">
                            <span>Средняя скорость:</span>
                            <span id="averageSpeed">--</span>
                        </div>
                        <div class="data-row">
                            <span>Актуальный пробег:</span>
                            <span id="actualMileage">--</span>
                        </div>
                        <div class="data-row">
                            <span>Прогн. пробег:</span>
                            <span id="predictedMileage">--</span>
                        </div>
                        <div class="data-row">
                            <span>Версия BMS:</span>
                            <span id="bmsVersion">--</span>
                        </div>
                        <div class="data-row">
                            <span>Версия BLE:</span>
                            <span id="bleVersion">--</span>
                        </div>
                    </div>
                </div>
                <button class="button btn-primary" onclick="updateExtendedData()">Обновить расширенные данные</button>
            </div>

            <div class="section">
                <h2>Настройки Bluetooth</h2>
                <input type="text" id="btPassword" placeholder="000000" maxlength="6">
                <button class="button btn-primary" onclick="setBtPassword()">Установить пароль</button>
                <div style="font-size: 12px; color: #666; margin-top: 5px;">
                    По умолчанию: 000000. После изменения требуется перезагрузка.
                </div>
            </div>

            <div class="section">
                <h2>Настройки дисплея</h2>
                <div class="checkbox-group">
                    <div class="checkbox-item">
                        <input type="checkbox" id="headlightAlwaysOn" onchange="setDisplaySetting(0x0001, this.checked)">
                        <label>Фары всегда включены</label>
                    </div>
                    <div class="checkbox-item">
                        <input type="checkbox" id="speedInMPH" onchange="setDisplaySetting(0x0040, this.checked)">
                        <label>Скорость в MPH</label>
                    </div>
                    <div class="checkbox-item">
                        <input type="checkbox" id="panelDisplay" onchange="setDisplaySetting(0x0200, this.checked)" checked>
                        <label>Дисплей включен</label>
                    </div>
                    <div class="checkbox-item">
                        <input type="checkbox" id="batteryDisplay" onchange="setDisplaySetting(0x8000, this.checked)" checked>
                        <label>Отображение батареи</label>
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Функции аренды</h2>
                <button class="button btn-warning" onclick="setRentalFunction(0x0001)">Фары всегда ВКЛ</button>
                <button class="button btn-warning" onclick="setRentalFunction(0x0002)">Мигание фар</button>
                <button class="button btn-warning" onclick="setRentalFunction(0x0020)">Без звука при блокировке</button>
            </div>
        </div>

        <!-- Вкладка Подсветка -->
        <div id="Lights" class="tabcontent">
            <div class="section">
                <h2>Режимы подсветки</h2>
                <select id="ledMode" onchange="updateLedMode()">
                    <option value="0">Выключена</option>
                    <option value="1">Один цвет (дыхание)</option>
                    <option value="2">Все цвета (дыхание)</option>
                    <option value="3">Два цвета (раздельно)</option>
                    <option value="4">Все цвета (раздельно)</option>
                    <option value="5">Один цвет (мигание)</option>
                    <option value="6">Все цвета (мигание)</option>
                    <option value="7">Полиция (режим 1)</option>
                    <option value="8">Полиция (режим 2)</option>
                    <option value="9">Полиция (режим 3)</option>
                </select>
                <button class="button btn-primary" onclick="applyLedMode()">Применить режим</button>
            </div>

            <div class="section">
                <h2>Цвета подсветки</h2>
                <div class="grid-2">
                    <div>
                        <h3>Цвет 1 (Синий)</h3>
                        <input type="color" id="color1" value="#0000a0">
                        <button class="button btn-primary" onclick="setLedColor(1)">Установить</button>
                    </div>
                    <div>
                        <h3>Цвет 2 (Зеленый)</h3>
                        <input type="color" id="color2" value="#005000">
                        <button class="button btn-primary" onclick="setLedColor(2)">Установить</button>
                    </div>
                    <div>
                        <h3>Цвет 3 (Красный)</h3>
                        <input type="color" id="color3" value="#a00000">
                        <button class="button btn-primary" onclick="setLedColor(3)">Установить</button>
                    </div>
                    <div>
                        <h3>Цвет 4 (Фиолетовый)</h3>
                        <input type="color" id="color4" value="#5000a0">
                        <button class="button btn-primary" onclick="setLedColor(4)">Установить</button>
                    </div>
                </div>
            </div>

            <div class="section">
                <h2>Быстрые пресеты</h2>
                <button class="button btn-primary" onclick="setLedPreset('blue')">Синяя</button>
                <button class="button btn-success" onclick="setLedPreset('green')">Зеленая</button>
                <button class="button btn-danger" onclick="setLedPreset('red')">Красная</button>
                <button class="button btn-info" onclick="setLedPreset('rainbow')">Радуга</button>
                <button class="button btn-warning" onclick="setLedPreset('police')">Полиция</button>
            </div>
        </div>
    </div>

    <script>
        function openTab(evt, tabName) {
            var i, tabcontent, tablinks;
            tabcontent = document.getElementsByClassName("tabcontent");
            for (i = 0; i < tabcontent.length; i++) {
                tabcontent[i].style.display = "none";
            }
            tablinks = document.getElementsByClassName("tablinks");
            for (i = 0; i < tablinks.length; i++) {
                tablinks[i].className = tablinks[i].className.replace(" active", "");
            }
            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
        }

        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('speed').textContent = (data.speed / 10).toFixed(1) + ' km/h';
                    document.getElementById('temperature').textContent = (data.bodyTemp / 10).toFixed(1) + ' °C';
                    document.getElementById('battery').textContent = data.batteryPercent + '%';
                    document.getElementById('battery1').textContent = data.battery1Percent + '%';
                    document.getElementById('battery2').textContent = data.battery2Percent + '%';
                    document.getElementById('totalMileage').textContent = (data.totalMileage / 1000).toFixed(1) + ' km';
                    document.getElementById('singleMileage').textContent = (data.singleMileage / 100).toFixed(1) + ' km';
                    document.getElementById('lockStatus').textContent = data.isLocked ? 'Заблокирован' : 'Разблокирован';
                    document.getElementById('lockStatus').className = data.isLocked ? 'status-off' : 'status-on';
                    document.getElementById('errorCode').textContent = data.errorCode;
                    document.getElementById('workMode').textContent = getWorkModeName(data.workMode);
                    
                    // Расширенные данные
                    document.getElementById('bat1Temp').textContent = (data.bat1Temp / 10).toFixed(1) + ' °C';
                    document.getElementById('bat2Temp').textContent = (data.bat2Temp / 10).toFixed(1) + ' °C';
                    document.getElementById('mosTemp').textContent = (data.mosTemp / 10).toFixed(1) + ' °C';
                    document.getElementById('motorCurrent').textContent = (data.motorCurrent / 100).toFixed(2) + ' A';
                    document.getElementById('voltage').textContent = (data.voltage / 100).toFixed(1) + ' V';
                    document.getElementById('averageSpeed').textContent = (data.averageSpeed / 10).toFixed(1) + ' km/h';
                    document.getElementById('actualMileage').textContent = (data.actualMileage / 100).toFixed(1) + ' km';
                    document.getElementById('predictedMileage').textContent = (data.predictedMileage / 100).toFixed(1) + ' km';
                    document.getElementById('bmsVersion').textContent = 'v' + (data.bmsVersion >> 8) + '.' + (data.bmsVersion & 0xFF);
                    document.getElementById('bleVersion').textContent = 'v' + (data.bleVersion >> 8) + '.' + (data.bleVersion & 0xFF);
                });
        }

        function getWorkModeName(mode) {
            switch(mode) {
                case 0: return 'NORMAL';
                case 1: return 'ECO';
                case 2: return 'SPORT';
                default: return 'UNKNOWN';
            }
        }

        function updateConnectionStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    const statusElement = document.getElementById('connectionStatus');
                    const lastResponseElement = document.getElementById('lastResponse');
                    
                    if (data.connected) {
                        statusElement.textContent = 'ПОДКЛЮЧЕНО';
                        statusElement.className = 'status-connected';
                    } else {
                        statusElement.textContent = 'НЕТ ПОДКЛЮЧЕНИЯ';
                        statusElement.className = 'status-disconnected';
                    }
                    
                    if (data.lastResponse > 0) {
                        const date = new Date(data.lastResponse);
                        lastResponseElement.textContent = date.toLocaleTimeString();
                    } else {
                        lastResponseElement.textContent = 'никогда';
                    }
                });
        }

        function forceUpdateData() {
            const button = event.target;
            const originalText = button.textContent;
            button.textContent = 'Обновление...';
            button.disabled = true;
            
            fetch('/control?cmd=forceupdate')
                .then(response => response.text())
                .then(result => {
                    updateData();
                    button.textContent = originalText;
                    button.disabled = false;
                })
                .catch(error => {
                    button.textContent = originalText;
                    button.disabled = false;
                });
        }

        function updateExtendedData() {
            fetch('/control?cmd=updateextended')
                .then(response => response.text())
                .then(result => {
                    updateData();
                });
        }

        function checkConnection() {
            fetch('/control?cmd=ping')
                .then(response => response.text())
                .then(result => {
                    updateConnectionStatus();
                });
        }

        function sendCommand(cmd) {
            fetch('/control?cmd=' + cmd)
                .then(response => response.text())
                .then(result => {
                    updateData();
                });
        }

        function setMode(mode) {
            fetch('/control?cmd=mode&value=' + mode)
                .then(response => response.text())
                .then(result => {
                    updateData();
                });
        }

        function updateSpeedLimit(value) {
            document.getElementById('speedLimitValue').textContent = (value / 10).toFixed(1) + ' km/h';
        }

        function applySpeedLimit() {
            const value = document.getElementById('speedLimit').value;
            fetch('/control?cmd=speedlimit&value=' + value)
                .then(response => response.text())
                .then(result => {
                    updateData();
                });
        }

        function setCruise(state) {
            fetch('/control?cmd=cruise&value=' + state)
                .then(response => response.text())
                .then(result => {
                    updateData();
                });
        }

        function controlLight(state) {
            fetch('/control?cmd=light&value=' + state)
                .then(response => response.text())
                .then(result => {
                    updateData();
                });
        }

        function findScooter() {
            fetch('/control?cmd=find')
                .then(response => response.text())
                .then(result => {
                    updateData();
                });
        }

        function setBtPassword() {
            const password = document.getElementById('btPassword').value;
            if (password.length === 6 && !isNaN(password)) {
                fetch('/control?cmd=btpassword&value=' + password)
                    .then(response => response.text())
                    .then(result => {
                        alert('Пароль Bluetooth установлен: ' + result);
                    });
            } else {
                alert('Пароль должен быть 6 цифр!');
            }
        }

        function setDisplaySetting(bitmask, enabled) {
            fetch('/control?cmd=displaysetting&bitmask=' + bitmask + '&enabled=' + (enabled ? '1' : '0'))
                .then(response => response.text())
                .then(result => {
                    console.log('Display setting updated: ' + result);
                });
        }

        function setRentalFunction(functionId) {
            fetch('/control?cmd=rentalfunction&value=' + functionId)
                .then(response => response.text())
                .then(result => {
                    alert('Функция аренды установлена: ' + result);
                });
        }

        function updateLedMode() {
            // Предпросмотр режима
            const mode = document.getElementById('ledMode').value;
            console.log('LED mode selected: ' + mode);
        }

        function applyLedMode() {
            const mode = document.getElementById('ledMode').value;
            fetch('/control?cmd=ledmode&value=' + mode)
                .then(response => response.text())
                .then(result => {
                    alert('Режим подсветки установлен: ' + result);
                });
        }

        function setLedColor(colorNum) {
            const colorInput = document.getElementById('color' + colorNum);
            const color = colorInput.value.substring(1); // Убираем #
            fetch('/control?cmd=ledcolor&colorNum=' + colorNum + '&color=' + color)
                .then(response => response.text())
                .then(result => {
                    alert('Цвет ' + colorNum + ' установлен: ' + result);
                });
        }

        function setLedPreset(preset) {
            fetch('/control?cmd=ledpreset&value=' + preset)
                .then(response => response.text())
                .then(result => {
                    alert('Пресет подсветки установлен: ' + result);
                });
        }

        // Автообновление данных каждые 3 секунды
        setInterval(updateData, 3000);
        // Автообновление статуса подключения каждые 3 секунды
        setInterval(updateConnectionStatus, 3000);
        // Первое обновление при загрузке
        updateData();
        updateConnectionStatus();
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200, SERIAL_8N1);
  Serial.swap();
  
  // Инициализация WiFi
  WiFi.softAP(ssid, password);
  
  // Инициализация данных самоката
  memset(&scooterData, 0, sizeof(scooterData));
  scooterData.startupTime = millis();
  scooterData.isConnected = false;
  
  // Настройка веб-сервера
  server.on("/", []() {
    server.send(200, "text/html", htmlPage);
  });
  
  server.on("/data", []() {
    StaticJsonDocument<1024> doc;
    doc["speed"] = scooterData.speed;
    doc["bodyTemp"] = scooterData.bodyTemp;
    doc["batteryPercent"] = scooterData.batteryPercent;
    doc["battery1Percent"] = scooterData.battery1Percent;
    doc["battery2Percent"] = scooterData.battery2Percent;
    doc["totalMileage"] = scooterData.totalMileage;
    doc["singleMileage"] = scooterData.singleMileage;
    doc["errorCode"] = scooterData.errorCode;
    doc["alarmCode"] = scooterData.alarmCode;
    doc["isLocked"] = scooterData.isLocked;
    doc["isActivated"] = scooterData.isActivated;
    doc["workMode"] = scooterData.workMode;
    doc["workSystem"] = scooterData.workSystem;
    doc["bat1Temp"] = scooterData.bat1Temp;
    doc["bat2Temp"] = scooterData.bat2Temp;
    doc["mosTemp"] = scooterData.mosTemp;
    doc["motorCurrent"] = scooterData.motorCurrent;
    doc["voltage"] = scooterData.voltage;
    doc["averageSpeed"] = scooterData.averageSpeed;
    doc["actualMileage"] = scooterData.actualMileage;
    doc["predictedMileage"] = scooterData.predictedMileage;
    doc["bmsVersion"] = scooterData.bmsVersion;
    doc["bleVersion"] = scooterData.bleVersion;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  server.on("/status", []() {
    StaticJsonDocument<256> doc;
    doc["connected"] = scooterData.isConnected;
    doc["lastResponse"] = scooterData.lastResponseTime;
    doc["timeoutCount"] = scooterData.connectionTimeout;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  server.on("/control", []() {
    String command = server.arg("cmd");
    String value = server.arg("value");
    String bitmask = server.arg("bitmask");
    String enabled = server.arg("enabled");
    String colorNum = server.arg("colorNum");
    String color = server.arg("color");
    String result = "OK";
    
    if (command == "lock") {
      uint8_t data[] = {0x01, 0x00};
      writeMemory(0x70, data, 2);
    } else if (command == "unlock") {
      uint8_t data[] = {0x01, 0x00};
      writeMemory(0x71, data, 2);
    } else if (command == "reboot") {
      uint8_t data[] = {0x01, 0x00};
      writeMemory(0x78, data, 2);
    } else if (command == "poweroff") {
      uint8_t data[] = {0x01, 0x00};
      writeMemory(0x79, data, 2);
    } else if (command == "engine") {
      uint8_t data[] = {0x01, 0x00};
      writeMemory(0x77, data, 2);
    } else if (command == "mode") {
      uint8_t modeValue = value.toInt();
      uint8_t data[] = {modeValue, 0x00};
      writeMemory(0x75, data, 2);
    } else if (command == "speedlimit") {
      uint16_t speedValue = value.toInt();
      uint8_t data[] = {speedValue & 0xFF, (speedValue >> 8) & 0xFF};
      writeMemory(0x74, data, 2);
    } else if (command == "cruise") {
      uint8_t cruiseValue = value.toInt();
      uint8_t data[] = {cruiseValue, 0x00};
      writeMemory(0x7C, data, 2);
    } else if (command == "light") {
      uint8_t lightValue = value.toInt();
      uint8_t data[] = {lightValue, 0x00};
      writeMemory(0x90, data, 2);
    } else if (command == "find") {
      uint8_t data[] = {0x01, 0x00};
      writeMemory(0x7E, data, 2);
    } else if (command == "ping") {
      readMemory(0x3E, 2);
      result = scooterData.isConnected ? "Connected" : "Disconnected";
    } else if (command == "forceupdate") {
      forceUpdateData();
      result = "Data updated";
    } else if (command == "updateextended") {
      updateExtendedData();
      result = "Extended data updated";
    } else if (command == "btpassword") {
      // Установка пароля Bluetooth (0x17-0x19)
      if (value.length() == 6) {
        uint8_t data[6];
        for (int i = 0; i < 6; i++) {
          data[i] = value.charAt(i) - '0';
        }
        writeMemory(0x17, data, 6);
        result = "Bluetooth password set";
      }
    } else if (command == "displaysetting") {
      // Настройки дисплея (0x7D, 0x80, 0x81)
      uint16_t mask = bitmask.toInt();
      uint8_t enabledVal = enabled.toInt();
      // Здесь нужно прочитать текущее значение, изменить бит и записать обратно
      result = "Display setting updated";
    } else if (command == "rentalfunction") {
      // Функции аренды (0x80-0x81)
      uint16_t functionId = value.toInt();
      uint8_t data[] = {functionId & 0xFF, (functionId >> 8) & 0xFF};
      writeMemory(0x80, data, 2);
      result = "Rental function set";
    } else if (command == "ledmode") {
      // Режим LED ленты (0xC6)
      uint8_t mode = value.toInt();
      uint8_t data[] = {mode, 0x00};
      writeMemory(0xC6, data, 2);
      result = "LED mode set";
    } else if (command == "ledcolor") {
      // Цвета LED ленты (0xC8, 0xCA, 0xCC, 0xCE)
      uint8_t colorIndex = colorNum.toInt();
      uint32_t colorValue = strtoul(color.c_str(), NULL, 16);
      uint8_t data[] = {colorValue & 0xFF, (colorValue >> 8) & 0xFF};
      
      uint8_t address = 0xC8 + (colorIndex - 1) * 2;
      writeMemory(address, data, 2);
      result = "LED color " + colorNum + " set";
    } else if (command == "ledpreset") {
      // Быстрые пресеты подсветки
      if (value == "blue") {
        uint8_t data[] = {1, 0x00}; // Режим один цвет
        writeMemory(0xC6, data, 2);
        result = "Blue preset applied";
      } else if (value == "police") {
        uint8_t data[] = {7, 0x00}; // Режим полиция
        writeMemory(0xC6, data, 2);
        result = "Police preset applied";
      }
    }
    
    server.send(200, "text/plain", result);
  });
  
  server.begin();
  
  delay(1000);
}

void loop() {
  server.handleClient();
  
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastDataUpdate = 0;
  static unsigned long lastPing = 0;
  static unsigned long lastExtendedUpdate = 0;
  
  // Отправка heartbeat каждые 4 секунды
  if (millis() - lastHeartbeat > 4000) {
    handleHeartbeat();
    lastHeartbeat = millis();
  }
  
  // Обновление основных данных каждые 2 секунды
  if (millis() - lastDataUpdate > 2000) {
    updateScooterData();
    lastDataUpdate = millis();
  }
  
  // Обновление расширенных данных каждые 10 секунд
  if (millis() - lastExtendedUpdate > 10000) {
    updateExtendedData();
    lastExtendedUpdate = millis();
  }
  
  // Проверка подключения каждые 5 секунд
  checkConnection();
  
  // Тестовый пинг каждые 30 секунд
  if (millis() - lastPing > 30000) {
    pingScooter();
    lastPing = millis();
  }
  
  // Обработка входящих данных
  if (Serial.available()) {
    static uint8_t buffer[64];
    static int bufferIndex = 0;
    
    while (Serial.available()) {
      uint8_t byte = Serial.read();
      
      if (bufferIndex == 0 && byte == 0x5A) {
        buffer[bufferIndex++] = byte;
      } else if (bufferIndex == 1 && byte == 0xA5) {
        buffer[bufferIndex++] = byte;
      } else if (bufferIndex >= 2) {
        buffer[bufferIndex++] = byte;
        
        if (bufferIndex >= 3) {
          uint8_t packetLength = buffer[2];
          
          if (bufferIndex >= packetLength + 7) {
            parseResponse(buffer, bufferIndex);
            bufferIndex = 0;
          }
        }
        
        if (bufferIndex >= sizeof(buffer)) {
          bufferIndex = 0;
        }
      } else {
        bufferIndex = 0;
      }
    }
  }
}

uint16_t calculateChecksum(uint8_t* data, int len) {
  uint16_t checksum = 0;
  for (int i = 0; i < len; i++) {
    checksum += data[i];
  }
  return ~checksum;
}

bool sendCommand(uint8_t targetID, uint8_t command, uint8_t index, uint8_t* data, uint8_t dataLen) {
  uint8_t packet[64];
  int packetIndex = 0;
  
  packet[packetIndex++] = 0x5A;
  packet[packetIndex++] = 0xA5;
  packet[packetIndex++] = dataLen;
  packet[packetIndex++] = 0x3E;
  packet[packetIndex++] = targetID;
  packet[packetIndex++] = command;
  packet[packetIndex++] = index;
  
  for (int i = 0; i < dataLen; i++) {
    packet[packetIndex++] = data[i];
  }
  
  uint16_t checksum = calculateChecksum(&packet[2], packetIndex - 2);
  packet[packetIndex++] = checksum & 0xFF;
  packet[packetIndex++] = (checksum >> 8) & 0xFF;
  
  Serial.write(packet, packetIndex);
  Serial.flush();
  
  return true;
}

bool readMemory(uint8_t index, uint8_t length) {
  uint8_t data[] = {length};
  return sendCommand(0x20, 0x01, index, data, 1);
}

bool writeMemory(uint8_t index, uint8_t* data, uint8_t dataLen, bool withResponse) {
  uint8_t command = withResponse ? 0x02 : 0x03;
  return sendCommand(0x20, command, index, data, dataLen);
}

void handleHeartbeat() {
  uint8_t data[] = {0x7C};
  writeMemory(0x7C, data, 1, false);
}

void parseResponse(uint8_t* data, uint8_t length) {
  uint16_t receivedChecksum = (data[length-1] << 8) | data[length-2];
  uint16_t calculatedChecksum = calculateChecksum(&data[2], length - 4);
  
  if (receivedChecksum != calculatedChecksum) {
    return;
  }
  
  // Обновляем время последнего ответа
  scooterData.lastResponseTime = millis();
  scooterData.isConnected = true;
  
  uint8_t command = data[5];
  uint8_t index = data[6];
  
  if (command == 0x04) {
    uint8_t dataLength = data[2];
    
    switch (index) {
      case 0x3E: // Температура самоката
        if (dataLength >= 2) scooterData.bodyTemp = (data[8] << 8) | data[7];
        break;
      case 0x26: // Скорость
        if (dataLength >= 2) scooterData.speed = (data[8] << 8) | data[7];
        break;
      case 0x22: // Процент батареи
        if (dataLength >= 2) scooterData.batteryPercent = (data[8] << 8) | data[7];
        break;
      case 0x1D: // Boolean состояние
        if (dataLength >= 2) {
          scooterData.booleanState = (data[8] << 8) | data[7];
          scooterData.isLocked = (scooterData.booleanState & 0x0002) != 0;
          scooterData.isActivated = (scooterData.booleanState & 0x0800) != 0;
        }
        break;
      case 0xB0: // Код ошибки
        if (dataLength >= 2) scooterData.errorCode = (data[8] << 8) | data[7];
        break;
      case 0x29: // Общий пробег (младшие 16 бит)
        if (dataLength >= 2) scooterData.totalMileage = (scooterData.totalMileage & 0xFFFF0000) | ((data[8] << 8) | data[7]);
        break;
      case 0x2A: // Общий пробег (старшие 16 бит)
        if (dataLength >= 2) scooterData.totalMileage = (scooterData.totalMileage & 0xFFFF) | (((data[8] << 8) | data[7]) << 16);
        break;
      case 0x2F: // Пробег поездки
        if (dataLength >= 2) scooterData.singleMileage = (data[8] << 8) | data[7];
        break;
      case 0xB3: // Батарея 1 и 2
        if (dataLength >= 2) {
          uint16_t batData = (data[8] << 8) | data[7];
          scooterData.battery1Percent = batData & 0xFF;
          scooterData.battery2Percent = (batData >> 8) & 0xFF;
        }
        break;
      case 0x1F: // Режим работы
        if (dataLength >= 2) scooterData.workMode = (data[8] << 8) | data[7];
        break;
      case 0x3F: // Температура батареи 1
        if (dataLength >= 2) scooterData.bat1Temp = (data[8] << 8) | data[7];
        break;
      case 0x40: // Температура батареи 2
        if (dataLength >= 2) scooterData.bat2Temp = (data[8] << 8) | data[7];
        break;
      case 0x41: // Температура MOSFET
        if (dataLength >= 2) scooterData.mosTemp = (data[8] << 8) | data[7];
        break;
      case 0x47: // Напряжение
        if (dataLength >= 2) scooterData.voltage = (data[8] << 8) | data[7];
        break;
      case 0x53: // Ток мотора
        if (dataLength >= 2) scooterData.motorCurrent = (data[8] << 8) | data[7];
        break;
      case 0x65: // Средняя скорость
        if (dataLength >= 2) scooterData.averageSpeed = (data[8] << 8) | data[7];
        break;
      case 0x66: // Версия BMS
        if (dataLength >= 2) scooterData.bmsVersion = (data[8] << 8) | data[7];
        break;
      case 0x68: // Версия BLE
        if (dataLength >= 2) scooterData.bleVersion = (data[8] << 8) | data[7];
        break;
      case 0x24: // Актуальный пробег
        if (dataLength >= 2) scooterData.actualMileage = (data[8] << 8) | data[7];
        break;
      case 0x25: // Прогнозируемый пробег
        if (dataLength >= 2) scooterData.predictedMileage = (data[8] << 8) | data[7];
        break;
    }
  }
}

void updateScooterData() {
  readMemory(0x3E, 2);  // Температура
  readMemory(0x26, 2);  // Скорость
  readMemory(0x22, 2);  // Процент батареи
  readMemory(0x1D, 2);  // Boolean состояние
  readMemory(0xB0, 2);  // Код ошибки
  readMemory(0xB3, 2);  // Батарея 1 и 2
  readMemory(0x1F, 2);  // Режим работы
}

void updateExtendedData() {
  readMemory(0x3F, 2);  // Температура батареи 1
  readMemory(0x40, 2);  // Температура батареи 2
  readMemory(0x41, 2);  // Температура MOSFET
  readMemory(0x47, 2);  // Напряжение
  readMemory(0x53, 2);  // Ток мотора
  readMemory(0x65, 2);  // Средняя скорость
  readMemory(0x66, 2);  // Версия BMS
  readMemory(0x68, 2);  // Версия BLE
  readMemory(0x24, 2);  // Актуальный пробег
  readMemory(0x25, 2);  // Прогнозируемый пробег
  readMemory(0x29, 2);  // Общий пробег (младшие)
  readMemory(0x2A, 2);  // Общий пробег (старшие)
}

void forceUpdateData() {
  updateScooterData();
  updateExtendedData();
}

void checkConnection() {
  static unsigned long lastCheck = 0;
  
  if (millis() - lastCheck > 5000) {
    if (millis() - scooterData.startupTime > 10000) {
      if (millis() - scooterData.lastResponseTime > 15000) {
        scooterData.isConnected = false;
        scooterData.connectionTimeout++;
        
        if (scooterData.connectionTimeout > 2) {
          readMemory(0x3E, 2);
        }
      } else {
        scooterData.isConnected = true;
        scooterData.connectionTimeout = 0;
      }
    }
    
    lastCheck = millis();
  }
}

bool pingScooter() {
  readMemory(0x3E, 2);
  return true;
}