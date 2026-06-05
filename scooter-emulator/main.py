#!/usr/bin/env python3
"""
Эмулятор ESP8266 для тестирования фронтенда Ninebot Scooter Dashboard
Полностью повторяет API оригинального бекенда
"""

import json
import time
import random
import threading
from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import os

# ============================================================================
# КОНФИГУРАЦИЯ ЭМУЛЯТОРА
# ============================================================================

CONFIG = {
    'wifi_ssid_ap': 'Scooter-Emulator',
    'wifi_password_ap': '12345678',
    'wifi_ssid_sta': 'HomeWiFi',
    'ota_path': '/firmware',
    'ota_username': 'admin',
    'ota_password': 'admin',
    'port': 80,
    'static_dir': 'static',  # Папка со статическими файлами
}

# ============================================================================
# ЭМУЛЯЦИЯ ДАННЫХ САМОКАТА
# ============================================================================

class ScooterState:
    """Эмуляция состояния самоката"""
    
    def __init__(self):
        # Основные данные
        self.is_locked = False
        self.speed_limit = 250
        self.normal_speed_limit = 250
        self.speed_limit_mode = 60
        self.engine_state = True
        self.cruise_control = False
        self.headlight_state = True
        self.beep_state = True
        self.beep_alarm_state = True
        self.beep_total_state = True
        self.work_mode = 0  # 0=Normal, 1=Eco, 2=Sport
        
        # Данные самоката
        self.scooter_speed = 0
        self.battery_total = 85
        self.body_temperature = 25
        self.total_mileage = 12345
        self.error_code = 0
        self.alarm_code = 0
        self.bool_status = 0x0800  # Активирован
        
        # Расширенные данные
        self.battery1_capacity = 85
        self.battery2_capacity = 0
        self.actual_range = 25
        self.predicted_range = 30
        self.single_mileage = 5
        self.total_operation_time = 100
        self.total_ride_time = 80
        self.single_ride_time = 15
        self.battery1_temp = 25
        self.battery2_temp = 0
        self.mos_temp = 30
        self.drive_voltage = 36.5
        self.motor_current = 2.5
        self.avg_speed = 20
        self.scooter_power = 250
        self.scooter_temperature = 28
        self.scooter_error_code = 0
        
        # Серийные номера и версии
        self.scooter_serial = "N2GSD1234C56789"
        self.bms_version = "1.3.5"
        self.bms2_version = "N/A"
        self.ble_version = "1.2.3"
        
        # Подсветка
        self.led_mode = 1
        self.led_color1 = 0xA0F0  # Синий
        self.led_color2 = 0x50F0  # Зеленый
        self.led_color3 = 0x00F0  # Красный
        self.led_color4 = 0xC8F0  # Фиолетовый
        
        # Настройки функций
        self.fun_bool_settings = 0x0000
        self.fun_bool1_settings = 0x0000
        self.fun_bool2_settings = 0x0000
        
        # WiFi состояние
        self.wifi_mode = 'AP'  # 'AP' или 'STA'
        self.wifi_connected = True
        self.local_ip = '192.168.4.1'
        self.signal_strength = -40
        
        # Время запуска
        self.start_time = time.time()
        
        # Поток для эмуляции движения
        self.running = True
        self.simulation_thread = threading.Thread(target=self._simulate_scooter)
        self.simulation_thread.daemon = True
        self.simulation_thread.start()
    
    def _simulate_scooter(self):
        """Эмуляция изменения параметров самоката"""
        while self.running:
            time.sleep(2)
            
            # Эмуляция скорости (если разблокирован и двигатель включен)
            if not self.is_locked and self.engine_state:
                self.scooter_speed = random.randint(0, 25)
                self.avg_speed = int(self.scooter_speed * 0.8)
                self.motor_current = round(random.uniform(0.5, 5.0), 1)
            else:
                self.scooter_speed = 0
                self.avg_speed = 0
                self.motor_current = 0
            
            # Эмуляция температуры
            self.mos_temp = random.randint(25, 45)
            self.body_temperature = self.mos_temp - 5
            self.battery1_temp = random.randint(22, 35)
            
            # Эмуляция разряда батареи
            if self.scooter_speed > 0:
                self.battery_total = max(0, self.battery_total - 0.01)
                self.battery1_capacity = self.battery_total
                self.actual_range = max(0, self.actual_range - 0.05)
                self.predicted_range = max(0, self.predicted_range - 0.03)
            
            # Эмуляция пробега
            if self.scooter_speed > 0:
                self.total_mileage += random.randint(0, 10)
                self.single_mileage += 1
    
    def get_bool_status_string(self):
        """Получение строки статусов"""
        status = []
        if self.bool_status & 0x0001:
            status.append("Ограничение скорости")
        if self.bool_status & 0x0002:
            status.append("Заблокирован")
        if self.bool_status & 0x0004:
            status.append("Звуковой сигнал")
        if self.bool_status & 0x0200:
            status.append("Батарея 2 подключена")
        if self.bool_status & 0x0800:
            status.append("Активирован")
        return ", ".join(status) if status else "Нет статусов"
    
    def get_alarm_string(self):
        """Получение строки тревог"""
        alarms = {
            9: "Толкают в заблокированном режиме",
            12: "Высокое напряжение при торможении"
        }
        return alarms.get(self.alarm_code, f"Тревога: {self.alarm_code}" if self.alarm_code else "Нет тревог")
    
    def get_led_mode_name(self):
        """Получение названия режима подсветки"""
        modes = {
            0: "Выключено",
            1: "Одноцветное дыхание",
            2: "Всецветное дыхание",
            3: "Два цвета раздельно",
            4: "Все цвета раздельно",
            5: "Одноцветное мерцание",
            6: "Всецветное мерцание",
            7: "Полиция 1",
            8: "Полиция 2",
            9: "Полиция 3"
        }
        return modes.get(self.led_mode, "Неизвестно")

