#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "registers.h"
#include <LittleFS.h>



// Режим работы Wi-Fi
IPAddress localIP;
bool wifiConnected = false;

// Переменные для управления WiFi
unsigned long lastWiFiCheck = 0;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

// ============================================================================
// СТРУКТУРЫ И КОНСТАНТЫ
// ============================================================================

struct NinebotCommand {
    std::vector<uint8_t> data;
    String description;
};


// ============================================================================
// ПЕРЕМЕННЫЕ
// ============================================================================

bool isLocked = true;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastHeartbeatTime = 0;


// Данные самоката
// Основные данные

String btPassword = "000000";
int fwVersion = 0;
int errorCode = 0;
int scooterBattery = 0;
int scooterTemperature = 0;
int scooterErrorCode = 0;
int speedLimit = 0;
bool beepState = true;
String scooterSerial = "";
int scooterPower = 0;
int alarmCode = 0;
uint16_t boolStatus = 0;
int workSystem = 1;
int workMode = 0;
int battery1Capacity = 0;
int battery2Capacity = 0;
int batteryTotal = 0;
int actualRange = 0;
int predictedRange = 0;
int scooterSpeed = 0;
unsigned long totalMileage = 0;
int singleMileage = 0;
unsigned long totalOperationTime = 0;
unsigned long totalRideTime = 0;
int singleOperationTime = 0;
int singleRideTime = 0;
int bodyTemperature = 0;
int battery1Temp = 0;
int battery2Temp = 0;
int mosTemp = 0;
float driveVoltage = 0;
int bat2Temp2 = 0;
float motorCurrent = 0;
int avgSpeed = 0;
String bms2Version = "";
String bmsVersion = "";
String bleVersion = "";

// Данные управления
int normalSpeedLimit = 250;
int speedLimitMode = 60;
bool engineState = true;
bool cruiseControl = false;
uint16_t funBoolSettings = 0;
uint16_t funBool1Settings = 0;
uint16_t funBool2Settings = 0;
bool headlightState = true;
bool beepAlarmState = true;
bool beepTotalState = true;

// Быстрые данные
int quickError = 0;
int quickAlarm = 0;
uint16_t quickBoolStatus = 0;
int quickBatBoth = 0;
int quickBattery = 0;
int quickSpeed = 0;
int quickAvgSpeed = 0;
unsigned long quickMileage = 0;
int quickSingleMileage = 0;
int quickSingleTime = 0;
int quickBodyTemp = 0;
int quickCurrentLimit = 0;
int quickPower = 0;
int quickAlarmDelay = 0;
int quickPredictRange = 0;

// Подсветка
int ledMode = 1;
uint16_t ledColor1 = 0xA0F0; // Синий
uint16_t ledColor2 = 0x50F0; // Зеленый
uint16_t ledColor3 = 0x00F0; // Красный
uint16_t ledColor4 = 0xC8F0; // Фиолетовый

// CPU ID
uint16_t cpuIdA = 0;
uint16_t cpuIdB = 0;
uint16_t cpuIdC = 0;
uint16_t cpuIdD = 0;
uint16_t cpuIdE = 0;
uint16_t cpuIdF = 0;

// ============================================================================
// ФУНКЦИИ УПРАВЛЕНИЯ WIFI
// ============================================================================

void setupWiFiAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    localIP = WiFi.softAPIP();
    wifiConnected = true;
    wifiStationMode = false;
}

void setupWiFiSTA() {
    if (strlen(WIFI_SSID_STA) == 0) {
        setupWiFiAP();
        return;
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID_STA, WIFI_PASSWORD_STA);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        localIP = WiFi.localIP();
        wifiConnected = true;
        wifiStationMode = true;
    } else {
        setupWiFiAP();
    }
}

void switchToAPMode() {
    if (wifiStationMode) {
        WiFi.disconnect();
        delay(100);
        setupWiFiAP();
    }
}

void switchToSTAMode() {
    if (!wifiStationMode) {
        setupWiFiSTA();
    }
}

bool checkWiFiConnection() {
    if (wifiStationMode) {
        if (WiFi.status() != WL_CONNECTED) {
            wifiConnected = false;
            return false;
        }
        return true;
    }
    return wifiConnected; // Для AP режима всегда true
}

void handleWiFiToggle() {
    if (wifiStationMode) {
        switchToAPMode();
    } else {
        switchToSTAMode();
    }
}

