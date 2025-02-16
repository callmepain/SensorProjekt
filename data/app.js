// ========== ROUTER & PAGE MANAGEMENT ==========
class Router {
    constructor() {
        this.routes = {
            '': 'home',
            '#home': 'home',
            '#charts': 'charts',
            '#files': 'files',
            '#config': 'config',
            '#upload': 'upload',
            '#ota': 'ota',
            '#logs': 'logs'
        };

        // Event Listener für Hash-Änderungen
        window.addEventListener('hashchange', () => this.handleRoute());
        window.addEventListener('load', () => this.handleRoute());
    }

    async handleRoute() {
        const hash = window.location.hash;
        const page = this.routes[hash] || 'home';
        
        // Aktiven Link markieren
        document.querySelectorAll('.menu a').forEach(link => {
            link.classList.remove('active');
            if (link.getAttribute('href') === hash || (hash === '' && link.getAttribute('href') === '#home')) {
                link.classList.add('active');
            }
        });

        // Template laden und anzeigen
        const template = document.getElementById(`${page}-template`);
        if (template) {
            const content = template.content.cloneNode(true);
            const appContent = document.getElementById('app-content');
            appContent.innerHTML = '';
            appContent.appendChild(content);

            // Seitenspezifische Initialisierung
            switch(page) {
                case 'home':
                    initHome();
                    break;
                case 'charts':
                    initCharts();
                    break;
                case 'files':
                    initFiles();
                    break;
                case 'config':
                    initConfig();
                    break;
                case 'upload':
                    checkUploaded();
                    break;
                case 'ota':
                    // OTA spezifische Initialisierung wenn nötig
                    break;
            }

            // Wenn Logs-Seite geladen wird, automatisch Logs abrufen
            if (page === 'logs') {
                refreshLogs();  // Logs sofort laden
            }
        }
    }
}

// ========== ERROR HANDLING & API ==========
const USE_MOCK_API = false;

function setupErrorHandling() {
    window.onerror = function(msg, url, line) {
        console.error('JavaScript Error:', msg, 'Line:', line);
        return false;
    };

    window.addEventListener('unhandledrejection', event => {
        console.error('Unhandled Promise Rejection:', event.reason);
    });

    window.addEventListener('online', () => {
        console.log('Verbindung wiederhergestellt');
        location.reload();
    });

    window.addEventListener('offline', () => {
        console.log('Verbindung verloren');
        showOfflineWarning();
    });
}

async function checkAPIConnection() {
    try {
        const response = await fetch('/api/status');
        if (!response.ok) {
            showAPIError();
        }
    } catch (error) {
        console.error('API nicht erreichbar:', error);
        showAPIError();
    }
}

// ========== HOME PAGE FUNCTIONS ==========
function initHome() {
    getSensorData();
    checkDisplayState(); // Status beim Laden prüfen
    setInterval(getSensorData, 30000);
}

async function getSensorData() {
    const fields = [
        'currentTemp', 'minTemp', 'maxTemp', 'humidity', 'pressure',
        'voltage', 'current', 'power', 'lux', 'distance',
        'batteryVoltage', 'batteryCurrent', 'batteryPower'
    ];
    
    try {
        fields.forEach(id => toggleFieldLoader(id, true));
        
        const response = await fetch('/api/sensordaten');
        if (!response.ok) throw new Error('Netzwerk-Fehler');
        const data = await response.json();

        updateValue('currentTemp', data.temperatur + ' °C');
        drawGauge('temperatureGauge', data.temperatur, 30, 'Temperatur', data.minTemperatur, data.maxTemperatur, '°C', 15);
        updateValue('minTemp', data.minTemperatur + ' °C');
        drawGauge('mintemperatureGauge', data.minTemperatur, 30, 'minTemperatur', 0, 0, '°C', 15);
        updateValue('maxTemp', data.maxTemperatur + ' °C');
        drawGauge('maxtemperatureGauge', data.maxTemperatur, 30, 'maxTemperatur', 0, 0, '°C', 15);
        updateValue('humidity', data.luftfeuchtigkeit + ' %');
        drawGauge('humidityGauge', data.luftfeuchtigkeit, 100, 'Luftfeuchtigkeit', 0, 0, '%', 0);
        updateValue('pressure', data.druck + ' hPa');
        drawGauge('pressureGauge', data.druck, 1500, 'Druck', 0, 0, 'hPa', 0);
        updateValue('voltage', data.spannung + ' V');
        updateValue('current', data.stromstärke + ' mA');
        updateValue('power', data.leistung + ' mW');
        updateValue('lux', data.helligkeit + ' lx');
        drawGauge('luxGauge', data.helligkeit, 4000, 'Lux', 0, 0, 'lx', 0);
        updateValue('batteryVoltage', data.battery_spannung + ' V');
        updateValue('batteryCurrent', data.battery_stromstärke_mA + ' mA');
        updateValue('batteryPower', data.battery_leistung + ' mW');
        
        // Prüfe ob Laden oder Entladen
        if (data.battery_stromstärke_mA > 0) {
            // Entladen: Zeige Restlaufzeit
            updateValue('batterytime', data.remaining_time + ' h');
        } else {
            // Laden: Zeige geschätzte Ladezeit
            updateValue('batterytime', data.charging_time + ' h');
        }
        
        updateBatteryStatus(data.battery_stromstärke_mA, data.battery_prozent);

        checkDisplayState(); // Status nach dem Toggle aktualisieren
    } catch (error) {
        console.error(error);
        fields.forEach(id => updateValue(id, 'Fehler', true));
    } finally {
        fields.forEach(id => toggleFieldLoader(id, false));
    }
}

