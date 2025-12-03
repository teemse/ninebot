#ifndef CONFIG_H
#define CONFIG_H

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

// Периоды обновления и проверки
const unsigned long WIFI_CHECK_INTERVAL = 30000; // Проверка каждые 30 секунд
const unsigned long HEARTBEAT_INTERVAL = 4000; // Heartbeat каждые 4 секунды

// Аппаратные настройки
const int BUTTON_PIN = 5;
const unsigned long debounceDelay = 50;

// Режим работы Wi-Fi по умолчанию
bool wifiStationMode = false;  // true = STA, false = AP

#endif // CONFIG_H