void handleWiFiStatus() {
    DynamicJsonDocument doc(300);
    doc["success"] = true;
    doc["mode"] = wifiStationMode ? "STA (Клиент)" : "AP (Точка доступа)";
    doc["connected"] = wifiConnected;
    doc["ip"] = localIP.toString();
    doc["ssid"] = wifiStationMode ? WIFI_SSID_STA : ssid;
    
    if (wifiStationMode && WiFi.status() == WL_CONNECTED) {
        doc["signalStrength"] = WiFi.RSSI();
        doc["channel"] = WiFi.channel();
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

String getValueType(int value) {
    if (value == 0 || value == 1) {
        return "BOOL";
    } else if (value >= 0 && value <= 100) {
        return "PERCENT";
    } else if (value >= 100 && value <= 300) {
        return "SPEED_X10";
    } else if (value >= 1000 && value <= 30000) {
        return "DISTANCE";
    } else if (value >= 200 && value <= 500) {
        return "VOLTAGE_X100";
    } else if (value >= -20 && value <= 50) {
        return "TEMPERATURE";
    } else if ((value & 0xFF) == value) {
        return "BYTE";
    } else if (value >= 0x0000 && value <= 0xFFFF) {
        return "UINT16";
    } else {
        return "UNKNOWN";
    }
}

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
    std::vector<uint8_t> dataSegment;
    
    if (commandType == CMD_CMAP_RD) {
        // Для команды чтения dataValue - это длина чтения
        dataLength = 1;
        dataSegment.push_back(dataValue & 0xFF);
    } else if (commandType == CMD_CMAP_WR || commandType == CMD_CMAP_WR_NR) {
        // Для команды записи dataValue - это данные для записи
        dataLength = 2;
        dataSegment.push_back(dataValue & 0xFF);        // младший байт
        dataSegment.push_back((dataValue >> 8) & 0xFF); // старший байт
    } else if (commandType == CMD_HEARTBEAT) {
        // Heartbeat имеет специальный формат
        dataLength = 2;
        dataSegment.push_back(0x7C); // данные heartbeat
        dataSegment.push_back(0x00); // второй байт
    }
    
    int totalLength = 9 + dataLength;
    cmd.data.resize(totalLength);
    
    // Заголовок пакета
    cmd.data[0] = 0x5A;
    cmd.data[1] = 0xA5;
    cmd.data[2] = dataLength;
    cmd.data[3] = 0x3D;
    cmd.data[4] = 0x20;
    cmd.data[5] = commandType;
    cmd.data[6] = dataIndex;
    
    // Данные
    for (int i = 0; i < dataLength; i++) {
        cmd.data[7 + i] = dataSegment[i];
    }
    
    // Расчет контрольной суммы
    uint16_t crc = calculateChecksum(cmd.data.data() + 2, dataLength + 5);
    cmd.data[7 + dataLength] = crc & 0xFF;
    cmd.data[8 + dataLength] = (crc >> 8) & 0xFF;
    
    return cmd;
}

// ============================================================================
// ФУНКЦИИ ЧТЕНИЯ ДАННЫХ С САМОКАТА
// ============================================================================

// Анализ булевых статусов
String getBoolStatusString() {
    String status = "";
    if (boolStatus & 0x0001) status += "Ограничение скорости, ";
    if (boolStatus & 0x0002) status += "Заблокирован, ";
    if (boolStatus & 0x0004) status += "Звуковой сигнал, ";
    if (boolStatus & 0x0200) status += "Батарея 2 подключена, ";
    if (boolStatus & 0x0800) status += "Активирован, ";
    return status;
}

bool testWriteWithResponse(uint8_t index, uint16_t testValue) {
    // Очищаем буфер serial
    while (Serial.available() > 0) Serial.read();
    
    // Отправляем команду записи С ОТВЕТОМ
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, index, testValue, "Test write with response");
    Serial.write(cmd.data.data(), cmd.data.size());
    Serial.flush();
    
    // Ждем ответа (максимум 500мс)
    unsigned long startTime = millis();
    while (Serial.available() < 10 && millis() - startTime < 500) {
        delay(10);
    }
    
    // Проверяем ответ
    if (Serial.available() >= 10) {
        uint8_t response[20];
        int bytesRead = Serial.readBytes(response, min(Serial.available(), 20));
        
        // Проверяем что это ответ на запись (CMD_CMAP_ACK_WR)
        if (response[0] == 0x5A && response[1] == 0xA5 && response[5] == CMD_CMAP_ACK_WR) {
            uint8_t responseIndex = response[6];
            uint8_t successFlag = (bytesRead > 7) ? response[7] : 0;
            
            return (successFlag == 0x01); // 0x01 = успешная запись
        }
    }
    
    return false;
}

// Анализ кодов тревоги
String getAlarmString() {
    switch(alarmCode) {
        case 9: return "Толкают в заблокированном режиме";
        case 12: return "Высокое напряжение при торможении";
        default: return (alarmCode == 0) ? "Нет тревог" : "Тревога: " + String(alarmCode);
    }
}

