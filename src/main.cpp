#include <Arduino.h>

// Команды протокола
const byte unlock[] = {0x5A, 0xA5, 0x02, 0x3D, 0x20, 0x02, 0x71, 0x01, 0x00, 0x2C, 0xFF};
const byte lock[] = {0x5A, 0xA5, 0x02, 0x3D, 0x20, 0x02, 0x70, 0x01, 0x00, 0x2D, 0xFF};
const byte heartbeat[] = {0x5A, 0xA5, 0x01, 0x3D, 0x20, 0x55, 0x7C, 0x7C, 0x54, 0xFE};

// Настройки кнопки
const int BUTTON_PIN = 5;  // GPIO5 на ESP8266 d1
bool isLocked = true;       // Начальное состояние
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Тайминги
unsigned long lastHeartbeatTime = 0;
const unsigned long HEARTBEAT_INTERVAL = 4000;  // 4 секунды
unsigned long lastActionTime = 0;

// Визуальная обратная связь через встроенный LED
void blinkLED(int times) {
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, LOW);   // LED ON (активный низкий уровень)
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);  // LED OFF
    if (i < times - 1) delay(150);
  }
}





void sendUnlock() {
  Serial.write(unlock, sizeof(unlock));
  isLocked = false;
  lastActionTime = millis();
  blinkLED(2);  // 2 быстрых мигания для разблокировки
}

void sendLock() {
  Serial.write(lock, sizeof(lock));
  isLocked = true;
  lastActionTime = millis();
  blinkLED(1);  // 1 длинное мигание для блокировки
}

void toggleLockState() {
  if (isLocked) {
    sendUnlock();
  } else {
    sendLock();
  }
}

void sendHeartbeat() {
  Serial.write(heartbeat, sizeof(heartbeat));
}

void handleButton() {
  int reading = digitalRead(BUTTON_PIN);
  
  // Если состояние кнопки изменилось
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // Если прошло достаточно времени для устранения дребезга
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Если кнопка нажата (LOW, так как INPUT_PULLUP)
    if (reading == LOW) {
      toggleLockState();
      lastDebounceTime = millis();  // Защита от многократных нажатий
    }
  }
  
  lastButtonState = reading;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Кнопка на замыкание на GND
  delay(1000);
  // Начальная разблокировка при старте
  sendUnlock();
}

void loop() {
  // Обработка кнопки
  handleButton();
  
  // Отправка heartbeat каждые 4 секунды
  if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeatTime = millis();
  }
  
  delay(10);
}





