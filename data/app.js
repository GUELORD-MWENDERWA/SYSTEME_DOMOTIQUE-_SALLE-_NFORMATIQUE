// Configuration
const API_INTERVAL = 250; // 250ms for ultra-fast updates
const CHART_POINTS = 60;
const API_TIMEOUT = 5000; // 5 second timeout for API calls
let updateInterval = API_INTERVAL;

// State
let systemData = null;
let rfidListData = [];
let relayStates = [false, false, false, false, false, false, false, false];
let chartData = {
    labels: [],
    voltages: [],
    currents: [],
    powers: []
};
let chart = null;
let lastUpdateTime = 0;
let updating = false;
let consecutiveErrors = 0;
let maxConsecutiveErrors = 3;

// Theme Management
function initTheme() {
    const saved = localStorage.getItem('theme') || 'light';
    setTheme(saved);
    document.getElementById('themeBtn').addEventListener('click', toggleTheme);
}

function setTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);
    const icon = theme === 'dark' ? '☀️' : '🌙';
    document.getElementById('themeBtn').textContent = icon;
}

function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme') || 'light';
    const next = current === 'light' ? 'dark' : 'light';
    setTheme(next);
}

// Navigation
function initNavigation() {
    document.querySelectorAll('.nav-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const page = e.target.getAttribute('data-page');
            showPage(page);
            
            // Update active button
            document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
            e.target.classList.add('active');
        });
    });
}

function showPage(pageName) {
    // Hide all pages
    document.querySelectorAll('.page-content').forEach(p => p.classList.remove('active'));
    
    // Show selected page
    const page = document.getElementById(pageName + '-page');
    if (page) {
        page.classList.add('active');
        
        // Update title
        const titles = {
            dashboard: '📊 Dashboard',
            relays: '⚙️ Relais',
            energy: '⚡ Énergie',
            rfid: '🔐 Cartes RFID',
            commands: '💻 Commandes'
        };
        document.getElementById('pageTitle').textContent = titles[pageName] || 'Page';
    }
}

// Real-time data updates (optimized with Promise.all for parallel requests)
async function updateSystemData() {
    if (updating) return; // Skip if already updating
    updating = true;
    
    try {
        // Create timeout promise
        const timeoutPromise = new Promise((_, reject) =>
            setTimeout(() => reject(new Error('API timeout')), API_TIMEOUT)
        );
        
        // Fetch both system data and RFID list in parallel with timeout
        const fetchPromise = Promise.all([
            fetch('/api/system'),
            fetch('/api/rfid/list')
        ]);
        
        const [systemResponse, rfidResponse] = await Promise.race([fetchPromise, timeoutPromise]);
        
        if (!systemResponse.ok || !rfidResponse.ok) {
            throw new Error(`API response not ok: system=${systemResponse.status}, rfid=${rfidResponse.status}`);
        }
        
        // Parse responses in parallel too
        const [newSystemData, newRFIDList] = await Promise.all([
            systemResponse.json(),
            rfidResponse.json()
        ]);
        
        systemData = newSystemData;
        rfidListData = newRFIDList || [];
        consecutiveErrors = 0; // Reset error counter on success
        
        // Update all UI elements at once
        updateEnergy();
        updateStatus();
        updateRelayDisplay();
        updateChartData();
        updateRFIDList();
        
        // Log heartbeat
        console.log(`💚 Data updated at ${new Date().toLocaleTimeString()}`);
        
    } catch (error) {
        consecutiveErrors++;
        console.error(`❌ Data update failed (${consecutiveErrors}/${maxConsecutiveErrors}):`, error);
        
        // If too many consecutive errors, show warning
        if (consecutiveErrors >= maxConsecutiveErrors) {
            const nowLabel = new Date().toLocaleTimeString();
            console.warn(`⚠️ Connection lost at ${nowLabel}`);
        }
    } finally {
        updating = false;
    }
}

function updateEnergy() {
    if (!systemData) return;
    
    const voltage = systemData.voltage || 0;
    const current = systemData.current || 0;
    const power = systemData.power || 0;
    const frequency = systemData.frequency || 0;
    
    // Dashboard
    document.getElementById('voltage').textContent = voltage.toFixed(1) + 'V';
    document.getElementById('current').textContent = current.toFixed(2) + 'A';
    document.getElementById('power').textContent = Math.round(power) + 'W';
    document.getElementById('frequency').textContent = frequency.toFixed(1) + 'Hz';
    
    // Energy page
    document.getElementById('voltage2').textContent = voltage.toFixed(1);
    document.getElementById('current2').textContent = current.toFixed(2);
    document.getElementById('power2').textContent = Math.round(power);
    document.getElementById('frequency2').textContent = frequency.toFixed(1);
    
    // Progress bars
    document.getElementById('voltageBar').style.width = Math.min(100, (voltage / 250) * 100) + '%';
    document.getElementById('currentBar').style.width = Math.min(100, (current / 50) * 100) + '%';
    document.getElementById('powerBar').style.width = Math.min(100, (power / 5000) * 100) + '%';
}

