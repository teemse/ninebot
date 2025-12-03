#ifndef REGISTERS_H
#define REGISTERS_H

// ============================================================================
// КОМАНДЫ ПРОТОКОЛА
// ============================================================================

#define CMD_CMAP_RD        0x01
#define CMD_CMAP_WR        0x02
#define CMD_CMAP_WR_NR     0x03
#define CMD_CMAP_ACK_RD    0x04
#define CMD_CMAP_ACK_WR    0x05
#define CMD_HEARTBEAT      0x55

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

#endif // REGISTERS_H