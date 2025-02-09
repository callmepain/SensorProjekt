import { fetchData } from '../utils.js';

export async function loadCharts() {
    const content = document.getElementById('content');
    
    // Styles für das Grid-Layout und Canvas
    content.innerHTML = `
        <style>
            .section {
                margin: 20px;
            }

            h1 {
                color: #FFFFFF;
                text-align: center;
                margin-bottom: 20px;
                font-size: 2rem;
                background: linear-gradient(90deg, #FF00FF, #8800FF);
                -webkit-background-clip: text;
                color: transparent;
            }

            .section h2 {
                text-align: center;
                margin-bottom: 20px;
            }

            .grid-container {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(700px, 1fr));
                justify-items: center;
                gap: 20px;
                padding: 20px;
            }

            .chart-container {
                min-width: 700px;
                min-height: 400px;
                display: flex;
                flex-direction: column;
                align-items: center;
                border: 2px solid #8800FF;
                border-radius: 10px;
                background-color: #1E1E1E;
                box-shadow: 0 4px 15px rgba(136, 0, 255, 0.4);
                padding: 10px;
            }

            canvas {
                width: 100%;
                max-width: 800px;
                height: auto;
                background-color: #1E1E1E;
                border-radius: 10px;
            }

            h2 {
                background: linear-gradient(90deg, #FF00FF, #8800FF);
                text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000;
            }

            h3 {
                color: #FFFFFF;
                text-align: center;
                margin-bottom: 10px;
            }
        </style>

        <h1>Datenvisualisierung</h1>
        
        <!-- Dropdown-Menü für Limit-Auswahl -->
        <div style="text-align: center; margin-bottom: 20px;">
            <label for="data-limit" style="font-size: 18px; margin-right: 10px;">Anzahl der Datenpunkte:</label>
            <select id="data-limit">
                <option value="10">10</option>
                <option value="25">25</option>
                <option value="50" selected>50</option>
                <option value="100">100</option>
                <option value="200">200</option>
                <option value="500">500</option>
                <option value="24h">24 Stunden</option>
                <option value="3d">3 Tage</option>
                <option value="7d">7 Tage</option>
            </select>
        </div>

        <!-- Solarpanel Sektion -->
        <div class="section">
            <h2>Solarpanel</h2>
            <div class="grid-container">
                <div class="chart-container">
                    <h3>Voltage (V)</h3>
                    <canvas id="voltageChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Current (mA)</h3>
                    <canvas id="currentChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Power (mW)</h3>
                    <canvas id="powerChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Lux</h3>
                    <canvas id="luxChart"></canvas>
                </div>
            </div>
        </div>

        <!-- Batterie Sektion -->
        <div class="section">
            <h2>Batterie</h2>
            <div class="grid-container">
                <div class="chart-container">
                    <h3>Battery Voltage (V)</h3>
                    <canvas id="batteryVoltageChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Battery Current (mA)</h3>
                    <canvas id="batteryCurrentChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Battery Power (mW)</h3>
                    <canvas id="batteryPowerChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Battery Percentage (%)</h3>
                    <canvas id="batteryPercentageChart"></canvas>
                </div>
            </div>
        </div>

        <!-- Sensoren Sektion -->
        <div class="section">
            <h2>Sensoren</h2>
            <div class="grid-container">
                <div class="chart-container">
                    <h3>Temperature (°C)</h3>
                    <canvas id="temperatureChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Humidity (%)</h3>
                    <canvas id="humidityChart"></canvas>
                </div>
                <div class="chart-container">
                    <h3>Pressure (hPa)</h3>
                    <canvas id="pressureChart"></canvas>
                </div>
            </div>
        </div>
    `;

    // Event Listener für Limit-Auswahl
    const limitSelect = document.getElementById('data-limit');
    limitSelect.addEventListener('change', async (e) => {
        console.log("Limit geändert zu:", e.target.value);
        await renderCharts(e.target.value);
    });

    // Initial Charts rendern
    await renderCharts('50');
    startChartUpdates();
}