int readScooterData(uint8_t index, uint8_t readLength = 2) {
    // Очищаем буфер serial
    while (Serial.available() > 0) Serial.read();
    
    // Отправляем команду чтения
    NinebotCommand cmd = createCommand(CMD_CMAP_RD, index, readLength, "Read data");
    Serial.write(cmd.data.data(), cmd.data.size());
    Serial.flush();
    
    // Ждем ответа с таймаутом
    unsigned long startTime = millis();
    while (Serial.available() < 10 && millis() - startTime < 500) {
        delay(10);
    }
    
    if (Serial.available() >= 10) {
        uint8_t response[20];
        int bytesRead = Serial.readBytes(response, min(Serial.available(), 20));
        
        // Проверяем заголовок и команду
        if (response[0] == 0x5A && response[1] == 0xA5 && response[5] == CMD_CMAP_ACK_RD) {
            uint8_t dataLen = response[2];
            if (dataLen >= 2 && bytesRead >= 9) {
                // Извлекаем данные (little-endian)
                int16_t value = (response[8] << 8) | response[7];
                return value;
            }
        }
    }
    return -1; // Ошибка чтения
}

// Функция для чтения строковых данных (серийный номер и т.д.)
String readStringData(uint8_t index, uint8_t length) {
    String result = "";
    for (int i = 0; i < length; i += 2) {
        int16_t value = readScooterData(index + i/2);
        if (value != -1) {
            result += char(value & 0xFF);
            if ((value >> 8) != 0) {
                result += char(value >> 8);
            }
        }
    }
    return result;
}

// Функция для чтения многобайтовых значений
uint32_t readLongData(uint8_t indexLow, uint8_t indexHigh) {
    int16_t low = readScooterData(indexLow);
    int16_t high = readScooterData(indexHigh);
    if (low != -1 && high != -1) {
        return ((uint32_t)high << 16) | (uint16_t)low;
    }
    return 0;
}

// Чтение версии прошивки в строковом формате
String readFirmwareVersion(uint8_t index) {
    int16_t versionRaw = readScooterData(index);
    if (versionRaw != -1) {
        uint8_t pcbVersion = (versionRaw >> 12) & 0x0F;
        uint8_t major = (versionRaw >> 8) & 0x0F;
        uint8_t minor = (versionRaw >> 4) & 0x0F;
        uint8_t patch = versionRaw & 0x0F;
        return String(major) + "." + String(minor) + "." + String(patch);
    }
    return "N/A";
}

void updateAllScooterData() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 3000) return; // Каждые 3 секунды
    lastUpdate = millis();

    // Основные данные
    scooterSpeed = readScooterData(INDEX_SPEED) / 10;
    batteryTotal = readScooterData(INDEX_BATTERY);
    bodyTemperature = readScooterData(INDEX_BODY_TEMP);
    totalMileage = readLongData(INDEX_MILEAGE_L, INDEX_MILEAGE_H) / 1000;
    errorCode = readScooterData(INDEX_ERROR_CODE);
    workMode = readScooterData(INDEX_WORK_MODE);
    
    // Батареи и пробег
    battery1Capacity = readScooterData(INDEX_BATTERY1_CAP);
    battery2Capacity = readScooterData(INDEX_BATTERY2_CAP);
    actualRange = readScooterData(INDEX_ACTUAL_RANGE) / 100;
    predictedRange = readScooterData(INDEX_PREDICT_RANGE) / 100;
    singleMileage = readScooterData(INDEX_SINGLE_MILEAGE) / 100;
    
    // Время
    totalOperationTime = readLongData(INDEX_TOTAL_TIME_L, INDEX_TOTAL_TIME_H) / 3600;
    totalRideTime = readLongData(INDEX_RIDE_TIME_L, INDEX_RIDE_TIME_H) / 3600;
    singleRideTime = readScooterData(INDEX_SINGLE_RIDE_TIME) / 60;
    
    // Температуры
    battery1Temp = readScooterData(INDEX_BAT1_TEMP);
    battery2Temp = readScooterData(INDEX_BAT2_TEMP);
    mosTemp = readScooterData(INDEX_MOS_TEMP);
    scooterTemperature = readScooterData(INDEX_MOS_TEMP);
    
    // Электрические параметры
    driveVoltage = readScooterData(INDEX_DRIVE_VOLTAGE) / 100.0;
    motorCurrent = readScooterData(INDEX_MOTOR_CURRENT) / 100.0;
    avgSpeed = readScooterData(INDEX_AVG_SPEED) / 10;
    
    // Быстрые данные
    quickSpeed = readScooterData(INDEX_QUICK_SPEED) / 10;
    quickBattery = readScooterData(INDEX_QUICK_BATTERY);
    quickPower = readScooterData(INDEX_QUICK_POWER);
    quickBodyTemp = readScooterData(INDEX_QUICK_BODY_TEMP);
    
    // Статусы
    boolStatus = readScooterData(INDEX_BOOL_STATUS);
    alarmCode = readScooterData(INDEX_ALARM_CODE);
    quickBoolStatus = readScooterData(INDEX_QUICK_BOOL);
    
    // Строковые данные (читаем реже)
    static unsigned long lastStringRead = 0;
    if (millis() - lastStringRead > 15000) {
        scooterSerial = readStringData(INDEX_SERIAL_NUMBER, 14);
        bmsVersion = readFirmwareVersion(INDEX_BMS_VERSION);
        bms2Version = readFirmwareVersion(INDEX_BMS2_VERSION);
        bleVersion = readFirmwareVersion(INDEX_BLE_VERSION);
        lastStringRead = millis();
    }
}

