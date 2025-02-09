import { mockFetchAPI } from './mock-api.js';

export const USE_MOCK_API = false;
export const CHART_API_URL = 'http://192.168.2.127';

// Diese Funktion exakt wie in charts.html kopiert
export async function fetchData(action, limit) {
    try {
        // Zeitspannen in Anzahl der Datenpunkte umrechnen
        let dataLimit;
        if (limit === "24h") {
            dataLimit = 288; // 24 Stunden * 60 Minuten / 5 Minuten = 288
        } else if (limit === "3d") {
            dataLimit = 3 * 288; // 3 Tage = 3 * 288
        } else if (limit === "7d") {
            dataLimit = 7 * 288; // 7 Tage = 7 * 288
        } else {
            dataLimit = parseInt(limit); // Standard-Datenpunkte
        }

        const url = `${CHART_API_URL}/cgi-bin/api_handler2.py?action=${action}&limit=${dataLimit}`;
        console.log("Fetching URL:", url);
        
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        return data;
    } catch (error) {
        console.error("Failed to fetch data:", error);
        return { data: [] };
    }
}

// Diese Funktion verwendet Mock-API nur für Sensor-Daten
export async function fetchAPI(endpoint, options = {}) {
    if (USE_MOCK_API) {
        return mockFetchAPI(endpoint);
    }

    const defaultOptions = {
        headers: {
            'Content-Type': 'application/json'
        }
    };

    try {
        const response = await fetch(
            `/api/${endpoint}`, 
            { ...defaultOptions, ...options }
        );
        
        if (!response.ok) {
            throw new Error(`API Error: ${response.status}`);
        }

        const contentType = response.headers.get('content-type');
        if (contentType && contentType.includes('application/json')) {
            return await response.json();
        }
        
        return await response.text();

    } catch (error) {
        console.error('API Request failed:', error);
        throw error;
    }
}