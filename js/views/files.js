import { fetchAPI } from '../utils.js';

export async function loadFiles() {
    const content = document.getElementById('content');
    
    content.innerHTML = `
        <h1>Dateiverwaltung</h1>
        <div id="storage-info"></div>
        <div id="file-list"></div>
        
        <!-- Upload Form -->
        <div class="upload-section">
            <h2>Datei hochladen</h2>
            <form id="upload-form">
                <input type="file" name="file" required>
                <button type="submit">Hochladen</button>
            </form>
        </div>
    `;

    // Event Listener für Upload
    document.getElementById('upload-form').addEventListener('submit', handleUpload);
    
    // Initial laden
    await Promise.all([
        fetchStorageInfo(),
        fetchFileList()
    ]);
}

async function fetchStorageInfo() {
    const info = await fetchAPI('storage');
    const storageInfo = document.getElementById('storage-info');
    
    storageInfo.innerHTML = `
        <div class="storage-stats">
            <div>SPIFFS: ${formatBytes(info.SPIFFS.used)} / ${formatBytes(info.SPIFFS.total)}</div>
            <div>SD: ${info.SD.total ? formatBytes(info.SD.used) + ' / ' + formatBytes(info.SD.total) : 'Nicht verfügbar'}</div>
        </div>
    `;
}

async function fetchFileList() {
    const files = await fetchAPI('files');
    const fileList = document.getElementById('file-list');
    
    fileList.innerHTML = `
        <table>
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Größe</th>
                    <th>Aktionen</th>
                </tr>
            </thead>
            <tbody>
                ${files.map(file => `
                    <tr>
                        <td>${file.name}</td>
                        <td>${formatBytes(file.size)}</td>
                        <td>
                            <button onclick="FILES.edit('${file.name}')">Bearbeiten</button>
                            <button onclick="FILES.delete('${file.name}')">Löschen</button>
                        </td>
                    </tr>
                `).join('')}
            </tbody>
        </table>
    `;
}

async function handleUpload(event) {
    event.preventDefault();
    const formData = new FormData(event.target);
    
    try {
        await fetch('/upload', {
            method: 'POST',
            body: formData
        });
        await fetchFileList(); // Liste aktualisieren
    } catch (error) {
        console.error('Upload failed:', error);
    }
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

// Globale Funktionen für Button-Clicks
window.FILES = {
    async delete(filename) {
        if (confirm(`Datei "${filename}" wirklich löschen?`)) {
            await fetchAPI(`delete/${filename}`, { method: 'DELETE' });
            await fetchFileList();
        }
    },
    
    async edit(filename) {
        const content = await fetchAPI(`file/${filename}`);
        // Editor-Logik hier implementieren
    }
}; 