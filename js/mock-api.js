// Simulierte Sensor-Daten
const mockSensorData = {
    temperature: 23.5,
    humidity: 45,
    pressure: 1013.25,
    lux: 500,
    voltage: 12.3,
    current: 0.5,
    power: 6.15,
    batteryVoltage: 3.7,
    batteryCurrent: 250,
    batteryPower: 0.925,
    batteryPercentage: 85,
    batteryTime: "4h 30min",
    minTemperature: 20.5,
    maxTemperature: 25.8
};

// Mock API Endpunkte
const mockEndpoints = {
    'status': () => ({ status: 'online', uptime: '2d 4h 35m' }),
    'sensor_data': () => mockSensorData,
    'reset_minmax': () => ({ success: true }),
    'display/on': () => ({ success: true }),
    'display/off': () => ({ success: true })
};

// Original fetchAPI überschreiben
export async function mockFetchAPI(endpoint) {
    console.log(`Mock API Call: ${endpoint}`);
    
    // Simulierte Netzwerklatenz
    await new Promise(resolve => setTimeout(resolve, 200));
    
    if (mockEndpoints[endpoint]) {
        return mockEndpoints[endpoint]();
    }
    
    throw new Error(`Mock API Error: Endpoint '${endpoint}' not found`);
}

// Zufällige Änderungen der Sensor-Daten
export function startMockDataUpdates() {
    setInterval(() => {
        mockSensorData.temperature += (Math.random() - 0.5);
        mockSensorData.humidity += (Math.random() - 0.5) * 2;
        mockSensorData.pressure += (Math.random() - 0.5) * 0.5;
        mockSensorData.lux += (Math.random() - 0.5) * 50;
        
        // Batterie langsam entladen
        mockSensorData.batteryPercentage -= 0.01;
        if (mockSensorData.batteryPercentage < 0) mockSensorData.batteryPercentage = 100;
        
        console.log('Mock data updated');
    }, 5000); // Alle 5 Sekunden aktualisieren
} 