String getBoolStatusDetails() {
    String status = "";
    if (boolStatus & 0x0001) status += "Ограничение скорости, ";
    if (boolStatus & 0x0002) status += "Заблокирован, ";
    if (boolStatus & 0x0004) status += "Звуковой сигнал, ";
    if (boolStatus & 0x0200) status += "Батарея 2 подключена, ";
    if (boolStatus & 0x0800) status += "Активирован, ";
    return status;
}

String getAlarmDetails() {
    switch(alarmCode) {
        case 9: return "Толкают в заблокированном режиме";
        case 12: return "Высокое напряжение при торможении";
        default: return (alarmCode == 0) ? "Нет тревог" : "Тревога: " + String(alarmCode);
    }
}

String getLedModeName(uint8_t mode) {
    switch(mode) {
        case 0: return "Выключено";
        case 1: return "Одноцветное дыхание";
        case 2: return "Всецветное дыхание";
        case 3: return "Два цвета раздельно";
        case 4: return "Все цвета раздельно";
        case 5: return "Одноцветное мерцание";
        case 6: return "Всецветное мерцание";
        case 7: return "Полиция 1";
        case 8: return "Полиция 2";
        case 9: return "Полиция 3";
        default: return "Неизвестно";
    }
}

// ============================================================================
// ПОЛНЫЙ НАБОР ФУНКЦИЙ УПРАВЛЕНИЯ
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

void sendHeartbeat() {
    NinebotCommand cmd = createCommand(CMD_HEARTBEAT, INDEX_CRUISE, 0x007C, "Heartbeat");
    Serial.write(cmd.data.data(), cmd.data.size());
}

void setSpeedLimit(uint16_t limit) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_SPEED_LIMIT, limit, "Лимит скорости");
    Serial.write(cmd.data.data(), cmd.data.size());
    speedLimit = limit;
}

void setBeep(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_BEEP_TOTAL, enabled ? 0x0001 : 0x0000, "Звук");
    Serial.write(cmd.data.data(), cmd.data.size());
    beepState = enabled;
}

void toggleBeep() {
    beepState = !beepState;
    setBeep(beepState);
}

// Основные функции управления
void sendLock() {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_LOCK, 0x0001, "Блокировка");
    Serial.write(cmd.data.data(), cmd.data.size());
    isLocked = true;
}

void sendUnlock() {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_UNLOCK, 0x0001, "Разблокировка");
    Serial.write(cmd.data.data(), cmd.data.size());
    isLocked = false;
}

void sendOpenDeck() {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_DECK_LOCK, 0x0001, "Открыть деку");
    Serial.write(cmd.data.data(), cmd.data.size());
}

void toggleLockState() {
    if (isLocked) {
        sendUnlock();
    } else {
        sendLock();
    }
}

void toggleSpeedLimit() {
    // Переключение ограничения скорости
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_LIMIT_SPD, 0x0001, "Переключение лимита скорости");
    Serial.write(cmd.data.data(), cmd.data.size());
}

void setNormalSpeedLimit(uint16_t limit) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_NORMAL_SPEED, limit, "Лимит Normal режима");
    Serial.write(cmd.data.data(), cmd.data.size());
    normalSpeedLimit = limit;
}

void setSpeedLimitMode(uint16_t limit) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_SPEED_LIMIT, limit, "Лимит Limit режима");
    Serial.write(cmd.data.data(), cmd.data.size());
    speedLimitMode = limit;
}

void setWorkMode(uint8_t mode) {
    if (mode <= 2) {
        NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_WORK_MODE_CTL, mode, "Режим работы");
        Serial.write(cmd.data.data(), cmd.data.size());
        workMode = mode;
    }
}

void setEngineState(bool state) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_ENGINE, state ? 0x0001 : 0x0000, "Двигатель");
    Serial.write(cmd.data.data(), cmd.data.size());
    engineState = state;
}

void rebootSystem() {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_REBOOT, 0x0001, "Перезагрузка");
    Serial.write(cmd.data.data(), cmd.data.size());
}

void powerOff() {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_POWER_OFF, 0x0001, "Выключение");
    Serial.write(cmd.data.data(), cmd.data.size());
}

void setCruiseControl(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_CRUISE, enabled ? 0x0001 : 0x0000, "Круиз-контроль");
    Serial.write(cmd.data.data(), cmd.data.size());
    cruiseControl = enabled;
}

