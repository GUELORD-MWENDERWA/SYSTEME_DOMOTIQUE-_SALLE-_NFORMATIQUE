const UPDATE_INTERVAL = 500;
const CHART_POINTS = 60;
const API_BASE = "/api";

let systemData = null;
let chartData = {
  voltage: [],
  current: [],
  power: [],
};
let charts = {};
let relayStates = [false, false, false, false, false, false, false, false];
let updating = false;

// Initialize
window.addEventListener("load", () => {
  for (let i = 0; i < CHART_POINTS; i++) {
    chartData.voltage.push(0);
    chartData.current.push(0);
    chartData.power.push(0);
  }
  initTheme();
  initNavigation();
  initCharts();
  initRelays();
  startUpdates();
});

// Theme
function initTheme() {
  const saved = localStorage.getItem("theme") || "light";
  setTheme(saved);
  document.getElementById("themeBtn").addEventListener("click", toggleTheme);
}

function setTheme(theme) {
  document.documentElement.setAttribute("data-theme", theme);
  localStorage.setItem("theme", theme);
  document.getElementById("themeBtn").textContent =
    theme === "dark" ? "☀️" : "🌙";
}

function toggleTheme() {
  const current =
    document.documentElement.getAttribute("data-theme") || "light";
  setTheme(current === "light" ? "dark" : "light");
}

// Navigation
function initNavigation() {
  document.querySelectorAll(".nav-btn").forEach((btn) => {
    btn.addEventListener("click", (e) => {
      const page = e.target.getAttribute("data-page");
      showPage(page);
      document
        .querySelectorAll(".nav-btn")
        .forEach((b) => b.classList.remove("active"));
      e.target.classList.add("active");
    });
  });
}

function showPage(pageName) {
  document.querySelectorAll(".page").forEach((p) => {
    p.style.display = "none";
    p.classList.remove("active");
  });
  const page = document.getElementById(pageName);
  if (page) {
    page.style.display = "block";
    page.classList.add("active");
  }

  const titles = {
    dashboard: "📊 Dashboard",
    charts: "📈 Graphiques",
    relays: "⚙️ Relais",
    rfid: "🔐 RFID",
    commands: "💻 Commandes",
  };
  document.getElementById("pageTitle").textContent = titles[pageName] || "Page";

  if (pageName === "rfid") {
    loadRFID();
  }
}

