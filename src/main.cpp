#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <LittleFS.h>

// Настройки Wi-Fi
const char* ssid = "NinebotESx";
const char* password = "12345678";
// Настройки OTA
const char* OTA_USERNAME = "admin";
const char* OTA_PASSWORD = "NinebotOTA123!"; // Сложный пароль
const char* OTA_PATH = "/update"; // Стандартный путь

// Настройки для подключения к существующей сети
// УКАЖИТЕ ВАШИ ДАННЫЕ ЗДЕСЬ:
const char* WIFI_SSID_STA = "teemse";  // Имя вашей Wi-Fi сети
const char* WIFI_PASSWORD_STA = "turbina7";  // Пароль вашей Wi-Fi сети
// Оставьте WIFI_SSID_STA пустым для отключения STA режима

// Режим работы Wi-Fi
bool wifiStationMode = false;  // true = STA, false = AP
IPAddress localIP;
bool wifiConnected = false;

// Переменные для управления WiFi
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 30000; // Проверка каждые 30 секунд

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

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
// ============================================================================
// ПОЛНЫЙ СПИСОК РЕГИСТРОВ ИЗ ДОКУМЕНТА
// ============================================================================

// Основные регистры (0x00-0x6F)
// ============================================================================
// ПОЛНАЯ КАРТА ПАМЯТИ SAMOKATA NINEBOT ES
// На основании документации и реальных данных из лога
// ============================================================================

// ██████████████████████████████████████████████████████████████████████████
// СИСТЕМНЫЕ РЕГИСТРЫ (0x00-0x0F) - НЕДОКУМЕНТИРОВАННЫЕ
// ██████████████████████████████████████████████████████████████████████████
#define INDEX_SYSTEM_VERSION     0x00  // [20828] Версия аппаратного обеспечения
#define INDEX_SYSTEM_CONFIG      0x01  // [2030] Конфигурация системы
#define INDEX_RESERVED_02        0x02  // [0] Зарезервировано
#define INDEX_RESERVED_03        0x03  // [0] Зарезервировано
#define INDEX_RESERVED_04        0x04  // [0] Зарезервировано
#define INDEX_RESERVED_05        0x05  // [0] Зарезервировано
#define INDEX_RESERVED_06        0x06  // [0] Зарезервировано
#define INDEX_RESERVED_07        0x07  // [0] Зарезервировано
#define INDEX_RESERVED_08        0x08  // [0] Зарезервировано
#define INDEX_RESERVED_09        0x09  // [0] Зарезервировано
#define INDEX_RESERVED_0A        0x0A  // [0] Зарезервировано
#define INDEX_RESERVED_0B        0x0B  // [0] Зарезервировано
#define INDEX_RESERVED_0C        0x0C  // [0] Зарезервировано
#define INDEX_DYNAMIC_STATUS     0x0D  // [2049-2050] Динамический системный статус
#define INDEX_UNKNOWN_COUNTER1   0x0E  // [2069-2070] Счетчик операций 1
#define INDEX_UNKNOWN_COUNTER2   0x0F  // [2067-2069] Счетчик операций 2

// ██████████████████████████████████████████████████████████████████████████
// ИНФОРМАЦИОННЫЕ РЕГИСТРЫ (0x10-0x6F)
// ██████████████████████████████████████████████████████████████████████████

// Серийные номера и идентификация
#define INDEX_SERIAL_NUMBER      0x10  // [13139] Серийный номер (14 байт)
#define INDEX_SERIAL_CONTINUED   0x11  // [21827] Продолжение серийного номера
#define INDEX_SERIAL_CONTINUED2  0x12  // [12870] Продолжение серийного номера
#define INDEX_SERIAL_CONTINUED3  0x13  // [13618] Продолжение серийного номера
#define INDEX_SERIAL_CONTINUED4  0x14  // [16946] Продолжение серийного номера
#define INDEX_SERIAL_CONTINUED5  0x15  // [13360] Продолжение серийного номера
#define INDEX_SERIAL_CONTINUED6  0x16  // [14649] Продолжение серийного номера

// Настройки подключения
#define INDEX_BT_PASSWORD        0x17  // [12336→1] Пароль Bluetooth (6 байт)
#define INDEX_BT_PASSWORD2       0x18  // [12336→1] Пароль Bluetooth
#define INDEX_BT_PASSWORD3       0x19  // [12336→1] Пароль Bluetooth

