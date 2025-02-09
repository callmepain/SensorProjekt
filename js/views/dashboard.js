import { fetchAPI, USE_MOCK_API } from '../utils.js';

// Mock-API Daten für die Gauge-Tests
const mockSensorData = {
    temperatur: 23.5,
    minTemperatur: 18.2,
    maxTemperatur: 26.8,
    luftfeuchtigkeit: 45,
    druck: 1013,
    helligkeit: 450,
    voltage: 12.3,
    current: 1.2,
    power: 14.76,
    batteryVoltage: 3.7,
    batteryCurrent: 250,
    batteryPower: 0.925,
    batteryPercentage: 85,
    batteryTime: "12.5"
};

const styles = `
    .battery-container {
        width: 90%;
        max-width: 300px;
        height: 20px;
        border: 2px solid #FF00FF;
        border-radius: 5px;
        background-color: #1E1E1E;
        position: relative;
        overflow: hidden;
        margin: 10px auto;
    }
    .battery-progress {
        height: 100%;
        width: 0%;
        transition: width 0.5s ease-in-out, background-color 0.3s ease;
        box-shadow: 0 2px 5px rgba(255, 0, 255, 0.3);
        border-radius: 2px;
    }
    .battery-percentage-text {
        text-align: center;
        font-size: 14px;
        font-weight: bold;
        color: #FFFFFF;
        text-shadow: -1px -1px 0 #000, 1px -1px 0 #000,
                    -1px 1px 0 #000, 1px 1px 0 #000;
    }
    .battery-progress.green {
        background: linear-gradient(90deg, #33FF33, #009900);
    }
    .battery-progress.orange {
        background: linear-gradient(90deg, #FFCC00, #FF9900);
    }
    .battery-progress.red {
        background: linear-gradient(90deg, #FF0000, #990000);
    }
`;

// Styles zum Dokument hinzufügen
const styleSheet = document.createElement("style");
styleSheet.textContent = styles;
document.head.appendChild(styleSheet);

