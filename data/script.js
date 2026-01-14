// Глобальные переменные
let isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
let isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
let currentTab = "dashboard";
let autoRefreshInterval = null;
let lastTapTime = 0;
let scanActive = false;
let isLocked = false;
const CURRENT_VERSION = "0.0.0";
const REPO_OWNER = "teemse";
const REPO_NAME = "ninebot";

// Инициализация
document.addEventListener("DOMContentLoaded", function () {
  initApp();
  loadStatus();
  startAutoRefresh();
  initUpdateChecker(); // Инициализация проверки обновлений

  if (isIOS) {
    setupIOSSafeAreas();
  }
});

// Инициализация приложения
function initApp() {
  initSwipeControl();
  updateOTAInfo();
  
  const slider = document.getElementById("speedSlider");
  if (slider) {
    updateSpeedLimit(slider.value);
  }
  
  updateSpeedLimit(30);
  
  // Инициализация OTA элементов
  initOTAElements();
}

/**
 * Инициализация элементов OTA
 */
function initOTAElements() {
  // Добавляем обработчики для кнопок OTA
  const checkUpdateBtn = document.getElementById('checkUpdateBtn');
  const checkFirmwareInfoBtn = document.getElementById('checkFirmwareInfoBtn');
  const testOTAConnectionBtn = document.getElementById('testOTAConnectionBtn');
  const showInstructionsBtn = document.getElementById('showInstructionsBtn');
  const uploadFirmwareBtn = document.getElementById('uploadFirmwareBtn');
  const manualUpdateBtn = document.getElementById('manualUpdateBtn');
  
  if (checkUpdateBtn) {
    checkUpdateBtn.onclick = () => checkForUpdates(CURRENT_VERSION, REPO_OWNER, REPO_NAME, true);
  }
  
  if (checkFirmwareInfoBtn) {
    checkFirmwareInfoBtn.onclick = checkFirmwareInfo;
  }
  
  if (testOTAConnectionBtn) {
    testOTAConnectionBtn.onclick = testOTAConnection;
  }
  
  if (showInstructionsBtn) {
    showInstructionsBtn.onclick = showOTAInstructions;
  }
  
  if (uploadFirmwareBtn) {
    uploadFirmwareBtn.onclick = () => document.getElementById('firmwareFile').click();
  }
  
  if (manualUpdateBtn) {
    manualUpdateBtn.onclick = startManualUpdate;
  }
  
  // Инициализация загрузки файла
  const firmwareFile = document.getElementById('firmwareFile');
  if (firmwareFile) {
    firmwareFile.onchange = handleFirmwareUpload;
  }
  
  // Загружаем информацию о прошивке при открытии вкладки OTA
  if (currentTab === 'ota') {
    setTimeout(() => {
      checkFirmwareInfo();
      checkForUpdates(CURRENT_VERSION, REPO_OWNER, REPO_NAME, false);
    }, 500);
  }
}

/**
 * Запуск ручного обновления
 */
async function startManualUpdate() {
  try {
    // Проверяем наличие обновлений
    const updateInfo = await checkForUpdates(CURRENT_VERSION, REPO_OWNER, REPO_NAME, true);
    
    if (!updateInfo || !updateInfo.hasUpdate) {
      showNotification("Нет доступных обновлений", "warning");
      return;
    }
    
    // Показываем подтверждение
    if (!confirm(`Установить обновление v${updateInfo.latestVersion}?\n\nТекущая версия: v${updateInfo.currentVersion}\n\nЭто займет около 1-2 минут.`)) {
      return;
    }
    
    showNotification("Начинаем загрузку обновления...", "info");
    
    // Ищем URL для скачивания прошивки
    let firmwareUrl = null;
    
    // Пытаемся найти .bin файл в assets
    if (updateInfo.assets && updateInfo.assets.length > 0) {
      const binAsset = updateInfo.assets.find(asset => 
        asset.name.endsWith('.bin') || 
        asset.name.includes('firmware') ||
        asset.name.includes('esp8266') ||
        asset.name.includes('esp32')
      );
      
      if (binAsset) {
        firmwareUrl = binAsset.browser_download_url;
      }
    }
    
    // Если не нашли в assets, используем основной URL
    if (!firmwareUrl) {
      firmwareUrl = updateInfo.downloadUrl;
    }
    
    if (!firmwareUrl) {
      throw new Error("Не удалось найти ссылку на прошивку");
    }
    
    // Скачиваем прошивку
    showNotification("Скачивание прошивки...", "info");
    const response = await fetch(firmwareUrl);
    
    if (!response.ok) {
      throw new Error(`Ошибка скачивания: ${response.status}`);
    }
    
    const firmwareBlob = await response.blob();
    
    // Создаем FormData
    const formData = new FormData();
    formData.append('firmware', firmwareBlob, `firmware_${updateInfo.latestVersion}.bin`);
    
    // Загружаем на устройство
    showNotification("Загрузка прошивки на устройство...", "info");
    const uploadResponse = await fetch('/update', {
      method: 'POST',
      body: formData
    });
    
    const result = await uploadResponse.json();
    
    if (result.success) {
      showNotification("Прошивка успешно загружена! Устройство перезагружается...", "success");
      
      // Таймер обратного отсчета
      let countdown = 30;
      const countdownInterval = setInterval(() => {
        showNotification(`Перезагрузка через ${countdown} сек...`, "info", {duration: 1000});
        countdown--;
        
        if (countdown <= 0) {
          clearInterval(countdownInterval);
          location.reload();
        }
      }, 1000);
      
    } else {
      throw new Error(result.message || "Ошибка обновления");
    }
    
  } catch (error) {
    console.error("Ошибка при обновлении:", error);
    showNotification(`Ошибка обновления: ${error.message}`, "error");
  }
}

/**
 * Обработка загрузки файла прошивки
 */
