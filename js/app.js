import { USE_MOCK_API, fetchAPI } from './utils.js';
import { startMockDataUpdates } from './mock-api.js';

// Zentrale App-Initialisierung und gemeinsame Funktionen
export function initApp() {
    // Globale Event Listener
    setupEventListeners();
    
    // Globale Error Handler
    setupErrorHandling();
    
    // Initialer API-Check
    checkAPIConnection();

    // Mock-Daten starten wenn Mock-API aktiv ist
    if (USE_MOCK_API) {
        startMockDataUpdates();
        console.log('Mock API aktiviert');
    }
}

function setupEventListeners() {
    // Automatisches Neuladen bei Verbindungsverlust
    window.addEventListener('online', () => {
        console.log('Verbindung wiederhergestellt');
        location.reload();
    });

    // Warnung bei Verbindungsverlust
    window.addEventListener('offline', () => {
        console.log('Verbindung verloren');
        showOfflineWarning();
    });
}

function setupErrorHandling() {
    window.onerror = function(msg, url, line) {
        console.error('JavaScript Error:', msg, 'Line:', line);
        return false;
    };

    window.addEventListener('unhandledrejection', event => {
        console.error('Unhandled Promise Rejection:', event.reason);
    });
}

async function checkAPIConnection() {
    try {
        const status = await fetchAPI('status');
        if (!status) {
            showAPIError();
        }
    } catch (error) {
        console.error('API nicht erreichbar:', error);
        showAPIError();
    }
}

function showOfflineWarning() {
    // Hier können Sie eine Benachrichtigung anzeigen
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