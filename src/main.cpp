#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Настройки Wi-Fi
const char *ssid = "NinebotESx";
const char *password = "12345678";

ESP8266WebServer server(80);

// Команды протокола
const byte unlock[] = {0x5A, 0xA5, 0x02, 0x3D, 0x20, 0x02, 0x71, 0x01, 0x00, 0x2C, 0xFF};
const byte lock[] = {0x5A, 0xA5, 0x02, 0x3D, 0x20, 0x02, 0x70, 0x01, 0x00, 0x2D, 0xFF};
const byte heartbeat[] = {0x5A, 0xA5, 0x01, 0x3D, 0x20, 0x55, 0x7C, 0x7C, 0x54, 0xFE};

// HTML страница
const char *html_page = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Ninebot Controller</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 20px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
    }
    .container {
      background: white;
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.2);
      max-width: 400px;
      width: 90%;
    }
    h1 {
      color: #333;
      margin-bottom: 30px;
    }
    .button {
      background-color: #4CAF50;
      border: none;
      color: white;
      padding: 15px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 18px;
      margin: 10px;
      cursor: pointer;
      border-radius: 8px;
      width: 200px;
      transition: all 0.3s ease;
      font-weight: bold;
    }
    .button:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(0,0,0,0.2);
    }
    .button:active {
      transform: translateY(0);
    }
    .lock {
      background-color: #f44336;
    }
    .unlock {
      background-color: #4CAF50;
    }
    .toggle {
      background-color: #2196F3;
    }
    .status {
      margin: 20px 0;
      padding: 15px;
      border-radius: 8px;
      font-size: 16px;
      font-weight: bold;
    }
    .locked {
      background-color: #ffebee;
      color: #c62828;
      border: 2px solid #c62828;
    }
    .unlocked {
      background-color: #e8f5e8;
      color: #2e7d32;
      border: 2px solid #2e7d32;
    }
    .info {
      margin-top: 20px;
      padding: 10px;
      background-color: #f5f5f5;
      border-radius: 5px;
      font-size: 14px;
      color: #666;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🚀 Ninebot Controller</h1>
    
    <div class="status" id="status">Загрузка...</div>
    
    <button class="button unlock" onclick="sendCommand('unlock')">🔓 Разблокировать</button>
    <button class="button lock" onclick="sendCommand('lock')">🔒 Заблокировать</button>
    <button class="button toggle" onclick="sendCommand('toggle')">🔄 Переключить</button>
    
    <div class="info">
      <p>IP: 192.168.4.1</p>
      <p>Статус: <span id="connectionStatus">Подключено</span></p>
    </div>
  </div>

  <script>
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

    function sendCommand(cmd) {
      fetch('/' + cmd)
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            updateStatus(data.isLocked);
          } else {
            alert('Ошибка: ' + data.message);
          }
        })
        .catch(error => {
          console.error('Error:', error);
          alert('Ошибка соединения');
        });
    }

    // Запрос статуса при загрузке страницы
    fetch('/status')
      .then(response => response.json())
      .then(data => {
        updateStatus(data.isLocked);
      });

    // Автообновление статуса каждые 3 секунды
    setInterval(() => {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          updateStatus(data.isLocked);
        });
    }, 3000);
  </script>
</body>
</html>
)rawliteral";

// Настройки кнопки
const int BUTTON_PIN = 5; // GPIO5 на ESP8266 d1
bool isLocked = true;     // Начальное состояние
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Тайминги
unsigned long lastHeartbeatTime = 0;
const unsigned long HEARTBEAT_INTERVAL = 4000; // 4 секунды
unsigned long lastActionTime = 0;

// Визуальная обратная связь через встроенный LED
void blinkLED(int times)
{
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < times; i++)
  {
    digitalWrite(LED_BUILTIN, LOW); // LED ON (активный низкий уровень)
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF
    if (i < times - 1)
      delay(150);
  }
}

void sendUnlock()
{
  Serial.write(unlock, sizeof(unlock));
  isLocked = false;
  lastActionTime = millis();
  blinkLED(2); // 2 быстрых мигания для разблокировки
}

void sendLock()
{
  Serial.write(lock, sizeof(lock));
  isLocked = true;
  lastActionTime = millis();
  blinkLED(1); // 1 длинное мигание для блокировки
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

void handleButton()
{
  int reading = digitalRead(BUTTON_PIN);

  // Если состояние кнопки изменилось
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  // Если прошло достаточно времени для устранения дребезга
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    // Если кнопка нажата (LOW, так как INPUT_PULLUP)
    if (reading == LOW)
    {
      toggleLockState();
      lastDebounceTime = millis(); // Защита от многократных нажатий
    }
  }

  lastButtonState = reading;
}

// Обработчики веб-запросов
void handleRoot() {
  server.send(200, "text/html", html_page);
}

void handleUnlock() {
  sendUnlock();
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["isLocked"] = isLocked;
  doc["message"] = "Успешно разблокировано";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleLock() {
  sendLock();
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["isLocked"] = isLocked;
  doc["message"] = "Успешно заблокировано";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleToggle() {
  toggleLockState();
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["isLocked"] = isLocked;
  doc["message"] = isLocked ? "Заблокировано" : "Разблокировано";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStatus() {
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["isLocked"] = isLocked;
  doc["uptime"] = millis() / 1000;
  
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

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Кнопка на замыкание на GND

  // Создаем точку доступа
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // Настраиваем веб-сервер
  server.on("/", handleRoot);
  server.on("/unlock", handleUnlock);
  server.on("/lock", handleLock);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);

  server.begin();

  // Ждем запуска Wi-Fi
  delay(1000);

  // Начальная разблокировка при старте
  sendUnlock();
}

void loop()
{
  // Обработка кнопки
  handleButton();

  // Обрабатываем запросы клиентов
  server.handleClient();

  // Отправка heartbeat каждые 4 секунды
  if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL)
  {
    sendHeartbeat();
    lastHeartbeatTime = millis();
  }

  delay(10);
}