async function handleFirmwareUpload(event) {
  const file = event.target.files[0];
  if (!file) return;
  
  // Проверка типа файла
  if (!file.name.endsWith('.bin')) {
    showNotification("Файл должен быть в формате .bin", "error");
    return;
  }
  
  // Проверка размера файла
  const maxSize = 1024 * 1024; // 1MB
  if (file.size > maxSize) {
    showNotification("Файл слишком большой (максимум 1MB)", "error");
    return;
  }
  
  showNotification(`Загружается файл: ${file.name} (${formatBytes(file.size)})`, "info");
  
  // Показываем прогресс
  const progressBar = document.getElementById('uploadProgress');
  const progressText = document.getElementById('uploadProgressText');
  
  if (progressBar && progressText) {
    progressBar.style.width = '0%';
    progressText.textContent = '0%';
  }
  
  // Создаем FormData
  const formData = new FormData();
  formData.append('firmware', file);
  
  try {
    const response = await fetch('/update', {
      method: 'POST',
      body: formData,
      // Отслеживаем прогресс загрузки
      // Примечание: Fetch API не поддерживает отслеживание прогресса напрямую,
      // но мы можем эмулировать его для небольших файлов
    });
    
    // Создаем интервал для имитации прогресса
    let progress = 0;
    const progressInterval = setInterval(() => {
      progress += 10;
      if (progress > 90) progress = 90;
      
      if (progressBar && progressText) {
        progressBar.style.width = `${progress}%`;
        progressText.textContent = `${progress}%`;
      }
    }, 200);
    
    const result = await response.json();
    clearInterval(progressInterval);
    
    if (progressBar && progressText) {
      progressBar.style.width = '100%';
      progressText.textContent = '100%';
    }
    
    if (result.success) {
      showNotification("Прошивка успешно загружена, устройство перезагружается...", "success");
      
      // Ожидаем перезагрузку устройства
      setTimeout(() => {
        showNotification("Устройство должно перезагрузиться. Обновите страницу через 30 секунд.", "info");
        
        // Таймер обратного отсчета
        let countdown = 30;
        const countdownInterval = setInterval(() => {
          if (countdown > 0) {
            showNotification(`Перезагрузка... ${countdown} сек`, "info", {duration: 1000});
            countdown--;
          } else {
            clearInterval(countdownInterval);
            location.reload();
          }
        }, 1000);
      }, 2000);
      
    } else {
      throw new Error(result.message || "Ошибка загрузки прошивки");
    }
  } catch (error) {
    console.error("Ошибка загрузки прошивки:", error);
    
    if (progressBar && progressText) {
      progressBar.style.width = '0%';
      progressText.textContent = 'Ошибка';
    }
    
    showNotification(`Ошибка загрузки: ${error.message}`, "error");
  } finally {
    // Сбрасываем input
    event.target.value = '';
  }
}

/**
 * Проверка информации о прошивке устройства
 */
async function checkFirmwareInfo() {
  try {
    showNotification("Загрузка информации о прошивке...", "info");
    
    const response = await fetch('/firmware_info');
    if (!response.ok) {
      throw new Error(`Ошибка HTTP: ${response.status}`);
    }
    
    const firmwareInfo = await response.json();
    
    if (firmwareInfo.success) {
      // Формируем данные для отображения
      const otaData = {
        firmwareVersion: firmwareInfo.version || CURRENT_VERSION,
        chipId: firmwareInfo.chip_id,
        freeMemory: firmwareInfo.free_heap,
        sketchSize: firmwareInfo.sketch_size,
        flashSize: firmwareInfo.flash_size,
        sdkVersion: firmwareInfo.sdk_version,
        coreVersion: firmwareInfo.core_version,
        cycleCount: firmwareInfo.cycle_count,
        lastUpdate: new Date().toLocaleString('ru-RU'),
        updateAvailable: false,
        updateStatus: "Актуальная версия"
      };
      
      // Обновляем информацию на странице
      updateOTAInfo(otaData);
      
      // Обновляем глобальную переменную версии
      if (firmwareInfo.version && firmwareInfo.version !== CURRENT_VERSION) {
        // Можно обновить CURRENT_VERSION, если нужно
        console.log(`Версия устройства: ${firmwareInfo.version}`);
      }
      
      showNotification("Информация о прошивке загружена", "success");
      return firmwareInfo;
    } else {
      throw new Error(firmwareInfo.message || "Ошибка получения информации");
    }
  } catch (error) {
    console.error("Ошибка при проверке информации о прошивке:", error);
    
    // Показываем информацию по умолчанию
    updateOTAInfo({
      firmwareVersion: CURRENT_VERSION,
      updateStatus: `Ошибка: ${error.message}`,
      lastUpdate: new Date().toLocaleString('ru-RU')
    });
    
    showNotification(`Ошибка: ${error.message}`, "error");
    return null;
  }
}

// Настройка безопасных зон для iOS
function setupIOSSafeAreas() {
  const style = document.createElement("style");
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
  const sidebar = document.querySelector(".sidebar");
  const overlay = document.querySelector(".sidebar-overlay");

  if (sidebar.classList.contains("mobile-visible")) {
    sidebar.classList.remove("mobile-visible");
    overlay.classList.remove("active");
    document.body.style.overflow = "auto";
  } else {
    sidebar.classList.add("mobile-visible");
    overlay.classList.add("active");
    document.body.style.overflow = "hidden";
  }
}

// Инициализация свайп-контроля
function initSwipeControl() {
  const swipeHandle = document.getElementById("swipeHandle");
  const swipeTrack = document.querySelector(".swipe-track");
  const swipeProgress = document.getElementById("swipeProgress");

  let isDragging = false;
  let startX = 0;
  let currentX = 0;
  const maxSwipe = swipeTrack.offsetWidth - swipeHandle.offsetWidth;

  swipeHandle.addEventListener("touchstart", function (e) {
    e.preventDefault();
    isDragging = true;
    startX = e.touches[0].clientX;
    swipeHandle.style.transition = "none";
    swipeProgress.style.transition = "none";
  });

  swipeHandle.addEventListener("touchmove", function (e) {
    if (!isDragging) return;
    e.preventDefault();

    currentX = e.touches[0].clientX - startX;
    if (currentX < 0) currentX = 0;
    if (currentX > maxSwipe) currentX = maxSwipe;

    swipeHandle.style.transform = `translateX(${currentX}px)`;
    swipeProgress.style.width = `${(currentX / maxSwipe) * 100}%`;

    // Изменение цвета при достижении порога
    if (currentX > maxSwipe * 0.5) {
      document.getElementById("swipeControl").style.background =
        "linear-gradient(135deg, var(--success), var(--info))";
    } else {
      document.getElementById("swipeControl").style.background =
        "linear-gradient(135deg, var(--accent), var(--info))";
    }
  });

  swipeHandle.addEventListener("touchend", function (e) {
    if (!isDragging) return;
    isDragging = false;

    swipeHandle.style.transition = "transform 0.3s ease";
    swipeProgress.style.transition = "width 0.3s ease";

    if (currentX > maxSwipe * 0.8) {
      // Успешный свайп
      toggleLock();
      vibrate();
    }

    // Сброс позиции
    setTimeout(() => {
      swipeHandle.style.transform = "translateX(0)";
      swipeProgress.style.width = "0%";
      document.getElementById("swipeControl").style.background =
        "linear-gradient(135deg, var(--accent), var(--info))";
    }, 300);
  });
}

// Настройка долгого нажатия
function setupLongPress() {
  const elements = ["headlightBtn", "beepBtn"];

  elements.forEach((id) => {
    const element = document.getElementById(id);
    let pressTimer;

    element.addEventListener("touchstart", function (e) {
      pressTimer = setTimeout(() => {
        if (id === "headlightBtn") {
          showActionSheet("lights");
        } else if (id === "beepBtn") {
          showActionSheet("sound");
        }
        vibrate(50);
      }, 500);
    });

    element.addEventListener("touchend", function () {
      clearTimeout(pressTimer);
    });

    element.addEventListener("touchmove", function () {
      clearTimeout(pressTimer);
    });
  });
}

