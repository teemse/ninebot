    let autoRefreshInterval = null;
    let currentStatus = true;
    let currentLedMode = 1;
    let scanInProgress = false;
    let packetLogging = false;
    let packetLog = [];
    let otaInProgress = false;

// Загрузка системной информации
async function loadSystemInfo() {
    try {
        const response = await fetch('/firmware_info');
        const data = await response.json();
        
        if (data.success) {
            document.getElementById('systemInfo').innerHTML = `
                <p><strong>Версия прошивки:</strong> ${data.version || 'N/A'}</p>
                <p><strong>ID устройства:</strong> ${data.chip_id || 'N/A'}</p>
                <p><strong>Свободная память:</strong> ${Math.round(data.free_heap/1024)} KB</p>
                <p><strong>Размер прошивки:</strong> ${Math.round(data.sketch_size/1024)} KB</p>
                <p><strong>Свободное место:</strong> ${Math.round(data.free_sketch_space/1024)} KB</p>
                <p><strong>Flash размер:</strong> ${Math.round(data.flash_size/1024)} KB</p>
            `;
            document.getElementById('currentVersion').textContent = data.version || 'N/A';
            document.getElementById('freeMemory').textContent = Math.round(data.free_heap/1024) + ' KB';
            document.getElementById('deviceId').textContent = data.chip_id || 'N/A';
        }
    } catch (error) {
        showNotification('Ошибка загрузки системной информации', true);
    }
}

// Обработка формы OTA
document.getElementById('otaForm').addEventListener('submit', async function(e) {
    e.preventDefault();
    
    if (otaInProgress) {
        showNotification('Обновление уже выполняется', true);
        return;
    }
    
    const fileInput = document.querySelector('input[type="file"]');
    const file = fileInput.files[0];
    
    if (!file) {
        showNotification('Выберите файл прошивки', true);
        return;
    }
    
    if (!file.name.endsWith('.bin')) {
        showNotification('Файл должен иметь расширение .bin', true);
        return;
    }
    
    if (!confirm(`🚀 Начать обновление прошивки?\n\nФайл: ${file.name}\nРазмер: ${Math.round(file.size/1024)} KB\n\nПосле начала обновление нельзя прервать!`)) {
        return;
    }
    
    await uploadFirmware(file);
});

// Загрузка прошивки
async function uploadFirmware(file) {
    otaInProgress = true;
    const formData = new FormData();
    formData.append('firmware', file);
    
    // Показываем прогресс
    document.getElementById('otaProgress').style.display = 'block';
    document.getElementById('uploadBtn').disabled = true;
    updateOtaProgress(0, 'Начало загрузки...');
    
    try {
        const response = await fetch('/firmware', {
            method: 'POST',
            body: formData,
            credentials: 'include' // Для базовой аутентификации
        });
        
        if (response.ok) {
            updateOtaProgress(100, '✅ Обновление успешно! Перезагрузка...');
            showNotification('Прошивка успешно обновлена! Устройство перезагружается...');
            
            // Перезагрузка через 3 секунды
            setTimeout(() => {
                window.location.reload();
            }, 3000);
        } else {
            throw new Error('Ошибка сервера: ' + response.status);
        }
    } catch (error) {
        updateOtaProgress(0, '❌ Ошибка обновления: ' + error.message);
        showNotification('Ошибка обновления: ' + error.message, true);
        otaInProgress = false;
        document.getElementById('uploadBtn').disabled = false;
    }
}

// Обновление прогресса OTA
function updateOtaProgress(percent, message) {
    document.getElementById('otaProgressBar').style.width = percent + '%';
    document.getElementById('otaProgressText').textContent = percent + '%';
    document.getElementById('otaStatus').textContent = message;
    
    // Добавляем в детали
    const details = document.getElementById('otaDetails');
    const timestamp = new Date().toLocaleTimeString();
    details.innerHTML += `[${timestamp}] ${message}\n`;
    details.scrollTop = details.scrollHeight;
}