async function toggleDisplay() {
    try {
        const currentState = document.getElementById('display-status').textContent;
        const newState = currentState === 'AN' ? 'off' : 'on';
        
        const response = await fetch('/api/display', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `state=${newState}`
        });
        
        if (!response.ok) throw new Error('Netzwerk-Fehler');
        const data = await response.json();
        if (!data.message) {
            throw new Error(data.error || 'Unbekannter Fehler');
        }
        checkDisplayState();
    } catch (error) {
        showMessage('Fehler: ' + error.message, true);
    }
}

async function resetMinMax() {
    try {
        toggleLoadingIndicator(true);
        const response = await fetch('/api/reset', { method: 'POST' });
        if (response.ok) {
            showMessage("Werte erfolgreich zurückgesetzt!");
            await getSensorData();
        } else {
            showMessage("Fehler beim Zurücksetzen der Werte!", true);
        }
    } catch (error) {
        console.error(error);
        showMessage("Fehler: " + error.message, true);
    } finally {
        toggleLoadingIndicator(false);
    }
}

// ========== CHARTS PAGE FUNCTIONS ==========
let charts = {};
let dataLimit = 50;

function initCharts() {
    const limitDropdown = document.getElementById('data-limit');
    if (limitDropdown) {
        limitDropdown.addEventListener('change', (event) => {
            const value = event.target.value;
            if (value === "24h") {
                dataLimit = 288; // 24 Stunden * 60 Minuten / 5 Minuten
            } else if (value === "3d") {
                dataLimit = 3 * 288;
            } else if (value === "7d") {
                dataLimit = 7 * 288;
            } else {
                dataLimit = parseInt(value);
            }
            renderCharts();
        });
    }
    renderCharts();
}

async function fetchData(url) {
    try {
        const response = await fetch(url);
        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        return await response.json();
    } catch (error) {
        console.error("Failed to fetch data:", error);
        return [];
    }
}