// Всплывающие меню
function showActionSheet(type) {
  const sheet = document.getElementById("actionSheet");
  const title = document.getElementById("actionSheetTitle");
  const content = document.getElementById("actionSheetContent");

  let html = "";

  switch (type) {
    case "power":
      title.textContent = "Управление питанием";
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

    case "lights":
      title.textContent = "Управление подсветкой";
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

    case "sound":
      title.textContent = "Управление звуком";
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

    case "more":
      title.textContent = "Навигация";
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
  sheet.classList.add("active");
  document.body.style.overflow = "hidden";
}

function hideActionSheet() {
  const sheet = document.getElementById("actionSheet");
  sheet.classList.remove("active");
  document.body.style.overflow = "auto";
}

// Вибрация
function vibrate(duration = 30) {
  if (navigator.vibrate) {
    navigator.vibrate(duration);
  }
}

// Переключение вкладок
function switchTab(tabName) {
  // Закрыть боковую панель на мобильных
  // if (window.innerWidth <= 768) {
  //   toggleSidebar();
  // }

  // Скрыть все вкладки
  document.querySelectorAll(".tab-content").forEach((tab) => {
    tab.classList.remove("active");
    tab.classList.remove("animate-slide");
  });

  // Скрыть все активные элементы навигации
  document.querySelectorAll(".mobile-nav-item").forEach((item) => {
    item.classList.remove("active");
  });
  document.querySelectorAll(".nav-item").forEach((item) => {
    item.classList.remove("active");
  });

  // Показать выбранную вкладку
  const tabElement = document.getElementById(tabName + "-tab");
  if (tabElement) {
    tabElement.classList.add("active");
    setTimeout(() => {
      tabElement.classList.add("animate-slide");
    }, 10);

    // Обновить заголовок
    document.getElementById("mobileTitle").textContent = getTabTitle(tabName);

    // Найти и активировать соответствующий элемент навигации
    // Для боковой панели (.nav-item)
    const navItems = document.querySelectorAll(".nav-item");
    const tabIndexMap = {
      dashboard: 0,
      extended: 1,
      lights: 2,
      stats: 3,
      settings: 4,
      research: 5,
      system: 6,
      ota: 7,
    };

    if (tabIndexMap[tabName] !== undefined && navItems[tabIndexMap[tabName]]) {
      navItems[tabIndexMap[tabName]].classList.add("active");
    }

    // Для мобильной навигации (.mobile-nav-item)
    const mobileNavItems = document.querySelectorAll(".mobile-nav-item");
    const mobileTabIndexMap = {
      dashboard: 0,
      lights: 1,
      stats: 2,
      // Для остальных вкладок мобильная навигация не используется
    };

    if (
      mobileTabIndexMap[tabName] !== undefined &&
      mobileNavItems[mobileTabIndexMap[tabName]]
    ) {
      mobileNavItems[mobileTabIndexMap[tabName]].classList.add("active");
    }
  }

  currentTab = tabName;
  
  // Если переключились на вкладку OTA, обновляем информацию
  if (tabName === 'ota') {
    setTimeout(() => {
      checkFirmwareInfo();
      checkForUpdates(CURRENT_VERSION, REPO_OWNER, REPO_NAME, false);
    }, 100);
  }
  
  document.querySelector(".main-content").scrollTop = 0;
}

function getTabTitle(tabName) {
  const titles = {
    dashboard: "Панель управления",
    extended: "Детали",
    lights: "Подсветка",
    stats: "Статистика",
    settings: "Настройки",
    research: "Исследование",
    system: "Система",
    ota: "Обновление",
  };
  return titles[tabName] || "Управление";
}

// Обновление статуса для мобильных
// function updateMobileStatus(data) {
//   // Обновление основных значений
//   document.getElementById("batteryValue").innerHTML = `${
//     data.battery || 0
//   }<span class="value-unit">%</span>`;
//   document.getElementById("speedValue").innerHTML = `${
//     data.speed || 0
//   }<span class="value-unit">км/ч</span>`;
//   document.getElementById("tempValue").innerHTML = `${
//     data.temperature || 0
//   }<span class="value-unit">°C</span>`;

//   // Обновление статуса
//   document.getElementById("statusText").textContent =
//     data.statusText || "Загрузка...";

//   // Обновление сайдбара
//   document.getElementById("sidebarBattery").textContent = `${
//     data.battery || 0
//   }%`;
//   document.getElementById("sidebarSpeed").textContent = `Скорость: ${
//     data.speed || 0
//   } км/ч`;

//   // Обновление состояния блокировки
//   if (data.isLocked !== undefined) {
//     isLocked = data.isLocked;
//     updateLockStatus(isLocked);
//   }

//   // Обновление статуса соединения
//   if (data.connected) {
//     document.getElementById("connectionStatus").style.background =
//       "var(--success)";
//     document.getElementById("connectionText").textContent = "Подключено";
//   } else {
//     document.getElementById("connectionStatus").style.background =
//       "var(--danger)";
//     document.getElementById("connectionText").textContent = "Отключено";
//   }
// }

function updateLockStatus(locked) {
  // 1. Обновляем текст статуса
  const statusElement = document.getElementById("statusText");
  if (locked) {
    statusElement.textContent = "ЗАБЛОКИРОВАН";
    statusElement.style.color = "var(--danger)";
  } else {
    statusElement.textContent = "РАЗБЛОКИРОВАН";
    statusElement.style.color = "var(--success)";
  }
}

function toggleLock() {
  if (isLocked) {
    sendCommand("unlock");
  } else {
    sendCommand("lock");
  }
  vibrate();
}

function toggleFunction(func) {
  sendCommand(func + "_toggle");
  vibrate();
}

function toggleCruise() {
  sendCommand("cruise_toggle");
  vibrate();
}

function updateSpeedLimit(value) {
  const limit = parseInt(value);
  document.getElementById("speedLimitValue").textContent = value + " км/ч";
  setSpeedLimit(limit);
}

// Функции API из первого файла
// async function loadStatus() {
//   try {
//     const [statusRes, dataRes] = await Promise.all([
//       fetch('/status'),
//       fetch('/data')
//     ]);

//     const statusData = await statusRes.json();
//     const scooterData = await dataRes.json();

//     if (statusData.success) {
//       // Обновление основных данных
//       updateMobileStatus({
//         ...statusData,
//         ...scooterData,
//         battery: scooterData.battery || 0,
//         speed: scooterData.speed || 0,
//         temperature: scooterData.temperature || 0,
//         connected: statusData.connected
//       });

//       // Обновление расширенной информации если на соответствующей вкладке
//       if (currentTab === 'extended') {
//         updateExtendedInfo(scooterData);
//       }

//       // Обновление статистики
//       if (currentTab === 'stats') {
//         updateStats(scooterData);
//       }

//       // Обновление системы
//       if (currentTab === 'system') {
//         updateSystemInfo(scooterData);
//       }
//     }
//   } catch (error) {
//     console.error('Ошибка загрузки данных:', error);
//     showNotification('Ошибка загрузки данных', 'error');
//   }
// }

// Модифицированная функция loadStatus()
async function loadStatus() {
  try {
    const [statusRes, dataRes] = await Promise.all([
      fetch("/status"),
      fetch("/data"),
    ]);

    const statusData = await statusRes.json();
    const scooterData = await dataRes.json();

    if (true) {
      // Обновление основных значений
      const batteryValue = scooterData.battery || 0;
      const actualRangeValue = scooterData.actualRange || 0;
      const tempValue = scooterData.temperature || 0;
      const speedValue = scooterData.speed || 0;

      // Оригинальный элемент батареи
      document.getElementById(
        "batteryValue"
      ).innerHTML = `${batteryValue}<span class="value-unit">%</span>`;

      // НОВОЕ: Обновляем Material Design батарею
      updateMaterialBattery(batteryValue, scooterData.isCharging || false);

      document.getElementById(
        "actualRangeValue"
      ).innerHTML = `${actualRangeValue}<span class="value-unit">км</span>`;
      document.getElementById(
        "tempValue"
      ).innerHTML = `${tempValue}<span class="value-unit">°C</span>`;

      // Обновление статуса
      document.getElementById("statusText").textContent =
        statusData.success || "Загрузка...";

      // Обновление сайдбара
      document.getElementById(
        "sidebarBattery"
      ).textContent = `${batteryValue}%`;
      document.getElementById(
        "sidebarSpeed"
      ).textContent = `Скорость: ${speedValue} км/ч`;

      // Обновление состояния блокировки

      isLocked = statusData.isLocked;
      updateLockStatus(isLocked);

      // Обновление статуса соединения
      if (statusData.success) {
        document.getElementById("connectionStatus").style.background =
          "var(--success)";
        document.getElementById("connectionText").textContent = "Подключено";
      } else {
        document.getElementById("connectionStatus").style.background =
          "var(--danger)";
        document.getElementById("connectionText").textContent = "Отключено";
      }

      // Обновление расширенной информации если на соответствующей вкладке
      if (currentTab === "extended") {
        updateExtendedInfo(scooterData);
      }

      // Обновление статистики
      if (currentTab === "stats") {
        console.log("Обновление статистики с данными:", scooterData);
        updateStats(scooterData);
      }

      // Обновление системы
      if (currentTab === "system") {
        console.log("Обновление системы с данными:", scooterData);
        updateSystemInfo(scooterData);
      }
      // Обновление неизвестных регистров при открытии вкладки Исследование
      if (currentTab === "research") {
        updateUnknownRegisters(scooterData);
      }
    }
  } catch (error) {
    console.error("Ошибка загрузки данных:", error);
    showNotification("Ошибка загрузки данных", "error");
  }
}

async function sendCommand(cmd) {
  try {
    vibrate();
    const response = await fetch("/" + cmd);
    const data = await response.json();

    if (data.success) {
      showNotification(data.message || "Команда выполнена");
      setTimeout(loadStatus, 500);
    } else {
      showNotification("Ошибка: " + data.message, "error");
    }
  } catch (error) {
    console.error("Ошибка отправки команды:", error);
    showNotification("Ошибка соединения", "error");
  }
}

function setSpeedLimit(limit) {
  fetch("/speed_" + limit)
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showNotification("Лимит скорости установлен: " + limit + " км/ч");
      } else {
        showNotification("Ошибка: " + data.message, "error");
      }
    })
    .catch((error) => {
      console.error("Ошибка установки лимита:", error);
      showNotification("Ошибка соединения", "error");
    });
}