void toggleCruiseControl() {
    cruiseControl = !cruiseControl;
    setCruiseControl(cruiseControl);
}

// Функции для проката
void findScooter() {
    // Включение мигания фар и звука для поиска
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_FIND_SCOOTER, 0x0001, "Поиск самоката");
    Serial.write(cmd.data.data(), cmd.data.size());
}

void setHeadlight(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_HEADLIGHT, enabled ? 0x0001 : 0x0000, "Фары");
    Serial.write(cmd.data.data(), cmd.data.size());
    headlightState = enabled;
}

void toggleHeadlight() {
    headlightState = !headlightState;
    setHeadlight(headlightState);
}

void setBeepAlarm(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_BEEP_ALARM, enabled ? 0x0001 : 0x0000, "Звук сигнала");
    Serial.write(cmd.data.data(), cmd.data.size());
    beepAlarmState = enabled;
}

void setBeepTotal(bool enabled) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_BEEP_TOTAL, enabled ? 0x0001 : 0x0000, "Общий звук");
    Serial.write(cmd.data.data(), cmd.data.size());
    beepTotalState = enabled;
}

// Управление подсветкой
void setLedMode(uint8_t mode) {
    if (mode <= 9) {
        NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_LED_MODE, mode, "Режим подсветки");
        Serial.write(cmd.data.data(), cmd.data.size());
        ledMode = mode;
    }
}

void setLedColor(uint8_t colorIndex, uint16_t color) {
    uint8_t index = 0;
    switch(colorIndex) {
        case 1: index = INDEX_LED_COLOR1; break;
        case 2: index = INDEX_LED_COLOR2; break;
        case 3: index = INDEX_LED_COLOR3; break;
        case 4: index = INDEX_LED_COLOR4; break;
        default: return;
    }
    
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, index, color, "Цвет подсветки");
    Serial.write(cmd.data.data(), cmd.data.size());
    
    switch(colorIndex) {
        case 1: ledColor1 = color; break;
        case 2: ledColor2 = color; break;
        case 3: ledColor3 = color; break;
        case 4: ledColor4 = color; break;
    }
}

// Настройки функций (битовые маски)
void setFunBoolSettings(uint16_t settings) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_FUN_BOOL, settings, "Настройки функций");
    Serial.write(cmd.data.data(), cmd.data.size());
    funBoolSettings = settings;
}

void setFunBool1Settings(uint16_t settings) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_FUN_BOOL_1, settings, "Настройки функций 1");
    Serial.write(cmd.data.data(), cmd.data.size());
    funBool1Settings = settings;
}

void setFunBool2Settings(uint16_t settings) {
    NinebotCommand cmd = createCommand(CMD_CMAP_WR, INDEX_FUN_BOOL_2, settings, "Настройки функций 2");
    Serial.write(cmd.data.data(), cmd.data.size());
    funBool2Settings = settings;
}

// Конкретные настройки функций
void setHeadlightAlwaysOn(bool enabled) {
    if (enabled) funBool1Settings |= 0x0001;
    else funBool1Settings &= ~0x0001;
    setFunBool1Settings(funBool1Settings);
}

void setSpeedInMPH(bool enabled) {
    if (enabled) funBool1Settings |= 0x0040;
    else funBool1Settings &= ~0x0040;
    setFunBool1Settings(funBool1Settings);
}

void setNoAlarmWhenLocked(bool enabled) {
    if (enabled) funBool1Settings |= 0x0020;
    else funBool1Settings &= ~0x0020;
    setFunBool1Settings(funBool1Settings);
}