// Dark Mode Optionen exakt wie im Original
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

let charts = {}; // Speichert aktive Charts

async function renderCharts(dataLimit) {
    try {
        console.log("Aktuelles Limit:", dataLimit);
        
        // Voltage-Daten abrufen
        const response = await fetchData('voltage_data', dataLimit);
        console.log("API Response:", response);
        
        if (!response || !response.data || !Array.isArray(response.data)) {
            console.error("Voltage data is invalid or empty.");
            return;
        }

        const data = response.data;
        const labels = data.map(d => d.timestamp).reverse();
        
        // Charts aktualisieren
        updateChart('voltageChart', labels, data.map(d => d.voltage).reverse(), 'Voltage (V)', 'red');
        updateChart('currentChart', labels, data.map(d => d.current_mA).reverse(), 'Current (mA)', 'blue');
        updateChart('powerChart', labels, data.map(d => d.power).reverse(), 'Power (mW)', 'green');
        updateChart('luxChart', labels, data.map(d => d.lux).reverse(), 'Lux', 'orange');

        // Batterie-Charts
        const batteryData = await fetchData('battery_data', dataLimit);
        if (batteryData && batteryData.data && Array.isArray(batteryData.data)) {
            const batteryLabels = batteryData.data.map(d => d.timestamp).reverse();
            updateChart('batteryVoltageChart', batteryLabels, batteryData.data.map(d => d.voltage).reverse(), 'Battery Voltage (V)', 'purple');
            updateChart('batteryCurrentChart', batteryLabels, batteryData.data.map(d => d.current).reverse(), 'Battery Current (mA)', 'teal');
            updateChart('batteryPowerChart', batteryLabels, batteryData.data.map(d => d.power).reverse(), 'Battery Power (mW)', 'brown');
            updateChart('batteryPercentageChart', batteryLabels, batteryData.data.map(d => d.percentage).reverse(), 'Battery Percentage (%)', 'orange');
        }

        // Sensor-Charts
        const sensorData = await fetchData('sensor_data', dataLimit);
        if (sensorData && sensorData.data && Array.isArray(sensorData.data)) {
            const sensorLabels = sensorData.data.map(d => d.timestamp).reverse();
            updateChart('temperatureChart', sensorLabels, sensorData.data.map(d => d.temperature).reverse(), 'Temperature (°C)', 'red');
            updateChart('humidityChart', sensorLabels, sensorData.data.map(d => d.humidity).reverse(), 'Humidity (%)', 'blue');
            updateChart('pressureChart', sensorLabels, sensorData.data.map(d => d.pressure).reverse(), 'Pressure (hPa)', 'green');
        }

    } catch (error) {
        console.error('Error rendering charts:', error);
    }
}

function updateChart(chartId, labels, values, label, color) {
    const ctx = document.getElementById(chartId).getContext('2d');
    
    if (charts[chartId]) {
        charts[chartId].destroy();
    }

    charts[chartId] = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [{
                label: label,
                data: values,
                borderColor: color,
                fill: false
            }]
        },
        options: {
            responsive: true,
            plugins: darkModeOptions.plugins,
            scales: {
                x: darkModeOptions.scales.x,
                y: {
                    ...darkModeOptions.scales.y,
                    title: { display: true, text: label, color: '#ffffff' }
                }
            },
            elements: darkModeOptions.elements
        }
    });
}

// Periodisches Update
function startChartUpdates() {
    setInterval(async () => {
        const limitSelect = document.getElementById('data-limit');
        const currentLimit = limitSelect.value;
        console.log("Auto-Update mit Limit:", currentLimit);
        await renderCharts(currentLimit);
    }, 30000); // Alle 30 Sekunden
}

// Debug-Funktion zum Testen
async function testFetch(limit) {
    try {
        const data = await fetchData('voltage_data', limit);
        console.log('Rohdaten von API:', data);
        return data;
    } catch (error) {
        console.error('Fehler beim Datenabruf:', error);
        return null;
    }
} 