// Версии прошивок
#define INDEX_FW_VERSION         0x1A  // [5396] Версия прошивки контроллера (1.5.1.4)

// Статусы и ошибки
#define INDEX_ERROR_CODE         0x1B  // [0→32→55] Код ошибки (ВНИМАНИЕ: ОШИБКИ!)
#define INDEX_ALARM_CODE         0x1C  // [0] Код тревоги
#define INDEX_BOOL_STATUS        0x1D  // [2050] Булевы статусы (активен+заблокирован)
#define INDEX_WORK_SYSTEM        0x1E  // [0] Текущая рабочая система
#define INDEX_WORK_MODE          0x1F  // [1] Текущий режим работы (1=ECO)

// Батарея и пробег
#define INDEX_BATTERY1_CAP       0x20  // [0] Емкость батареи 1
#define INDEX_BATTERY2_CAP       0x21  // [0] Емкость батареи 2
#define INDEX_BATTERY            0x22  // [76] Общая емкость батареи (76%)
#define INDEX_BATTERY_CAPACITY   0x23  // [8000] Емкость батареи в mAh (8Ah)
#define INDEX_ACTUAL_RANGE       0x24  // [3100→2896] Фактический запас хода (310м→289м)
#define INDEX_PREDICT_RANGE      0x25  // [3876] Прогнозируемый запас хода (387м)
#define INDEX_SPEED              0x26  // [0] Текущая скорость (0 км/ч)
#define INDEX_UNKNOWN_PARAM1     0x27  // [17] Неизвестный параметр

// Пробег и статистика
#define INDEX_RESERVED_28        0x28  // [0] Зарезервировано
#define INDEX_MILEAGE_L          0x29  // [1→2] Пробег младшие 16 бит (1-2м)
#define INDEX_MILEAGE_H          0x2A  // [0] Пробег старшие 16 бит
#define INDEX_RESERVED_2B        0x2B  // [0] Зарезервировано
#define INDEX_DYNAMIC_FLAG1      0x2C  // [0→1] Динамический флаг 1
#define INDEX_SYSTEM_CONSTANT1   0x2D  // [4] Системная константа 1
#define INDEX_STATUS_FLAG1       0x2E  // [1] Флаг состояния 1
#define INDEX_SINGLE_MILEAGE     0x2F  // [0] Пробег одной поездки

// Время работы
#define INDEX_RESERVED_30        0x30  // [0] Зарезервировано
#define INDEX_RESERVED_31        0x31  // [0] Зарезервировано
#define INDEX_TOTAL_TIME_L       0x32  // [1316→3969→4104] Общее время работы (младшие)
#define INDEX_TOTAL_TIME_H       0x33  // [0] Общее время работы (старшие)
#define INDEX_RIDE_TIME_L        0x34  // [-5359→-5354] Время поездки (младшие)
#define INDEX_RIDE_TIME_H        0x35  // [8] Время поездки (старшие)
#define INDEX_RESERVED_36        0x36  // [0] Зарезервировано
#define INDEX_RESERVED_37        0x37  // [0] Зарезервировано
#define INDEX_RESERVED_38        0x38  // [0] Зарезервировано
#define INDEX_RESERVED_39        0x39  // [0] Зарезервировано
#define INDEX_SINGLE_TIME        0x3A  // [465→58→193] Время одной операции
#define INDEX_SINGLE_RIDE_TIME   0x3B  // [0] Время движения одной поездки
#define INDEX_RESERVED_3C        0x3C  // [0] Зарезервировано
#define INDEX_RESERVED_3D        0x3D  // [0] Зарезервировано

// Температуры
#define INDEX_BODY_TEMP          0x3E  // [27] Температура корпуса (2.7°C)
#define INDEX_BAT1_TEMP          0x3F  // [0] Температура батареи 1
#define INDEX_BAT2_TEMP          0x40  // [0] Температура батареи 2
#define INDEX_MOS_TEMP           0x41  // [28→29] Температура MOS (2.8-2.9°C)
#define INDEX_RESERVED_42        0x42  // [0] Зарезервировано
#define INDEX_RESERVED_43        0x43  // [0] Зарезервировано
#define INDEX_RESERVED_44        0x44  // [0] Зарезервировано
#define INDEX_RESERVED_45        0x45  // [0] Зарезервировано
#define INDEX_RESERVED_46        0x46  // [0] Зарезервировано