void setBluetoothBroadcast(bool enabled) {
    if (enabled) funBool1Settings |= 0x0400;
    else funBool1Settings &= ~0x0400;
    setFunBool1Settings(funBool1Settings);
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
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Ошибка загрузки HTML");
        return;
    }
    
    server.streamFile(file, "text/html");
    file.close();
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
void handleOpenDeck() { sendOpenDeck();sendSuccess("Дека открыт"); }

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
    DynamicJsonDocument doc(1024);
    doc["success"] = true;
    
    // Основные данные
    doc["speed"] = scooterSpeed;
    doc["battery"] = batteryTotal;  // Использовать обновляемую переменную
    doc["temperature"] = scooterTemperature;
    doc["mileage"] = totalMileage;
    doc["errorCode"] = scooterErrorCode;
    doc["workMode"] = workMode;
    
    doc["speedLimit"] = speedLimit;
    doc["headlightState"] = headlightState;
    doc["beepState"] = beepState;
    doc["cruiseControl"] = cruiseControl;
    doc["engineState"] = engineState;
    
    // Новые данные
    doc["battery1"] = battery1Capacity;
    doc["battery2"] = battery2Capacity;
    doc["actualRange"] = actualRange;
    doc["predictedRange"] = predictedRange;
    doc["singleMileage"] = singleMileage;
    doc["totalOperationTime"] = totalOperationTime;
    doc["totalRideTime"] = totalRideTime;
    doc["singleRideTime"] = singleRideTime;
    doc["battery1Temp"] = battery1Temp;
    doc["battery2Temp"] = battery2Temp;
    doc["mosTemp"] = mosTemp;
    doc["driveVoltage"] = driveVoltage;
    doc["motorCurrent"] = motorCurrent;
    doc["avgSpeed"] = avgSpeed;
    doc["power"] = scooterPower;
    doc["serial"] = scooterSerial;
    doc["bmsVersion"] = bmsVersion;
    doc["bms2Version"] = bms2Version;
    doc["bleVersion"] = bleVersion;
    doc["boolStatus"] = getBoolStatusString();
    doc["alarmStatus"] = getAlarmString();
    
    //Неизвестные регистры
    doc["UnkReg1"] = readScooterData(INDEX_SYSTEM_VERSION     , 2);
    doc["UnkReg2"] = readScooterData(INDEX_SYSTEM_CONFIG      , 2);
    doc["UnkReg3"] = readScooterData(INDEX_DYNAMIC_STATUS     , 2);
    doc["UnkReg4"] = readScooterData(INDEX_UNKNOWN_COUNTER1   , 2);
    doc["UnkReg5"] = readScooterData(INDEX_UNKNOWN_COUNTER2, 2);
    doc["UnkReg6"] = readScooterData(INDEX_BATTERY_CAPACITY   , 2);
    doc["UnkReg7"] = readScooterData(INDEX_UNKNOWN_PARAM1     , 2);
    doc["UnkReg8"] = readScooterData(INDEX_DYNAMIC_FLAG1      , 2);
    doc["UnkReg9"] = readScooterData(INDEX_SYSTEM_CONSTANT1   , 2);
    doc["UnkReg10"] = readScooterData(INDEX_STATUS_FLAG1       , 2);
    doc["UnkReg11"] = readScooterData(INDEX_VOLTAGE_EXT1       , 2);
    doc["UnkReg12"] = readScooterData(INDEX_VOLTAGE_EXT2       , 2);
    doc["UnkReg13"] = readScooterData(INDEX_TEMP_SENSOR1       , 2);
    doc["UnkReg14"] = readScooterData(INDEX_TEMP_SENSOR2       , 2);
    doc["UnkReg15"] = readScooterData(INDEX_SYSTEM_TEMP1       , 2);
    doc["UnkReg16"] = readScooterData(INDEX_SYSTEM_TEMP2       , 2);
    doc["UnkReg17"] = readScooterData(INDEX_SYSTEM_TEMP3       , 2);
    doc["UnkReg18"] = readScooterData(INDEX_EXT_TEMP1          , 2);
    doc["UnkReg19"] = readScooterData(INDEX_EXT_TEMP2          , 2);
    doc["UnkReg20"] = readScooterData(INDEX_MOTOR_FLAG1, 2);
    doc["UnkReg21"] = readScooterData(INDEX_MOTOR_FLAG2        , 2);
    doc["UnkReg22"] = readScooterData(INDEX_MOTOR_CONSTANT     , 2);
    doc["UnkReg23"] = readScooterData(INDEX_PWM_PARAM          , 2);
    doc["UnkReg24"] = readScooterData(INDEX_DYNAMIC_PARAM1, 2);
    doc["UnkReg25"] = readScooterData(INDEX_SYSTEM_FLAGS       , 2);
    doc["UnkReg26"] = readScooterData(INDEX_BMS2_VERSION   , 2);
    doc["UnkReg27"] = readScooterData(INDEX_BMS_VERSION     , 2);
    doc["UnkReg28"] = readScooterData(INDEX_UNKNOWN_LIMIT      , 2);
    doc["UnkReg29"] = readScooterData(INDEX_FUN_BOOL_1   , 2);
    doc["UnkReg30"] = readScooterData(INDEX_BATTERY_RELATED    , 2);
    doc["UnkReg31"] = readScooterData(INDEX_LARGE_TIMER        , 2);
    doc["UnkReg32"] = readScooterData(INDEX_DYNAMIC_COUNTER    , 2);
    doc["UnkReg33"] = readScooterData(INDEX_SYSTEM_CONSTANT3   , 2);
    doc["UnkReg34"] = readScooterData(INDEX_SYSTEM_CONSTANT4   , 2);
    doc["UnkReg35"] = readScooterData(INDEX_BATTERY_DUPLICATE  , 2);
    doc["UnkReg36"] = readScooterData(INDEX_CALIBRATION_VALUE1 , 2);
    doc["UnkReg37"] = readScooterData(INDEX_CALIBRATION_VALUE2 , 2);

    

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleExtendedInfo() {
    String html = "<html><body><h1>Расширенная информация</h1>";
    html += "<p><b>Серийный номер:</b> " + scooterSerial + "</p>";
    html += "<p><b>Версия BMS:</b> " + bmsVersion + "</p>";
    html += "<p><b>Версия внешней BMS:</b> " + bms2Version + "</p>";
    html += "<p><b>Версия BLE:</b> " + bleVersion + "</p>";
    html += "<p><b>Статусы:</b> " + getBoolStatusString() + "</p>";
    html += "<p><b>Тревоги:</b> " + getAlarmString() + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleNotFound() {
    DynamicJsonDocument doc(200);
    doc["success"] = false;
    doc["message"] = "Страница не найдена";
    String response;
    serializeJson(doc, response);
    server.send(404, "application/json", response);
}

void handleLedMode() {
    if (server.hasArg("mode")) {
        uint8_t mode = server.arg("mode").toInt();
        setLedMode(mode);
        sendSuccess("Режим подсветки установлен");
    } else {
        sendSuccess("Не указан режим");
    }
}

void handleLedColor() {
    if (server.hasArg("color") && server.hasArg("index")) {
        uint8_t index = server.arg("index").toInt();
        uint16_t color = strtol(server.arg("color").c_str(), NULL, 16);
        setLedColor(index, color);
        sendSuccess("Цвет установлен");
    } else {
        sendSuccess("Не указаны параметры цвета");
    }
}

void handleFindScooter() {
    findScooter();
    sendSuccess("Поиск самоката активирован");
}

void handleReboot() {
    rebootSystem();
    sendSuccess("Система перезагружается");
}

void handlePowerOff() {
    powerOff();
    sendSuccess("Система выключается");
}

void handleToggleLimit() {
    toggleSpeedLimit();
    sendSuccess("Лимит скорости переключен");
}

// Обработчики для настроек функций
void handleHeadlightAlwaysOn() {
    bool enabled = server.hasArg("enabled") ? server.arg("enabled").toInt() : true;
    setHeadlightAlwaysOn(enabled);
    sendSuccess(enabled ? "Фары всегда включены" : "Фары по умолчанию");
}

void handleSpeedMPH() {
    bool enabled = server.hasArg("enabled") ? server.arg("enabled").toInt() : true;
    setSpeedInMPH(enabled);
    sendSuccess(enabled ? "Скорость в MPH" : "Скорость в KM/H");
}

void handleNoAlarmLock() {
    bool enabled = server.hasArg("enabled") ? server.arg("enabled").toInt() : true;
    setNoAlarmWhenLocked(enabled);
    sendSuccess(enabled ? "Тревога при блокировке отключена" : "Тревога при блокировке включена");
}

void handleBTBroadcast() {
    bool enabled = server.hasArg("enabled") ? server.arg("enabled").toInt() : true;
    setBluetoothBroadcast(enabled);
    sendSuccess(enabled ? "Bluetooth broadcast включен" : "Bluetooth broadcast выключен");
}

// ============================================================================
// ОБРАБОТЧИКИ ДЛЯ СКАНИРОВАНИЯ РЕГИСТРОВ
// ============================================================================

void handleScanRead() {
    if (!server.hasArg("index")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing index parameter\"}");
        return;
    }
    
    String indexStr = server.arg("index");
    uint8_t index = (uint8_t)strtol(indexStr.c_str(), NULL, 16);
        
    // Пробуем прочитать регистр
    int value = readScooterData(index, 2);
    
    if (value != -1) {
        String response = "{";
        response += "\"success\":true,";
        response += "\"index\":\"" + indexStr + "\",";
        response += "\"value\":" + String(value) + ",";
        response += "\"valueHex\":\"0x" + String(value, HEX) + "\",";
        response += "\"type\":\"" + getValueType(value) + "\"";
        response += "}";
        
        server.send(200, "application/json", response);
    } else {
        String response = "{";
        response += "\"success\":false,";
        response += "\"index\":\"" + indexStr + "\",";
        response += "\"message\":\"Read failed or timeout\"";
        response += "}";
        
        server.send(200, "application/json", response);
    }
}

