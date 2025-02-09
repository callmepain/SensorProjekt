import { loadDashboard } from './views/dashboard.js';
import { loadCharts } from './views/charts.js';
import { loadFiles } from './views/files.js';
import { loadConfig } from './views/config.js';
import { loadOTA } from './views/ota.js';

const routes = {
    '#dashboard': loadDashboard,
    '#charts': loadCharts,
    '#files': loadFiles,
    '#config': loadConfig,
    '#ota': loadOTA
};

export function initRouter() {
    // Initial Route laden
    handleRoute();
    
    // Route-Änderungen überwachen
    window.addEventListener('hashchange', handleRoute);
}

async function handleRoute() {
    const hash = window.location.hash || '#dashboard';
    const loadView = routes[hash];
    
    if (loadView) {
        // Aktiven Menüpunkt markieren
        updateActiveMenuItem(hash);
        // View laden
        await loadView();
    }
}

function updateActiveMenuItem(hash) {
    document.querySelectorAll('.menu-item').forEach(item => {
        item.classList.toggle('active', item.getAttribute('href') === hash);
    });
} 