export async function loadDashboard() {
    const content = document.getElementById('content');
    
    content.innerHTML = `
        <style>
            /* Beispielhaftes Styling für die Meldung */
            #reset-message {
                position: fixed; 
                top: 10px; 
                right: 10px;
                padding: 10px 15px;
                background-color: #333;
                color: #fff;
                border: 2px solid #FF00FF;
                border-radius: 5px;
                font-weight: bold;
                display: none; /* Start: ausgeblendet */
                z-index: 9999; /* Damit es oben drüber liegt */
            }
            /* ============ GLOBAL STYLES ============ */
            body {
                margin: 0;
                font-family: Arial, sans-serif;
                background-color: #121212;
                color: #E0E0E0;
            }

            #menu-placeholder {
                /* Hier kommt dein dynamisch geladenes Menü rein. 
                Falls du noch spezielle Styles für den Menübalken brauchst, 
                kannst du das hier tun. */
            }

            h1 {
                margin-top: 20px;
                text-align: center;
                font-size: 2rem;
                background: linear-gradient(90deg, #FF00FF, #8800FF);
                -webkit-background-clip: text;
                color: transparent;
            }

            /* ============ LAYOUT WRAPPER ============ */
            .content-wrapper {
                display: flex;
                flex-direction: column;
                align-items: center; 
                gap: 40px; 
                margin-bottom: 40px;
            }

            /* ============ OBERE TABELLE (breit) ============ */
            .table-wide {
                width: 80%;
                max-width: 1000px; 
            }

            .table-wide tbody tr:hover td {
                background-color: transparent; /* Kein Hover-Highlighting */
            }

            /* ============ UNTERE REIHE (2 Tabellen nebeneinander) ============ */
            .table-row {
                display: flex;
                flex-wrap: wrap;
                gap: 20px;
                justify-content: center;
                width: 80%;
                max-width: 1000px; 
            }

            .table-row2 {
                display: flex;
                flex-wrap: wrap;
                gap: 20px;
                justify-content: center;
                width: 80%;
                max-width: 1000px; 
            }

            .table-row2 tbody tr:hover td {
                background-color: transparent; /* Kein Hover-Highlighting */
            }

            .table-container {
                flex: 1 1 300px;
                min-width: 300px;
            }

            /* ============ ALLGEMEINES TABELLEN-STYLING ============ */
            table {
                width: 100%;
                border-collapse: separate;
                border-spacing: 0;
                border: 2px solid #FF00FF;
                border-radius: 10px; /* Rundung der Außenkanten */
                background-color: #1E1E1E;
                overflow: hidden; /* Damit die Ecken wirklich abgerundet wirken */
                table-layout: fixed;
            }
            /* -- Kopfzeilen-Design -- */
            thead {
                /* Keine eigene Hintergrundfarbe hier direkt, 
                wir machen das in den tr/th, damit die Ränder abgerundet sind */
            }
            /* Die erste Kopfzeile (mit dem colspan) soll die oberen Ecken abrunden */
            thead tr:first-child th {
                background: linear-gradient(90deg, #FF00FF, #8800FF);
                color: #FFFFFF;
                font-weight: bold;
                text-align: center;
                padding: 10px;
                border-top-left-radius: 8px;  /* Noch mal speziell abrunden */
                border-top-right-radius: 8px;
                border-bottom: 1px solid #FF00FF;
                text-shadow: -1px -1px 0 #000,
                            1px -1px 0 #000,
                            -1px 1px 0 #000,
                            1px 1px 0 #000;
            }

            /* Die zweite Kopfzeile (Parameter/Aktuell/Min/Max/Aktion) */
            thead tr:last-child {
                background: linear-gradient(90deg, #FF00FF, #8800FF);
            }

            thead tr:last-child th {
                color: #FFFFFF;
                font-weight: bold;
                padding: 10px;
                text-align: left;
                border-right: 1px solid #FF00FF;
                border-bottom: 1px solid #FF00FF;
                text-shadow: -1px -1px 0 #000,
                            1px -1px 0 #000,
                            -1px 1px 0 #000,
                            1px 1px 0 #000;
            }

            thead th {
                text-shadow: -1px -1px 0 #000,
                            1px -1px 0 #000,
                            -1px 1px 0 #000,
                            1px 1px 0 #000;
            }

            thead tr:last-child th:last-child {
                border-right: none;
            }

            /* -- Inhalt-Design -- */
            tbody tr:nth-child(even) {
                background-color: #2A2A2A;
            }

            tbody tr:hover td {
                background-color: #3A3A3A;
                transition: all 0.3s ease;
            }

            tbody th, tbody td {
                vertical-align: middle;
                line-height: 1.5;
                color: #FFFFFF;
                padding: 10px;
                text-align: left;
                border-bottom: 1px solid #FF00FF;
                border-right: 1px solid #FF00FF;
            }

            /* Letzte Spalte ohne rechte Linie */
            tbody td:last-child, thead tr:last-child th:last-child {
                border-right: none;
            }

            /* Letzte Zeile ohne untere Linie + abgerundete Ecken, falls gewünscht */
            tbody tr:last-child td {
                border-bottom: none;
            }
            /* Falls du auch unten abgerundete Ecken willst, brauchst du
            ggf. :last-child-Selektoren, oder du rundest nur oben ab. */

            /* ============ BUTTONS ============ */
            button {
                width: 100%;
                padding: 8px 15px;
                border: 2px solid #33aaff;
                color: #33aaff;
                font-size: 14px;
                font-weight: bold;
                background: transparent;
                cursor: pointer;
                border-radius: 8px;
                transition: all 0.3s ease;
                text-shadow: none;
                box-shadow: none;
            }

            button:hover {
                background: #33aaff;
                color: #ffffff;
                box-shadow: 0 4px 10px rgba(51, 170, 255, 0.4);
            }

            button:active {
                background: linear-gradient(90deg, #0077CC, #00BBCC); /* Dunklere Cyan-Töne für "Drücken" */
                box-shadow: 0 2px 4px rgba(0, 0, 0, 0.6), inset 0 2px 4px rgba(255, 255, 255, 0.2); /* Inset-Schatten für Tiefe */
                transform: translateY(2px); /* Effekt: Knopf wird "gedrückt" */
            }

            /* ============ LADE-INDIKATOR ============ */
            #loading-indicator {
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                color: #FFFFFF;
                font-size: 24px;
                background-color: rgba(0, 0, 0, 0.7);
                padding: 20px;
                border-radius: 10px;
                display: none;
                text-align: center;
            }
            .loader {
                border: 4px solid rgba(255, 255, 255, 0.2);
                border-radius: 50%;
                border-top: 4px solid #FF00FF;
                width: 20px;
                height: 20px;
                animation: spin 1s linear infinite;
                display: inline-block;
                margin: 10px auto;
            }
            @keyframes spin {
                0% { transform: rotate(0deg); }
                100% { transform: rotate(360deg); }
            }

            /* ============ BATTERIE-BALKEN ============ */
            .battery-container {
                width: 90%;
                max-width: 300px;
                height: 20px;
                border: 2px solid #FF00FF;
                border-radius: 5px;
                background-color: #1E1E1E;
                position: relative;
                overflow: hidden;
                margin: 10px auto;
            }
            .battery-progress {
                height: 100%;
                width: 0%;
                transition: width 0.5s ease-in-out, background-color 0.3s ease;
                box-shadow: 0 2px 5px rgba(255, 0, 255, 0.3);
                border-radius: 2px;
            }
            .battery-percentage-text {
                text-align: center;
                font-size: 14px;
                font-weight: bold;
                color: #FFFFFF;
                text-shadow: -1px -1px 0 #000, 1px -1px 0 #000,
                            -1px 1px 0 #000, 1px 1px 0 #000;
            }
            .battery-progress.green {
                background: linear-gradient(90deg, #33FF33, #009900);
            }
            .battery-progress.orange {
                background: linear-gradient(90deg, #FFCC00, #FF9900);
            }
            .battery-progress.red {
                background: linear-gradient(90deg, #FF0000, #990000);
            }

            /* Optional: Scrollbar-Anpassungen */
            ::-webkit-scrollbar {
                width: 10px;
            }
            ::-webkit-scrollbar-track {
                background: #1E1E1E;
                border-radius: 10px;
            }
            ::-webkit-scrollbar-thumb {
                background: linear-gradient(90deg, #FF00FF, #8800FF);
                border-radius: 10px;
                box-shadow: inset 0 0 5px rgba(0, 0, 0, 0.5);
            }
            ::-webkit-scrollbar-thumb:hover {
                background: linear-gradient(90deg, #8800FF, #FF00FF);
            }
            canvas {
                margin: 10px auto;
                display: block;
            }
        </style>

        <h1>ESP32 Umgebungsdaten</h1>
        
        <div id="loading-indicator">
            <div class="loader"></div>
            <p>Daten werden geladen...</p>
        </div>

        <div id="reset-message">Werte erfolgreich zurückgesetzt!</div>

        <div class="content-wrapper">
            <!-- Umgebungswerte Tabelle -->
            <div class="table-wide">
                <table>
                    <thead>
                        <tr>
                            <th colspan="3">Umgebungswerte</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr>
                            <td><canvas id="temperatureGauge" width="200" height="200"></canvas></td>
                            <td><canvas id="mintemperatureGauge" width="200" height="200"></canvas></td>
                            <td><canvas id="maxtemperatureGauge" width="200" height="200"></canvas></td>
                        </tr>
                        <tr>
                            <td><canvas id="humidityGauge" width="200" height="200"></canvas></td>
                            <td><canvas id="pressureGauge" width="200" height="200"></canvas></td>
                            <td><canvas id="luxGauge" width="200" height="200"></canvas></td>
                        </tr>
                    </tbody>
                </table>
            </div>

            <!-- Buttons -->
            <div class="table-row2">
                <table>
                    <thead>
                        <tr><th colspan="4">Buttons</th></tr>
                    </thead>
                    <tbody>
                        <tr>
                            <td><button onclick="DASHBOARD.resetMinMax()">Reset Min/Max</button></td>
                            <td><button onclick="DASHBOARD.toggleDisplay('on')">Display An</button></td>
                            <td><button onclick="DASHBOARD.toggleDisplay('off')">Display Aus</button></td>
                            <td><button onclick="DASHBOARD.updateData()">Aktualisieren</button></td>
                        </tr>
                    </tbody>
                </table>
            </div>

            <!-- Solar Panel + Batteriewerte -->
            <div class="table-row">
                <div class="table-container">
                    <table>
                        <thead>
                            <tr><th colspan="2">Aktuell</th></tr>
                        </thead>
                        <tbody>
                            <tr>
                                <td>Spannung</td>
                                <td><span id="voltage">Laden...</span></td>
                            </tr>
                            <tr>
                                <td>Stromstärke</td>
                                <td><span id="current">Laden...</span></td>
                            </tr>
                            <tr>
                                <td>Leistung</td>
                                <td><span id="power">Laden...</span></td>
                            </tr>
                        </tbody>
                    </table>
                </div>

                <div class="table-container">
                    <table>
                        <thead>
                            <tr><th colspan="2">Batteriewerte</th></tr>
                        </thead>
                        <tbody>
                            <tr>
                                <td>Batterie Spannung</td>
                                <td><span id="batteryVoltage">Laden...</span></td>
                            </tr>
                            <tr>
                                <td>Batterie Stromstärke</td>
                                <td><span id="batteryCurrent">Laden...</span></td>
                            </tr>
                            <tr>
                                <td>Batterie Leistung</td>
                                <td><span id="batteryPower">Laden...</span></td>
                            </tr>
                            <tr>
                                <td>Rest Laufzeit:</td>
                                <td><span id="batterytime">Laden...</span></td>
                            </tr>
                            <tr>
                                <td>Batterie Ladezustand</td>
                                <td>
                                    <div class="battery-container">
                                        <div id="battery-progress" class="battery-progress">
                                            <div id="battery-percentage-text" class="battery-percentage-text">Laden...</div>
                                        </div>
                                    </div>
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    `;

    // Dashboard initialisieren
    await updateDashboard();
    startPeriodicUpdate();
}