async function renderCharts() {
    const darkModeOptions = {
        plugins: {
            tooltip: {
                backgroundColor: 'rgba(136, 0, 255, 0.9)',
                borderColor: '#FF00FF',
                borderWidth: 2,
                titleColor: '#FFFFFF',
                bodyColor: '#FFFFFF',
                titleFont: { size: 14, weight: 'bold' },
                bodyFont: { size: 12 },
                displayColors: false,
                callbacks: {
                    labelTextColor: () => '#FFFFFF',
                },
                caretPadding: 10,
                padding: 12,
            },
            legend: {
                labels: { color: '#FFFFFF', font: { size: 14 } }
            }
        },
        scales: {
            x: {
                ticks: { color: '#FFFFFF', font: { size: 12 } },
                title: { display: true, text: 'Timestamp', color: '#FFFFFF', font: { size: 16 } }
            },
            y: {
                ticks: { color: '#FFFFFF', font: { size: 12 } },
                title: { display: true, text: '', color: '#FFFFFF', font: { size: 16 } }
            }
        },
        elements: {
            line: { borderWidth: 2 },
            point: { radius: 3, backgroundColor: '#FFFFFF' }
        }
    };

    try {
        // Voltage-Daten
        const voltageData = (await fetchData(`http://192.168.2.127/cgi-bin/api_handler2.py?action=voltage_data&limit=${dataLimit}`)).data;
        if (voltageData && Array.isArray(voltageData)) {
            const labels = voltageData.map(d => d.timestamp).reverse();
            const voltageValues = voltageData.map(d => d.voltage).reverse();
            const currentValues = voltageData.map(d => d.current_mA).reverse();
            const powerValues = voltageData.map(d => d.power).reverse();
            const luxValues = voltageData.map(d => d.lux).reverse();

            // Voltage Chart
            if (charts['voltageChart']) charts['voltageChart'].destroy();
            charts['voltageChart'] = new Chart(document.getElementById('voltageChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Voltage (V)', data: voltageValues, borderColor: 'red', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Current Chart
            if (charts['currentChart']) charts['currentChart'].destroy();
            charts['currentChart'] = new Chart(document.getElementById('currentChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Current (mA)', data: currentValues, borderColor: 'blue', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Power Chart
            if (charts['powerChart']) charts['powerChart'].destroy();
            charts['powerChart'] = new Chart(document.getElementById('powerChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Power (mW)', data: powerValues, borderColor: 'green', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Lux Chart
            if (charts['luxChart']) charts['luxChart'].destroy();
            charts['luxChart'] = new Chart(document.getElementById('luxChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Lux', data: luxValues, borderColor: 'orange', fill: false }]
                },
                options: { ...darkModeOptions }
            });
        }

        // Battery-Daten
        const batteryData = (await fetchData(`http://192.168.2.127/cgi-bin/api_handler2.py?action=battery_data&limit=${dataLimit}`)).data;
        if (batteryData && Array.isArray(batteryData)) {
            const labels = batteryData.map(d => d.timestamp).reverse();
            const voltageValues = batteryData.map(d => d.battery_voltage).reverse();
            const currentValues = batteryData.map(d => d.battery_current_mA).reverse();
            const powerValues = batteryData.map(d => d.battery_power).reverse();
            const percentageValues = batteryData.map(d => d.battery_percentage).reverse();

            // Battery Voltage Chart
            if (charts['batteryVoltageChart']) charts['batteryVoltageChart'].destroy();
            charts['batteryVoltageChart'] = new Chart(document.getElementById('batteryVoltageChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Battery Voltage (V)', data: voltageValues, borderColor: 'red', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Battery Current Chart
            if (charts['batteryCurrentChart']) charts['batteryCurrentChart'].destroy();
            charts['batteryCurrentChart'] = new Chart(document.getElementById('batteryCurrentChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Battery Current (mA)', data: currentValues, borderColor: 'blue', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Battery Power Chart
            if (charts['batteryPowerChart']) charts['batteryPowerChart'].destroy();
            charts['batteryPowerChart'] = new Chart(document.getElementById('batteryPowerChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Battery Power (mW)', data: powerValues, borderColor: 'green', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Battery Percentage Chart
            if (charts['batteryPercentageChart']) charts['batteryPercentageChart'].destroy();
            charts['batteryPercentageChart'] = new Chart(document.getElementById('batteryPercentageChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Battery Percentage (%)', data: percentageValues, borderColor: 'purple', fill: false }]
                },
                options: { ...darkModeOptions }
            });
        }

        // Sensor-Daten
        const sensorData = (await fetchData(`http://192.168.2.127/cgi-bin/api_handler2.py?action=sensor_data&limit=${dataLimit}`)).data;
        if (sensorData && Array.isArray(sensorData)) {
            const labels = sensorData.map(d => d.timestamp).reverse();
            const tempValues = sensorData.map(d => d.temperature).reverse();
            const humidityValues = sensorData.map(d => d.humidity).reverse();
            const pressureValues = sensorData.map(d => d.pressure).reverse();

            // Temperature Chart
            if (charts['temperatureChart']) charts['temperatureChart'].destroy();
            charts['temperatureChart'] = new Chart(document.getElementById('temperatureChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Temperature (°C)', data: tempValues, borderColor: 'orange', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Humidity Chart
            if (charts['humidityChart']) charts['humidityChart'].destroy();
            charts['humidityChart'] = new Chart(document.getElementById('humidityChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Humidity (%)', data: humidityValues, borderColor: 'blue', fill: false }]
                },
                options: { ...darkModeOptions }
            });

            // Pressure Chart
            if (charts['pressureChart']) charts['pressureChart'].destroy();
            charts['pressureChart'] = new Chart(document.getElementById('pressureChart').getContext('2d'), {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{ label: 'Pressure (hPa)', data: pressureValues, borderColor: 'green', fill: false }]
                },
                options: { ...darkModeOptions }
            });
        }
    } catch (error) {
        console.error("Fehler beim Laden der Chart-Daten:", error);
        showMessage("Fehler beim Laden der Chart-Daten", true);
    }
}

// ========== FILES PAGE FUNCTIONS ==========
async function initFiles() {
    await fetchStorageInfo();
    await fetchFileList();
    checkUploaded();
    checkSaved();
}

async function fetchFileList() {
    try {
        const response = await fetch('/api/files');
        const files = await response.json();
        const fileList = document.getElementById('file-list');
        if (!fileList) return;
        
        fileList.innerHTML = '';

        // Root-Ordner für SPIFFS
        const spiffsRow = document.createElement('tr');
        spiffsRow.innerHTML = `
            <td>
                <span style="cursor: pointer" onclick="toggleFolder('SPIFFS:')">
                    📁 SPIFFS
                </span>
            </td>
            <td>-</td>
            <td></td>
        `;
        fileList.appendChild(spiffsRow);

        // Container für SPIFFS-Inhalt
        const spiffsContainer = document.createElement('tr');
        spiffsContainer.id = 'folder-SPIFFS:';
        spiffsContainer.style.display = 'none';
        spiffsContainer.innerHTML = `
            <td colspan="3" style="padding-left: 30px">
                <div id="content-SPIFFS:"></div>
            </td>
        `;
        fileList.appendChild(spiffsContainer);

        // Root-Ordner für SD
        const sdRow = document.createElement('tr');
        sdRow.innerHTML = `
            <td>
                <span style="cursor: pointer" onclick="toggleFolder('SD:')">
                    📁 SD
                </span>
            </td>
            <td>-</td>
            <td></td>
        `;
        fileList.appendChild(sdRow);

        // Container für SD-Inhalt
        const sdContainer = document.createElement('tr');
        sdContainer.id = 'folder-SD:';
        sdContainer.style.display = 'none';
        sdContainer.innerHTML = `
            <td colspan="3" style="padding-left: 30px">
                <div id="content-SD:"></div>
            </td>
        `;
        fileList.appendChild(sdContainer);

    } catch (error) {
        console.error("Fehler beim Laden der Dateiliste:", error);
        showMessage("Fehler beim Laden der Dateiliste", true);
    }
}

async function fetchStorageInfo() {
    try {
        const response = await fetch('/api/storage');
        const info = await response.json();
        const storageInfo = document.getElementById('storage-info');

        function bytesToMB(bytes) {
            return (bytes / (1024 * 1024)).toFixed(2);
        }

        // Speicherinfo-Tabelle aktualisieren
        storageInfo.innerHTML = `
            <tr>
                <td>SPIFFS</td>
                <td>${bytesToMB(info.SPIFFS.total)} MB</td>
                <td>${bytesToMB(info.SPIFFS.used)} MB</td>
                <td>${bytesToMB(info.SPIFFS.free)} MB</td>
            </tr>
            <tr>
                <td>SD</td>
                <td>${info.SD.total ? bytesToMB(info.SD.total) + " MB" : "N/A"}</td>
                <td>${info.SD.used ? bytesToMB(info.SD.used) + " MB" : "N/A"}</td>
                <td>${info.SD.free ? bytesToMB(info.SD.free) + " MB" : "N/A"}</td>
            </tr>
        `;

        // Progress Bars aktualisieren
        const spiffsProgress = document.getElementById('spiffs-progress');
        const spiffsPercentage = ((info.SPIFFS.used / info.SPIFFS.total) * 100).toFixed(1);
        spiffsProgress.style.width = `${spiffsPercentage}%`;
        spiffsProgress.textContent = `${spiffsPercentage}%`;

        const sdProgress = document.getElementById('sd-progress');
        if (info.SD.total) {
            const sdPercentage = ((info.SD.used / info.SD.total) * 100).toFixed(1);
            sdProgress.style.width = `${sdPercentage}%`;
            sdProgress.textContent = `${sdPercentage}%`;
        } else {
            sdProgress.style.width = '0%';
            sdProgress.textContent = 'N/A';
        }
    } catch (error) {
        console.error('Error fetching storage info:', error);
    }
}

async function deleteFile(fileName) {
    if (confirm(`Möchten Sie die Datei '${fileName}' wirklich löschen?`)) {
        try {
            const response = await fetch(`/delete?name=${fileName}`, { method: 'DELETE' });
            if (response.ok) {
                showMessage('Datei erfolgreich gelöscht!');
                fetchFileList();
            } else {
                showMessage('Fehler beim Löschen der Datei!', true);
            }
        } catch (error) {
            console.error("Fehler beim Löschen:", error);
            showMessage('Fehler beim Löschen der Datei!', true);
        }
    }
}

async function toggleFolder(path) {
    const folderId = `folder-${path}`;
    const folderElement = document.getElementById(folderId);
    const contentId = `content-${path}`;
    
    if (folderElement.style.display === 'none') {
        folderElement.style.display = 'table-row';
        try {
            const response = await fetch(`/api/files?path=${encodeURIComponent(path)}`);
            const items = await response.json();
            
            if (!Array.isArray(items) || items.length === 0) {
                document.getElementById(contentId).innerHTML = `
                    <table style="width: 100%; margin: 0;"><tbody>
                        <tr><td colspan="3">Ordner ist leer</td></tr>
                    </tbody></table>`;
                return;
            }

            const prefix = path.startsWith('SD:') ? 'SD:' : 'SPIFFS:';
            document.getElementById(contentId).innerHTML = `
                <table style="width: 100%; margin: 0;"><tbody>
                    ${items.map(item => {
                        const fullPath = `${path}/${item.name}`.replace(/\/+/g, '/');
                        return `<tr>
                            <td style="padding-left: 20px">
                                ${item.type === 'dir' ? 
                                    `<span style="cursor: pointer" onclick="toggleFolder('${fullPath}')">
                                        📁 ${item.name}
                                    </span>` : 
                                    `📄 ${item.name}`}
                            </td>
                            <td>${item.type === 'dir' ? '-' : item.size + ' Bytes'}</td>
                            <td>
                                ${item.type === 'dir' ? '' : `
                                    <button class="download-button" onclick="location.href='/download?name=${fullPath}'">Download</button>
                                    <button class="delete-button" onclick="deleteFile('${fullPath}')">Löschen</button>
                                    <button class="edit-button" onclick="editFile('${fullPath}')">Edit</button>
                                `}
                            </td>
                        </tr>
                        ${item.type === 'dir' ? `
                        <tr id="folder-${fullPath}" style="display: none;">
                            <td colspan="3" style="padding-left: 40px">
                                <div id="content-${fullPath}"></div>
                            </td>
                        </tr>` : ''}`
                    }).join('')}
                </tbody></table>
            `;
        } catch (error) {
            console.error('Error fetching folder contents:', error);
            document.getElementById(contentId).innerHTML = `
                <table style="width: 100%; margin: 0;"><tbody>
                    <tr><td colspan="3">Fehler beim Laden des Ordnerinhalts: ${error.message}</td></tr>
                </tbody></table>
            `;
        }
    } else {
        folderElement.style.display = 'none';
    }
}

// ========== CONFIG PAGE FUNCTIONS ==========
function initConfig() {
    loadConfig();
    
    document.querySelector('button[type="submit"]')?.addEventListener('click', async function(event) {
        event.preventDefault();
        await saveConfig();
    });
}

async function loadConfig() {
    try {
        const response = await fetch('/api/loadConfig');
        const data = await response.json();
        if (data) {
            // Formularfelder mit den geladenen Daten füllen
            Object.keys(data).forEach(key => {
                const element = document.getElementById(key.replace(/_/g, '-'));
                if (element) {
                    if (element.type === 'checkbox') {
                        element.checked = data[key];
                    } else {
                        element.value = data[key];
                    }
                }
            });
            showMessage('Konfiguration erfolgreich geladen');
        }
    } catch (error) {
        console.error('Fehler beim Laden der Konfiguration:', error);
        showMessage('Fehler beim Laden der Konfiguration', true);
    }
}

async function saveConfig() {
    const configData = {
        device_name: document.getElementById('device-name').value,
        wifi_ssid: document.getElementById('wifi-ssid').value,
        wifi_password: document.getElementById('wifi-password').value,
        ip_address: document.getElementById('ip-address').value,
        subnet_mask: document.getElementById('subnet-mask').value,
        gateway: document.getElementById('gateway').value,
        primary_dns: document.getElementById('primary-dns').value,
        secondary_dns: document.getElementById('secondary-dns').value,
        tx_power: document.getElementById('tx-power').value,
        bluetooth_name: document.getElementById('bluetooth-name').value,
        bluetooth_enable: document.getElementById('bluetooth-enable').value === "1",
        led_enable: document.getElementById('led-enable').value === "1"
    };

    try {
        const response = await fetch('/api/saveConfig', {
            method: 'POST',
            body: JSON.stringify(configData),
            headers: {
                'Content-Type': 'application/json'
            }
        });

        if (response.ok) {
            showMessage('Einstellungen erfolgreich gespeichert!');
        } else {
            showMessage('Fehler beim Speichern der Einstellungen!', true);
        }
    } catch (error) {
        console.error('Fehler beim Speichern:', error);
        showMessage('Fehler beim Speichern der Einstellungen!', true);
    }
}

// ========== OTA FUNCTIONS ==========
function showLoader() {
    document.querySelector("form").style.display = "none";
    document.getElementById("loader").style.display = "block";
    document.getElementById("message").textContent = "Bitte warten, das Update wird durchgeführt...";
}

// ========== UTILITY FUNCTIONS ==========
function showMessage(text, isError = false) {
    const msg = document.getElementById('success-message');
    if (msg) {
        msg.textContent = text;
        msg.style.display = 'block';
        msg.style.background = isError ? 
            'linear-gradient(90deg, #FF0000, #CC0000)' : 
            'linear-gradient(90deg, #4CAF50, #2E7D32)';
        
        setTimeout(() => {
            msg.style.display = 'none';
        }, 5000);
    }
}

function updateValue(id, value, isError = false) {
    const element = document.getElementById(id);
    if (!element) return;
    element.innerText = value;
    element.style.color = isError ? 'red' : '';
}

function toggleFieldLoader(id, show) {
    const field = document.getElementById(id);
    if (!field) return;
    const loader = field.querySelector('.loader');
    const text = field.querySelector('span');
    if (!loader || !text) return;
    loader.style.display = show ? 'inline-block' : 'none';
    text.style.display = show ? 'none' : 'inline';
}

function toggleLoadingIndicator(show) {
    const indicator = document.getElementById('loading-indicator');
    if (indicator) {
        indicator.style.display = show ? 'block' : 'none';
    }
}

function updateBatteryStatus(current_mA, percentage) {
    const batteryProgress = document.getElementById('battery-progress');
    const batteryText = document.getElementById('battery-percentage-text');
    const timeLabel = document.getElementById('time-label');
    
    // Aktualisiere die Breite
    batteryProgress.style.width = percentage + '%';
    
    // Prüfe ob die Batterie geladen wird (negativer Strom = Laden)
    const isCharging = current_mA > 0;  // Korrekte Logik: Negativ = Laden
    
    if (isCharging) {
        batteryProgress.className = 'battery-progress charging';
        batteryText.className = 'battery-percentage-text charging';
        batteryText.textContent = `${percentage}% (Laden...)`;
        timeLabel.textContent = 'Ladezeit bis voll:';
    } else {
        batteryProgress.className = 'battery-progress ' + 
            (percentage > 60 ? 'green' : percentage > 30 ? 'orange' : 'red');
        batteryText.className = 'battery-percentage-text';
        batteryText.textContent = `${percentage}%`;
        timeLabel.textContent = 'Rest Laufzeit:';
    }
}

function showOfflineWarning() {
    const warning = document.createElement('div');
    warning.className = 'offline-warning';
    warning.textContent = 'Keine Verbindung zum Server';
    document.body.appendChild(warning);
}

function showAPIError() {
    const error = document.createElement('div');
    error.className = 'api-error';
    error.textContent = 'API nicht erreichbar';
    document.body.appendChild(error);
}

function checkUploaded() {
    const params = new URLSearchParams(window.location.search);
    if (params.get('uploaded') === 'true') {
        showMessage("Datei erfolgreich hochgeladen!");
    }
}

function checkSaved() {
    const params = new URLSearchParams(window.location.search);
    if (params.get('saved') === 'true') {
        showMessage("Datei erfolgreich gespeichert!");
    }
}

// ========== GAUGE FUNCTIONS ==========
function calculateTickInterval(maxValue, range) {
    if (range <= 15) return 3;
    if (range <= 50) return 5;
    if (range <= 200) return 10;
    if (range <= 1000) return 200;
    if (range <= 1500) return 300;
    if (range <= 4000) return 500;
    return Math.ceil(range / 10);
}

function drawMinMaxMarkers(ctx, centerX, centerY, radius, minValue, maxValue, max, startValue = 0) {
    const startAngle = Math.PI;
    const endAngle = 2 * Math.PI;
    const range = max - startValue;

    if (minValue > startValue) {
        const minAngle = startAngle + ((minValue - startValue) / range) * (endAngle - startAngle);
        const minX = centerX + radius * Math.cos(minAngle);
        const minY = centerY + radius * Math.sin(minAngle);
        ctx.beginPath();
        ctx.arc(minX, minY, 5, 0, 2 * Math.PI);
        ctx.fillStyle = '#00FFFF';
        ctx.fill();
    }

    if (maxValue > startValue) {
        const maxAngle = startAngle + ((maxValue - startValue) / range) * (endAngle - startAngle);
        const maxX = centerX + radius * Math.cos(maxAngle);
        const maxY = centerY + radius * Math.sin(maxAngle);
        ctx.beginPath();
        ctx.arc(maxX, maxY, 5, 0, 2 * Math.PI);
        ctx.fillStyle = '#FF00FF';
        ctx.fill();
    }
}

function drawGauge(canvasId, value, max, label, minValue = 0, maxValue = 0, unit = '', startValue = 0) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const radius = Math.min(canvas.width, canvas.height) / 2 - 10;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Hintergrund
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
    const gradient = ctx.createRadialGradient(centerX, centerY, radius / 2, centerX, centerY, radius);
    gradient.addColorStop(0, '#2e015c');
    gradient.addColorStop(1, '#1E1E1E');
    ctx.fillStyle = gradient;
    ctx.fill();

    // Rand
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
    ctx.lineWidth = 3;
    ctx.strokeStyle = ctx.createLinearGradient(0, 0, canvas.width, 0);
    ctx.strokeStyle.addColorStop(0, '#FF00FF');
    ctx.strokeStyle.addColorStop(1, '#6600AA');
    ctx.stroke();

    // Skala
    const startAngle = Math.PI;
    const endAngle = 2 * Math.PI;
    const tickInterval = calculateTickInterval(max, max - startValue);
    const subTickInterval = tickInterval / 4;

    for (let i = startValue; i <= max; i += subTickInterval) {
        const angle = startAngle + ((i - startValue) / (max - startValue)) * (endAngle - startAngle);
        const x = centerX + radius * Math.cos(angle);
        const y = centerY + radius * Math.sin(angle);
        const innerX = centerX + (radius - (i % tickInterval === 0 ? 10 : 5)) * Math.cos(angle);
        const innerY = centerY + (radius - (i % tickInterval === 0 ? 10 : 5)) * Math.sin(angle);

        ctx.beginPath();
        ctx.moveTo(innerX, innerY);
        ctx.lineTo(x, y);
        ctx.strokeStyle = i % tickInterval === 0 ? '#FFFFFF' : '#8800FF';
        ctx.lineWidth = i % tickInterval === 0 ? 2 : 1;
        ctx.stroke();

        if (i % tickInterval === 0 || i === startValue || i === max || i === max + tickInterval) {
            const textX = centerX + (radius - 25) * Math.cos(angle);
            const textY = centerY + (radius - 25) * Math.sin(angle);
            ctx.font = '14px "Fira Code", monospace';
            ctx.fillStyle = '#FF00FF';
            ctx.textAlign = 'center';
            ctx.fillText(i, textX, textY);
        }
    }

    drawMinMaxMarkers(ctx, centerX, centerY, radius, minValue, maxValue, max, startValue);

    // Zeiger
    const angle = startAngle + ((value - startValue) / (max - startValue)) * (endAngle - startAngle);
    const needleX = centerX + (radius - 20) * Math.cos(angle);
    const needleY = centerY + (radius - 20) * Math.sin(angle);

    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.lineTo(needleX, needleY);
    ctx.strokeStyle = '#00FFFF';
    ctx.lineWidth = 3;
    ctx.shadowColor = '#000';
    ctx.shadowBlur = 6;
    ctx.shadowOffsetX = 2;
    ctx.shadowOffsetY = 2;
    ctx.stroke();

    // Mittelpunkt
    ctx.beginPath();
    ctx.arc(centerX, centerY, 12, 0, 2 * Math.PI);
    ctx.fillStyle = '#02346e';
    ctx.shadowColor = '#000';
    ctx.shadowBlur = 8;
    ctx.fill();

    // Text
    ctx.font = '20px "Fira Code", monospace';
    ctx.fillStyle = '#FFFFFF';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.shadowColor = '#000';
    ctx.shadowBlur = 4;
    ctx.fillText(`${value.toFixed(1)} ${unit}`, centerX, centerY);

    ctx.font = '14px "Fira Code", monospace';
    ctx.fillStyle = '#FF00FF';
    ctx.shadowColor = 'transparent';
    ctx.shadowBlur = 0;
    ctx.fillText(label, centerX, centerY + radius / 2);
}

// ========== NEW LOGS FUNCTIONS ==========
async function refreshLogs() {
    try {
        const response = await fetch('/api/logs');
        if (!response.ok) throw new Error('Fehler beim Laden der Logs');
        const logs = await response.text();
        const logContent = document.getElementById('log-content');
        logContent.textContent = logs;
        
        // Zum Ende scrollen
        const logViewer = document.querySelector('.log-viewer');
        logViewer.scrollTop = logViewer.scrollHeight;
    } catch (error) {
        showMessage('Fehler beim Laden der Logs: ' + error.message, true);
    }
}

async function clearLogs() {
    if (!confirm('Möchten Sie wirklich alle Logs löschen?')) return;
    
    try {
        const response = await fetch('/api/logs', { method: 'DELETE' });
        if (!response.ok) throw new Error('Fehler beim Löschen der Logs');
        showMessage('Logs erfolgreich gelöscht');
        refreshLogs();
    } catch (error) {
        showMessage('Fehler beim Löschen der Logs: ' + error.message, true);
    }
}

async function downloadLogs() {
    try {
        const response = await fetch('/api/logs/download');
        if (!response.ok) throw new Error('Fehler beim Herunterladen der Logs');
        
        const blob = await response.blob();
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'system_logs.txt';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        window.URL.revokeObjectURL(url);
    } catch (error) {
        showMessage('Fehler beim Herunterladen der Logs: ' + error.message, true);
    }
}

// ========== INITIALIZE APP ==========
setupErrorHandling();
checkAPIConnection();
const router = new Router();

async function checkDisplayState() {
    try {
        const response = await fetch('/api/display/status');
        const data = await response.json();
        const statusElement = document.getElementById('display-status');
        if (statusElement) {
            statusElement.textContent = data.display_state === "on" ? "AN" : "AUS";
            statusElement.className = 'display-status ' + data.display_state;
        }
    } catch (error) {
        console.error('Fehler beim Abrufen des Display-Status:', error);
    }
}

async function restartESP() {
    if (confirm('Möchten Sie den ESP32 wirklich neustarten?')) {
        try {
            const response = await fetch('/api/restart', { method: 'POST' });
            if (response.ok) {
                showMessage('ESP32 wird neugestartet...');
                // Warte 5 Sekunden und lade dann die Seite neu
                setTimeout(() => {
                    location.reload();
                }, 5000);
            } else {
                showMessage('Fehler beim Neustarten des ESP32', true);
            }
        } catch (error) {
            console.error('Fehler:', error);
            showMessage('Fehler beim Neustarten des ESP32', true);
        }
    }
}

async function editFile(fileName) {
    try {
        // Lade den Dateiinhalt
        const response = await fetch(`/api/file?name=${encodeURIComponent(fileName)}`);
        if (!response.ok) throw new Error('Fehler beim Laden der Datei');
        const content = await response.text();

        // Erstelle einen modalen Dialog für die Bearbeitung
        const modal = document.createElement('div');
        modal.className = 'edit-modal';
        modal.innerHTML = `
            <div class="edit-modal-content">
                <h2>Datei bearbeiten: ${fileName}</h2>
                <textarea id="file-editor">${content}</textarea>
                <div class="edit-modal-buttons">
                    <button onclick="saveFile('${fileName}')" class="save-button">Speichern</button>
                    <button onclick="closeEditModal()" class="cancel-button">Abbrechen</button>
                </div>
            </div>
        `;
        document.body.appendChild(modal);
    } catch (error) {
        showMessage('Fehler: ' + error.message, true);
    }
}

async function saveFile(fileName) {
    try {
        const content = document.getElementById('file-editor').value;
        const response = await fetch('/save', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                name: fileName,
                content: content
            })
        });

        if (!response.ok) throw new Error('Fehler beim Speichern');
        
        closeEditModal();
        showMessage('Datei erfolgreich gespeichert');
        // Aktualisiere die Dateiliste
        initFiles();
    } catch (error) {
        showMessage('Fehler beim Speichern: ' + error.message, true);
    }
}

function closeEditModal() {
    const modal = document.querySelector('.edit-modal');
    if (modal) {
        modal.remove();
    }
} 