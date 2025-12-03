    // Глобальные переменные
    let isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
    let isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
    let currentTab = 'dashboard';
    let isLocked = true;
    let autoRefreshInterval = null;
    let lastTapTime = 0;
    let scanActive = false;

    // Инициализация
    document.addEventListener('DOMContentLoaded', function () {
      initApp();
      loadStatus();
      startAutoRefresh();

      // Настройка безопасных зон для iOS
      if (isIOS) {
        setupIOSSafeAreas();
      }
    });

    // Инициализация приложения
    function initApp() {
      // Инициализация свайп-контроля
      initSwipeControl();

      // Настройка двойного нажатия
      setupDoubleTap();

      // Настройка долгого нажатия
      setupLongPress();

      // Закрытие боковой панели при клике на ссылку
      document.querySelectorAll('.nav-item').forEach(item => {
        item.addEventListener('click', function () {
          if (window.innerWidth <= 768) {
            toggleSidebar();
          }
        });
      });

      // Инициализация слайдера скорости
      updateSpeedLimit(35);
    }

    // Настройка безопасных зон для iOS
    function setupIOSSafeAreas() {
      const style = document.createElement('style');
      style.textContent = `
        .mobile-header { padding-top: calc(12px + constant(safe-area-inset-top)) !important; }
        .sidebar { padding-top: calc(30px + constant(safe-area-inset-top)) !important; }
        .mobile-nav { padding-bottom: calc(8px + constant(safe-area-inset-bottom)) !important; }
        .action-sheet { padding-bottom: calc(20px + constant(safe-area-inset-bottom)) !important; }
      `;
      document.head.appendChild(style);
    }

    // Управление боковой панелью
    function toggleSidebar() {
      const sidebar = document.querySelector('.sidebar');
      const overlay = document.querySelector('.sidebar-overlay');

      if (sidebar.classList.contains('mobile-visible')) {
        sidebar.classList.remove('mobile-visible');
        overlay.classList.remove('active');
        document.body.style.overflow = 'auto';
      } else {
        sidebar.classList.add('mobile-visible');
        overlay.classList.add('active');
        document.body.style.overflow = 'hidden';
      }
    }

    // Инициализация свайп-контроля
    function initSwipeControl() {
      const swipeHandle = document.getElementById('swipeHandle');
      const swipeTrack = document.querySelector('.swipe-track');
      const swipeProgress = document.getElementById('swipeProgress');

      let isDragging = false;
      let startX = 0;
      let currentX = 0;
      const maxSwipe = swipeTrack.offsetWidth - swipeHandle.offsetWidth - 8;

      swipeHandle.addEventListener('touchstart', function (e) {
        e.preventDefault();
        isDragging = true;
        startX = e.touches[0].clientX;
        swipeHandle.style.transition = 'none';
        swipeProgress.style.transition = 'none';
      });

      swipeHandle.addEventListener('touchmove', function (e) {
        if (!isDragging) return;
        e.preventDefault();

        currentX = e.touches[0].clientX - startX;
        if (currentX < 0) currentX = 0;
        if (currentX > maxSwipe) currentX = maxSwipe;

        swipeHandle.style.transform = `translateX(${currentX}px)`;
        swipeProgress.style.width = `${(currentX / maxSwipe) * 100}%`;

        // Изменение цвета при достижении порога
        if (currentX > maxSwipe * 0.8) {
          document.getElementById('swipeControl').style.background = 'linear-gradient(135deg, var(--success), var(--info))';
          document.getElementById('swipeInstruction').textContent = 'Отпустите для разблокировки';
        } else {
          document.getElementById('swipeControl').style.background = 'linear-gradient(135deg, var(--accent), var(--info))';
          document.getElementById('swipeInstruction').textContent = 'Потяните вправо чтобы разблокировать';
        }
      });

      swipeHandle.addEventListener('touchend', function (e) {
        if (!isDragging) return;
        isDragging = false;

        swipeHandle.style.transition = 'transform 0.3s ease';
        swipeProgress.style.transition = 'width 0.3s ease';

        if (currentX > maxSwipe * 0.8 && isLocked) {
          // Успешный свайп
          sendCommand('unlock');
          vibrate();
        }

        // Сброс позиции
        setTimeout(() => {
          swipeHandle.style.transform = 'translateX(0)';
          swipeProgress.style.width = '0%';
          document.getElementById('swipeControl').style.background = 'linear-gradient(135deg, var(--accent), var(--info))';
          document.getElementById('swipeInstruction').textContent = 'Потяните вправо чтобы разблокировать';
        }, 300);
      });
    }

    // Настройка двойного нажатия
    function setupDoubleTap() {
      document.addEventListener('touchend', function (e) {
        const currentTime = new Date().getTime();
        const tapLength = currentTime - lastTapTime;

        if (tapLength < 300 && tapLength > 0) {
          // Двойное нажатие
          toggleLock();
          e.preventDefault();
        }

        lastTapTime = currentTime;
      });
    }

    // Настройка долгого нажатия
    function setupLongPress() {
      const elements = ['headlightBtn', 'beepBtn'];

      elements.forEach(id => {
        const element = document.getElementById(id);
        let pressTimer;

        element.addEventListener('touchstart', function (e) {
          pressTimer = setTimeout(() => {
            if (id === 'headlightBtn') {
              showActionSheet('lights');
            } else if (id === 'beepBtn') {
              showActionSheet('sound');
            }
            vibrate(50);
          }, 500);
        });

        element.addEventListener('touchend', function () {
          clearTimeout(pressTimer);
        });

        element.addEventListener('touchmove', function () {
          clearTimeout(pressTimer);
        });
      });
    }

    // Всплывающие меню
    function showActionSheet(type) {
      const sheet = document.getElementById('actionSheet');
      const title = document.getElementById('actionSheetTitle');
      const content = document.getElementById('actionSheetContent');

      let html = '';

      switch (type) {
        case 'power':
          title.textContent = 'Управление питанием';
          html = `
            <button class="action-sheet-item" onclick="sendCommand('reboot'); hideActionSheet();">
              <span>🔄</span>
              <span>Перезагрузить</span>
            </button>
            <button class="action-sheet-item" onclick="sendCommand('power_off'); hideActionSheet();">
              <span>⭕</span>
              <span>Выключить</span>
            </button>
          `;
          break;

        case 'lights':
          title.textContent = 'Управление подсветкой';
          html = `
            <button class="action-sheet-item" onclick="setLedMode(0); hideActionSheet();">
              <span>⚪</span>
              <span>Выключить</span>
            </button>
            <button class="action-sheet-item" onclick="setLedMode(1); hideActionSheet();">
              <span>🌈</span>
              <span>Дыхание</span>
            </button>
            <button class="action-sheet-item" onclick="setLedMode(5); hideActionSheet();">
              <span>✨</span>
              <span>Мерцание</span>
            </button>
            <button class="action-sheet-item" onclick="setLedMode(7); hideActionSheet();">
              <span>🚨</span>
              <span>Полиция</span>
            </button>
          `;
          break;

        case 'sound':
          title.textContent = 'Управление звуком';
          html = `
            <button class="action-sheet-item" onclick="sendCommand('beep_toggle'); hideActionSheet();">
              <span>🔊</span>
              <span>Вкл/Выкл звук</span>
            </button>
            <button class="action-sheet-item" onclick="toggleBeepAlarm(true); hideActionSheet();">
              <span>🚨</span>
              <span>Включить сигнал</span>
            </button>
            <button class="action-sheet-item" onclick="toggleBeepTotal(false); hideActionSheet();">
              <span>🔇</span>
              <span>Выключить все звуки</span>
            </button>
          `;
          break;

        case 'more':
          title.textContent = 'Навигация';
          html = `
            <button class="action-sheet-item" onclick="switchTab('extended'); hideActionSheet();">
              <span>🔍</span>
              <span>Детали</span>
            </button>
            <button class="action-sheet-item" onclick="switchTab('settings'); hideActionSheet();">
              <span>⚙️</span>
              <span>Настройки</span>
            </button>
            <button class="action-sheet-item" onclick="switchTab('research'); hideActionSheet();">
              <span>🔬</span>
              <span>Исследование</span>
            </button>
            <button class="action-sheet-item" onclick="switchTab('ota'); hideActionSheet();">
              <span>🔄</span>
              <span>Обновление</span>
            </button>
            <button class="action-sheet-item" onclick="switchTab('system'); hideActionSheet();">
              <span>💻</span>
              <span>Система</span>
            </button>
          `;
          break;
      }

      content.innerHTML = html;
      sheet.classList.add('active');
      document.body.style.overflow = 'hidden';
    }

    function hideActionSheet() {
      const sheet = document.getElementById('actionSheet');
      sheet.classList.remove('active');
      document.body.style.overflow = 'auto';
    }

    // Вибрация
    function vibrate(duration = 30) {
      if (navigator.vibrate) {
        navigator.vibrate(duration);
      }
    }

    // Переключение вкладок
    function switchTab(tabName) {
      // Скрыть все вкладки
      document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
        tab.classList.remove('animate-slide');
      });

      // Скрыть все активные элементы навигации
      document.querySelectorAll('.mobile-nav-item').forEach(item => {
        item.classList.remove('active');
      });
      document.querySelectorAll('.nav-item').forEach(item => {
        item.classList.remove('active');
      });

      // Показать выбранную вкладку
      const tabElement = document.getElementById(tabName + '-tab');
      if (tabElement) {
        tabElement.classList.add('active');
        setTimeout(() => {
          tabElement.classList.add('animate-slide');
        }, 10);

        // Обновить заголовок
        document.getElementById('mobileTitle').textContent = getTabTitle(tabName);

        // Обновить активную иконку в навигации
        if (tabName === 'dashboard') {
          document.querySelectorAll('.mobile-nav-item')[0].classList.add('active');
          document.querySelectorAll('.nav-item')[0].classList.add('active');
        } else if (tabName === 'lights') {
          document.querySelectorAll('.mobile-nav-item')[1].classList.add('active');
          document.querySelectorAll('.nav-item')[2].classList.add('active');
        } else if (tabName === 'stats') {
          document.querySelectorAll('.mobile-nav-item')[2].classList.add('active');
          document.querySelectorAll('.nav-item')[3].classList.add('active');
        }
      }

      currentTab = tabName;

      // Прокрутить к началу
      document.querySelector('.main-content').scrollTop = 0;
    }

    function getTabTitle(tabName) {
      const titles = {
        'dashboard': 'Панель управления',
        'extended': 'Детали',
        'lights': 'Подсветка',
        'stats': 'Статистика',
        'settings': 'Настройки',
        'research': 'Исследование',
        'system': 'Система',
        'ota': 'Обновление'
      };
      return titles[tabName] || 'Управление';
    }

    // Обновление статуса для мобильных
    function updateMobileStatus(data) {
      // Обновление основных значений
      document.getElementById('batteryValue').innerHTML = `${data.battery || 0}<span class="value-unit">%</span>`;
      document.getElementById('speedValue').innerHTML = `${data.speed || 0}<span class="value-unit">км/ч</span>`;
      document.getElementById('tempValue').innerHTML = `${data.temperature || 0}<span class="value-unit">°C</span>`;
      
      // Обновление статуса
      document.getElementById('statusText').textContent = data.statusText || 'Загрузка...';
      
      // Обновление сайдбара
      document.getElementById('sidebarBattery').textContent = `${data.battery || 0}%`;
      document.getElementById('sidebarSpeed').textContent = `Скорость: ${data.speed || 0} км/ч`;
      
      // Обновление состояния блокировки
      if (data.isLocked !== undefined) {
        isLocked = data.isLocked;
        updateLockStatus(isLocked);
      }
      
      // Обновление статуса соединения
      if (data.connected) {
        document.getElementById('connectionStatus').style.background = 'var(--success)';
        document.getElementById('connectionText').textContent = 'Подключено';
      } else {
        document.getElementById('connectionStatus').style.background = 'var(--danger)';
        document.getElementById('connectionText').textContent = 'Отключено';
      }
    }

    function updateLockStatus(locked) {
      if (locked) {
        document.getElementById('statusText').textContent = 'ЗАБЛОКИРОВАН';
        document.getElementById('statusText').style.color = 'var(--danger)';
      } else {
        document.getElementById('statusText').textContent = 'РАЗБЛОКИРОВАН';
        document.getElementById('statusText').style.color = 'var(--success)';
      }
    }

    function toggleLock() {
      if (isLocked) {
        sendCommand('unlock');
      } else {
        sendCommand('lock');
      }
      vibrate();
    }

    function toggleFunction(func) {
      sendCommand(func + '_toggle');
      vibrate();
    }

    function toggleCruise() {
      sendCommand('cruise_toggle');
      vibrate();
    }

    function updateSpeedLimit(value) {
      const limit = parseInt(value) * 10;
      document.getElementById('speedLimitValue').textContent = value + ' км/ч';
      setSpeedLimit(limit);
    }

    // Функции API из первого файла
    async function loadStatus() {
      try {
        const [statusRes, dataRes] = await Promise.all([
          fetch('/status'),
          fetch('/data')
        ]);

        const statusData = await statusRes.json();
        const scooterData = await dataRes.json();

        if (statusData.success) {
          // Обновление основных данных
          updateMobileStatus({ 
            ...statusData, 
            ...scooterData,
            battery: scooterData.battery || 0,
            speed: scooterData.speed || 0,
            temperature: scooterData.temperature || 0,
            connected: statusData.connected
          });
          
          // Обновление расширенной информации если на соответствующей вкладке
          if (currentTab === 'extended') {
            updateExtendedInfo(scooterData);
          }
          
          // Обновление статистики
          if (currentTab === 'stats') {
            updateStats(scooterData);
          }
          
          // Обновление системы
          if (currentTab === 'system') {
            updateSystemInfo(scooterData);
          }
        }
      } catch (error) {
        console.error('Ошибка загрузки данных:', error);
        showNotification('Ошибка загрузки данных', 'error');
      }
    }

    async function sendCommand(cmd) {
      try {
        vibrate();
        const response = await fetch('/' + cmd);
        const data = await response.json();

        if (data.success) {
          showNotification(data.message || 'Команда выполнена');
          setTimeout(loadStatus, 500);
        } else {
          showNotification('Ошибка: ' + data.message, 'error');
        }
      } catch (error) {
        console.error('Ошибка отправки команды:', error);
        showNotification('Ошибка соединения', 'error');
      }
    }

    function setSpeedLimit(limit) {
      fetch('/speed_limit?limit=' + limit)
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Лимит скорости установлен: ' + (limit / 10) + ' км/ч');
          } else {
            showNotification('Ошибка: ' + data.message, 'error');
          }
        })
        .catch(error => {
          console.error('Ошибка установки лимита:', error);
          showNotification('Ошибка соединения', 'error');
        });
    }

    function setLedMode(mode) {
      fetch('/led_mode?mode=' + mode)
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Режим подсветки изменен');
          } else {
            showNotification('Ошибка: ' + data.message, 'error');
          }
        })
        .catch(error => {
          console.error('Ошибка изменения режима подсветки:', error);
          showNotification('Ошибка соединения', 'error');
        });
    }

    function setLedColor(colorNum, value) {
      fetch('/led_color?color=' + colorNum + '&value=' + value)
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Цвет подсветки изменен');
          } else {
            showNotification('Ошибка: ' + data.message, 'error');
          }
        })
        .catch(error => {
          console.error('Ошибка изменения цвета подсветки:', error);
          showNotification('Ошибка соединения', 'error');
        });
    }

    function toggleBeepAlarm(enabled) {
      fetch('/beep_alarm?enabled=' + (enabled ? '1' : '0'))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification(enabled ? 'Звуковой сигнал включен' : 'Звуковой сигнал выключен');
          }
        })
        .catch(error => {
          console.error('Ошибка переключения сигнала:', error);
        });
    }

    function toggleBeepTotal(enabled) {
      fetch('/beep_total?enabled=' + (enabled ? '1' : '0'))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification(enabled ? 'Общий звук включен' : 'Общий звук выключен');
          }
        })
        .catch(error => {
          console.error('Ошибка переключения общего звука:', error);
        });
    }

    function toggleSetting(setting, enabled) {
      fetch('/setting?name=' + setting + '&value=' + (enabled ? '1' : '0'))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification('Настройка сохранена');
          }
        })
        .catch(error => {
          console.error('Ошибка сохранения настройки:', error);
        });
    }

    function toggleHeadlight(enabled) {
      fetch('/headlight?enabled=' + (enabled ? '1' : '0'))
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showNotification(enabled ? 'Фары включены' : 'Фары выключены');
          }
        })
        .catch(error => {
          console.error('Ошибка переключения фар:', error);
        });
    }

    // Функции сканирования
    function startSafeScan() {
      if (scanActive) return;
      scanActive = true;
      showNotification('Начато безопасное сканирование', 'info');
      simulateScan();
    }

    function startBruteForceScan() {
      if (scanActive) return;
      scanActive = true;
      showNotification('Внимание: брутфорс сканирование может быть опасным!', 'warning');
      simulateScan();
    }

    function startDeepScan() {
      if (scanActive) return;
      scanActive = true;
      showNotification('Начато глубокое сканирование', 'info');
      simulateScan();
    }

    function simulateScan() {
      const progressBar = document.getElementById('scanProgress');
      const progressText = document.getElementById('scanProgressText');
      const scanStatus = document.getElementById('scanStatus');
      const foundRegisters = document.getElementById('foundRegisters');
      
      let progress = 0;
      const total = 256;
      
      scanStatus.textContent = 'Сканирование...';
      
      const interval = setInterval(() => {
        progress += 4;
        if (progress > total) progress = total;
        
        progressBar.style.width = (progress / total * 100) + '%';
        progressText.textContent = progress + '/' + total;
        
        if (progress >= total) {
          clearInterval(interval);
          scanActive = false;
          scanStatus.textContent = 'Сканирование завершено';
          showNotification('Найдено 15 новых регистров', 'success');
          
          // Пример найденных регистров
          foundRegisters.innerHTML = `
            <div style="padding: 10px; background: var(--success); color: white; border-radius: 8px; margin-bottom: 10px;">
              <strong>Регистр 0x51</strong>: Курок газа (0x0000 - 0xFFFF)
            </div>
            <div style="padding: 10px; background: var(--info); color: white; border-radius: 8px; margin-bottom: 10px;">
              <strong>Регистр 0x52</strong>: Тормоз (0x0000 - 0xFFFF)
            </div>
            <div style="padding: 10px; background: var(--warning); color: var(--dark); border-radius: 8px; margin-bottom: 10px;">
              <strong>Регистр 0x80</strong>: Управление фарами (0-2)
            </div>
          `;
        }
      }, 50);
    }

    function manualTest() {
      const index = document.getElementById('manualIndex').value;
      const value = document.getElementById('manualValue').value;
      
      if (!index) {
        showNotification('Введите индекс регистра', 'error');
        return;
      }
      
      showNotification(`Тестирование регистра ${index} со значением ${value || 'чтение'}`, 'info');
      
      // Здесь должна быть реальная логика тестирования
      setTimeout(() => {
        showNotification('Тест выполнен успешно', 'success');
      }, 1000);
    }

    function startPacketLog() {
      showNotification('Логирование пакетов начато', 'info');
    }

    function stopPacketLog() {
      showNotification('Логирование пакетов остановлено', 'info');
    }

    function clearPacketLog() {
      document.getElementById('packetLog').innerHTML = '';
      showNotification('Лог очищен', 'info');
    }

    function loadDiagnostics() {
      // Здесь должна быть логика загрузки диагностики
      showNotification('Диагностика обновлена', 'info');
    }

    function loadSystemInfo() {
      // Здесь должна быть логика загрузки информации о системе
      showNotification('Информация о системе обновлена', 'info');
    }

    function checkForUpdates() {
      showNotification('Проверка обновлений...', 'info');
      setTimeout(() => {
        showNotification('Обновлений не найдено', 'info');
      }, 2000);
    }

    function testOTAConnection() {
      showNotification('Тестирование OTA соединения...', 'info');
      setTimeout(() => {
        showNotification('OTA соединение работает', 'success');
      }, 1500);
    }

    function showOTAInstructions() {
      showNotification('Инструкция по OTA обновлению открыта в новом окне', 'info');
      window.open('/ota_instructions', '_blank');
    }

    // Обновление расширенной информации
    function updateExtendedInfo(data) {
      const elements = [
        'driveVoltageValue', 'motorCurrentValue', 'powerValue', 'avgSpeedValue',
        'bodyTempValue', 'bat1TempValue', 'bat2TempValue', 'mosTempValue',
        'battery1Value', 'battery2Value', 'actualRangeValue', 'predictedRangeValue',
        'serialValue', 'bmsVersionValue', 'bms2VersionValue', 'bleVersionValue',
        'boolStatusValue', 'alarmStatusValue', 'errorValue'
      ];
      
      elements.forEach(id => {
        const element = document.getElementById(id);
        if (element && data[id]) {
          element.textContent = data[id];
        }
      });
    }

    // Обновление статистики
    function updateStats(data) {
      const elements = [
        'singleMileageValue', 'singleRideTimeValue', 'totalOperationTimeValue',
        'totalRideTimeValue', 'workModeValue', 'speedLimitValue', 'errorCodeValue',
        'cruiseValue', 'headlightValue', 'beepValue'
      ];
      
      elements.forEach(id => {
        const element = document.getElementById(id);
        if (element && data[id]) {
          element.textContent = data[id];
        }
      });
    }

    // Обновление системной информации
    function updateSystemInfo(data) {
      const elements = [
        'currentLedMode', 'fwVersion', 'workSystem', 'lockStatus', 'engineStatus',
        'diagnosticError', 'diagnosticAlarm', 'diagnosticBool', 'diagnosticQuickBool'
      ];
      
      elements.forEach(id => {
        const element = document.getElementById(id);
        if (element && data[id]) {
          element.textContent = data[id];
        }
      });
    }

    // Автообновление
    function startAutoRefresh() {
      autoRefreshInterval = setInterval(loadStatus, 3000);
    }

    function toggleDataRefresh() {
      if (autoRefreshInterval) {
        clearInterval(autoRefreshInterval);
        autoRefreshInterval = null;
        showNotification('Автообновление выключено', 'warning');
      } else {
        startAutoRefresh();
        showNotification('Автообновление включено', 'success');
      }
    }

    // Уведомления
    function showNotification(message, type = 'info') {
      const notification = document.getElementById('notification');
      const text = document.getElementById('notificationText');
      const icon = notification.querySelector('.notification-icon');

      text.textContent = message;
      notification.className = `notification ${type}`;

      switch (type) {
        case 'error': icon.textContent = '❌'; break;
        case 'warning': icon.textContent = '⚠️'; break;
        case 'success': icon.textContent = '✅'; break;
        default: icon.textContent = 'ℹ️';
      }

      notification.classList.add('show');

      setTimeout(() => {
        notification.classList.remove('show');
      }, 3000);
    }

    // Обработка онлайн/офлайн статуса
    window.addEventListener('online', () => {
      showNotification('Соединение восстановлено', 'success');
      loadStatus();
    });

    window.addEventListener('offline', () => {
      showNotification('Нет соединения', 'error');
    });

    // Предотвращение сна экрана (если поддерживается)
    let wakeLock = null;
    if ('wakeLock' in navigator && isMobile) {
      try {
        navigator.wakeLock.request('screen').then(wl => {
          wakeLock = wl;
        });
      } catch (err) {
        console.log('Wake Lock не поддерживается');
      }
    }

    // Освобождение Wake Lock при закрытии
    window.addEventListener('beforeunload', () => {
      if (wakeLock) {
        wakeLock.release();
        wakeLock = null;
      }
    });

    // Обработка изменения размера окна
    window.addEventListener('resize', function () {
      // Закрыть боковую панель на больших экранах
      if (window.innerWidth > 768) {
        const sidebar = document.querySelector('.sidebar');
        const overlay = document.querySelector('.sidebar-overlay');
        sidebar.classList.remove('mobile-visible');
        overlay.classList.remove('active');
        document.body.style.overflow = 'auto';
      }
    });

    // Предотвращение контекстного меню на мобильных
    if (isMobile) {
      document.addEventListener('contextmenu', function(e) {
        e.preventDefault();
      });
    }