async function updateDashboard() {
    try {
        const data = USE_MOCK_API ? mockSensorData : await fetchAPI('sensor_data');
        
        if (data) {
            updateGauges(data);
            updateValues(data);
            updateBatteryState(data.batteryPercentage);
        }
    } catch (error) {
        console.error('Dashboard update failed:', error);
    }
}

function updateGauges(data) {
    drawGauge('temperatureGauge', data.temperatur, 30, 'Temperatur', data.minTemperatur, data.maxTemperatur, '°C', 15);
    drawGauge('mintemperatureGauge', data.minTemperatur, 30, 'minTemperatur', 0, 0, '°C', 15);
    drawGauge('maxtemperatureGauge', data.maxTemperatur, 30, 'maxTemperatur', 0, 0, '°C', 15);
    drawGauge('humidityGauge', data.luftfeuchtigkeit, 100, 'Luftfeuchtigkeit', 0, 0, '%', 0);
    drawGauge('pressureGauge', data.druck, 1500, 'Druck', 0, 0, 'hPa', 0);
    drawGauge('luxGauge', data.helligkeit, 1000, 'Lux', 0, 0, 'lx', 0);
}

function updateValues(data) {
    // Solar Panel Werte
    document.getElementById('voltage').textContent = `${data.voltage.toFixed(2)}V`;
    document.getElementById('current').textContent = `${data.current.toFixed(2)}A`;
    document.getElementById('power').textContent = `${data.power.toFixed(2)}W`;
    
    // Batterie Werte
    document.getElementById('batteryVoltage').textContent = `${data.batteryVoltage.toFixed(2)}V`;
    document.getElementById('batteryCurrent').textContent = `${data.batteryCurrent.toFixed(2)}mA`;
    document.getElementById('batteryPower').textContent = `${data.batteryPower.toFixed(2)}W`;
    document.getElementById('batterytime').textContent = data.batteryTime;
    
    // Expliziter Aufruf der Battery-State-Update Funktion
    updateBatteryState(data.batteryPercentage);
}