// Проверка обновлений
async function checkForUpdates() {
    try {
        showNotification('🔍 Проверка обновлений...');
        
        const response = await fetch('/check_updates');
        const data = await response.json();
        
        if (data.success) {
            if (data.update_available) {
                showNotification(`🎯 Доступно обновление! Текущая: ${data.current_version}, Новая: ${data.latest_version}`);
            } else {
                showNotification('✅ У вас установлена последняя версия прошивки');
            }
        }
    } catch (error) {
        showNotification('Ошибка проверки обновлений', true);
    }
}

// Тест OTA соединения
async function testOTAConnection() {
    try {
        showNotification('🧪 Тестирование OTA соединения...');
        
        // Попытка доступа к OTA странице
        const response = await fetch('/update', {
            method: 'HEAD',
            credentials: 'include'
        });
        
        if (response.status === 200 || response.status === 401) {
            showNotification('✅ OTA интерфейс доступен');
        } else {
            showNotification('❌ OTA интерфейс недоступен', true);
        }
    } catch (error) {
        showNotification('❌ Ошибка доступа к OTA', true);
    }
}

// Показать инструкцию
function showOTAInstructions() {
    const instructions = `
🚀 ИНСТРУКЦИЯ OTA ОБНОВЛЕНИЯ:

1. 📤 Нажмите "Открыть OTA интерфейс"
2. 🔐 Введите логин и пароль:
   - Логин: admin
   - Пароль: NinebotOTA123!
3. 📁 Выбете файл прошивки (.bin)
4. ⚡ Нажмите "Update"
5. ⏳ Ждите завершения процесса
6. 🔄 Устройство перезагрузится автоматически

⚠️ ВАЖНО:
- Не прерывайте процесс обновления
- Убедитесь в стабильном WiFi соединении
- Используйте правильный файл прошивки
    `;
    
    alert(instructions);
}

// Обработка формы (альтернативный метод)
document.getElementById('otaForm').addEventListener('submit', function(e) {
    if (!confirm('🚀 Начать процесс OTA обновления?\n\nБудет открыта стандартная страница обновления.')) {
        e.preventDefault();
        return;
    }
    showNotification('📤 Открытие OTA интерфейса...');
});

// Автоматическая загрузка информации при открытии вкладки
function switchTab(tabName) {
    // Существующий код переключения вкладок...
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
    });
    document.querySelectorAll('.tab-button').forEach(button => {
        button.classList.remove('active');
    });

    document.getElementById(tabName + '-tab').classList.add('active');
    event.target.classList.add('active');
    
    // Автоматически загружаем информацию при открытии вкладки OTA
    if (tabName === 'ota') {
        loadSystemInfo();
    }
}

// Настройки автоматических обновлений
function toggleAutoUpdate(enabled) {
    showNotification(enabled ? 'Автопроверка обновлений включена' : 'Автопроверка обновлений выключена');
    // Здесь можно сохранить настройку
}

function toggleUpdateNotifications(enabled) {
    showNotification(enabled ? 'Уведомления включены' : 'Уведомления выключены');
    // Здесь можно сохранить настройку
}

// Сброс настроек
function resetSettings() {
    if (confirm('Вы уверены, что хотите сбросить настройки OTA?')) {
        showNotification('Настройки OTA сброшены');
        document.getElementById('autoUpdateCheck').checked = false;
        document.getElementById('updateNotifications').checked = true;
    }
}

// Фабричный сброс
function factoryReset() {
    if (confirm('🚨 ВНИМАНИЕ: Это приведет к полному сбросу всех настроек! Продолжить?')) {
        showNotification('Фабричный сброс запущен...', true);
        // Здесь можно вызвать API для сброса
    }
}