void handleScanWrite() {
    if (!server.hasArg("index") || !server.hasArg("value")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing index or value parameter\"}");
        return;
    }
    
    String indexStr = server.arg("index");
    String valueStr = server.arg("value");
    
    uint8_t index = (uint8_t)strtol(indexStr.c_str(), NULL, 16);
    uint16_t value = (uint16_t)strtol(valueStr.c_str(), NULL, 16);
        
    // Пробуем записать регистр с ответом
    bool writeSuccess = testWriteWithResponse(index, value);
    
    if (writeSuccess) {
        // Читаем значение после записи для подтверждения
        delay(50);
        int readBackValue = readScooterData(index, 2);
        
        String response = "{";
        response += "\"success\":true,";
        response += "\"index\":\"" + indexStr + "\",";
        response += "\"valueWritten\":\"0x" + String(value, HEX) + "\",";
        response += "\"valueReadback\":" + String(readBackValue) + ",";
        response += "\"valueReadbackHex\":\"0x" + String(readBackValue, HEX) + "\",";
        response += "\"message\":\"Write successful\"";
        response += "}";
        
        server.send(200, "application/json", response);
    } else {
        String response = "{";
        response += "\"success\":false,";
        response += "\"index\":\"" + indexStr + "\",";
        response += "\"value\":\"0x" + String(value, HEX) + "\",";
        response += "\"message\":\"Write failed or no response\"";
        response += "}";
        
        server.send(200, "application/json", response);
    }
}