function updateStatus() {
    if (!systemData) return;
    
    document.getElementById('presence').textContent = systemData.presence_count || 0;
    document.getElementById('entries').textContent = systemData.entries_count || 0;
    document.getElementById('exits').textContent = systemData.exits_count || 0;
    
    const alarm = document.getElementById('alarm');
    alarm.textContent = systemData.intrusion_detected ? 'ON' : 'OFF';
    alarm.style.color = systemData.intrusion_detected ? '#ff6b6b' : '#48c6a6';
    
    // Update time
    const now = new Date().toLocaleTimeString('fr-FR');
    document.getElementById('timeBadge').textContent = now;
    
    // Mode badge
    const modeBadge = document.getElementById('modeBadge');
    modeBadge.textContent = systemData.current_mode === 0 ? 'ACCÈS' : 'ENREGISTREMENT';
    
    // Day/Night badge
    const dayNightBadge = document.getElementById('dayNightBadge');
    dayNightBadge.textContent = systemData.daynight === 0 ? 'JOUR' : 'NUIT';
}

function updateRelayDisplay() {
    if (!systemData || !systemData.relays) return;
    
    relayStates = systemData.relays;
    
    relayStates.forEach((state, index) => {
        // Update cards on relays page
        const card = document.querySelector(`.relay-card[data-relay="${index}"]`);
        if (card) {
            const btn = card.querySelector('.btn-toggle-relay');
            btn.textContent = state ? 'ON' : 'OFF';
            btn.classList.toggle('on', state);
            card.classList.toggle('active', state);
        }
        
        // Update quick buttons
        const quickBtn = document.querySelector(`.relay-quick-btn[data-relay="${index}"]`);
        if (quickBtn) {
            quickBtn.classList.toggle('active', state);
        }
    });
}

function updateChartData() {
    if (!systemData) return;
    
    const now = new Date();
    const timeLabel = now.toLocaleTimeString('fr-FR', { hour: '2-digit', minute: '2-digit' });
    
    chartData.labels.push(timeLabel);
    chartData.voltages.push(systemData.voltage || 0);
    chartData.currents.push(systemData.current || 0);
    chartData.powers.push(systemData.power || 0);
    
    // Keep only last CHART_POINTS
    if (chartData.labels.length > CHART_POINTS) {
        chartData.labels.shift();
        chartData.voltages.shift();
        chartData.currents.shift();
        chartData.powers.shift();
    }
    
    if (chart) {
        chart.data.labels = chartData.labels;
        chart.data.datasets[0].data = chartData.voltages;
        chart.data.datasets[1].data = chartData.currents;
        chart.data.datasets[2].data = chartData.powers;
        chart.update();
    }
}

async function updateRFIDList() {
    const tbody = document.getElementById('rfidBody');
    if (!tbody) return;
    
    const cards = rfidListData;
    
    if (!cards || cards.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" style="text-align:center;">Aucune carte enregistrée</td></tr>';
        return;
    }
    
    tbody.innerHTML = cards.map(card => `
        <tr>
            <td>${card.uid}</td>
            <td>${card.name || '-'}</td>
            <td>${card.authorized ? '✓' : '✗'}</td>
            <td>${card.isInside ? '✓' : '✗'}</td>
            <td><button class="btn-delete" onclick="deleteCard('${card.uid}')">Supprimer</button></td>
        </tr>
    `).join('');
}

// Relay Control
async function toggleRelay(index) {
    try {
        const response = await fetch(`/api/relay?id=${index}`, {
            method: 'POST'
        });
        
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        // Update local state immediately
        relayStates[index] = !relayStates[index];
        updateRelayDisplay();
        
    } catch (error) {
        console.error('Toggle relay failed:', error);
        alert('Erreur: Impossible de basculer le relais');
    }
}

// Add relay button handlers
function initRelayButtons() {
    document.querySelectorAll('.relay-quick-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const relay = parseInt(btn.getAttribute('data-relay'));
            toggleRelay(relay);
        });
    });
    
    document.querySelectorAll('.btn-toggle-relay').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const relay = parseInt(e.target.getAttribute('data-relay'));
            toggleRelay(relay);
        });
    });
}