function startPeriodicUpdate() {
    if (window.dashboardInterval) {
        clearInterval(window.dashboardInterval);
    }
    window.dashboardInterval = setInterval(updateDashboard, 30000);
}

// Globale Funktionen für Button-Clicks
window.DASHBOARD = {
    resetMinMax: async () => {
        await fetchAPI('reset_minmax');
        document.getElementById('reset-message').style.display = 'block';
        setTimeout(() => {
            document.getElementById('reset-message').style.display = 'none';
        }, 3000);
        await updateDashboard();
    },
    toggleDisplay: async (state) => {
        await fetchAPI(`display/${state}`);
    },
    updateData: () => updateDashboard()
};

function getColorForValue(value, max) {
    const percent = value / max;
    if (percent < 0.5) return '#33FF33'; // Grün
    if (percent < 0.75) return '#FFCC00'; // Gelb
    return '#FF3300'; // Rot
}

function calculateTickInterval(maxValue, range) {
    if (range <= 15) return 3;      // Kleine Bereiche, z. B. Temperatur (17–28)
    if (range <= 50) return 5;      // Für moderate Bereiche wie Luftfeuchtigkeit
    if (range <= 200) return 10;    // Für Lux
    if (range <= 1000) return 200;  // Für Lux
    if (range <= 1500) return 300;  // Für hohe Werte wie Druck
    return Math.ceil(range / 10);   // Standard für sehr große Werte
}