// Электрические параметры
#define INDEX_DRIVE_VOLTAGE      0x47  // [3882→3880] Напряжение системы (38.8V)
#define INDEX_VOLTAGE_EXT1       0x48  // [3901] Дополнительное напряжение 1
#define INDEX_VOLTAGE_EXT2       0x49  // [0] Дополнительное напряжение 2
#define INDEX_TEMP_SENSOR1       0x4A  // [0→13] Температурный датчик 1
#define INDEX_TEMP_SENSOR2       0x4B  // [0→14] Температурный датчик 2
#define INDEX_SYSTEM_TEMP1       0x4C  // [0] Системная температура 1
#define INDEX_SYSTEM_TEMP2       0x4D  // [26→27] Системная температура 2
#define INDEX_SYSTEM_TEMP3       0x4E  // [26] Системная температура 3
#define INDEX_RESERVED_4F        0x4F  // [0] Зарезервировано
#define INDEX_BAT2_TEMP2         0x50  // [0] Температура внешней батареи 2
#define INDEX_EXT_TEMP1          0x51  // [40→41] Курок газа
#define INDEX_EXT_TEMP2          0x52  // [40] Тормоз
#define INDEX_MOTOR_CURRENT      0x53  // [0] Ток мотора (0A)
#define INDEX_MOTOR_FLAG1        0x54  // [0→1] Флаг мотора 1
#define INDEX_RESERVED_55        0x55  // [0] Зарезервировано
#define INDEX_MOTOR_FLAG2        0x56  // [0→1] Флаг мотора 2
#define INDEX_MOTOR_CONSTANT     0x57  // [4] Константа управления мотором
#define INDEX_RESERVED_58        0x58  // [0] Зарезервировано
#define INDEX_RESERVED_59        0x59  // [0] Зарезервировано
#define INDEX_RESERVED_5A        0x5A  // [0] Зарезервировано
#define INDEX_RESERVED_5B        0x5B  // [0] Зарезервировано
#define INDEX_RESERVED_5C        0x5C  // [0] Зарезервировано
#define INDEX_RESERVED_5D        0x5D  // [0] Зарезервировано
#define INDEX_RESERVED_5E        0x5E  // [0] Зарезервировано
#define INDEX_RESERVED_5F        0x5F  // [0] Зарезервировано

// Дополнительные параметры
#define INDEX_PWM_PARAM          0x60  // [1200] Параметр PWM/управления
#define INDEX_DYNAMIC_PARAM1     0x61  // [100→50] Динамический параметр 1
#define INDEX_RESERVED_62        0x62  // [0] Зарезервировано
#define INDEX_RESERVED_63        0x63  // [0] Зарезервировано
#define INDEX_SYSTEM_FLAGS       0x64  // [16386] Системные флаги
#define INDEX_AVG_SPEED          0x65  // [0] Средняя скорость
#define INDEX_BMS2_VERSION       0x66  // [3] Версия внешней BMS
#define INDEX_BMS_VERSION        0x67  // [1616] Версия встроенной BMS
#define INDEX_BLE_VERSION        0x68  // [0] Версия прошивки панели
#define INDEX_RESERVED_69        0x69  // [0] Зарезервировано
#define INDEX_RESERVED_6A        0x6A  // [0] Зарезервировано
#define INDEX_RESERVED_6B        0x6B  // [0] Зарезервировано
#define INDEX_RESERVED_6C        0x6C  // [0] Зарезервировано
#define INDEX_RESERVED_6D        0x6D  // [0] Зарезервировано
#define INDEX_RESERVED_6E        0x6E  // [0] Зарезервировано
#define INDEX_RESERVED_6F        0x6F  // [0] Зарезервировано