function setLedMode(mode) {
  fetch("/led_mode?mode=" + mode)
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showNotification("Режим подсветки изменен");
      } else {
        showNotification("Ошибка: " + data.message, "error");
      }
    })
    .catch((error) => {
      console.error("Ошибка изменения режима подсветки:", error);
      showNotification("Ошибка соединения", "error");
    });
}

function setLedColor(colorIndex, color) {
  fetch("/led_color?index=" + colorIndex + "&color=" + color.toString(16))
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showNotification("Цвет " + colorIndex + " установлен");
        updateColorButtons(colorIndex, color);
      } else {
        showNotification("Ошибка: " + data.message, true);
      }
    })
    .catch((error) => {
      showNotification("Ошибка соединения", true);
    });
}

function toggleBeepAlarm(enabled) {
  fetch("/beep_alarm?enabled=" + (enabled ? "1" : "0"))
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showNotification(
          enabled ? "Звуковой сигнал включен" : "Звуковой сигнал выключен"
        );
      }
    })
    .catch((error) => {
      console.error("Ошибка переключения сигнала:", error);
    });
}

function toggleBeepTotal(enabled) {
  fetch("/beep_total?enabled=" + (enabled ? "1" : "0"))
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showNotification(
          enabled ? "Общий звук включен" : "Общий звук выключен"
        );
      }
    })
    .catch((error) => {
      console.error("Ошибка переключения общего звука:", error);
    });
}

function toggleSetting(setting, enabled) {
  fetch("/setting?name=" + setting + "&value=" + (enabled ? "1" : "0"))
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showNotification("Настройка сохранена");
      }
    })
    .catch((error) => {
      console.error("Ошибка сохранения настройки:", error);
    });
}

function toggleHeadlight(enabled) {
  fetch("/headlight?enabled=" + (enabled ? "1" : "0"))
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showNotification(enabled ? "Фары включены" : "Фары выключены");
      }
    })
    .catch((error) => {
      console.error("Ошибка переключения фар:", error);
    });
}

// Функции сканирования
function startSafeScan() {
  if (scanActive) return;
  scanActive = true;
  showNotification("Начато безопасное сканирование", "info");
  simulateScan();
}

function startBruteForceScan() {
  if (scanActive) return;
  scanActive = true;
  showNotification(
    "Внимание: брутфорс сканирование может быть опасным!",
    "warning"
  );
  simulateScan();
}

function startDeepScan() {
  if (scanActive) return;
  scanActive = true;
  showNotification("Начато глубокое сканирование", "info");
  simulateScan();
}

