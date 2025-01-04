// ========== MENÜ LADEN ==========
fetch('menu.html')
    .then(response => response.text())
    .then(data => {
        document.getElementById('menu-placeholder').innerHTML = data;
    })
    .catch(error => console.error('Fehler beim Laden des Menüs:', error));

// ========== BUTTON-FUNKTIONEN ==========
function toggleDisplay(state) {
    fetch('/api/display', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `state=${state}`
    })
    .then(response => response.json())
    .then(data => {
        if (data.message) {
            console.log(data.message);
        } else {
            alert('Fehler: ' + (data.error || 'Unbekannter Fehler'));
        }
    })
    .catch(error => {
        alert('Fehler: ' + error.message);
    });
}

function getColorForValue(value, max) {
    const percent = value / max;
    if (percent < 0.5) return '#33FF33'; // Grün
    if (percent < 0.75) return '#FFCC00'; // Gelb
    return '#FF3300'; // Rot
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
    gradient.addColorStop(0, '#2e015c'); // Dunkler Hintergrund
    gradient.addColorStop(1, '#1E1E1E'); // Lila Rand
    ctx.fillStyle = gradient;
    ctx.fill();

    // Heller Rand
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
    ctx.lineWidth = 3;
    ctx.strokeStyle = ctx.createLinearGradient(0, 0, canvas.width, 0);
    ctx.strokeStyle.addColorStop(0, '#FF00FF'); // Neon Pink
    ctx.strokeStyle.addColorStop(1, '#6600AA'); // Dunkler Lila-Ton
    ctx.stroke();

    // Skala
    const startAngle = Math.PI;
    const endAngle = 2 * Math.PI;
    const tickInterval = calculateTickInterval(max, max - startValue);
    const subTickInterval = tickInterval / 4; // Zwischenwerte

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

        // Nur beschriftete Hauptwerte anzeigen
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
    ctx.arc(centerX, centerY, 12, 0, 2 * Math.PI); // Radius von 6
    ctx.fillStyle = '#02346e'; // Farbe des Punktes
    ctx.shadowColor = '#000'; // Schatteneffekt
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

    // Schatten für Zeiger
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

    // Schatten für den Text aktivieren
    ctx.shadowColor = '#000';
    ctx.shadowBlur = 4;
    ctx.shadowOffsetX = 2;
    ctx.shadowOffsetY = 2;

    ctx.fillText(`${value.toFixed(1)} ${unit}`, centerX, centerY);

    // Text: Label
    ctx.font = '14px "Fira Code", monospace';
    ctx.fillStyle = '#FF00FF';
    ctx.textAlign = 'center';

    // Schatten für Label deaktivieren
    ctx.shadowColor = 'transparent';
    ctx.shadowBlur = 0;

    ctx.fillText(label, centerX, centerY + radius / 2);
}

function drawMinMaxMarkers(ctx, centerX, centerY, radius, minValue, maxValue, max, startValue = 0) {
    const startAngle = Math.PI; // Anfangswinkel der Skala
    const endAngle = 2 * Math.PI; // Endwinkel der Skala
    const range = max - startValue; // Tatsächlicher Bereich der Skala

    // Min-Marker
    if (minValue > startValue) {
        const minAngle = startAngle + ((minValue - startValue) / range) * (endAngle - startAngle);
        const minX = centerX + radius * Math.cos(minAngle);
        const minY = centerY + radius * Math.sin(minAngle);
        ctx.beginPath();
        ctx.arc(minX, minY, 5, 0, 2 * Math.PI);
        ctx.fillStyle = '#00FFFF'; // Farbe für den Min-Marker
        ctx.fill();
    }

    // Max-Marker
    if (maxValue > startValue) {
        const maxAngle = startAngle + ((maxValue - startValue) / range) * (endAngle - startAngle);
        const maxX = centerX + radius * Math.cos(maxAngle);
        const maxY = centerY + radius * Math.sin(maxAngle);
        ctx.beginPath();
        ctx.arc(maxX, maxY, 5, 0, 2 * Math.PI);
        ctx.fillStyle = '#FF00FF'; // Farbe für den Max-Marker
        ctx.fill();
    }
}

function calculateTickInterval(maxValue, range) {
    if (range <= 15) return 3; // Kleine Bereiche, z. B. Temperatur (17–28)
    if (range <= 50) return 5; // Für moderate Bereiche wie Luftfeuchtigkeit
    if (range <= 200) return 10; // Für Lux
    if (range <= 1000) return 200; // Für Lux
    if (range <= 1500) return 300; // Für hohe Werte wie Druck
    return Math.ceil(range / 10); // Standard für sehr große Werte
}

function toggleLoadingIndicator(show) {
    const indicator = document.getElementById('loading-indicator');
    indicator.style.display = show ? 'block' : 'none';
}

async function resetMinMax() {
    try {
        // Lade-Indikator (wenn du ihn hast) einblenden
        toggleLoadingIndicator(true);

        const response = await fetch('/api/reset', { method: 'POST' });
        if (response.ok) {
        // An dieser Stelle Meldung anzeigen
        showResetMessage("Werte erfolgreich zurückgesetzt!");

        // Optional: Deine getSensorData() neu aufrufen
        // getSensorData();
        } else {
        showResetMessage("Fehler beim Zurücksetzen der Werte!");
        }
    } catch (error) {
        console.error(error);
        showResetMessage("Fehler: " + error.message);
    } finally {
        toggleLoadingIndicator(false);
    }
}

function showResetMessage(text) {
const msg = document.getElementById("reset-message");
msg.textContent = text;
msg.style.display = "block";

// Nach 5 Sekunden wieder ausblenden
setTimeout(() => {
        msg.style.display = "none";
    }, 5000);
}

// ========== SENSOR-DATEN HOLEN ==========
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
        drawGauge('maxtemperatureGauge', data.maxTemperatur, 30, 'maxTemperatur', 0, 0, '°C',15);
        updateValue('humidity', data.luftfeuchtigkeit + ' %');
        drawGauge('humidityGauge', data.luftfeuchtigkeit, 100, 'Luftfeuchtigkeit', 0, 0, '%', 0);
        updateValue('pressure', data.druck + ' hPa');
        drawGauge('pressureGauge', data.druck, 1500, 'Druck', 0, 0, 'hPa', 0);
        updateValue('voltage', data.spannung + ' V');
        updateValue('current', data.stromstärke + ' mA');
        updateValue('power', data.leistung + ' mW');
        updateValue('lux', data.helligkeit + ' lx');
        drawGauge('luxGauge', data.helligkeit, 1000, 'Lux', 0, 0, 'lx', 0);
        updateValue('distance', data.entfernung + ' mm');
        updateValue('batteryVoltage', data.battery_spannung + ' V');
        updateValue('batteryCurrent', data.battery_stromstärke_mA + ' mA');
        updateValue('batteryPower', data.battery_leistung + ' mW');
        updateValue('batterytime', data.remaining_time + ' h');
        updateBatteryState(data.battery_prozent);

    } catch (error) {
        console.error(error);
        fields.forEach(id => updateValue(id, 'Fehler', true));
    } finally {
        fields.forEach(id => toggleFieldLoader(id, false));
    }
}

// ========== HILFSFUNKTIONEN ========== 
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
    if (!loader) return;
    loader.style.display = show ? 'inline-block' : 'none';
    text.style.display = show ? 'none' : 'inline';
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

// ========== BEIM SEITENSTART ==========
window.onload = () => {
    getSensorData();
    setInterval(getSensorData, 30000);
};