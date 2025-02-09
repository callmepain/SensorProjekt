import { fetchAPI } from '../utils.js';

export async function loadOTA() {
    const content = document.getElementById('content');
    
    content.innerHTML = `
        <h1>OTA Update</h1>
        <form id="ota-form">
            <input type="file" name="firmware" accept=".bin" required>
            <button type="submit">Update Firmware</button>
        </form>
        <div id="update-progress" style="display: none;">
            <div class="progress-bar">
                <div class="progress" id="progress-indicator"></div>
            </div>
            <div id="progress-text">0%</div>
        </div>
    `;

    document.getElementById('ota-form').addEventListener('submit', handleUpdate);
}

async function handleUpdate(event) {
    event.preventDefault();
    const formData = new FormData(event.target);
    
    try {
        // Update-Formular ausblenden
        event.target.style.display = 'none';
        
        // Fortschrittsanzeige einblenden
        const progress = document.getElementById('update-progress');
        progress.style.display = 'block';
        
        const response = await fetch('/ota', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            showUpdateSuccess();
        } else {
            throw new Error('Update failed');
        }
    } catch (error) {
        console.error('Update failed:', error);
        showUpdateError();
    }
}

function showUpdateSuccess() {
    const content = document.getElementById('content');
    content.innerHTML = `
        <h1>Update erfolgreich!</h1>
        <p>Das Gerät wird neu gestartet...</p>
        <div class="countdown">15</div>
    `;
    
    startCountdown();
}

function showUpdateError() {
    const content = document.getElementById('content');
    content.innerHTML += `
        <div class="error-message">
            Update fehlgeschlagen! Bitte versuchen Sie es erneut.
        </div>
    `;
}

function startCountdown() {
    let seconds = 15;
    const countdownElement = document.querySelector('.countdown');
    
    const interval = setInterval(() => {
        seconds--;
        countdownElement.textContent = seconds;
        
        if (seconds <= 0) {
            clearInterval(interval);
            window.location.href = '/';
        }
    }, 1000);
} 