// ============================================================================
// SETUP И LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    
    // Инициализация LittleFS
    if (!LittleFS.begin()) {
        // Можно продолжить работу, но без веб-интерфейса
    } else {        
        // Выводим список файлов для отладки
        Dir dir = LittleFS.openDir("/");
        while (dir.next()) {
        }
    }

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Инициализация WiFi с автоматическим переключением
    if (strlen(WIFI_SSID_STA) > 0 && strcmp(WIFI_SSID_STA, "YOUR_WIFI_SSID") != 0) {
        // Пробуем подключиться к существующей сети
        setupWiFiSTA();
    } else {
        // Если SSID не указан или это placeholder, используем AP режим
        setupWiFiAP();
    }

    // ИЗМЕНЕННЫЙ обработчик главной страницы
    server.on("/", []() {
        if (!LittleFS.exists("/index.html")) {
            server.send(500, "text/plain", "HTML файл не найден. Загрузите файлы в LittleFS.");
            return;
        }
        
        File file = LittleFS.open("/index.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Ошибка открытия HTML файла");
            return;
        }
        
        server.streamFile(file, "text/html");
        file.close();
    });


        // Обработчик для OTA

    
    server.on("/firmware_info", HTTP_GET, []() {
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["version"] = "0.0.1";
        doc["chip_id"] = String(ESP.getChipId());
        doc["free_heap"] = ESP.getFreeHeap();
        doc["sketch_size"] = ESP.getSketchSize();
        doc["free_sketch_space"] = ESP.getFreeSketchSpace();
        doc["sdk_version"] = ESP.getSdkVersion();
        doc["core_version"] = ESP.getCoreVersion();
        doc["flash_size"] = ESP.getFlashChipSize();
        doc["cycle_count"] = ESP.getCycleCount();
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    // Проверка обновлений (заглушка)
    server.on("/check_updates", HTTP_GET, []() {
        DynamicJsonDocument doc(200);
        doc["success"] = true;
        doc["update_available"] = false;
        doc["current_version"] = "1.2.0";
        doc["latest_version"] = "1.2.0";
        doc["message"] = "У вас установлена последняя версия";
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    // Обработчики WiFi
    server.on("/wifi_status", handleWiFiStatus);
    server.on("/wifi_toggle", handleWiFiToggle);
    
    server.on("/unlock", handleUnlock);
    server.on("/lock", handleLock);
    server.on("/open_deck", handleOpenDeck);
    server.on("/toggle", handleToggle);
    server.on("/status", handleStatus);
    server.on("/data", handleData);
    server.on("/extended", handleExtendedInfo);

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

    server.on("/led_mode", handleLedMode);
    server.on("/led_color", handleLedColor);
    server.on("/find_scooter", handleFindScooter);
    server.on("/reboot", handleReboot);
    server.on("/power_off", handlePowerOff);
    server.on("/toggle_limit", handleToggleLimit);
    
    // Обработчики для конкретных настроек
    server.on("/headlight_always_on", handleHeadlightAlwaysOn);
    server.on("/speed_mph", handleSpeedMPH);
    server.on("/no_alarm_lock", handleNoAlarmLock);
    server.on("/bt_broadcast", handleBTBroadcast);
    server.on("/scan_read", handleScanRead);
    server.on("/scan_write", handleScanWrite);

    // Обслуживание статических файлов
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/script.js", LittleFS, "/script.js");
    server.serveStatic("/head.png", LittleFS, "/head.png");

    // НАСТРОЙКА OTA СЕРВЕРА (ВАЖНО!)
    httpUpdater.setup(&server, OTA_PATH, OTA_USERNAME, OTA_PASSWORD);
    server.onNotFound(handleNotFound);
    server.begin();

    delay(1000);
    sendUnlock();
}

void loop() {
    handleButton();
    server.handleClient();

    // Проверка WiFi подключения каждые 30 секунд
    if (millis() - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
        if (!checkWiFiConnection() && strlen(WIFI_SSID_STA) > 0 && strcmp(WIFI_SSID_STA, "YOUR_WIFI_SSID") != 0) {
            Serial.println("🔄 Попытка переподключения к WiFi...");
            setupWiFiSTA();
        }
        lastWiFiCheck = millis();
    }
    
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        lastHeartbeatTime = millis();
    }
    
    // Обновление данных самоката
    updateAllScooterData();
    
    delay(10);
}