# Глобальный экземпляр состояния
scooter = ScooterState()

# ============================================================================
# HTTP ОБРАБОТЧИК
# ============================================================================

class ScooterHTTPHandler(SimpleHTTPRequestHandler):
    """Обработчик HTTP запросов, эмулирующий ESP8266"""
    
    def __init__(self, *args, **kwargs):
        self.directory = CONFIG['static_dir']
        super().__init__(*args, **kwargs)
    
    def log_message(self, format, *args):
        """Кастомное логирование с цветами для терминала"""
        timestamp = time.strftime('%H:%M:%S')
        print(f"\033[36m[{timestamp}]\033[0m {self.client_address[0]} - {format % args}")
    
    def _send_json(self, data, status=200):
        """Отправка JSON ответа"""
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())
    
    def _send_html(self, html, status=200):
        """Отправка HTML ответа"""
        self.send_response(status)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(html.encode('utf-8'))
    
    def _get_arg(self, key, default=None):
        """Получение параметра из URL"""
        parsed_url = urlparse(self.path)
        params = parse_qs(parsed_url.query)
        return params.get(key, [default])[0]
    
    # ============================================================================
    # ОСНОВНЫЕ ОБРАБОТЧИКИ
    # ============================================================================
    
    def handle_root(self):
        """Главная страница"""
        try:
            with open(os.path.join(CONFIG['static_dir'], 'index.html'), 'r', encoding='utf-8') as f:
                html = f.read()
            self._send_html(html)
        except FileNotFoundError:
            self._send_json({
                'success': False,
                'message': 'HTML файл не найден. Создайте папку static/index.html'
            }, 500)
    
    def handle_status(self):
        """Статус системы"""
        self._send_json({
            'success': True,
            'isLocked': scooter.is_locked,
            'uptime': int(time.time() - scooter.start_time)
        })
    
    def handle_data(self):
        """Все данные самоката"""
        self._send_json({
            'success': True,
            'speed': scooter.scooter_speed,
            'battery': scooter.battery_total,
            'temperature': scooter.scooter_temperature,
            'mileage': scooter.total_mileage,
            'errorCode': scooter.scooter_error_code,
            'workMode': scooter.work_mode,
            'speedLimit': scooter.speed_limit,
            'headlightState': scooter.headlight_state,
            'beepState': scooter.beep_state,
            'cruiseControl': scooter.cruise_control,
            'engineState': scooter.engine_state,
            'battery1': scooter.battery1_capacity,
            'battery2': scooter.battery2_capacity,
            'actualRange': scooter.actual_range,
            'predictedRange': scooter.predicted_range,
            'singleMileage': scooter.single_mileage,
            'totalOperationTime': scooter.total_operation_time,
            'totalRideTime': scooter.total_ride_time,
            'singleRideTime': scooter.single_ride_time,
            'battery1Temp': scooter.battery1_temp,
            'battery2Temp': scooter.battery2_temp,
            'mosTemp': scooter.mos_temp,
            'driveVoltage': scooter.drive_voltage,
            'motorCurrent': scooter.motor_current,
            'avgSpeed': scooter.avg_speed,
            'power': scooter.scooter_power,
            'serial': scooter.scooter_serial,
            'bmsVersion': scooter.bms_version,
            'bms2Version': scooter.bms2_version,
            'bleVersion': scooter.ble_version,
            'boolStatus': scooter.get_bool_status_string(),
            'alarmStatus': scooter.get_alarm_string(),
            # Эмуляция неизвестных регистров
            'UnkReg1': random.randint(0, 100),
            'UnkReg2': random.randint(0, 200),
            'UnkReg3': random.randint(0, 50),
        })
    
    def handle_wifi_status(self):
        """Статус WiFi"""
        data = {
            'success': True,
            'mode': f"{scooter.wifi_mode} {'(Клиент)' if scooter.wifi_mode == 'STA' else '(Точка доступа)'}",
            'connected': scooter.wifi_connected,
            'ip': scooter.local_ip,
            'ssid': CONFIG['wifi_ssid_sta'] if scooter.wifi_mode == 'STA' else CONFIG['wifi_ssid_ap']
        }
        if scooter.wifi_mode == 'STA' and scooter.wifi_connected:
            data['signalStrength'] = scooter.signal_strength
            data['channel'] = 6
        self._send_json(data)
    
    # ============================================================================
    # КОМАНДЫ УПРАВЛЕНИЯ
    # ============================================================================
    
    def handle_unlock(self):
        scooter.is_locked = False
        scooter.bool_status &= ~0x0002  # Снимаем флаг блокировки
        self._send_json({'success': True, 'message': 'Успешно разблокировано'})
    
    def handle_lock(self):
        scooter.is_locked = True
        scooter.bool_status |= 0x0002  # Устанавливаем флаг блокировки
        scooter.scooter_speed = 0
        self._send_json({'success': True, 'message': 'Успешно заблокировано'})
    
    def handle_toggle(self):
        if scooter.is_locked:
            self.handle_unlock()
        else:
            self.handle_lock()
    
    def handle_open_deck(self):
        self._send_json({'success': True, 'message': 'Дека открыт'})
    
    def handle_mode_normal(self):
        scooter.work_mode = 0
        self._send_json({'success': True, 'message': 'Режим NORMAL'})
    
    def handle_mode_eco(self):
        scooter.work_mode = 1
        self._send_json({'success': True, 'message': 'Режим ECO'})
    
    def handle_mode_sport(self):
        scooter.work_mode = 2
        self._send_json({'success': True, 'message': 'Режим SPORT'})
    
    def handle_speed(self, limit):
        scooter.speed_limit = limit
        self._send_json({'success': True, 'message': f'Лимит скорости {limit // 10} км/ч'})
    
    def handle_headlight_toggle(self):
        scooter.headlight_state = not scooter.headlight_state
        state = "включены" if scooter.headlight_state else "выключены"
        self._send_json({'success': True, 'message': f'Фары {state}'})
    
    def handle_beep_toggle(self):
        scooter.beep_state = not scooter.beep_state
        state = "включен" if scooter.beep_state else "выключен"
        self._send_json({'success': True, 'message': f'Звук {state}'})
    
    def handle_cruise_toggle(self):
        scooter.cruise_control = not scooter.cruise_control
        state = "включен" if scooter.cruise_control else "выключен"
        self._send_json({'success': True, 'message': f'Круиз-контроль {state}'})
    
    def handle_engine(self, state):
        scooter.engine_state = state
        msg = "включен" if state else "выключен"
        self._send_json({'success': True, 'message': f'Двигатель {msg}'})
    
    def handle_led_mode(self):
        mode = int(self._get_arg('mode', 1))
        scooter.led_mode = mode
        self._send_json({'success': True, 'message': 'Режим подсветки установлен'})
    
    def handle_led_color(self):
        color = self._get_arg('color', 'A0F0')
        index = int(self._get_arg('index', 1))
        color_int = int(color, 16)
        colors = {1: 'led_color1', 2: 'led_color2', 3: 'led_color3', 4: 'led_color4'}
        if index in colors:
            setattr(scooter, colors[index], color_int)
        self._send_json({'success': True, 'message': 'Цвет установлен'})
    
    def handle_find_scooter(self):
        self._send_json({'success': True, 'message': 'Поиск самоката активирован'})
    
    def handle_reboot(self):
        self._send_json({'success': True, 'message': 'Система перезагружается'})
    
    def handle_power_off(self):
        self._send_json({'success': True, 'message': 'Система выключается'})
    
    def handle_toggle_limit(self):
        self._send_json({'success': True, 'message': 'Лимит скорости переключен'})
    
    def handle_firmware_info(self):
        self._send_json({
            'success': True,
            'version': '1.2.0',
            'chip_id': 'EMULATOR-001',
            'free_heap': 1024000,
            'sketch_size': 512000,
            'free_sketch_space': 512000,
            'sdk_version': '3.0.0',
            'core_version': '3.0.0',
            'flash_size': 4096000,
            'cycle_count': 1000000
        })
    
    def handle_check_updates(self):
        self._send_json({
            'success': True,
            'update_available': False,
            'current_version': '1.2.0',
            'latest_version': '1.2.0',
            'message': 'У вас установлена последняя версия'
        })
    
    def handle_scan_read(self):
        index = self._get_arg('index', '0')
        value = random.randint(0, 65535) if random.random() > 0.1 else -1
        
        if value != -1:
            self._send_json({
                'success': True,
                'index': index,
                'value': value,
                'valueHex': hex(value),
                'type': 'UINT16'
            })
        else:
            self._send_json({
                'success': False,
                'index': index,
                'message': 'Read failed or timeout'
            })
    
    def handle_scan_write(self):
        index = self._get_arg('index', '0')
        value = self._get_arg('value', '0')
        self._send_json({
            'success': True,
            'index': index,
            'valueWritten': value,
            'valueReadback': int(value, 16),
            'valueReadbackHex': value,
            'message': 'Write successful'
        })
    
    def handle_wifi_toggle(self):
        if scooter.wifi_mode == 'AP':
            scooter.wifi_mode = 'STA'
            scooter.local_ip = '192.168.1.100'
            scooter.signal_strength = random.randint(-60, -30)
        else:
            scooter.wifi_mode = 'AP'
            scooter.local_ip = '192.168.4.1'
            scooter.signal_strength = -40
        self._send_json({'success': True, 'message': f'Переключено в {scooter.wifi_mode}'})
    
    def handle_unknown(self):
        self._send_json({
            'success': False,
            'message': 'Страница не найдена'
        }, 404)
    
    # ============================================================================
    # МАРШРУТИЗАЦИЯ
    # ============================================================================
    
    def do_GET(self):
        """Обработка GET запросов"""
        path = urlparse(self.path).path
        
        routes = {
            '/': self.handle_root,
            '/status': self.handle_status,
            '/data': self.handle_data,
            '/wifi_status': self.handle_wifi_status,
            '/wifi_toggle': self.handle_wifi_toggle,
            '/unlock': self.handle_unlock,
            '/lock': self.handle_lock,
            '/toggle': self.handle_toggle,
            '/open_deck': self.handle_open_deck,
            '/mode_normal': self.handle_mode_normal,
            '/mode_eco': self.handle_mode_eco,
            '/mode_sport': self.handle_mode_sport,
            '/speed_15': lambda: self.handle_speed(150),
            '/speed_20': lambda: self.handle_speed(200),
            '/speed_25': lambda: self.handle_speed(250),
            '/speed_30': lambda: self.handle_speed(300),
            '/headlight_toggle': self.handle_headlight_toggle,
            '/beep_toggle': self.handle_beep_toggle,
            '/cruise_toggle': self.handle_cruise_toggle,
            '/engine_on': lambda: self.handle_engine(True),
            '/engine_off': lambda: self.handle_engine(False),
            '/led_mode': self.handle_led_mode,
            '/led_color': self.handle_led_color,
            '/find_scooter': self.handle_find_scooter,
            '/reboot': self.handle_reboot,
            '/power_off': self.handle_power_off,
            '/toggle_limit': self.handle_toggle_limit,
            '/firmware_info': self.handle_firmware_info,
            '/check_updates': self.handle_check_updates,
            '/scan_read': self.handle_scan_read,
            '/scan_write': self.handle_scan_write,
        }
        
        handler = routes.get(path)
        if handler:
            handler()
        else:
            # Пробуем отдать статический файл
            self.serve_static()
    
    def do_POST(self):
        """Обработка POST запросов (для OTA и других)"""
        path = urlparse(self.path).path
        
        if path == CONFIG['ota_path']:
            self._send_json({
                'success': True,
                'message': 'OTA обновление успешно (эмулятор)'
            })
        else:
            self.do_GET()
    
    def serve_static(self):
        """Отдача статических файлов"""
        path = urlparse(self.path).path.lstrip('/')
        if not path:
            path = 'index.html'
        
        file_path = os.path.join(CONFIG['static_dir'], path)
        if os.path.exists(file_path) and os.path.isfile(file_path):
            content_type = 'text/html'
            if path.endswith('.css'):
                content_type = 'text/css'
            elif path.endswith('.js'):
                content_type = 'application/javascript'
            elif path.endswith('.png'):
                content_type = 'image/png'
            elif path.endswith('.jpg') or path.endswith('.jpeg'):
                content_type = 'image/jpeg'
            
            with open(file_path, 'rb') as f:
                content = f.read()
            
            self.send_response(200)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        else:
            self.handle_unknown()