// Автоматическая загрузка системной информации при открытии вкладки
function switchTab(tabName) {
    // Существующий код переключения вкладок...
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
    });
    document.querySelectorAll('.tab-button').forEach(button => {
        button.classList.remove('active');
    });

    document.getElementById(tabName + '-tab').classList.add('active');
    event.target.classList.add('active');
    
    // Автоматически загружаем информацию при открытии вкладки OTA
    if (tabName === 'ota') {
        loadSystemInfo();
    }
}


    async function startSafeScan() {
        if (scanInProgress) {
            showNotification('Сканирование уже выполняется', true);
            return;
        }

        scanInProgress = true;
        const results = document.getElementById('scanResults');
        results.innerHTML = '';
        updateScanProgress(0, 256);

        let foundCount = 0;

        for (let index = 0; index <= 0xFF; index++) {
            if (!scanInProgress) break;

            updateScanProgress(index + 1, 256);
            document.getElementById('scanStatus').textContent = `Сканирование: 0x${index.toString(16).toUpperCase()}`;

            try {
                const response = await fetch(`/scan_read?index=0x${index.toString(16)}`);
                const data = await response.json();

                if (data.success && data.value !== undefined) {
                    foundCount++;
                    addScanResult(`🎯 0x${index.toString(16).toUpperCase()}: ${data.value} (0x${data.value.toString(16)})`, 'found');
                    addFoundRegister(index, data.value, 'read');
                }
            } catch (error) {
                addScanResult(`❌ 0x${index.toString(16).toUpperCase()}: Ошибка чтения`, 'error');
            }

            // Задержка между запросами
            await new Promise(resolve => setTimeout(resolve, 50));
        }

        scanInProgress = false;
        showNotification(`Безопасное сканирование завершено! Найдено: ${foundCount} регистров`);
    }

    // Брутфорс сканирование (запись с ответом)
    async function startBruteForceScan() {
        if (!confirm('⚠️ ВНИМАНИЕ: Брутфорс сканирование может вызвать непредсказуемое поведение самоката! Продолжить?')) {
            return;
        }

        if (scanInProgress) {
            showNotification('Сканирование уже выполняется', true);
            return;
        }

        scanInProgress = true;
        const results = document.getElementById('scanResults');
        results.innerHTML = '';
        updateScanProgress(0, 256);

        let foundCount = 0;

        for (let index = 0; index <= 0xFF; index++) {
            if (!scanInProgress) break;

            updateScanProgress(index + 1, 256);
            document.getElementById('scanStatus').textContent = `Брутфорс: 0x${index.toString(16).toUpperCase()}`;

            try {
                const response = await fetch(`/scan_write?index=0x${index.toString(16)}&value=0x0001`);
                const data = await response.json();

                if (data.success) {
                    foundCount++;
                    addScanResult(`🎯 0x${index.toString(16).toUpperCase()}: Записываемый регистр!`, 'found');
                    addFoundRegister(index, 1, 'write');
                }
            } catch (error) {
                addScanResult(`❌ 0x${index.toString(16).toUpperCase()}: Ошибка записи`, 'error');
            }

            await new Promise(resolve => setTimeout(resolve, 100));
        }

        scanInProgress = false;
        showNotification(`Брутфорс сканирование завершено! Найдено: ${foundCount} регистров`);
    }

    // Глубокое сканирование
    async function startDeepScan() {
        if (!confirm('🚨 ОПАСНО: Глубокое сканирование может повредить настройки самоката! Продолжить?')) {
            return;
        }
        showNotification('Глубокое сканирование пока не реализовано', true);
    }

    // Ручное тестирование
    async function manualTest() {
        const indexInput = document.getElementById('manualIndex').value;
        const valueInput = document.getElementById('manualValue').value;

        if (!indexInput) {
            showNotification('Введите индекс для тестирования', true);
            return;
        }

        // Преобразуем индекс в число
        const index = parseInt(indexInput.replace('0x', ''), 16);
        if (isNaN(index)) {
            showNotification('Неверный формат индекса', true);
            return;
        }

        try {
            let response, data;

            if (valueInput) {
                // Тест записи
                const value = parseInt(valueInput.replace('0x', ''), 16);
                response = await fetch(`/scan_write?index=0x${index.toString(16)}&value=0x${value.toString(16)}`);
            } else {
                // Тест чтения
                response = await fetch(`/scan_read?index=0x${index.toString(16)}`);
            }

            data = await response.json();
            
            if (data.success) {
                showNotification(`✅ Успех! ${valueInput ? 'Запись' : 'Чтение'} выполнена`);
                addScanResult(`🔧 0x${index.toString(16).toUpperCase()}: ${valueInput ? `Запись 0x${value.toString(16)}` : `Чтение ${data.value}`}`, 'found');
            } else {
                showNotification(`❌ Ошибка: ${data.message}`, true);
            }
        } catch (error) {
            showNotification('Ошибка соединения', true);
        }
    }

    // Логирование пакетов
    function startPacketLog() {
        packetLogging = true;
        packetLog = [];
        showNotification('Логирование пакетов запущено');
        
        // Симуляция получения пакетов (в реальности будет через WebSocket)
        simulatePacketLog();
    }

    function stopPacketLog() {
        packetLogging = false;
        showNotification('Логирование пакетов остановлено');
    }

    function clearPacketLog() {
        document.getElementById('packetLog').innerHTML = '';
    }

    function simulatePacketLog() {
        if (!packetLogging) return;
        
        // В реальности здесь будет WebSocket соединение
        setTimeout(() => {
            if (packetLogging) {
                const logEntry = `[${new Date().toLocaleTimeString()}] Пакет: 5A A5 02 3D 20 04 3E 36 01`;
                packetLog.push(logEntry);
                
                const logElement = document.getElementById('packetLog');
                logElement.innerHTML = packetLog.slice(-10).join('<br>');
                logElement.scrollTop = logElement.scrollHeight;
                
                simulatePacketLog();
            }
        }, 1000);
    }

    // Вспомогательные функции
    function updateScanProgress(current, total) {
        const progress = (current / total) * 100;
        document.getElementById('scanProgress').style.width = `${progress}%`;
        document.getElementById('scanProgressText').textContent = `${current}/${total}`;
    }

    function addScanResult(message, type = '') {
        const results = document.getElementById('scanResults');
        const item = document.createElement('div');
        item.className = `scan-result-item ${type}`;
        item.textContent = message;
        results.appendChild(item);
        results.scrollTop = results.scrollHeight;
    }

    function addFoundRegister(index, value, type) {
        const foundElement = document.getElementById('foundRegisters');
        const registerInfo = document.createElement('div');
        registerInfo.style.padding = '8px';
        registerInfo.style.borderBottom = '1px solid #eee';
        registerInfo.style.fontFamily = 'monospace';
        
        const typeIcon = type === 'read' ? '📖' : '✏️';
        registerInfo.innerHTML = `${typeIcon} <strong>0x${index.toString(16).toUpperCase()}</strong>: ${value} <span style="color: #666;">(${type})</span>`;
        
        foundElement.appendChild(registerInfo);
    }

    // Инициализация
    document.addEventListener('DOMContentLoaded', function() {
      loadStatus();
      startAutoRefresh();
      updateLedModeButtons();
    // Предзагрузка системной информации
      setTimeout(loadSystemInfo, 1000);
    });