function drawMinMaxMarkers(ctx, centerX, centerY, radius, minValue, maxValue, max, startValue = 0) {
    const startAngle = Math.PI;
    const endAngle = 2 * Math.PI;
    const range = max - startValue;

    // Min-Marker
    if (minValue > startValue) {
        const minAngle = startAngle + ((minValue - startValue) / range) * (endAngle - startAngle);
        const minX = centerX + radius * Math.cos(minAngle);
        const minY = centerY + radius * Math.sin(minAngle);
        ctx.beginPath();
        ctx.arc(minX, minY, 5, 0, 2 * Math.PI);
        ctx.fillStyle = '#00FFFF';
        ctx.fill();
    }

    // Max-Marker
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

    // Schatten deaktivieren vor dem Hintergrund
    ctx.shadowColor = 'transparent';
    ctx.shadowBlur = 0;
    ctx.shadowOffsetX = 0;
    ctx.shadowOffsetY = 0;

    // Hintergrundkreis
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
    const gradient = ctx.createRadialGradient(centerX, centerY, radius / 2, centerX, centerY, radius);
    gradient.addColorStop(0, '#2e015c');
    gradient.addColorStop(1, '#1E1E1E');
    ctx.fillStyle = gradient;
    ctx.fill();

    // Heller Rand
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

    // Mittelpunkt des Zeigers (Kreis)
    ctx.beginPath();
    ctx.arc(centerX, centerY, 12, 0, 2 * Math.PI);
    ctx.fillStyle = '#02346e';
    ctx.shadowColor = '#000';
    ctx.shadowBlur = 8;
    ctx.shadowOffsetX = 2;
    ctx.shadowOffsetY = 2;
    ctx.fill();

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

    // Text: Wert mit Einheit
    ctx.font = '20px "Fira Code", monospace';
    ctx.fillStyle = '#FFFFFF';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.shadowColor = '#000';
    ctx.shadowBlur = 4;
    ctx.shadowOffsetX = 2;
    ctx.shadowOffsetY = 2;
    ctx.fillText(`${value.toFixed(1)} ${unit}`, centerX, centerY);

    // Text: Label
    ctx.font = '14px "Fira Code", monospace';
    ctx.fillStyle = '#FF00FF';
    ctx.textAlign = 'center';
    ctx.shadowColor = 'transparent';
    ctx.shadowBlur = 0;
    ctx.fillText(label, centerX, centerY + radius / 2);
}

function updateBatteryState(percentage) {
    const progressBar = document.getElementById('battery-progress');
    const percentageText = document.getElementById('battery-percentage-text');
    if (!progressBar || !percentageText) return;

    progressBar.style.width = `${percentage}%`;
    percentageText.innerText =
        (percentage !== undefined && percentage !== null)
        ? `${percentage}%`
        : 'N/A';

    // Farbe je nach Ladezustand
    progressBar.classList.remove('green', 'orange', 'red');
    if (percentage > 75) {
        progressBar.classList.add('green');
    } else if (percentage > 25) {
        progressBar.classList.add('orange');
    } else {
        progressBar.classList.add('red');
    }
}