// RFID Management
async function registerRFIDCard(event) {
    if (event) event.preventDefault();
    
    const uid = document.getElementById('uidInput').value;
    const name = document.getElementById('nameInput').value;
    const access = document.getElementById('accessInput').value;
    
    if (!uid || !name) {
        alert('UID et Nom sont obligatoires');
        return;
    }
    
    try {
        const response = await fetch('/api/rfid/register', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ uid, name, authorized: access === '1' })
        });
        
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        document.getElementById('uidInput').value = '';
        document.getElementById('nameInput').value = '';
        
        alert('✅ Carte enregistrée avec succès!');
        await updateSystemData(); // Refresh all data
        
    } catch (error) {
        console.error('❌ Register card failed:', error);
        alert('❌ Erreur: Impossible d\'enregistrer la carte');
    }
}

async function deleteCard(uid) {
    if (!confirm(`🗑️ Supprimer la carte ${uid} ?`)) return;
    
    try {
        const response = await fetch(`/api/rfid/delete?uid=${uid}`, {
            method: 'POST'
        });
        
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        alert('✅ Carte supprimée!');
        await updateSystemData(); // Refresh all data
        
    } catch (error) {
        console.error('❌ Delete card failed:', error);
        alert('❌ Erreur: Impossible de supprimer la carte');
    }
}

// Commands
async function sendCommand(cmd) {
    if (!confirm(`Exécuter: ${cmd} ?`)) return;
    
    try {
        const response = await fetch(`/api/cmd?cmd=${cmd.toUpperCase()}`, {
            method: 'POST'
        });
        
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        alert(`Commande ${cmd} exécutée!`);
        
        // Force refresh
        setTimeout(updateSystemData, 500);
        
    } catch (error) {
        console.error('Command failed:', error);
        alert('Erreur: Impossible d\'exécuter la commande');
    }
}

function initCommandButtons() {
    document.querySelectorAll('.btn-cmd').forEach(btn => {
        btn.addEventListener('click', () => {
            const cmd = btn.getAttribute('data-cmd');
            sendCommand(cmd);
        });
    });
}

// Initialize Chart.js
function initChart() {
    const ctx = document.getElementById('energyChart');
    if (!ctx) return;
    
    chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Tension (V)',
                    data: [],
                    borderColor: '#667eea',
                    backgroundColor: 'rgba(102, 126, 234, 0.1)',
                    yAxisID: 'y',
                    tension: 0.4,
                    fill: true,
                    pointRadius: 2,
                    pointBackgroundColor: '#667eea'
                },
                {
                    label: 'Courant (A)',
                    data: [],
                    borderColor: '#764ba2',
                    backgroundColor: 'rgba(118, 75, 162, 0.1)',
                    yAxisID: 'y1',
                    tension: 0.4,
                    fill: true,
                    pointRadius: 2,
                    pointBackgroundColor: '#764ba2'
                },
                {
                    label: 'Puissance (W)',
                    data: [],
                    borderColor: '#48c6a6',
                    backgroundColor: 'rgba(72, 198, 166, 0.1)',
                    yAxisID: 'y2',
                    tension: 0.4,
                    fill: false,
                    pointRadius: 2,
                    pointBackgroundColor: '#48c6a6'
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            interaction: {
                mode: 'index',
                intersect: false
            },
            scales: {
                y: {
                    type: 'linear',
                    display: true,
                    position: 'left',
                    title: { display: true, text: 'Tension (V)' },
                    max: 250
                },
                y1: {
                    type: 'linear',
                    display: true,
                    position: 'right',
                    title: { display: true, text: 'Courant (A)' },
                    max: 50,
                    grid: { drawOnChartArea: false }
                },
                y2: {
                    type: 'linear',
                    display: false,
                    max: 5000
                }
            },
            plugins: {
                legend: { display: true }
            }
        }
    });
}

// RFID Form Handler
function initRFIDForm() {
    const form = document.getElementById('rfidForm');
    if (form) {
        form.addEventListener('submit', registerRFIDCard);
    }
}

// Initialize Application
function initApp() {
    initTheme();
    initNavigation();
    initRelayButtons();
    initCommandButtons();
    initRFIDForm();
    initChart();
    
    // Start data updates
    updateSystemData();
    setInterval(updateSystemData, updateInterval);
    
    // Show dashboard by default
    showPage('dashboard');
    
    console.log('🏠 Domotique App initialized');
}

// Start when DOM is ready
document.addEventListener('DOMContentLoaded', initApp);