// ██████████████████████████████████████████████████████████████████████████
// РЕГИСТРЫ УПРАВЛЕНИЯ (0x70-0x92)
// ██████████████████████████████████████████████████████████████████████████
#define INDEX_LOCK              0x70  // [0] Блокировка
#define INDEX_UNLOCK            0x71  // [0] Разблокировка
#define INDEX_LIMIT_SPD         0x72  // [250→50] Ограничение/снятие скорости
#define INDEX_NORMAL_SPEED      0x73  // [150→50] Лимит скорости в normal mode
#define INDEX_SPEED_LIMIT       0x74  // [80→50] Лимит скорости в limit mode
#define INDEX_WORK_MODE_CTL     0x75  // [1] Управление режимом работы (1=ECO)
#define INDEX_RESERVED_76       0x76  // [0→1] Зарезервировано
#define INDEX_ENGINE            0x77  // [0] Запуск/остановка двигателя
#define INDEX_REBOOT            0x78  // [0] Перезагрузка системы
#define INDEX_POWER_OFF         0x79  // [0] Выключение
#define INDEX_RESERVED_7A       0x7A  // [0] Зарезервировано
#define INDEX_RESERVED_7B       0x7B  // [0] Зарезервировано
#define INDEX_CRUISE            0x7C  // [0] Круиз-контроль
#define INDEX_FUN_BOOL          0x7D  // [0→3] Настройки функций bool
#define INDEX_FIND_SCOOTER      0x7E  // [0] Поиск самоката
#define INDEX_UNKNOWN_LIMIT     0x7F  // [60→2] Неизвестный лимит(не трогать)
#define INDEX_FUN_BOOL_1        0x80  // [16→789→784] Настройки функций 1(Фара?)
#define INDEX_FUN_BOOL_2        0x81  // [1] Настройки функций 2
#define INDEX_RESERVED_82       0x82  // [0] Зарезервировано
#define INDEX_RESERVED_83       0x83  // [0] Зарезервировано
#define INDEX_BATTERY_RELATED   0x84  // [0→76] Параметр связанный с батареей
#define INDEX_RESERVED_85       0x85  // [1] Зарезервировано
#define INDEX_RESERVED_86       0x86  // [0] Зарезервировано
#define INDEX_RESERVED_87       0x87  // [0] Зарезервировано
#define INDEX_RESERVED_88       0x88  // [0] Зарезервировано
#define INDEX_RESERVED_89       0x89  // [0] Зарезервировано
#define INDEX_RESERVED_8A       0x8A  // [0] Зарезервировано
#define INDEX_RESERVED_8B       0x8B  // [0] Зарезервировано
#define INDEX_LARGE_TIMER       0x8C  // [-26181] Большой таймер/счетчик
#define INDEX_DYNAMIC_COUNTER   0x8D  // [5890→2562→9219] Динамический счетчик
#define INDEX_DECK_LOCK        0x8E  // [0] Замок деки
#define INDEX_SYSTEM_CONSTANT3  0x8F  // [124] Системная константа 3
#define INDEX_HEADLIGHT         0x90  // [1] Управление фарами
#define INDEX_BEEP_ALARM        0x91  // [1] Звуковой сигнал
#define INDEX_BEEP_TOTAL        0x92  // [1] Общий звук

// ██████████████████████████████████████████████████████████████████████████
// БЫСТРЫЕ РЕГИСТРЫ (0xB0-0xBF)
// ██████████████████████████████████████████████████████████████████████████
#define INDEX_QUICK_ERROR       0xB0  // [0→32] Код ошибки (быстрый) - ОШИБКА!
#define INDEX_QUICK_ALARM       0xB1  // [0] Код тревоги (быстрый)
#define INDEX_QUICK_BOOL        0xB2  // [2050→2048] Булевы статусы (быстрый)
#define INDEX_QUICK_BAT_BOTH    0xB3  // [76] Емкость обеих батарей
#define INDEX_QUICK_BATTERY     0xB4  // [76] Общая емкость батареи (быстрый)
#define INDEX_QUICK_SPEED       0xB5  // [0] Скорость (быстрый)
#define INDEX_QUICK_AVG_SPEED   0xB6  // [0] Средняя скорость (быстрый)
#define INDEX_QUICK_MILEAGE_L   0xB7  // [2] Пробег младшие 16 бит (быстрый)
#define INDEX_QUICK_MILEAGE_H   0xB8  // [0] Пробег старшие 16 бит (быстрый)
#define INDEX_QUICK_SINGLE_MILEAGE 0xB9  // [0] Пробег поездки (быстрый)
#define INDEX_QUICK_SINGLE_TIME 0xBA  // [481→73→212] Время поездки (быстрый)
#define INDEX_QUICK_BODY_TEMP   0xBB  // [27] Температура корпуса (быстрый)
#define INDEX_QUICK_CURRENT_LIMIT 0xBC  // [1] Текущий лимит скорости
#define INDEX_QUICK_POWER       0xBD  // [0] Мощность (Вт)
#define INDEX_QUICK_ALARM_DELAY 0xBE  // [0] Код тревоги задержки
#define INDEX_QUICK_PREDICT_RANGE 0xBF  // [3876] Прогнозируемый запас хода