// Charts - SIMPLE & FUNCTIONAL
function initCharts() {
  const labels = Array(CHART_POINTS).fill(0).map((_, i) => i);

  // Voltage Chart
  const voltageCtx = document.getElementById("voltageChart")?.getContext("2d");
  if (voltageCtx) {
    charts.voltage = new Chart(voltageCtx, {
      type: "line",
      data: {
        labels: labels,
        datasets: [{
          label: "Tension (V)",
          data: chartData.voltage,
          borderColor: "#FF6B6B",
          backgroundColor: "rgba(255, 107, 107, 0.1)",
          borderWidth: 2,
          fill: true,
          tension: 0.3,
          pointRadius: 0,
        }],
      },
      options: {
        responsive: true,
        maintainAspectRatio: true,
        animation: false,
        plugins: { legend: { display: false } },
        scales: {
          y: {
            min: 0,
            max: 250,
            ticks: { stepSize: 50 }
          }
        },
      },
    });
  }

  // Current Chart
  const currentCtx = document.getElementById("currentChart")?.getContext("2d");
  if (currentCtx) {
    charts.current = new Chart(currentCtx, {
      type: "line",
      data: {
        labels: labels,
        datasets: [{
          label: "Courant (A)",
          data: chartData.current,
          borderColor: "#4ECDC4",
          backgroundColor: "rgba(78, 205, 196, 0.1)",
          borderWidth: 2,
          fill: true,
          tension: 0.3,
          pointRadius: 0,
        }],
      },
      options: {
        responsive: true,
        maintainAspectRatio: true,
        animation: false,
        plugins: { legend: { display: false } },
        scales: {
          y: {
            min: 0,
            max: 10,
            ticks: { stepSize: 2 }
          }
        },
      },
    });
  }

  // Power Chart
  const powerCtx = document.getElementById("powerChart")?.getContext("2d");
  if (powerCtx) {
    charts.power = new Chart(powerCtx, {
      type: "line",
      data: {
        labels: labels,
        datasets: [{
          label: "Puissance (W)",
          data: chartData.power,
          borderColor: "#FFE66D",
          backgroundColor: "rgba(255, 230, 109, 0.1)",
          borderWidth: 2,
          fill: true,
          tension: 0.3,
          pointRadius: 0,
        }],
      },
      options: {
        responsive: true,
        maintainAspectRatio: true,
        animation: false,
        plugins: { legend: { display: false } },
        scales: {
          y: {
            min: 0,
            max: 5000,
            ticks: { stepSize: 1000 }
          }
        },
      },
    });
  }

  // Full chart
  const fullCtx = document.getElementById("fullChart")?.getContext("2d");
  if (fullCtx) {
    charts.full = new Chart(fullCtx, {
      type: "line",
      data: {
        labels: labels,
        datasets: [
          {
            label: "V",
            data: chartData.voltage,
            borderColor: "#FF6B6B",
            borderWidth: 2,
            fill: false,
            tension: 0.3,
            pointRadius: 0,
          },
          {
            label: "A",
            data: chartData.current,
            borderColor: "#4ECDC4",
            borderWidth: 2,
            fill: false,
            tension: 0.3,
            pointRadius: 0,
          },
          {
            label: "W",
            data: chartData.power,
            borderColor: "#FFE66D",
            borderWidth: 2,
            fill: false,
            tension: 0.3,
            pointRadius: 0,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: true,
        animation: false,
        plugins: { legend: { display: true } },
        scales: {
          y: { beginAtZero: true }
        },
      },
    });
  }
}

// Data Updates
function startUpdates() {
  updateData();
  setInterval(updateData, UPDATE_INTERVAL);
  setInterval(updateTime, 1000);
}

async function updateData() {
  if (updating) return;
  updating = true;

  try {
    const response = await fetch(`${API_BASE}/system`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    const text = await response.text();
    systemData = JSON.parse(text);

    setApiStatus(true);
    updateUI();
    updateCharts_data();
  } catch (error) {
    console.error("❌ API Error:", error.message);
    setApiStatus(false);
  } finally {
    updating = false;
  }
}

function updateUI() {
  if (!systemData) return;

  document.getElementById("sysMode").textContent =
    systemData.current_mode === 0 ? "ACCESS" : "REG";
  document.getElementById("sysDayNight").textContent =
    systemData.daynight === 0 ? "☀️ Jour" : "🌙 Nuit";
  document.getElementById("dashVolt").textContent =
    (systemData.voltage || 0).toFixed(1) + "V";
  document.getElementById("dashAmp").textContent =
    (systemData.current || 0).toFixed(2) + "A";
  document.getElementById("dashEntries").textContent =
    systemData.entries_count || 0;
  document.getElementById("dashExits").textContent =
    systemData.exits_count || 0;

  if (Array.isArray(systemData.relays)) {
    relayStates = systemData.relays;
    updateRelayDisplay();
  }

  document.getElementById("footerStatus").textContent =
    systemData.intrusion_detected ? "🚨 ALARME!" : "✅ OK";
}

function updateCharts_data() {
  if (!systemData) return;

  chartData.voltage.shift();
  chartData.voltage.push(systemData.voltage || 0);

  chartData.current.shift();
  chartData.current.push(systemData.current || 0);

  chartData.power.shift();
  chartData.power.push(systemData.power || 0);

  // Update voltage chart avec échelle dynamique
  if (charts.voltage) {
    charts.voltage.data.datasets[0].data = chartData.voltage;
    const voltageMax = Math.max((systemData.voltage || 0) + 10, 250);
    charts.voltage.options.scales.y.max = voltageMax;
    charts.voltage.update("none");
  }

  // Update current chart avec échelle dynamique
  if (charts.current) {
    charts.current.data.datasets[0].data = chartData.current;
    const currentMax = Math.max((systemData.current || 0) + 5, 10);
    charts.current.options.scales.y.max = currentMax;
    charts.current.update("none");
  }

  // Update power chart avec échelle dynamique
  if (charts.power) {
    charts.power.data.datasets[0].data = chartData.power;
    const powerMax = Math.max((systemData.power || 0) + 1000, 5000);
    charts.power.options.scales.y.max = powerMax;
    charts.power.update("none");
  }

  // Update full chart
  if (charts.full) {
    charts.full.data.datasets[0].data = chartData.voltage;
    charts.full.data.datasets[1].data = chartData.current;
    charts.full.data.datasets[2].data = chartData.power;
    charts.full.update("none");
  }
}

function updateTime() {
  const now = new Date().toLocaleTimeString("fr-FR");
  const toEl = document.getElementById("footerTime");
  if (toEl) toEl.textContent = now;
  const luEl = document.getElementById("lastUpdate");
  if (luEl) luEl.textContent = now;
}

function setApiStatus(connected) {
  const el = document.getElementById("apiStatus");
  if (el) {
    el.classList.toggle("api-connected", connected);
    el.classList.toggle("api-disconnected", !connected);
    el.textContent = connected ? "✅ API OK" : "❌ API Err";
  }
}

function showError(message) {
  const el = document.getElementById("errorMessage");
  if (el) {
    el.textContent = message;
    el.style.display = "block";
    setTimeout(() => {
      el.style.display = "none";
    }, 3000);
  }
}

// Relays
function initRelays() {
  document.querySelectorAll(".relay-btn-quick").forEach((btn) => {
    const id = btn.getAttribute("data-relay");
    if (id !== null)
      btn.addEventListener("click", () => toggleRelay(parseInt(id)));
  });

  document.querySelectorAll(".btn-toggle").forEach((btn) => {
    const id = btn.getAttribute("data-id");
    if (id !== null)
      btn.addEventListener("click", () => toggleRelay(parseInt(id)));
  });
}

async function toggleRelay(id) {
  if (id < 0 || id > 7) return;

  try {
    const response = await fetch(`${API_BASE}/relay?id=${id}`, {
      method: "POST",
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    relayStates[id] = !relayStates[id];
    updateRelayDisplay();
    await updateData();
    showError("✓ Relais basculé");
  } catch (error) {
    showError(`Erreur: ${error.message}`);
  }
}

function updateRelayDisplay() {
  document.querySelectorAll(".relay-card").forEach((card, idx) => {
    const btn = card.querySelector(".btn-toggle");
    if (btn) {
      const state = relayStates[idx];
      card.classList.toggle("active", state);
      btn.textContent = state ? "✓ ON" : "✗ OFF";
      btn.classList.toggle("on", state);
    }
  });

  document.querySelectorAll(".relay-btn-quick").forEach((btn, idx) => {
    btn.classList.toggle("active", relayStates[idx]);
  });
}

// Commands
document.addEventListener("click", (e) => {
  if (e.target.classList.contains("cmd-btn")) {
    const cmd = e.target.getAttribute("data-cmd");
    if (cmd) execCommand(cmd);
  }
});

async function execCommand(cmd) {
  try {
    const response = await fetch(`${API_BASE}/cmd?cmd=${cmd}`, {
      method: "POST",
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    const text = await response.text();
    const result = JSON.parse(text);
    showError(`✅ ${cmd}: ${result.message || result.status}`);
    setTimeout(updateData, 500);
  } catch (error) {
    showError(`Erreur: ${error.message}`);
  }
}

// RFID Management
async function addRFID() {
  const uid = document.getElementById("rfidUid")?.value?.trim();
  const name = document.getElementById("rfidName")?.value?.trim();

  if (!uid || !name) {
    showError("❌ UID et Nom requis");
    return;
  }

  try {
    const response = await fetch(`${API_BASE}/rfid/register`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ uid, name }),
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    clearRFIDForm();
    showError("✅ Carte ajoutée!");
    loadRFID();
  } catch (error) {
    showError(`Erreur: ${error.message}`);
  }
}

async function authorizeRFID() {
  const uid = document.getElementById("rfidUid")?.value?.trim();
  if (!uid) {
    showError("❌ UID requis");
    return;
  }

  try {
    const response = await fetch(`${API_BASE}/rfid/authorize?uid=${encodeURIComponent(uid)}`, { method: "POST" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    showError("✅ Carte autorisée!");
    loadRFID();
  } catch (error) {
    showError(`Erreur: ${error.message}`);
  }
}

async function unauthorizeRFID() {
  const uid = document.getElementById("rfidUid")?.value?.trim();
  if (!uid) {
    showError("❌ UID requis");
    return;
  }

  try {
    const response = await fetch(`${API_BASE}/rfid/unauthorize?uid=${encodeURIComponent(uid)}`, { method: "POST" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    showError("✅ Carte refusée!");
    loadRFID();
  } catch (error) {
    showError(`Erreur: ${error.message}`);
  }
}

function clearRFIDForm() {
  document.getElementById("rfidUid").value = "";
  document.getElementById("rfidName").value = "";
}

// RFID Load
async function loadRFID() {
  const rfidList = document.getElementById("rfidList");
  if (!rfidList) return;

  rfidList.innerHTML = "Chargement...";

  try {
    const response = await fetch(`${API_BASE}/rfid/list`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    const cards = await response.json();

    if (!Array.isArray(cards) || cards.length === 0) {
      rfidList.innerHTML = "<p style='text-align:center; padding:20px; color:#999;'>Aucune carte RFID</p>";
      return;
    }

    let html = "<div style='display: flex; flex-direction: column; gap: 10px;'>";
    cards.forEach((card) => {
      const auth = card.authorized ? "✅" : "❌";
      const inside = card.isInside ? "🟢" : "🔴";
      html += `<div style='background: var(--color-card); padding: 12px; border-radius: 8px; border: 1px solid var(--color-border); display: flex; justify-content: space-between; align-items: center;'>
        <div>
          <div style='font-weight: bold;'>${card.name || "---"}</div>
          <div style='font-size: 12px; opacity: 0.7; margin-top: 5px;'>${card.uid}</div>
          <div style='font-size: 12px; margin-top: 5px;'>${auth} Auth | ${inside} Présent</div>
        </div>
        <div style='display: flex; gap: 5px;'>
          <button onclick="loadRFIDToForm('${card.uid}', '${card.name || ''}')" style='padding: 6px 10px; background: #667eea; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 11px;'>✏️ Éditer</button>
          <button onclick="deleteRFID('${card.uid}')" style='padding: 6px 10px; background: #f44336; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 11px;'>🗑️ Suppr</button>
        </div>
      </div>`;
    });
    html += "</div>";
    rfidList.innerHTML = html;
  } catch (error) {
    rfidList.innerHTML = `<p style='color: #ff6b6b;'>Erreur: ${error.message}</p>`;
  }
}

function loadRFIDToForm(uid, name) {
  document.getElementById("rfidUid").value = uid;
  document.getElementById("rfidName").value = name;
}

async function deleteRFID(uid) {
  if (!confirm(`Supprimer la carte ${uid}?`)) return;

  try {
    const response = await fetch(`${API_BASE}/rfid/delete?uid=${encodeURIComponent(uid)}`, { method: "POST" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    showError("✅ Carte supprimée!");
    loadRFID();
  } catch (error) {
    showError(`Erreur: ${error.message}`);
  }
}
