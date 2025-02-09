import { fetchAPI } from '../utils.js';

export async function loadConfig() {
    const content = document.getElementById('content');
    
    content.innerHTML = `
        <h1>Konfiguration</h1>
        <form id="config-form" class="form-container">
            <div class="form-section">
                <h3>Gerät</h3>
                <div class="form-group">
                    <label for="device-name">Gerätename</label>
                    <input type="text" id="device-name" name="device_name">
                </div>
            </div>

            <div class="form-section">
                <h3>WLAN</h3>
                <div class="form-group">
                    <label for="wifi-ssid">SSID</label>
                    <input type="text" id="wifi-ssid" name="wifi_ssid">
                </div>
                <div class="form-group">
                    <label for="wifi-password">Passwort</label>
                    <input type="password" id="wifi-password" name="wifi_password">
                </div>
            </div>

            <div class="form-section">
                <h3>Netzwerk</h3>
                <div class="form-group">
                    <label for="ip-address">IP-Adresse</label>
                    <input type="text" id="ip-address" name="ip_address">
                </div>
                <div class="form-group">
                    <label for="subnet-mask">Subnetzmaske</label>
                    <input type="text" id="subnet-mask" name="subnet_mask">
                </div>
                <div class="form-group">
                    <label for="gateway">Gateway</label>
                    <input type="text" id="gateway" name="gateway">
                </div>
            </div>

            <button type="submit">Speichern</button>
        </form>
    `;

    // Event Listener für Form
    document.getElementById('config-form').addEventListener('submit', handleSubmit);
    
    // Aktuelle Konfiguration laden
    await loadCurrentConfig();
}

async function loadCurrentConfig() {
    const config = await fetchAPI('config');
    
    // Formularfelder füllen
    Object.entries(config).forEach(([key, value]) => {
        const element = document.getElementById(key.replace('_', '-'));
        if (element) element.value = value;
    });
}

async function handleSubmit(event) {
    event.preventDefault();
    const formData = new FormData(event.target);
    const config = Object.fromEntries(formData);
    
    try {
        await fetchAPI('config', {
            method: 'POST',
            body: JSON.stringify(config)
        });
        alert('Konfiguration gespeichert!');
    } catch (error) {
        console.error('Save failed:', error);
        alert('Fehler beim Speichern!');
    }
}