// ██████████████████████████████████████████████████████████████████████████
// ПОДСВЕТКА И ДИАГНОСТИКА (0xC0-0xDF)
// ██████████████████████████████████████████████████████████████████████████
#define INDEX_RESERVED_C0       0xC0  // [0] Зарезервировано
#define INDEX_RESERVED_C1       0xC1  // [0] Зарезервировано
#define INDEX_RESERVED_C2       0xC2  // [0] Зарезервировано
#define INDEX_RESERVED_C3       0xC3  // [0] Зарезервировано
#define INDEX_CALIBRATION_VALUE1 0xC4  // [100] Калибровочное значение 1
#define INDEX_CALIBRATION_VALUE2 0xC5  // [100] Калибровочное значение 2
#define INDEX_LED_MODE          0xC6  // [8→2] Режим подсветки
#define INDEX_RESERVED_C7       0xC7  // [0] Зарезервировано
#define INDEX_LED_COLOR1        0xC8  // [-24336] Цвет подсветки 1
#define INDEX_RESERVED_C9       0xC9  // [0] Зарезервировано
#define INDEX_LED_COLOR2        0xCA  // [240] Цвет подсветки 2
#define INDEX_LED_COLOR2_EXT    0xCB  // [-14096] Расширение цвета 2
#define INDEX_LED_COLOR3        0xCC  // [240] Цвет подсветки 3
#define INDEX_RESERVED_CD       0xCD  // [0] Зарезервировано
#define INDEX_LED_COLOR4        0xCE  // [-14096] Цвет подсветки 4
#define INDEX_RESERVED_CF       0xCF  // [0] Зарезервировано

// CPU ID и системная информация
#define INDEX_RESERVED_D0       0xD0  // [0] Зарезервировано
#define INDEX_RESERVED_D1       0xD1  // [0] Зарезервировано
#define INDEX_RESERVED_D2       0xD2  // [0] Зарезервировано
#define INDEX_RESERVED_D3       0xD3  // [0] Зарезервировано
#define INDEX_RESERVED_D4       0xD4  // [0] Зарезервировано
#define INDEX_RESERVED_D5       0xD5  // [0] Зарезервировано
#define INDEX_RESERVED_D6       0xD6  // [0] Зарезервировано
#define INDEX_RESERVED_D7       0xD7  // [0] Зарезервировано
#define INDEX_RESERVED_D8       0xD8  // [0] Зарезервировано
#define INDEX_RESERVED_D9       0xD9  // [0] Зарезервировано
#define INDEX_CPUID_A           0xDA  // [-7679] CPU ID A
#define INDEX_CPUID_B           0xDB  // [4184] CPU ID B
#define INDEX_CPUID_C           0xDC  // [-32768] CPU ID C
#define INDEX_CPUID_D           0xDD  // [21522] CPU ID D
#define INDEX_CPUID_E           0xDE  // [-22764] CPU ID E
#define INDEX_CPUID_F           0xDF  // [1285] CPU ID F

// ██████████████████████████████████████████████████████████████████████████
// ДОПОЛНИТЕЛЬНЫЕ РЕГИСТРЫ (0xE0-0xFF)
// ██████████████████████████████████████████████████████████████████████████
#define INDEX_SYSTEM_CONSTANT4  0x9B  // [31779] Системная константа 4
#define INDEX_BATTERY_DUPLICATE 0x9F  // [76] Дублирует заряд батареи

// Зарезервированные регистры (все 0)
#define INDEX_RESERVED_E0       0xE0  // [0] Зарезервировано
// ... все остальные до 0xFF равны 0

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
        doc["version"] = "1.2.0";
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