// Безопасное сканирование (только чтение)
    async function startSafeScan() {
        if (scanInProgress) {
            showNotification('Сканирование уже выполняется', true);
            return;
        }

        scanInProgress = true;
        const results = document.getElementById('scanResults');
        results.innerHTML = '';
        updateScanProgress(0, 256);

        let foundCount = 0;

        for (let index = 0; index <= 0xFF; index++) {
            if (!scanInProgress) break;

            updateScanProgress(index + 1, 256);
            document.getElementById('scanStatus').textContent = `Сканирование: 0x${index.toString(16).toUpperCase()}`;

            try {
                const response = await fetch(`/scan_read?index=0x${index.toString(16)}`);
                const data = await response.json();

                if (data.success && data.value !== undefined) {
                    foundCount++;
                    addScanResult(`🎯 0x${index.toString(16).toUpperCase()}: ${data.value} (0x${data.value.toString(16)})`, 'found');
                    addFoundRegister(index, data.value, 'read');
                }
            } catch (error) {
                addScanResult(`❌ 0x${index.toString(16).toUpperCase()}: Ошибка чтения`, 'error');
            }

            // Задержка между запросами
            await new Promise(resolve => setTimeout(resolve, 50));
        }

        scanInProgress = false;
        showNotification(`Безопасное сканирование завершено! Найдено: ${foundCount} регистров`);
    }

    // Брутфорс сканирование (запись с ответом)
    async function startBruteForceScan() {
        if (!confirm('⚠️ ВНИМАНИЕ: Брутфорс сканирование может вызвать непредсказуемое поведение самоката! Продолжить?')) {
            return;
        }

        if (scanInProgress) {
            showNotification('Сканирование уже выполняется', true);
            return;
        }

        scanInProgress = true;
        const results = document.getElementById('scanResults');
        results.innerHTML = '';
        updateScanProgress(0, 256);

        let foundCount = 0;

        for (let index = 0; index <= 0xFF; index++) {
            if (!scanInProgress) break;

            updateScanProgress(index + 1, 256);
            document.getElementById('scanStatus').textContent = `Брутфорс: 0x${index.toString(16).toUpperCase()}`;

            try {
                const response = await fetch(`/scan_write?index=0x${index.toString(16)}&value=0x0001`);
                const data = await response.json();

                if (data.success) {
                    foundCount++;
                    addScanResult(`🎯 0x${index.toString(16).toUpperCase()}: Записываемый регистр!`, 'found');
                    addFoundRegister(index, 1, 'write');
                }
            } catch (error) {
                addScanResult(`❌ 0x${index.toString(16).toUpperCase()}: Ошибка записи`, 'error');
            }

            await new Promise(resolve => setTimeout(resolve, 100));
        }

        scanInProgress = false;
        showNotification(`Брутфорс сканирование завершено! Найдено: ${foundCount} регистров`);
    }

    // Глубокое сканирование
    async function startDeepScan() {
        if (!confirm('🚨 ОПАСНО: Глубокое сканирование может повредить настройки самоката! Продолжить?')) {
            return;
        }
        showNotification('Глубокое сканирование пока не реализовано', true);
    }

    // Ручное тестирование
    async function manualTest() {
        const indexInput = document.getElementById('manualIndex').value;
        const valueInput = document.getElementById('manualValue').value;

        if (!indexInput) {
            showNotification('Введите индекс для тестирования', true);
            return;
        }

        // Преобразуем индекс в число
        const index = parseInt(indexInput.replace('0x', ''), 16);
        if (isNaN(index)) {
            showNotification('Неверный формат индекса', true);
            return;
        }

        try {
            let response, data;

            if (valueInput) {
                // Тест записи
                const value = parseInt(valueInput.replace('0x', ''), 16);
                response = await fetch(`/scan_write?index=0x${index.toString(16)}&value=0x${value.toString(16)}`);
            } else {
                // Тест чтения
                response = await fetch(`/scan_read?index=0x${index.toString(16)}`);
            }

            data = await response.json();
            
            if (data.success) {
                showNotification(`✅ Успех! ${valueInput ? 'Запись' : 'Чтение'} выполнена`);
                addScanResult(`🔧 0x${index.toString(16).toUpperCase()}: ${valueInput ? `Запись 0x${value.toString(16)}` : `Чтение ${data.value}`}`, 'found');
            } else {
                showNotification(`❌ Ошибка: ${data.message}`, true);
            }
        } catch (error) {
            showNotification('Ошибка соединения', true);
        }
    }

    // Логирование пакетов
    function startPacketLog() {
        packetLogging = true;
        packetLog = [];
        showNotification('Логирование пакетов запущено');
        
        // Симуляция получения пакетов (в реальности будет через WebSocket)
        simulatePacketLog();
    }

    function stopPacketLog() {
        packetLogging = false;
        showNotification('Логирование пакетов остановлено');
    }

    function clearPacketLog() {
        document.getElementById('packetLog').innerHTML = '';
    }

    function simulatePacketLog() {
        if (!packetLogging) return;
        
        // В реальности здесь будет WebSocket соединение
        setTimeout(() => {
            if (packetLogging) {
                const logEntry = `[${new Date().toLocaleTimeString()}] Пакет: 5A A5 02 3D 20 04 3E 36 01`;
                packetLog.push(logEntry);
                
                const logElement = document.getElementById('packetLog');
                logElement.innerHTML = packetLog.slice(-10).join('<br>');
                logElement.scrollTop = logElement.scrollHeight;
                
                simulatePacketLog();
            }
        }, 1000);
    }

    // Переключение вкладок
    function switchTab(tabName) {
      // Скрыть все вкладки
      document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
      });
      document.querySelectorAll('.tab-button').forEach(button => {
        button.classList.remove('active');
      });

      // Показать выбранную вкладку
      document.getElementById(tabName + '-tab').classList.add('active');
      event.target.classList.add('active');
    }

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
      // Основные данные
      document.getElementById('batteryValue').textContent = data.battery + '%';
      document.getElementById('speedValue').textContent = data.speed;
      document.getElementById('tempValue').textContent = data.temperature + '°';
      document.getElementById('mileageValue').textContent = data.mileage;
      
      // Обновляем состояния кнопок
      updateButtonState('headlightBtn', '💡 Фары', data.headlightState);
      updateButtonState('beepBtn', '🔊 Звук', data.beepState);
      updateButtonState('cruiseBtn', '⏱️ Круиз', data.cruiseControl);
      updateButtonState('engineBtn', '⚡ Двигатель', data.engineState);

      // Расширенные данные
      document.getElementById('driveVoltageValue').textContent = data.driveVoltage;
      document.getElementById('motorCurrentValue').textContent = data.motorCurrent;
      document.getElementById('powerValue').textContent = data.power;
      document.getElementById('avgSpeedValue').textContent = data.avgSpeed;
      
      // Температуры
      document.getElementById('bodyTempValue').textContent = data.temperature + '°';
      document.getElementById('bat1TempValue').textContent = data.battery1Temp + '°';
      document.getElementById('bat2TempValue').textContent = data.battery2Temp + '°';
      document.getElementById('mosTempValue').textContent = data.mosTemp + '°';
      
      // Батареи
      document.getElementById('battery1Value').textContent = data.battery1 + '%';
      document.getElementById('battery2Value').textContent = data.battery2 + '%';
      document.getElementById('actualRangeValue').textContent = data.actualRange;
      document.getElementById('predictedRangeValue').textContent = data.predictedRange;
      
      // Системная информация
      document.getElementById('serialValue').textContent = data.serial || 'N/A';
      document.getElementById('bmsVersionValue').textContent = data.bmsVersion || 'N/A';
      document.getElementById('bms2VersionValue').textContent = data.bms2Version || 'N/A';
      document.getElementById('bleVersionValue').textContent = data.bleVersion || 'N/A';
      document.getElementById('boolStatusValue').textContent = data.boolStatus || 'N/A';
      document.getElementById('alarmStatusValue').textContent = data.alarmStatus || 'Нет';
      document.getElementById('errorValue').textContent = data.errorCode || '0';
      
      // Статистика
      document.getElementById('singleMileageValue').textContent = data.singleMileage;
      document.getElementById('singleRideTimeValue').textContent = data.singleRideTime;
      document.getElementById('totalOperationTimeValue').textContent = data.totalOperationTime;
      document.getElementById('totalRideTimeValue').textContent = data.totalRideTime;
      
      document.getElementById('workModeValue').textContent = getWorkModeName(data.workMode);
      document.getElementById('speedLimitValue').textContent = data.speedLimit;
      document.getElementById('errorCodeValue').textContent = data.errorCode;
      document.getElementById('cruiseValue').textContent = data.cruiseControl ? 'ВКЛ' : 'ВЫКЛ';
      document.getElementById('headlightValue').textContent = data.headlightState ? 'ВКЛ' : 'ВЫКЛ';
      document.getElementById('beepValue').textContent = data.beepState ? 'ВКЛ' : 'ВЫКЛ';

      // Системная информация
      document.getElementById('lockStatus').textContent = data.isLocked ? 'Заблокирован' : 'Разблокирован';
      document.getElementById('engineStatus').textContent = data.engineState ? 'ВКЛ' : 'ВЫКЛ';
      document.getElementById('currentLedMode').textContent = getLedModeName(data.ledMode || 1);
      
      // Неизвестные регистры
      document.getElementById('UnkReg1').textContent = data.UnkReg1 || 'N/A';
      document.getElementById('UnkReg2').textContent = data.UnkReg2 || 'N/A';
      document.getElementById('UnkReg3').textContent = data.UnkReg3 || 'N/A';
      document.getElementById('UnkReg4').textContent = data.UnkReg4 || 'N/A';
      document.getElementById('UnkReg5').textContent = data.UnkReg5 || 'N/A';
      document.getElementById('UnkReg6').textContent = data.UnkReg6 || 'N/A';
      document.getElementById('UnkReg7').textContent = data.UnkReg7 || 'N/A';
      document.getElementById('UnkReg8').textContent = data.UnkReg8 || 'N/A';
      document.getElementById('UnkReg9').textContent = data.UnkReg9 || 'N/A';
      document.getElementById('UnkReg10').textContent = data.UnkReg10 || 'N/A';
      document.getElementById('UnkReg11').textContent = data.UnkReg11 || 'N/A';
      document.getElementById('UnkReg12').textContent = data.UnkReg12 || 'N/A';
      document.getElementById('UnkReg13').textContent = data.UnkReg13 || 'N/A';
      document.getElementById('UnkReg14').textContent = data.UnkReg14 || 'N/A';
      document.getElementById('UnkReg15').textContent = data.UnkReg15 || 'N/A';
      document.getElementById('UnkReg16').textContent = data.UnkReg16 || 'N/A';
      document.getElementById('UnkReg17').textContent = data.UnkReg17 || 'N/A';
      document.getElementById('UnkReg18').textContent = data.UnkReg18 || 'N/A';
      document.getElementById('UnkReg19').textContent = data.UnkReg19 || 'N/A';
      document.getElementById('UnkReg20').textContent = data.UnkReg20 || 'N/A';
      document.getElementById('UnkReg21').textContent = data.UnkReg21 || 'N/A';
      document.getElementById('UnkReg22').textContent = data.UnkReg22 || 'N/A';
      document.getElementById('UnkReg23').textContent = data.UnkReg23 || 'N/A';
      document.getElementById('UnkReg24').textContent = data.UnkReg24 || 'N/A';
      document.getElementById('UnkReg25').textContent = data.UnkReg25 || 'N/A';
      document.getElementById('UnkReg26').textContent = data.UnkReg26 || 'N/A';
      document.getElementById('UnkReg27').textContent = data.UnkReg27 || 'N/A';
      document.getElementById('UnkReg28').textContent = data.UnkReg28 || 'N/A';
      document.getElementById('UnkReg29').textContent = data.UnkReg29 || 'N/A';
      document.getElementById('UnkReg30').textContent = data.UnkReg30 || 'N/A';
      document.getElementById('UnkReg31').textContent = data.UnkReg31 || 'N/A';
      document.getElementById('UnkReg32').textContent = data.UnkReg32 || 'N/A';
      document.getElementById('UnkReg33').textContent = data.UnkReg33 || 'N/A';
      document.getElementById('UnkReg34').textContent = data.UnkReg34 || 'N/A';
      document.getElementById('UnkReg35').textContent = data.UnkReg35 || 'N/A';
      document.getElementById('UnkReg36').textContent = data.UnkReg36 || 'N/A';
      document.getElementById('UnkReg37').textContent = data.UnkReg37 || 'N/A';
      document.getElementById('UnkReg38').textContent = data.UnkReg38 || 'N/A';
      document.getElementById('UnkReg39').textContent = data.UnkReg39 || 'N/A';
      


      
      // Обновляем переключатели настроек
      updateSettingsSwitches(data);
    }

    function getWorkModeName(mode) {
      switch(mode) {
        case 0: return 'NORMAL';
        case 1: return 'ECO';
        case 2: return 'SPORT';
        default: return 'N/A';
      }
    }

    function getLedModeName(mode) {
      const modes = {
        0: 'Выключено',
        1: 'Одноцветное дыхание',
        2: 'Всецветное дыхание',
        3: 'Два цвета раздельно',
        4: 'Все цвета раздельно',
        5: 'Одноцветное мерцание',
        6: 'Всецветное мерцание',
        7: 'Полиция 1',
        8: 'Полиция 2',
        9: 'Полиция 3'
      };
      return modes[mode] || 'Неизвестно';
    }

    // Обновление состояния кнопок
    function updateButtonState(btnId, prefix, state) {
      const btn = document.getElementById(btnId);
      btn.innerHTML = `<span>${prefix.split(' ')[0]}</span> ${prefix.split(' ')[1]} ${state ? 'ВКЛ' : 'ВЫКЛ'}`;
      btn.className = state ? 'btn btn-active' : 'btn btn-toggle';
    }

    // Обновление переключателей настроек
    function updateSettingsSwitches(data) {
      // Здесь можно обновить состояния переключателей на основе данных с самоката
      // Например: document.getElementById('headlightAlwaysOn').checked = data.headlightAlwaysOn;
    }

    // Управление подсветкой
    function setLedMode(mode) {
      fetch('/led_mode?mode=' + mode)
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Режим подсветки изменен: ' + getLedModeName(mode));
            updateLedModeButtons();
            currentLedMode = mode;
          } else {
            showNotification('Ошибка: ' + data.message, true);
          }
        })
        .catch(error => {
          showNotification('Ошибка соединения', true);
        });
    }

    function setLedColor(colorIndex, color) {
      fetch('/led_color?index=' + colorIndex + '&color=' + color.toString(16))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Цвет ' + colorIndex + ' установлен');
            updateColorButtons(colorIndex, color);
          } else {
            showNotification('Ошибка: ' + data.message, true);
          }
        })
        .catch(error => {
          showNotification('Ошибка соединения', true);
        });
    }

    function updateLedModeButtons() {
      // Сбрасываем все кнопки
      document.querySelectorAll('.led-mode-btn').forEach(btn => {
        btn.classList.remove('active');
      });
      // Активируем текущую
      if (document.getElementById('ledMode' + currentLedMode)) {
        document.getElementById('ledMode' + currentLedMode).classList.add('active');
      }
    }

    function updateColorButtons(colorIndex, color) {
      // Здесь можно обновить визуальное состояние кнопок цветов
    }

    // Управление настройками
    function toggleSetting(setting, enabled) {
      fetch('/' + setting + '?enabled=' + (enabled ? '1' : '0'))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Настройка изменена');
          } else {
            showNotification('Ошибка: ' + data.message, true);
          }
        })
        .catch(error => {
          showNotification('Ошибка соединения', true);
        });
    }

    function toggleHeadlight(enabled) {
      fetch('/headlight_toggle')
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification(enabled ? 'Фары включены' : 'Фары выключены');
          } else {
            showNotification('Ошибка: ' + data.message, true);
          }
        })
        .catch(error => {
          showNotification('Ошибка соединения', true);
        });
    }

    function toggleBeepAlarm(enabled) {
      fetch('/beep_alarm?enabled=' + (enabled ? '1' : '0'))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification(enabled ? 'Звуковой сигнал включен' : 'Звуковой сигнал выключен');
          } else {
            showNotification('Ошибка: ' + data.message, true);
          }
        })
        .catch(error => {
          showNotification('Ошибка соединения', true);
        });
    }

    function toggleBeepTotal(enabled) {
      fetch('/beep_total?enabled=' + (enabled ? '1' : '0'))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification(enabled ? 'Общий звук включен' : 'Общий звук выключен');
          } else {
            showNotification('Ошибка: ' + data.message, true);
          }
        })
        .catch(error => {
          showNotification('Ошибка соединения', true);
        });
    }

    // Установка лимита скорости
    function setSpeedLimit(limit) {
      fetch('/speed_limit?limit=' + limit)
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Лимит скорости установлен: ' + (limit / 10) + ' км/ч');
          } else {
            showNotification('Ошибка: ' + data.message, true);
          }
        })
        .catch(error => {
          showNotification('Ошибка соединения', true);
        });
    }

    // Диагностика
    function loadDiagnostics() {
      // Загружаем дополнительные диагностические данные
      loadStatus();
      showNotification('Диагностика обновлена');
    }

    // Автообновление
    function startAutoRefresh() {
      autoRefreshInterval = setInterval(loadStatus, 2000);
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
        if (currentStatus) sendCommand('unlock');
      } else if (touchEndX - touchStartX > swipeMin) {
        if (!currentStatus) sendCommand('lock');
      }
    }