# ============================================================================
# ЗАПУСК СЕРВЕРА
# ============================================================================

def create_static_files():
    """Создание базовых статических файлов если их нет"""
    if not os.path.exists(CONFIG['static_dir']):
        os.makedirs(CONFIG['static_dir'])
    
    # Создаем простой index.html если его нет
    index_path = os.path.join(CONFIG['static_dir'], 'index.html')
    if not os.path.exists(index_path):
        with open(index_path, 'w', encoding='utf-8') as f:
            f.write("""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Ninebot Scooter Emulator</title>
    <style>
        body { font-family: Arial; padding: 20px; background: #1a1a2e; color: #eee; }
        .card { background: #16213e; padding: 15px; margin: 10px 0; border-radius: 8px; }
        button { background: #0f3460; color: white; border: none; padding: 10px 15px; 
                 margin: 5px; border-radius: 5px; cursor: pointer; }
        button:hover { background: #1a508b; }
        .data { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; }
        .value { color: #e94560; font-weight: bold; }
    </style>
</head>
<body>
    <h1>🛴 Ninebot Scooter Emulator</h1>
    
    <div class="card">
        <h2>Управление</h2>
        <button onclick="fetch('/unlock')">🔓 Разблокировать</button>
        <button onclick="fetch('/lock')">🔒 Заблокировать</button>
        <button onclick="fetch('/toggle')">🔄 Переключить</button>
        <button onclick="fetch('/open_deck')">📦 Открыть деку</button>
    </div>
    
    <div class="card">
        <h2>Режимы</h2>
        <button onclick="fetch('/mode_normal')">Normal</button>
        <button onclick="fetch('/mode_eco')">Eco</button>
        <button onclick="fetch('/mode_sport')">Sport</button>
    </div>
    
    <div class="card">
        <h2>Скорость</h2>
        <button onclick="fetch('/speed_15')">15 km/h</button>
        <button onclick="fetch('/speed_20')">20 km/h</button>
        <button onclick="fetch('/speed_25')">25 km/h</button>
        <button onclick="fetch('/speed_30')">30 km/h</button>
    </div>
    
    <div class="card">
        <h2>Функции</h2>
        <button onclick="fetch('/headlight_toggle')">💡 Фары</button>
        <button onclick="fetch('/beep_toggle')">🔊 Звук</button>
        <button onclick="fetch('/cruise_toggle')">🚗 Круиз</button>
        <button onclick="fetch('/engine_on')">▶️ Двигатель ON</button>
        <button onclick="fetch('/engine_off')">⏹️ Двигатель OFF</button>
    </div>
    
    <div class="card" id="status">
        <h2>Статус</h2>
        <div class="data" id="statusData">Загрузка...</div>
    </div>
    
    <script>
        async function fetch(url) {
            try {
                const response = await fetch(url);
                const data = await response.json();
                console.log(data);
                updateStatus();
            } catch (e) {
                console.error('Error:', e);
            }
        }
        
        async function updateStatus() {
            try {
                const response = await fetch('/data');
                const data = await response.json();
                document.getElementById('statusData').innerHTML = `
                    <div>Скорость: <span class="value">${data.speed} km/h</span></div>
                    <div>Батарея: <span class="value">${data.battery}%</span></div>
                    <div>Пробег: <span class="value">${data.mileage} m</span></div>
                    <div>Режим: <span class="value">${data.workMode}</span></div>
                    <div>Фары: <span class="value">${data.headlightState ? 'Вкл' : 'Выкл'}</span></div>
                    <div>Двигатель: <span class="value">${data.engineState ? 'Вкл' : 'Выкл'}</span></div>
                    <div>Серийный: <span class="value">${data.serial}</span></div>
                `;
            } catch (e) {
                console.error('Error:', e);
            }
        }
        
        setInterval(updateStatus, 2000);
        updateStatus();
    </script>
</body>
</html>""")
        print(f"\033[32m✅ Создан {index_path}\033[0m")
    
    print(f"\033[32m✅ Статические файлы готовы\033[0m")

def main():
    """Запуск эмулятора"""
    print("\033[35m" + "="*60)
    print("🛴  ЭМУЛЯТОР ESP8266 ДЛЯ NINEBOT SCOOTER")
    print("="*60 + "\033[0m")
    
    # Создаем статические файлы
    create_static_files()
    
    # Запускаем сервер
    server = HTTPServer(('0.0.0.0', CONFIG['port']), ScooterHTTPHandler)
    
    print(f"\n\033[32m✅ Сервер запущен на http://localhost:{CONFIG['port']}")
    print(f"📁 Статические файлы: {os.path.abspath(CONFIG['static_dir'])}")
    print(f"🔧 WiFi режим: {scooter.wifi_mode}")
    print(f"📡 IP адрес: {scooter.local_ip}")
    print("\n\033[33mНажмите Ctrl+C для остановки\033[0m\n")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n\033[31m⏹️  Остановка сервера...\033[0m")
        scooter.running = False
        server.shutdown()

if __name__ == '__main__':
    main()