function simulateScan() {
  const progressBar = document.getElementById("scanProgress");
  const progressText = document.getElementById("scanProgressText");
  const scanStatus = document.getElementById("scanStatus");
  const foundRegisters = document.getElementById("foundRegisters");

  let progress = 0;
  const total = 256;

  scanStatus.textContent = "Сканирование...";

  const interval = setInterval(() => {
    progress += 4;
    if (progress > total) progress = total;

    progressBar.style.width = (progress / total) * 100 + "%";
    progressText.textContent = progress + "/" + total;

    if (progress >= total) {
      clearInterval(interval);
      scanActive = false;
      scanStatus.textContent = "Сканирование завершено";
      showNotification("Найдено 15 новых регистров", "success");

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
  const index = document.getElementById("manualIndex").value;
  const value = document.getElementById("manualValue").value;

  // Валидация hex значения
  if (!/^0x[0-9A-Fa-f]{1,2}$/.test(index)) {
    showNotification("Неверный формат индекса регистра", "error");
    return;
  }

  // Валидация значения
  if (value && !/^0x[0-9A-Fa-f]{1,4}$/.test(value)) {
    showNotification("Неверный формат значения", "error");
    return;
  }

  if (!index) {
    showNotification("Введите индекс регистра", "error");
    return;
  }

  showNotification(
    `Тестирование регистра ${index} со значением ${value || "чтение"}`,
    "info"
  );

  // Здесь должна быть реальная логика тестирования
  setTimeout(() => {
    showNotification("Тест выполнен успешно", "success");
  }, 1000);
}

function startPacketLog() {
  showNotification("Логирование пакетов начато", "info");
}

function stopPacketLog() {
  showNotification("Логирование пакетов остановлено", "info");
}

function clearPacketLog() {
  document.getElementById("packetLog").innerHTML = "";
  showNotification("Лог очищен", "info");
}

function loadDiagnostics() {
  // Здесь должна быть логика загрузки диагностики
  showNotification("Диагностика обновлена", "info");
}

function loadSystemInfo() {
  // Здесь должна быть логика загрузки информации о системе
  showNotification("Информация о системе обновлена", "info");
}

/**
 * Проверяет наличие обновлений на GitHub Releases
 * @param {string} currentVersion - Текущая версия приложения (формат: v1.0.0 или 1.0.0)
 * @param {string} repoOwner - Владелец репозитория (например: 'user')
 * @param {string} repoName - Название репозитория (например: 'my-app')
 * @returns {Promise<Object>} - Объект с информацией об обновлении или null
 */
async function checkForUpdates(currentVersion, repoOwner, repoName, forceCheck = false) {
  try {
    // Показываем статус проверки
    updateOTAInfo({
      updateAvailable: false,
      updateStatus: "Проверка обновлений..."
    });
    
    // Проверяем, не проверяли ли мы недавно (кэш на 1 час)
    const cacheKey = 'github_update_check';
    const now = Date.now();
    const oneHour = 3600000;
    
    if (!forceCheck) {
      const cached = localStorage.getItem(cacheKey);
      if (cached) {
        const { timestamp, data } = JSON.parse(cached);
        if (now - timestamp < oneHour) {
          // Используем кэшированные данные
          updateOTAInfo(data);
          showNotification("Используются кэшированные данные обновлений", "info");
          return data;
        }
      }
    }
    
    // Нормализация версии
    const normalizeVersion = (version) => version.replace(/^v/, '').trim();
    const current = normalizeVersion(currentVersion);
    
    // Запрос к GitHub API
    const response = await fetch(
      `https://api.github.com/repos/${repoOwner}/${repoName}/releases/latest`,
      { cache: 'no-cache' }
    );
    
    if (!response.ok) {
      throw new Error(`GitHub API error: ${response.status}`);
    }
    
    const latestRelease = await response.json();
    
    if (!latestRelease.tag_name) {
      throw new Error("Нет информации о версии в релизе");
    }
    
    const latest = normalizeVersion(latestRelease.tag_name);
    const hasUpdate = compareVersions(latest, current) > 0;
    
    // Формируем данные для OTA информации
    const otaData = {
      firmwareVersion: current,
      latestVersion: latest,
      updateAvailable: hasUpdate,
      freeMemory: navigator.deviceMemory ? `${navigator.deviceMemory * 1024 * 1024}` : 'Неизвестно',
      deviceId: generateDeviceId(),
      lastUpdate: new Date().toLocaleString('ru-RU'),
      releaseNotes: latestRelease.body || "Нет описания",
      downloadUrl: latestRelease.html_url,
      assets: latestRelease.assets || []
    };
    
    // Обновляем OTA информацию
    updateOTAInfo(otaData);
    
    // Кэшируем результат
    localStorage.setItem(cacheKey, JSON.stringify({
      timestamp: now,
      data: otaData
    }));
    
    // Показываем уведомление если есть обновление
    if (hasUpdate) {
      const updateMessage = `
Доступно обновление!
Текущая: v${current}
Новая: v${latest}
${latestRelease.name ? `Название: ${latestRelease.name}` : ''}
Нажмите для подробностей`.trim();
      
      showNotification(updateMessage, "warning", {
        duration: 8000,
        onClick: () => {
          // Открываем модальное окно или страницу с деталями
          showUpdateDetailsModal(otaData);
        }
      });
    } else {
      showNotification("У вас актуальная версия", "success");
    }
    
    return otaData;
    
  } catch (error) {
    console.error("Ошибка при проверке обновлений:", error);
    
    // Обновляем OTA информацию с ошибкой
    updateOTAInfo({
      updateAvailable: false,
      updateStatus: `Ошибка: ${error.message}`,
      lastUpdate: new Date().toLocaleString('ru-RU')
    });
    
    showNotification(`Ошибка проверки: ${error.message}`, "error");
    return null;
  }
}

/**
 * Сравнивает две версии в формате семантического версионирования
 * @param {string} a - Первая версия
 * @param {string} b - Вторая версия
 * @returns {number} - 1 если a > b, -1 если a < b, 0 если равны
 */
function compareVersions(a, b) {
  const partsA = a.split('.').map(Number);
  const partsB = b.split('.').map(Number);
  
  for (let i = 0; i < Math.max(partsA.length, partsB.length); i++) {
    const partA = partsA[i] || 0;
    const partB = partsB[i] || 0;
    
    if (partA > partB) return 1;
    if (partA < partB) return -1;
  }
  
  return 0;
}

/**
 * Генерирует уникальный ID устройства
 */
function generateDeviceId() {
  let deviceId = localStorage.getItem('device_id');
  if (!deviceId) {
    deviceId = 'device_' + Math.random().toString(36).substr(2, 9);
    localStorage.setItem('device_id', deviceId);
  }
  return deviceId;
}

/**
 * Модальное окно с деталями обновления
 */
function showUpdateDetailsModal(otaData) {
  // Создаем модальное окно
  const modal = document.createElement('div');
  modal.className = 'update-modal';
  modal.innerHTML = `
    <div class="update-modal-content">
      <h2>Информация об обновлении</h2>
      
      <div class="update-info">
        <div class="info-row">
          <span class="label">Текущая версия:</span>
          <span class="value">v${otaData.firmwareVersion}</span>
        </div>
        
        <div class="info-row">
          <span class="label">Доступная версия:</span>
          <span class="value">v${otaData.latestVersion}</span>
        </div>
        
        ${otaData.releaseNotes ? `
        <div class="info-row">
          <span class="label">Описание обновления:</span>
          <div class="release-notes">${otaData.releaseNotes}</div>
        </div>
        ` : ''}
        
        <div class="info-row">
          <span class="label">Дата проверки:</span>
          <span class="value">${otaData.lastUpdate}</span>
        </div>
        
        <div class="info-row">
          <span class="label">ID устройства:</span>
          <span class="value">${otaData.deviceId}</span>
        </div>
      </div>
      
      <div class="modal-buttons">
        <button id="closeUpdateModal" class="btn-secondary">Закрыть</button>
        <button id="downloadUpdate" class="btn-primary">Скачать обновление</button>
      </div>
    </div>
  `;
  
  document.body.appendChild(modal);
  
  // Обработчики событий
  document.getElementById('closeUpdateModal').onclick = () => {
    document.body.removeChild(modal);
  };
  
  document.getElementById('downloadUpdate').onclick = () => {
    if (otaData.downloadUrl) {
      window.open(otaData.downloadUrl, '_blank');
    }
  };
  
  // Закрытие по клику вне модалки
  modal.onclick = (e) => {
    if (e.target === modal) {
      document.body.removeChild(modal);
    }
  };
}

/**
 * Инициализация проверки обновлений (упрощенная версия)
 */
function initUpdateChecker() {
  // Автоматическая проверка при загрузке
  setTimeout(() => {
    checkForUpdates(CURRENT_VERSION, REPO_OWNER, REPO_NAME, false);
    checkFirmwareInfo();
  }, 3000);
  
  // Периодическая проверка каждые 30 минут
  setInterval(() => {
    checkForUpdates(CURRENT_VERSION, REPO_OWNER, REPO_NAME, false);
  }, 30 * 60 * 1000);
}

/**
 * Проверяет обновления и показывает уведомление
 * @param {string} currentVersion - Текущая версия приложения
 * @param {string} repoOwner - Владелец репозитория
 * @param {string} repoName - Название репозитория
 */
async function checkForUpdatesAndNotify(currentVersion, repoOwner, repoName) {
  const updateInfo = await checkForUpdates(currentVersion, repoOwner, repoName);

  if (updateInfo?.hasUpdate) {
    const notificationMessage = `
        Доступно обновление!
        Текущая версия: ${updateInfo.currentVersion}
        Новая версия: ${updateInfo.latestVersion}`;

    showNotification(notificationMessage, {
      duration: 10000,
    });
  }
}

/**
 * Проверка доступности OTA соединения
 */
async function testOTAConnection() {
  try {
    showNotification("Тестирование OTA соединения...", "info");
    
    const response = await fetch('/update', { method: 'HEAD' });
    
    if (response.ok) {
      showNotification("OTA соединение доступно", "success");
      return true;
    } else {
      showNotification(`OTA недоступно: ${response.status}`, "error");
      return false;
    }
  } catch (error) {
    showNotification(`Ошибка OTA: ${error.message}`, "error");
    return false;
  }
}

/**
 * Показ инструкций по OTA обновлению
 */
function showOTAInstructions() {
  const instructions = `
<h3>Инструкция по OTA обновлению</h3>

<strong>Автоматическое обновление:</strong>
<ol>
  <li>Нажмите "Проверить обновления"</li>
  <li>Если есть обновление, нажмите "Установить обновление"</li>
  <li>Дождитесь завершения процесса (1-2 минуты)</li>
  <li>Устройство перезагрузится автоматически</li>
</ol>

<strong>Ручное обновление:</strong>
<ol>
  <li>Скачайте файл прошивки (.bin) с GitHub</li>
  <li>Нажмите "Выбрать файл" и выберите скачанный файл</li>
  <li>Дождитесь завершения загрузки</li>
  <li>Устройство перезагрузится автоматически</li>
</ol>

<strong>Важно:</strong>
<ul>
  <li>Не выключайте устройство во время обновления</li>
  <li>Убедитесь, что батарея заряжена более чем на 50%</li>
  <li>Держите устройство в зоне стабильного Wi-Fi соединения</li>
</ul>
`;
  
  // Создаем модальное окно с инструкциями
  const modal = document.createElement('div');
  modal.className = 'update-modal';
  modal.innerHTML = `
    <div class="update-modal-content">
      <h2>Инструкция по обновлению</h2>
      <div class="instructions-content">
        ${instructions}
      </div>
      <div class="modal-buttons" style="margin-top: 20px;">
        <button id="closeInstructions" class="btn-primary">Понятно</button>
      </div>
    </div>
  `;
  
  document.body.appendChild(modal);
  
  document.getElementById('closeInstructions').onclick = () => {
    document.body.removeChild(modal);
  };
  
  modal.onclick = (e) => {
    if (e.target === modal) {
      document.body.removeChild(modal);
    }
  };
}

// Обновление расширенной информации (вкладка "Детали")
function updateExtendedInfo(data) {
  // Маппинг DOM id -> ключи в данных с форматированием
  const extendedMapping = {
    // Напряжение и ток
    driveVoltageValue: {
      key: "driveVoltage",
      format: (val) =>
        `${(val / 100).toFixed(1)}<span style="font-size: 0.8rem">V</span>`,
    },
    motorCurrentValue: {
      key: "motorCurrent",
      format: (val) =>
        `${(val / 100).toFixed(1)}<span style="font-size: 0.8rem">A</span>`,
    },
    powerValue: {
      key: "power",
      format: (val) => `${val}<span style="font-size: 0.8rem">W</span>`,
    },
    avgSpeedValue: {
      key: "averageSpeed",
      format: (val) => `${val}<span style="font-size: 0.8rem">км/ч</span>`,
    },

    // Температуры
    bodyTempValue: {
      key: "bodyTemp",
      format: (val) => `${val}°`,
    },
    bat1TempValue: {
      key: "bat1Temp",
      format: (val) => `${val}°`,
    },
    bat2TempValue: {
      key: "bat2Temp",
      format: (val) => `${val}°`,
    },
    mosTempValue: {
      key: "mosTemp",
      format: (val) => `${val}°`,
    },

    // Батареи
    battery1Value: {
      key: "battery1",
      format: (val) => `${val}%`,
    },
    battery2Value: {
      key: "battery2",
      format: (val) => `${val}%`,
    },
    actualRangeValue: {
      key: "actualRange",
      format: (val) => `${val}<span style="font-size: 0.8rem">км</span>`,
    },
    predictedRangeValue: {
      key: "predictedRange",
      format: (val) => `${val}<span style="font-size: 0.8rem">км</span>`,
    },

    // Системная информация
    serialValue: { key: "serial" },
    bmsVersionValue: { key: "bmsVersion" },
    bms2VersionValue: { key: "bms2Version" },
    bleVersionValue: { key: "bleVersion" },
    boolStatusValue: {
      key: "boolStatus",
      format: (val) => `0x${val.toString(16).toUpperCase()}`,
    },
    alarmStatusValue: {
      key: "alarmStatus",
      format: (val) =>
        val === 0 ? "Нет" : `0x${val.toString(16).toUpperCase()}`,
    },
    errorValue: { key: "errorCode" },
  };

  // Обновляем каждый элемент
  for (const [domId, mapping] of Object.entries(extendedMapping)) {
    const element = document.getElementById(domId);
    if (element) {
      const value = data[mapping.key];

      if (value !== undefined && value !== null) {
        // Форматируем значение, если есть функция форматирования
        if (mapping.format) {
          if (domId.includes("Value") && domId !== "serialValue") {
            // Для value-display элементов используем innerHTML
            element.innerHTML = mapping.format(value);
          } else {
            // Для обычных текстовых элементов
            element.textContent = mapping.format(value);
          }
        } else {
          element.textContent = value;
        }
      } else {
        // Если данных нет
        if (domId.includes("Value") && domId !== "serialValue") {
          element.innerHTML = `0<span style="font-size: 0.8rem">${getUnitFromId(
            domId
          )}</span>`;
        } else {
          element.textContent = "N/A";
        }
      }
    }
  }

  // Дополнительная обработка для элементов, которые уже есть на других вкладках
  // (чтобы избежать дублирования обновлений)
  updateDuplicateElements(data);
}

// Вспомогательная функция для определения единиц измерения по ID
function getUnitFromId(id) {
  if (id.includes("Voltage")) return "V";
  if (id.includes("Current")) return "A";
  if (id.includes("Speed")) return "км/ч";
  if (id.includes("Range")) return "км";
  if (id.includes("Temp")) return "°C";
  if (id.includes("Power")) return "W";
  return "";
}

// Функция для обновления элементов, которые есть на нескольких вкладках
function updateDuplicateElements(data) {
  // Элементы, которые могут быть на разных вкладках
  const duplicates = [
    { id: "actualRangeValue", key: "actualRange", isHtml: true },
    {
      id: "speedLimitValue",
      key: "speedLimit",
      format: (val) => `${val / 10} км/ч`,
    },
  ];

  duplicates.forEach((item) => {
    const element = document.getElementById(item.id);
    if (element && data[item.key] !== undefined) {
      if (item.format) {
        element.textContent = item.format(data[item.key]);
      } else if (item.isHtml) {
        element.innerHTML = `${
          data[item.key]
        }<span style="font-size: 0.8rem">км</span>`;
      } else {
        element.textContent = data[item.key];
      }
    }
  });
}
// Обновление статистики
function updateStats(data) {
  // Маппинг DOM id -> ключи в данных
  const statsMapping = {
    // DOM id: ключ в объекте data
    singleMileageValue: "singleMileage",
    singleRideTimeValue: "singleRideTime",
    totalOperationTimeValue: "totalOperationTime",
    totalRideTimeValue: "totalRideTime",
    workModeValue: "workMode",
    errorCodeValue: "errorCode",
    cruiseValue: "cruise",
    headlightValue: "headlight",
    beepValue: "beep",
  };

  // Обновляем каждый элемент
  for (const [domId, dataKey] of Object.entries(statsMapping)) {
    const element = document.getElementById(domId);
    if (element) {
      // Если данные есть - отображаем, иначе "N/A"
      if (data[dataKey] !== undefined && data[dataKey] !== null) {
        element.textContent = data[dataKey];
      } else if (dataKey === "speedLimit" && data.speedLimit !== undefined) {
        // Специальная обработка для лимита скорости
        element.textContent = data.speedLimit / 10 + " км/ч";
      } else {
        element.textContent = "N/A";
      }
    }
  }

  // Если в data есть статистические данные напрямую
  if (data.stats) {
    document.getElementById("singleMileageValue").textContent =
      data.stats.singleMileage || "0";
    document.getElementById("singleRideTimeValue").textContent =
      data.stats.singleRideTime || "0";
    // ... и так далее
  }
}

// Обновление системной информации
function updateSystemInfo(data) {
  // Маппинг DOM id -> ключи в данных
  const systemMapping = {
    currentLedMode: "ledMode",
    fwVersion: "firmwareVersion",
    workSystem: "workSystem",
    lockStatus: data.isLocked ? "Заблокирован" : "Разблокирован",
    engineStatus: "motorEnabled" ? "ВКЛ" : "ВЫКЛ",
    diagnosticError: "errorCode",
    diagnosticAlarm: "alarmCode",
    diagnosticBool: data.boolStatus
      ? "0x" + data.boolStatus.toString(16)
      : "0x0000",
    diagnosticQuickBool: data.quickBoolStatus
      ? "0x" + data.quickBoolStatus.toString(16)
      : "0x0000",
  };

  // Обновляем каждый элемент
  for (const [domId, value] of Object.entries(systemMapping)) {
    const element = document.getElementById(domId);
    if (element) {
      if (typeof value === "string" && value in data) {
        // Если значение - это ключ из data
        element.textContent = data[value] !== undefined ? data[value] : "N/A";
      } else {
        // Если значение уже готово для отображения
        element.textContent = value;
      }
    }
  }
}

// Автообновление
function startAutoRefresh() {
  autoRefreshInterval = setInterval(loadStatus, 3000);
}

/**
 * Обновление OTA информации (обновленная версия)
 */
function updateOTAInfo(data = {}) {
  const otaElements = {
    currentVersion: { 
      key: "firmwareVersion", 
      default: "Неизвестно",
      fallback: CURRENT_VERSION
    },
    freeMemory: {
      key: "freeMemory",
      format: (val) => {
        if (val >= 1048576) return `${(val / 1048576).toFixed(1)} MB`;
        if (val >= 1024) return `${(val / 1024).toFixed(1)} KB`;
        return `${val} байт`;
      },
      default: "Неизвестно",
    },
    deviceId: { 
      key: "deviceId", 
      default: "Неизвестно",
      fallback: generateDeviceId()
    },
    lastUpdate: { 
      key: "lastUpdate", 
      default: "Никогда",
      format: (val) => {
        if (val instanceof Date) return val.toLocaleString('ru-RU');
        if (typeof val === 'string') return val;
        return new Date().toLocaleString('ru-RU');
      }
    },
    updateStatus: {
      key: "updateAvailable",
      format: (val) => {
        if (val === true) return "Доступно обновление";
        if (val === false) return "Актуальная версия";
        if (typeof val === 'string') return val;
        return "Готов к обновлению";
      },
      default: "Готов к обновлению",
    },
    chipId: {
      key: "chipId",
      default: "Неизвестно",
      format: (val) => val ? `0x${parseInt(val).toString(16).toUpperCase()}` : "Неизвестно"
    },
    sketchSize: {
      key: "sketchSize",
      format: (val) => formatBytes(val),
      default: "Неизвестно"
    },
    flashSize: {
      key: "flashSize",
      format: (val) => formatBytes(val),
      default: "Неизвестно"
    },
    sdkVersion: {
      key: "sdkVersion",
      default: "Неизвестно"
    },
    cycleCount: {
      key: "cycleCount",
      format: (val) => val ? val.toLocaleString('ru-RU') : "0",
      default: "0"
    }
  };

  for (const [domId, mapping] of Object.entries(otaElements)) {
    const element = document.getElementById(domId);
    if (element) {
      const value = data[mapping.key];

      if (value !== undefined && value !== null) {
        if (mapping.format) {
          element.textContent = mapping.format(value);
        } else {
          element.textContent = value;
        }
      } else if (mapping.fallback !== undefined) {
        element.textContent = mapping.fallback;
      } else {
        element.textContent = mapping.default;
      }
    }
  }
  
  // Обновляем кнопку ручного обновления
  const manualUpdateBtn = document.getElementById('manualUpdateBtn');
  const autoUpdateCard = document.getElementById('autoUpdateCard');
  
  if (data.updateAvailable === true) {
    if (manualUpdateBtn) {
      manualUpdateBtn.disabled = false;
      manualUpdateBtn.textContent = "🚀 Установить обновление";
    }
    
    if (autoUpdateCard) {
      autoUpdateCard.style.display = 'block';
      document.getElementById('currentVersionDisplay').textContent = `v${data.currentVersion || CURRENT_VERSION}`;
      document.getElementById('latestVersionDisplay').textContent = `v${data.latestVersion || '0.0.0'}`;
      document.getElementById('newVersionText').textContent = `v${data.latestVersion} доступна`;
      
      if (data.releaseNotes) {
        document.getElementById('releaseNotes').innerHTML = `
          <p style="margin-top: 0; font-weight: bold;">Что нового:</p>
          <div style="white-space: pre-line;">${data.releaseNotes.substring(0, 300)}${data.releaseNotes.length > 300 ? '...' : ''}</div>
        `;
      }
    }
  } else {
    if (manualUpdateBtn) {
      manualUpdateBtn.disabled = true;
      manualUpdateBtn.textContent = "Нет обновлений";
    }
    
    if (autoUpdateCard) {
      autoUpdateCard.style.display = 'none';
    }
  }
  
  // Обновляем отображение текущей версии
  const currentVersionDisplay = document.getElementById('currentVersionDisplay');
  if (currentVersionDisplay) {
    currentVersionDisplay.textContent = `v${data.firmwareVersion || CURRENT_VERSION}`;
  }
}

/**
 * Форматирование байтов в читаемый вид
 */
function formatBytes(bytes) {
  if (!bytes || bytes === 0) return '0 Bytes';
  const k = 1024;
  const sizes = ['Bytes', 'KB', 'MB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

/**
 * Форматирование байтов в читаемый вид
 */
function formatBytes(bytes) {
  if (bytes === 0) return '0 Bytes';
  const k = 1024;
  const sizes = ['Bytes', 'KB', 'MB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function toggleDataRefresh() {
  if (autoRefreshInterval) {
    clearInterval(autoRefreshInterval);
    autoRefreshInterval = null;
    showNotification("Автообновление выключено", "warning");
  } else {
    startAutoRefresh();
    showNotification("Автообновление включено", "success");
  }
}

// Уведомления
function showNotification(message, type = "info") {
  const notification = document.getElementById("notification");
  const text = document.getElementById("notificationText");
  const icon = notification.querySelector(".notification-icon");

  text.textContent = message;
  notification.className = `notification ${type}`;

  switch (type) {
    case "error":
      icon.textContent = "❌";
      break;
    case "warning":
      icon.textContent = "⚠️";
      break;
    case "success":
      icon.textContent = "✅";
      break;
    default:
      icon.textContent = "ℹ️";
  }

  notification.classList.add("show");

  setTimeout(() => {
    notification.classList.remove("show");
  }, 3000);
}

// Функция для обновления неизвестных регистров
function updateUnknownRegisters(data) {
  // Маппинг: DOM id -> ключ в данных
  const registerMapping = {
    UnkReg1: "register0x00",
    UnkReg2: "register0x01",
    UnkReg8: "register0x2C",
    UnkReg15: "register0x4C",
    UnkReg18: "register0x51",
    UnkReg19: "register0x52",
    UnkReg20: "register0x54",
    UnkReg21: "register0x56",
    UnkReg22: "register0x57",
    UnkReg26: "register0x66",
    UnkReg28: "register0x7F",
    UnkReg30: "register0x80",
  };

  // Обновляем каждый регистр
  for (const [domId, dataKey] of Object.entries(registerMapping)) {
    const element = document.getElementById(domId);
    if (element) {
      // Проверяем наличие данных разными способами
      if (data[dataKey] !== undefined) {
        // Если данные пришли по прямому ключу
        element.textContent = formatRegisterValue(data[dataKey]);
      } else if (data.registers && data.registers[dataKey] !== undefined) {
        // Если данные в объекте registers
        element.textContent = formatRegisterValue(data.registers[dataKey]);
      } else if (
        data.unknownRegisters &&
        data.unknownRegisters[dataKey] !== undefined
      ) {
        // Если данные в объекте unknownRegisters
        element.textContent = formatRegisterValue(
          data.unknownRegisters[dataKey]
        );
      } else {
        element.textContent = "N/A";
      }
    }
  }
}

// Вспомогательная функция для форматирования значения регистра
function formatRegisterValue(value) {
  if (value === undefined || value === null) return "N/A";

  // Если значение - число, показываем в hex и decimal
  if (typeof value === "number") {
    return `0x${value.toString(16).toUpperCase()} (${value})`;
  }

  // Если значение - строка в hex
  if (typeof value === "string" && value.startsWith("0x")) {
    const decimal = parseInt(value, 16);
    return `${value.toUpperCase()} (${decimal})`;
  }

  // Любое другое значение
  return String(value);
}

// Обработка онлайн/офлайн статуса
window.addEventListener("online", () => {
  showNotification("Соединение восстановлено", "success");
  loadStatus();
});

window.addEventListener("offline", () => {
  showNotification("Нет соединения", "error");
});

// Предотвращение сна экрана (если поддерживается)
let wakeLock = null;
if ("wakeLock" in navigator && isMobile) {
  try {
    navigator.wakeLock.request("screen").then((wl) => {
      wakeLock = wl;
    });
  } catch (err) {
    console.log("Wake Lock не поддерживается");
  }
}

// Освобождение Wake Lock при закрытии
window.addEventListener("beforeunload", () => {
  if (wakeLock) {
    wakeLock.release();
    wakeLock = null;
  }
});

// Обработка изменения размера окна
window.addEventListener("resize", function () {
  // Закрыть боковую панель на больших экранах
  if (window.innerWidth > 768) {
    const sidebar = document.querySelector(".sidebar");
    const overlay = document.querySelector(".sidebar-overlay");
    sidebar.classList.remove("mobile-visible");
    overlay.classList.remove("active");
    document.body.style.overflow = "auto";
  }
});

// Предотвращение контекстного меню на мобильных
if (isMobile) {
  document.addEventListener("contextmenu", function (e) {
    e.preventDefault();
  });
}

function updateMaterialBattery(percent, isCharging = false) {
  // Получаем элементы
  const battery = document.getElementById("materialBattery");
  const fill = document.getElementById("materialBatteryFill");
  const percentText = document.getElementById("materialBatteryPercent");

  // Обновляем уровень
  if (fill) fill.style.width = `${percent}%`;
  if (percentText) percentText.textContent = `${percent}%`;

  // Сбрасываем классы
  if (battery) {
    battery.classList.remove("charging", "low");
    battery.removeAttribute("data-theme");

    // Устанавливаем состояние
    if (isCharging) {
      battery.classList.add("charging");
      battery.setAttribute("data-theme", "green");
    } else {
      // Устанавливаем цвет в зависимости от уровня
      if (percent > 50) {
        battery.setAttribute("data-theme", "green");
      } else if (percent > 20) {
        battery.setAttribute("data-theme", "yellow");
      } else {
        battery.setAttribute("data-theme", "red");
        battery.classList.add("low");
      }
    }
  }

  // Обновляем компактную версию для хедера
  updateCompactBattery(percent, isCharging);
}

// Функция для компактной батареи
function updateCompactBattery(percent, isCharging = false) {
  const compactBattery = document.getElementById("headerBattery");
  const compactFill = document.getElementById("compactBatteryFill");
  const compactText = document.getElementById("compactBatteryText");
  const boltIcon = document.getElementById("chargingBoltIcon");

  if (compactFill) compactFill.style.width = `${percent}%`;
  if (compactText) compactText.textContent = `${percent}%`;

  if (compactBattery) {
    compactBattery.classList.remove("charging", "low");

    if (isCharging) {
      compactBattery.classList.add("charging");
      if (boltIcon) boltIcon.style.display = "inline";
    } else {
      if (boltIcon) boltIcon.style.display = "none";

      if (percent < 20) {
        compactBattery.classList.add("low");
      }
    }
  }
}

if (history.scrollRestoration) {
  history.scrollRestoration = "manual";
}

window.addEventListener("load", function () {
  // Даем время на рендеринг
  setTimeout(function () {
    window.scrollTo(0, 0);
  }, 100);
});
