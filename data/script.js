// =========================================================
// DEWHEATER MK2 - MODERN OBSERVATORY DASHBOARD JAVASCRIPT
// =========================================================

window.addEventListener('load', function() {
  initGauges();
  getReadings();
  getBuildInfo();
  getPID();
  getDefaultOffsets();
  getSoftLandingConfig();
  getLearningConfig();
  getWifiStatus();
  initH0Chart();
  initH0ErrorChart();
  initH1Chart();
  initH1ErrorChart();
  initTunerChart();
  initConductanceChart();
  initConvergenceChart();
  initSliders();
  initBrightnessSlider();
  initMap();
  updateTunerPowerDisplay();
  
  // Start background polling for tuner status
  setInterval(pollTuningStatus, 1000);
});

// --- BUILD & VERSION INFO ---
function getBuildInfo() {
  fetch('/api/buildinfo')
    .then(r => r.json())
    .then(data => {
      document.getElementById('sw-version').textContent = data.version || 'v2.0';
      document.getElementById('compile-date').textContent = data.date || '';
      document.getElementById('compile-time').textContent = data.time || '';
    })
    .catch(e => {
      console.warn('Could not fetch buildinfo:', e);
      document.getElementById('sw-version').textContent = 'Mk2 RTOS';
    });
  
  // Set WebSerial URL dynamically
  const webserialUrl = `http://${window.location.hostname}:81/webserial`;
  const webserialLink = document.getElementById('webserial-url');
  if (webserialLink) {
    webserialLink.href = webserialUrl;
  }
}

// --- TAB SWITCHING ---
function switchTab(tabName) {
  const tabs = ['dashboard', 'charts', 'adaptive', 'tuner', 'settings'];
  tabs.forEach(t => {
    const content = document.getElementById(`tab-${t}`);
    const btn = document.getElementById(`btn-tab-${t}`);
    if (content && btn) {
      if (t === tabName) {
        content.classList.add('active');
        content.style.display = 'block';
        btn.classList.add('active');
      } else {
        content.classList.remove('active');
        content.style.display = 'none';
        btn.classList.remove('active');
      }
    }
  });

  // Re-size charts and maps upon tab reveal
  if (tabName === 'charts') {
    if (h0Chart) { h0Chart.resize(); h0Chart.update(); }
    if (h0ErrorChart) { h0ErrorChart.resize(); h0ErrorChart.update(); }
    if (h1Chart) { h1Chart.resize(); h1Chart.update(); }
    if (h1ErrorChart) { h1ErrorChart.resize(); h1ErrorChart.update(); }
  }
  if (tabName === 'adaptive') {
    if (conductanceChart) { conductanceChart.resize(); conductanceChart.update(); }
    if (convergenceChart) { convergenceChart.resize(); convergenceChart.update(); }
  }
  if (tabName === 'tuner') {
    updateTunerPowerDisplay();
    if (tunerChart) {
      tunerChart.resize();
      tunerChart.update();
    }
  }
  if (tabName === 'settings') {
    getPID();
    getDefaultOffsets();
    getSoftLandingConfig();
    getLearningConfig();
    getWifiStatus();
    if (locationMap) {
      setTimeout(() => { locationMap.invalidateSize(); }, 200);
    }
  }
}



// --- COMPACT OBSERVATORY GAUGE THEMES ---
const darkLinearConfig = {
  width: 85,
  height: 150,
  units: "°C",
  minValue: -10,
  maxValue: 50,
  majorTicks: ["-10", "10", "30", "50"],
  minorTicks: 2,
  strokeTicks: false,
  colorPlate: "transparent",
  colorPlateEnd: "transparent",
  colorMajorTicks: "#64748b",
  colorMinorTicks: "#475569",
  colorNumbers: "#94a3b8",
  colorUnits: "#64748b",
  colorBar: "#1e293b",
  colorBarProgress: "#0ea5e9",
  borders: false,
  borderShadowWidth: 0,
  needleType: "arrow",
  needleWidth: 3,
  needleCircleSize: 5,
  needleCircleOuter: false,
  needleCircleInner: false,
  colorNeedle: "#38bdf8",
  colorNeedleEnd: "#0284c7",
  animationDuration: 600,
  animationRule: "linear",
  barWidth: 8,
  valueDec: 1,
  fontNumbersSize: 18,
  fontUnitsSize: 14
};

const darkRadialConfig = {
  width: 130,
  height: 130,
  units: "%",
  minValue: 0,
  maxValue: 100,
  majorTicks: ["0", "25", "50", "75", "100"],
  minorTicks: 4,
  strokeTicks: false,
  colorPlate: "transparent",
  colorPlateEnd: "transparent",
  colorMajorTicks: "#64748b",
  colorMinorTicks: "#334155",
  colorNumbers: "#94a3b8",
  colorUnits: "#64748b",
  colorTitle: "#cbd5e1",
  colorNeedle: "#00d2ff",
  colorNeedleEnd: "#0ea5e9",
  colorNeedleCircleOuter: "#1e293b",
  colorNeedleCircleInner: "#00d2ff",
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: true,
  needleType: "line",
  needleWidth: 2,
  borders: false,
  borderShadowWidth: 0,
  highlights: [
    { from: 80, to: 100, color: "rgba(239, 68, 68, 0.4)" }
  ],
  animationDuration: 500,
  animationRule: "linear",
  fontNumbersSize: 16,
  fontUnitsSize: 12
};

// --- GAUGES STATE & SAFE INITIALIZATION ---
let gaugeOutTemp = null, gaugeHum = null, gaugeDewPoint = null;
let gaugePwr1 = null, gaugeTemp1 = null, gaugePwr2 = null, gaugeTemp2 = null;

function initGauges() {
  if (typeof LinearGauge === 'undefined' || typeof RadialGauge === 'undefined') {
    console.warn("Canvas-gauges library not loaded; using direct numeric telemetry.");
    return;
  }
  try {
    // 1. Environment
    gaugeOutTemp = new LinearGauge(Object.assign({}, darkLinearConfig, {
      renderTo: 'gauge-out-temp',
      title: ""
    })).draw();

    gaugeHum = new RadialGauge(Object.assign({}, darkRadialConfig, {
      renderTo: 'gauge-humidity',
      units: "%"
    })).draw();

    gaugeDewPoint = new LinearGauge(Object.assign({}, darkLinearConfig, {
      renderTo: 'gauge-dew-point',
      colorBarProgress: "#a855f7",
      colorNeedle: "#c084fc",
      colorNeedleEnd: "#9333ea"
    })).draw();

    // 2. Guide Scope (CH0)
    gaugePwr1 = new RadialGauge(Object.assign({}, darkRadialConfig, {
      renderTo: 'gauge-pwr1',
      units: "%"
    })).draw();

    gaugeTemp1 = new LinearGauge(Object.assign({}, darkLinearConfig, {
      renderTo: 'gauge-temp1',
      colorBarProgress: "#00d2ff",
      colorNeedle: "#38bdf8"
    })).draw();

    // 3. Main Scope (CH1)
    gaugePwr2 = new RadialGauge(Object.assign({}, darkRadialConfig, {
      renderTo: 'gauge-pwr2',
      units: "%"
    })).draw();

    gaugeTemp2 = new LinearGauge(Object.assign({}, darkLinearConfig, {
      renderTo: 'gauge-temp2',
      colorBarProgress: "#a855f7",
      colorNeedle: "#c084fc"
    })).draw();
  } catch (err) {
    console.warn("Error drawing canvas gauges:", err);
  }
}

// --- STATUS INDICATORS ---
function updateStatusIndicator(elementId, isActive) {
  const el = document.getElementById(elementId);
  if (el) {
    el.style.backgroundColor = isActive ? '#10b981' : '#ef4444';
    el.style.boxShadow = isActive ? '0 0 6px rgba(16, 185, 129, 0.6)' : '0 0 4px rgba(239, 68, 68, 0.4)';
  }
}

// Global cached readings for margin calculation
let currentDewPoint = null;
let heater0Enabled = true;
let heater1Enabled = true;

function applyReadings(data) {
  if (!data) return;

  // Environment
  if (data.out_temp !== undefined) {
    if (gaugeOutTemp) gaugeOutTemp.value = data.out_temp;
    const el = document.getElementById('metric-out-temp');
    if (el) el.textContent = data.out_temp.toFixed(1);
    const hdrOut = document.getElementById('hdr-out-temp');
    if (hdrOut) hdrOut.textContent = `${data.out_temp.toFixed(1)}°C`;
  }
  if (data.humidity !== undefined) {
    if (gaugeHum) gaugeHum.value = data.humidity;
    const el = document.getElementById('metric-humidity');
    if (el) el.textContent = data.humidity.toFixed(0);
  }
  if (data.dew_point !== undefined) {
    if (gaugeDewPoint) gaugeDewPoint.value = data.dew_point;
    currentDewPoint = data.dew_point;
    const el = document.getElementById('metric-dew-point');
    if (el) el.textContent = data.dew_point.toFixed(1);
    const hdrDp = document.getElementById('hdr-dew-point');
    if (hdrDp) hdrDp.textContent = `${data.dew_point.toFixed(1)}°C`;
  }

  // Status Indicators
  if (data.wxs_online !== undefined) updateStatusIndicator('wxs-online-status', data.wxs_online);
  if (data.weewx_valid !== undefined) updateStatusIndicator('weewx-valid-status', data.weewx_valid);

  // MQTT LWT Status
  if (data.lwt_online !== undefined) {
    const badge = document.getElementById('lwt-status-badge');
    if (badge) {
      badge.textContent = data.lwt_online ? 'Online' : 'Offline';
      badge.style.background = data.lwt_online ? 'rgba(16, 185, 129, 0.2)' : 'rgba(239, 68, 68, 0.2)';
      badge.style.color = data.lwt_online ? '#34d399' : '#fca5a5';
      badge.style.borderColor = data.lwt_online ? 'rgba(52, 211, 153, 0.4)' : 'rgba(239, 68, 68, 0.4)';
    }
  }

  // Guide Scope (CH0)
  if (data.heater0_enabled !== undefined) {
    heater0Enabled = data.heater0_enabled;
    const btn = document.getElementById('btn-toggle-heater0');
    if (btn) {
      btn.className = heater0Enabled ? 'btn-heater-toggle active' : 'btn-heater-toggle standby';
      btn.textContent = heater0Enabled ? '🔥 PID: ON' : '❄️ OFF (Cooldown)';
    }
    const cdState = document.getElementById('cd-state-ch0');
    if (cdState) {
      cdState.className = heater0Enabled ? 'status-active' : 'status-off';
      cdState.textContent = heater0Enabled ? 'PID ON' : 'OFF (Cooldown)';
    }
  }
  if (data.temp1 !== undefined) {
    const cdTemp = document.getElementById('cd-temp-ch0');
    if (cdTemp) cdTemp.textContent = `${data.temp1.toFixed(1)}°C`;
  }

  if (data.pwr1 !== undefined) {
    if (gaugePwr1) gaugePwr1.value = data.pwr1;
    const el = document.getElementById('metric-pwr1');
    if (el) el.textContent = Math.round(data.pwr1);
  }
  if (data.temp1 !== undefined) {
    if (gaugeTemp1) gaugeTemp1.value = data.temp1;
    if (currentDewPoint !== null) {
      const margin = data.temp1 - currentDewPoint;
      const marginEl = document.getElementById('margin1-display');
      if (marginEl) {
        marginEl.textContent = `${margin >= 0 ? '+' : ''}${margin.toFixed(1)}°C`;
        marginEl.style.color = margin >= 1.5 ? '#10b981' : (margin >= 0.5 ? '#f59e0b' : '#ef4444');
      }
    }
  }
  if (data.offset1 !== undefined) {
    const s = document.getElementById('offset1-slider');
    if (s && document.activeElement !== s) s.value = data.offset1;
    const badge = document.getElementById('offset1-slider-value');
    if (badge) badge.textContent = data.offset1.toFixed(1);
    const cur = document.getElementById('current-offset1-display');
    if (cur) cur.textContent = data.offset1.toFixed(1);
  }
  if (data.setpoint0 !== undefined) {
    const sp = document.getElementById('setpoint1-display');
    if (sp) sp.textContent = data.setpoint0.toFixed(1);
  }
  if (data.temp1 !== undefined && data.setpoint0 !== undefined) {
    const delta0 = data.temp1 - data.setpoint0;
    const deltaEl = document.getElementById('delta-target1');
    const badgeEl = document.getElementById('delta-target1-status');
    if (deltaEl) {
      deltaEl.textContent = `${delta0 >= 0 ? '+' : ''}${delta0.toFixed(1)}°C`;
      if (Math.abs(delta0) <= 0.3) {
        deltaEl.style.color = '#34d399';
      } else if (delta0 < 0) {
        deltaEl.style.color = delta0 >= -1.0 ? '#38bdf8' : '#f59e0b';
      } else {
        deltaEl.style.color = '#c084fc';
      }
    }
    if (badgeEl) {
      if (Math.abs(delta0) <= 0.3) {
        badgeEl.textContent = 'ON TARGET';
        badgeEl.className = 'delta-col-badge badge-locked';
      } else if (delta0 < 0) {
        badgeEl.textContent = 'BELOW';
        badgeEl.className = 'delta-col-badge badge-below';
      } else {
        badgeEl.textContent = 'ABOVE';
        badgeEl.className = 'delta-col-badge badge-above';
      }
    }
  }
  if (data.pwm1 !== undefined) {
    const p = document.getElementById('pwm1-input');
    if (p && document.activeElement !== p) p.value = data.pwm1;
  }

  // Main Scope (CH1)
  if (data.heater1_enabled !== undefined) {
    heater1Enabled = data.heater1_enabled;
    const btn = document.getElementById('btn-toggle-heater1');
    if (btn) {
      btn.className = heater1Enabled ? 'btn-heater-toggle active' : 'btn-heater-toggle standby';
      btn.textContent = heater1Enabled ? '🔥 PID: ON' : '❄️ OFF (Cooldown)';
    }
    const cdState = document.getElementById('cd-state-ch1');
    if (cdState) {
      cdState.className = heater1Enabled ? 'status-active' : 'status-off';
      cdState.textContent = heater1Enabled ? 'PID ON' : 'OFF (Cooldown)';
    }
  }
  if (data.temp2 !== undefined) {
    const cdTemp = document.getElementById('cd-temp-ch1');
    if (cdTemp) cdTemp.textContent = `${data.temp2.toFixed(1)}°C`;
  }

  if (data.pwr2 !== undefined) {
    if (gaugePwr2) gaugePwr2.value = data.pwr2;
    const el = document.getElementById('metric-pwr2');
    if (el) el.textContent = Math.round(data.pwr2);
  }
  if (data.temp2 !== undefined) {
    if (gaugeTemp2) gaugeTemp2.value = data.temp2;
    if (currentDewPoint !== null) {
      const margin = data.temp2 - currentDewPoint;
      const marginEl = document.getElementById('margin2-display');
      if (marginEl) {
        marginEl.textContent = `${margin >= 0 ? '+' : ''}${margin.toFixed(1)}°C`;
        marginEl.style.color = margin >= 1.5 ? '#10b981' : (margin >= 0.5 ? '#f59e0b' : '#ef4444');
      }
    }
  }
  if (data.offset2 !== undefined) {
    const s = document.getElementById('offset2-slider');
    if (s && document.activeElement !== s) s.value = data.offset2;
    const badge = document.getElementById('offset2-slider-value');
    if (badge) badge.textContent = data.offset2.toFixed(1);
    const cur = document.getElementById('current-offset2-display');
    if (cur) cur.textContent = data.offset2.toFixed(1);
  }
  if (data.setpoint1 !== undefined) {
    const sp = document.getElementById('setpoint2-display');
    if (sp) sp.textContent = data.setpoint1.toFixed(1);
  }
  if (data.temp2 !== undefined && data.setpoint1 !== undefined) {
    const delta1 = data.temp2 - data.setpoint1;
    const deltaEl = document.getElementById('delta-target2');
    const badgeEl = document.getElementById('delta-target2-status');
    if (deltaEl) {
      deltaEl.textContent = `${delta1 >= 0 ? '+' : ''}${delta1.toFixed(1)}°C`;
      if (Math.abs(delta1) <= 0.3) {
        deltaEl.style.color = '#34d399';
      } else if (delta1 < 0) {
        deltaEl.style.color = delta1 >= -1.0 ? '#38bdf8' : '#f59e0b';
      } else {
        deltaEl.style.color = '#c084fc';
      }
    }
    if (badgeEl) {
      if (Math.abs(delta1) <= 0.3) {
        badgeEl.textContent = 'ON TARGET';
        badgeEl.className = 'delta-col-badge badge-locked';
      } else if (delta1 < 0) {
        badgeEl.textContent = 'BELOW';
        badgeEl.className = 'delta-col-badge badge-below';
      } else {
        badgeEl.textContent = 'ABOVE';
        badgeEl.className = 'delta-col-badge badge-above';
      }
    }
  }
  if (data.pwm2 !== undefined) {
    const p = document.getElementById('pwm2-input');
    if (p && document.activeElement !== p) p.value = data.pwm2;
  }

  // Solar & Dimmer
  if (data.solar_elev !== undefined) {
    const elev = document.getElementById('solar-elev-display');
    if (elev) elev.textContent = `${data.solar_elev >= 0 ? '+' : ''}${data.solar_elev.toFixed(1)}°`;
  }
  if (data.is_day !== undefined) {
    const badge = document.getElementById('sun-status-badge');
    if (badge) {
      badge.className = data.is_day ? "badge-day-night badge-day" : "badge-day-night badge-night";
      badge.textContent = data.is_day ? "☀️ Day" : "🌙 Night";
    }
  }
  if (data.auto_dim !== undefined) {
    const toggle = document.getElementById('autodim-toggle');
    if (toggle) toggle.checked = data.auto_dim;
    const label = document.getElementById('autodim-status-label');
    if (label) {
      label.textContent = data.auto_dim ? "Active" : "Disabled";
      label.style.color = data.auto_dim ? "#34d399" : "#94a3b8";
    }
  }
  if (data.brightness !== undefined) {
    const bDisplay = document.getElementById('screen-bright-display');
    if (bDisplay) bDisplay.textContent = data.brightness;
    const bSlider = document.getElementById('brightness-slider');
    if (bSlider && document.activeElement !== bSlider) bSlider.value = data.brightness;
    const bVal = document.getElementById('brightness-slider-value');
    if (bVal) bVal.textContent = data.brightness;
  }

  // WiFi RSSI & Signal Quality
  if (data.rssi !== undefined) {
    const rEl = document.getElementById('wifi-rssi-display');
    if (rEl) rEl.textContent = `${data.rssi} dBm`;
  }
  if (data.wifi_quality !== undefined) {
    const qEl = document.getElementById('wifi-quality-badge');
    if (qEl) {
      qEl.textContent = `${data.wifi_quality}%`;
      if (data.wifi_quality >= 70) {
        qEl.style.color = '#34d399';
        qEl.style.borderColor = 'rgba(52, 211, 153, 0.4)';
      } else if (data.wifi_quality >= 40) {
        qEl.style.color = '#f59e0b';
        qEl.style.borderColor = 'rgba(245, 158, 11, 0.4)';
      } else {
        qEl.style.color = '#ef4444';
        qEl.style.borderColor = 'rgba(239, 68, 68, 0.4)';
      }
    }
  }

  // --- SOFT-LANDING PREDICTIVE GOVERNOR TELEMETRY ---
  if (data.ch0_rate !== undefined) {
    const el = document.getElementById('ch0-rate-display');
    if (el) {
      el.textContent = `${data.ch0_rate >= 0 ? '+' : ''}${data.ch0_rate.toFixed(2)}°C/m`;
      if (data.ch0_rate < -0.1) el.style.color = '#38bdf8';
      else if (data.ch0_rate > 0.1) el.style.color = '#f59e0b';
      else el.style.color = '#94a3b8';
    }
  }
  if (data.ch1_rate !== undefined) {
    const el = document.getElementById('ch1-rate-display');
    if (el) {
      el.textContent = `${data.ch1_rate >= 0 ? '+' : ''}${data.ch1_rate.toFixed(2)}°C/m`;
      if (data.ch1_rate < -0.1) el.style.color = '#38bdf8';
      else if (data.ch1_rate > 0.1) el.style.color = '#f59e0b';
      else el.style.color = '#94a3b8';
    }
  }

  if (data.ch0_landing_active !== undefined) {
    const el = document.getElementById('ch0-softland-status');
    if (el) {
      el.textContent = data.ch0_landing_active ? 'Braking Active' : 'Standby';
      el.style.color = data.ch0_landing_active ? '#34d399' : '#94a3b8';
      el.style.borderColor = data.ch0_landing_active ? 'rgba(52, 211, 153, 0.4)' : 'rgba(255, 255, 255, 0.1)';
    }
  }
  if (data.ch1_landing_active !== undefined) {
    const el = document.getElementById('ch1-softland-status');
    if (el) {
      el.textContent = data.ch1_landing_active ? 'Braking Active' : 'Standby';
      el.style.color = data.ch1_landing_active ? '#34d399' : '#94a3b8';
      el.style.borderColor = data.ch1_landing_active ? 'rgba(52, 211, 153, 0.4)' : 'rgba(255, 255, 255, 0.1)';
    }
  }

  if (data.softland_enabled !== undefined) {
    const toggle = document.getElementById('softland-toggle');
    if (toggle && document.activeElement !== toggle) toggle.checked = data.softland_enabled;
  }
  if (data.softland_agg0 !== undefined) {
    const act0 = document.getElementById('softland-agg0-active');
    if (act0) act0.textContent = parseFloat(data.softland_agg0).toFixed(1);
    if (!softlandAgg0UserEditing) {
      const s0 = document.getElementById('softland-agg0-slider');
      if (s0 && document.activeElement !== s0) s0.value = data.softland_agg0;
      const v0 = document.getElementById('softland-agg0-val');
      if (v0) v0.textContent = parseFloat(data.softland_agg0).toFixed(1);
    }
  }
  if (data.softland_agg1 !== undefined) {
    const act1 = document.getElementById('softland-agg1-active');
    if (act1) act1.textContent = parseFloat(data.softland_agg1).toFixed(1);
    if (!softlandAgg1UserEditing) {
      const s1 = document.getElementById('softland-agg1-slider');
      if (s1 && document.activeElement !== s1) s1.value = data.softland_agg1;
      const v1 = document.getElementById('softland-agg1-val');
      if (v1) v1.textContent = parseFloat(data.softland_agg1).toFixed(1);
    }
  }

  // --- ADAPTIVE SELF-LEARNING THERMAL ENGINE TELEMETRY ---
  if (data.learning_enabled !== undefined) {
    const masterToggle = document.getElementById('learning-master-toggle');
    if (masterToggle && document.activeElement !== masterToggle) masterToggle.checked = data.learning_enabled;
  }
  if (data.learning_speed !== undefined) {
    const spdSel = document.getElementById('learning-speed-select');
    if (spdSel && document.activeElement !== spdSel) spdSel.value = data.learning_speed;
  }

  // CH0 Learned Parameters
  if (data.kloss0 !== undefined) {
    const el = document.getElementById('ch0-learned-kloss');
    if (el) el.textContent = parseFloat(data.kloss0).toFixed(1);
  }
  if (data.lead0 !== undefined) {
    const el = document.getElementById('ch0-learned-lead');
    if (el) el.textContent = parseFloat(data.lead0).toFixed(1);
  }
  if (data.cycles0 !== undefined) {
    const el = document.getElementById('ch0-learned-cycles');
    if (el) el.textContent = data.cycles0;
  }
  if (data.lasterr0 !== undefined) {
    const el = document.getElementById('ch0-learned-lasterr');
    if (el) {
      el.textContent = `${data.lasterr0 >= 0 ? '+' : ''}${parseFloat(data.lasterr0).toFixed(2)}`;
      const wrap = document.getElementById('ch0-learned-lasterr-wrap');
      if (wrap) wrap.style.color = Math.abs(data.lasterr0) <= 0.25 ? '#34d399' : (data.lasterr0 > 0 ? '#38bdf8' : '#f59e0b');
    }
  }
  if (data.conf0 !== undefined) {
    const confVal = Math.round(data.conf0);
    const el = document.getElementById('ch0-learned-conf');
    if (el) el.textContent = `${confVal}%`;
    const bar = document.getElementById('ch0-conf-bar');
    if (bar) bar.style.width = `${confVal}%`;
  }
  if (data.eng_state0 !== undefined) {
    const badge = document.getElementById('ch0-eng-state-badge');
    if (badge) {
      if (data.eng_state0 === 0) {
        badge.textContent = '🟢 Steady Conductance Learning';
        badge.style.color = '#34d399';
        badge.style.borderColor = 'rgba(52, 211, 153, 0.4)';
        badge.style.background = 'rgba(52, 211, 153, 0.12)';
      } else if (data.eng_state0 === 1) {
        badge.textContent = '🔵 Transition Descent Learning';
        badge.style.color = '#38bdf8';
        badge.style.borderColor = 'rgba(56, 189, 248, 0.4)';
        badge.style.background = 'rgba(56, 189, 248, 0.12)';
      } else {
        badge.textContent = '⚪ Standby Tracking';
        badge.style.color = '#94a3b8';
        badge.style.borderColor = 'rgba(255, 255, 255, 0.1)';
        badge.style.background = 'rgba(255, 255, 255, 0.05)';
      }
    }
  }

  // CH1 Learned Parameters
  if (data.kloss1 !== undefined) {
    const el = document.getElementById('ch1-learned-kloss');
    if (el) el.textContent = parseFloat(data.kloss1).toFixed(1);
  }
  if (data.lead1 !== undefined) {
    const el = document.getElementById('ch1-learned-lead');
    if (el) el.textContent = parseFloat(data.lead1).toFixed(1);
  }
  if (data.cycles1 !== undefined) {
    const el = document.getElementById('ch1-learned-cycles');
    if (el) el.textContent = data.cycles1;
  }
  if (data.lasterr1 !== undefined) {
    const el = document.getElementById('ch1-learned-lasterr');
    if (el) {
      el.textContent = `${data.lasterr1 >= 0 ? '+' : ''}${parseFloat(data.lasterr1).toFixed(2)}`;
      const wrap = document.getElementById('ch1-learned-lasterr-wrap');
      if (wrap) wrap.style.color = Math.abs(data.lasterr1) <= 0.25 ? '#34d399' : (data.lasterr1 > 0 ? '#c084fc' : '#f59e0b');
    }
  }
  if (data.conf1 !== undefined) {
    const confVal = Math.round(data.conf1);
    const el = document.getElementById('ch1-learned-conf');
    if (el) el.textContent = `${confVal}%`;
    const bar = document.getElementById('ch1-conf-bar');
    if (bar) bar.style.width = `${confVal}%`;
  }
  if (data.eng_state1 !== undefined) {
    const badge = document.getElementById('ch1-eng-state-badge');
    if (badge) {
      if (data.eng_state1 === 0) {
        badge.textContent = '🟢 Steady Conductance Learning';
        badge.style.color = '#34d399';
        badge.style.borderColor = 'rgba(52, 211, 153, 0.4)';
        badge.style.background = 'rgba(52, 211, 153, 0.12)';
      } else if (data.eng_state1 === 1) {
        badge.textContent = '🔵 Transition Descent Learning';
        badge.style.color = '#c084fc';
        badge.style.borderColor = 'rgba(192, 132, 252, 0.4)';
        badge.style.background = 'rgba(192, 132, 252, 0.12)';
      } else {
        badge.textContent = '⚪ Standby Tracking';
        badge.style.color = '#94a3b8';
        badge.style.borderColor = 'rgba(255, 255, 255, 0.1)';
        badge.style.background = 'rgba(255, 255, 255, 0.05)';
      }
    }
  }

  // Update Dynamic Conductance Chart Curves
  if (data.kloss0 !== undefined || data.kloss1 !== undefined) {
    updateConductanceCurves(data.kloss0, data.kloss1);
  }

  // Track Convergence History when cycle updates
  if (data.cycles0 !== undefined || data.cycles1 !== undefined) {
    updateConvergenceHistory(data.cycles0, data.lasterr0, data.cycles1, data.lasterr1);
  }

  // --- IN-MEMORY ROLLING THERMAL HISTORY (0 FLASH WRITES / ZERO STORAGE WEAR) ---
  const isChartsTabActive = document.getElementById('tab-charts')?.classList.contains('active');

  if (data.input0 !== undefined && data.setpoint0 !== undefined && h0Chart) {
    h0TimeCounter++;
    h0Data.labels.push(h0TimeCounter);
    h0Data.datasets[0].data.push(data.input0);
    h0Data.datasets[1].data.push(data.setpoint0);
    h0Data.datasets[2].data.push(data.pwr1 !== undefined ? data.pwr1 : 0);
    if (h0Data.labels.length > MAX_POINTS) {
      h0Data.labels.shift();
      h0Data.datasets[0].data.shift();
      h0Data.datasets[1].data.shift();
      h0Data.datasets[2].data.shift();
    }
    if (isChartsTabActive) {
      h0Chart.update('none');
    }
  }

  if (data.input1 !== undefined && data.setpoint1 !== undefined && h1Chart) {
    h1TimeCounter++;
    h1Data.labels.push(h1TimeCounter);
    h1Data.datasets[0].data.push(data.input1);
    h1Data.datasets[1].data.push(data.setpoint1);
    h1Data.datasets[2].data.push(data.pwr2 !== undefined ? data.pwr2 : 0);
    if (h1Data.labels.length > MAX_POINTS) {
      h1Data.labels.shift();
      h1Data.datasets[0].data.shift();
      h1Data.datasets[1].data.shift();
      h1Data.datasets[2].data.shift();
    }
    if (isChartsTabActive) {
      h1Chart.update('none');
    }
  }

  // --- LONG-TERM 30-SECOND STABILITY SAMPLING (OVER/UNDERSHOOT ERROR) ---
  const now = Date.now();
  if (now - lastErrorSampleTime >= 30000 || lastErrorSampleTime === 0) {
    lastErrorSampleTime = now;
    const timeLabel = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    let err0 = null;
    if (data.temp1 !== undefined && data.setpoint0 !== undefined) {
      err0 = parseFloat((data.temp1 - data.setpoint0).toFixed(2));
    }
    let err1 = null;
    if (data.temp2 !== undefined && data.setpoint1 !== undefined) {
      err1 = parseFloat((data.temp2 - data.setpoint1).toFixed(2));
    }

    if (h0ErrorChart && err0 !== null) {
      h0ErrorData.labels.push(timeLabel);
      h0ErrorData.datasets[0].data.push(err0);
      h0ErrorData.datasets[1].data.push(0.0);

      if (h0ErrorData.labels.length > MAX_ERROR_POINTS) {
        h0ErrorData.labels.shift();
        h0ErrorData.datasets[0].data.shift();
        h0ErrorData.datasets[1].data.shift();
      }

      if (isChartsTabActive) {
        h0ErrorChart.update('none');
      }
    }

    if (h1ErrorChart && err1 !== null) {
      h1ErrorData.labels.push(timeLabel);
      h1ErrorData.datasets[0].data.push(err1);
      h1ErrorData.datasets[1].data.push(0.0);

      if (h1ErrorData.labels.length > MAX_ERROR_POINTS) {
        h1ErrorData.labels.shift();
        h1ErrorData.datasets[0].data.shift();
        h1ErrorData.datasets[1].data.shift();
      }

      if (isChartsTabActive) {
        h1ErrorChart.update('none');
      }
    }
  }
}



function getReadings() {
  fetch('/readings')
    .then(r => r.json())
    .then(data => applyReadings(data))
    .catch(e => console.warn("Failed to poll readings:", e));
}

// --- SLIDER CONTROLS & ENDPOINTS ---
function initSliders() {
  const s1 = document.getElementById("offset1-slider");
  if (s1) {
    s1.addEventListener('input', function() {
      const val = parseFloat(this.value);
      document.getElementById("offset1-slider-value").textContent = val.toFixed(1);
      fetch(`/set_offset?out=1&val=${val}`);
    });
  }

  const s2 = document.getElementById("offset2-slider");
  if (s2) {
    s2.addEventListener('input', function() {
      const val = parseFloat(this.value);
      document.getElementById("offset2-slider-value").textContent = val.toFixed(1);
      fetch(`/set_offset?out=2&val=${val}`);
    });
  }
}

function setPWM(outputNum) {
  const inputId = `pwm${outputNum}-input`;
  const val = document.getElementById(inputId).value;
  fetch(`/set_pwm?out=${outputNum}&val=${encodeURIComponent(val)}`);
}

function toggleHeater(heater) {
  const currentState = (heater === 0) ? heater0Enabled : heater1Enabled;
  const newState = !currentState;
  if (heater === 0) heater0Enabled = newState;
  else heater1Enabled = newState;

  // Immediate optimistic UI update
  const btn = document.getElementById(`btn-toggle-heater${heater}`);
  if (btn) {
    btn.className = newState ? 'btn-heater-toggle active' : 'btn-heater-toggle standby';
    btn.textContent = newState ? '🔥 PID: ON' : '❄️ OFF (Cooldown)';
  }
  const cdState = document.getElementById(`cd-state-ch${heater}`);
  if (cdState) {
    cdState.className = newState ? 'status-active' : 'status-off';
    cdState.textContent = newState ? 'PID ON' : 'OFF (Cooldown)';
  }

  fetch(`/set_heater?heater=${heater}&state=${newState ? 1 : 0}`)
    .catch(err => console.error("Failed to toggle heater:", err));
}

function setAllHeaters(enabled) {
  heater0Enabled = enabled;
  heater1Enabled = enabled;

  [0, 1].forEach(h => {
    const btn = document.getElementById(`btn-toggle-heater${h}`);
    if (btn) {
      btn.className = enabled ? 'btn-heater-toggle active' : 'btn-heater-toggle standby';
      btn.textContent = enabled ? '🔥 PID: ON' : '❄️ OFF (Cooldown)';
    }
    const cdState = document.getElementById(`cd-state-ch${h}`);
    if (cdState) {
      cdState.className = enabled ? 'status-active' : 'status-off';
      cdState.textContent = enabled ? 'PID ON' : 'OFF (Cooldown)';
    }
  });

  fetch(`/set_heater?heater=2&state=${enabled ? 1 : 0}`)
    .catch(err => console.error("Failed to set all heaters:", err));
}

function initBrightnessSlider() {
  const slider = document.getElementById("brightness-slider");
  if (slider) {
    slider.addEventListener('input', function() {
      const val = parseInt(this.value);
      document.getElementById("brightness-slider-value").textContent = val;
    });
    slider.addEventListener('change', function() {
      const val = parseInt(this.value);
      fetch(`/set_brightness?val=${val}`);
    });
  }

  const daySlider = document.getElementById("day-bright-slider");
  if (daySlider) {
    daySlider.addEventListener('input', function() {
      const val = parseInt(this.value);
      const valDisplay = document.getElementById("day-bright-val");
      if (valDisplay) valDisplay.textContent = val;
    });
  }

  const nightSlider = document.getElementById("night-bright-slider");
  if (nightSlider) {
    nightSlider.addEventListener('input', function() {
      const val = parseInt(this.value);
      const valDisplay = document.getElementById("night-bright-val");
      if (valDisplay) valDisplay.textContent = val;
    });
  }
}

function saveDimLevels() {
  const dayB = parseInt(document.getElementById('day-bright-slider').value) || 200;
  const nightB = parseInt(document.getElementById('night-bright-slider').value) || 20;
  const msg = document.getElementById('dim-save-msg');

  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = 'Saving dimming levels to flash...';
  }

  fetch(`/set_dim_levels?day=${dayB}&night=${nightB}`)
    .then(r => r.json())
    .then(data => {
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `✅ Saved! Day: ${data.day_bright}/255, Night: ${data.night_bright}/255`;
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    })
    .catch(() => {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to save dimming levels!';
      }
    });
}

function toggleAutoDim() {
  const toggle = document.getElementById('autodim-toggle');
  const enabled = toggle ? toggle.checked : true;
  const label = document.getElementById('autodim-status-label');
  if (label) {
    label.textContent = enabled ? "Active" : "Disabled";
    label.style.color = enabled ? "#34d399" : "#94a3b8";
  }
  fetch(`/set_autodim?val=${enabled ? 1 : 0}`);
}


// --- PID SETTINGS ---
function getPID() {
  fetch('/api/pid')
    .then(r => r.json())
    .then(data => {
      if (data.H0) {
        const h0kp = document.getElementById('h0-kp');
        const h0ki = document.getElementById('h0-ki');
        const h0kd = document.getElementById('h0-kd');
        if (h0kp && document.activeElement !== h0kp) h0kp.value = parseFloat(data.H0.Kp).toFixed(3);
        if (h0ki && document.activeElement !== h0ki) h0ki.value = parseFloat(data.H0.Ki).toFixed(3);
        if (h0kd && document.activeElement !== h0kd) h0kd.value = parseFloat(data.H0.Kd).toFixed(3);
      }
      if (data.H1) {
        const h1kp = document.getElementById('h1-kp');
        const h1ki = document.getElementById('h1-ki');
        const h1kd = document.getElementById('h1-kd');
        if (h1kp && document.activeElement !== h1kp) h1kp.value = parseFloat(data.H1.Kp).toFixed(3);
        if (h1ki && document.activeElement !== h1ki) h1ki.value = parseFloat(data.H1.Ki).toFixed(3);
        if (h1kd && document.activeElement !== h1kd) h1kd.value = parseFloat(data.H1.Kd).toFixed(3);
      }
    })
    .catch(e => console.warn('Failed to load PID:', e));
}

function setPID(heater) {
  const prefix = heater === 0 ? 'h0' : 'h1';
  const kp = parseFloat(document.getElementById(`${prefix}-kp`).value);
  const ki = parseFloat(document.getElementById(`${prefix}-ki`).value);
  const kd = parseFloat(document.getElementById(`${prefix}-kd`).value);
  const msg = document.getElementById('pid-save-msg');

  if (isNaN(kp) || isNaN(ki) || isNaN(kd)) {
    if (msg) {
      msg.style.color = '#ef4444';
      msg.textContent = '❌ Please enter valid numerical values for Kp, Ki, and Kd.';
    }
    return;
  }

  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = `Saving Heater ${heater} PID parameters...`;
  }

  fetch(`/set_pid?heater=${heater}&kp=${encodeURIComponent(kp)}&ki=${encodeURIComponent(ki)}&kd=${encodeURIComponent(kd)}`)
    .then(r => r.json())
    .then(() => {
      getPID();
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `✅ Saved & active on Heater ${heater}! (Kp: ${kp.toFixed(3)}, Ki: ${ki.toFixed(3)}, Kd: ${kd.toFixed(3)})`;
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    })
    .catch(e => {
      console.error('Failed to set PID:', e);
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = `❌ Failed to save Heater ${heater} PID!`;
      }
    });
}

// --- DEFAULT SETPOINT OFFSETS ---
function getDefaultOffsets() {
  fetch('/api/offsets')
    .then(r => r.json())
    .then(data => {
      const off0 = document.getElementById('def-offset0');
      const off1 = document.getElementById('def-offset1');
      if (off0 && document.activeElement !== off0 && data.offset0 !== undefined) {
        off0.value = parseFloat(data.offset0).toFixed(1);
      }
      if (off1 && document.activeElement !== off1 && data.offset1 !== undefined) {
        off1.value = parseFloat(data.offset1).toFixed(1);
      }
    })
    .catch(e => console.warn('Failed to load default offsets:', e));
}

function saveDefaultOffsets() {
  const off0 = parseFloat(document.getElementById('def-offset0').value);
  const off1 = parseFloat(document.getElementById('def-offset1').value);
  const msg = document.getElementById('offset-save-msg');

  if (isNaN(off0) || isNaN(off1)) {
    if (msg) {
      msg.style.color = '#ef4444';
      msg.textContent = '❌ Please enter valid numerical offset values.';
    }
    return;
  }

  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = 'Saving default offsets to flash...';
  }

  fetch(`/set_default_offsets?offset0=${encodeURIComponent(off0)}&offset1=${encodeURIComponent(off1)}`)
    .then(r => r.json())
    .then(data => {
      getDefaultOffsets();
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `✅ Saved! H0: ${parseFloat(data.offset0).toFixed(1)}°C, H1: ${parseFloat(data.offset1).toFixed(1)}°C`;
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    })
    .catch(e => {
      console.error('Failed to save default offsets:', e);
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to save default offsets!';
      }
    });
}

// --- THERMAL CHARTS (CHART.JS DARK THEME) ---
let h0Chart = null, h1Chart = null;
let h0TimeCounter = 0, h1TimeCounter = 0;
const MAX_POINTS = 60;

const commonChartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  animation: false,
  plugins: {
    legend: {
      position: 'top',
      labels: {
        color: '#94a3b8',
        font: { family: "'Inter', sans-serif", size: 11 }
      }
    }
  },
  scales: {
    y: {
      type: 'linear',
      display: true,
      position: 'left',
      grid: { color: 'rgba(255, 255, 255, 0.05)' },
      ticks: { color: '#64748b', font: { family: "'JetBrains Mono', monospace" } },
      title: { display: true, text: '°C', color: '#64748b' }
    },
    y1: {
      type: 'linear',
      display: true,
      position: 'right',
      min: 0,
      max: 100,
      grid: { drawOnChartArea: false },
      ticks: {
        color: '#f59e0b',
        font: { family: "'JetBrains Mono', monospace" },
        stepSize: 25,
        callback: function(val) { return val + '%'; }
      },
      title: { display: true, text: 'Power (%)', color: '#f59e0b' }
    },
    x: {
      grid: { color: 'rgba(255, 255, 255, 0.05)' },
      ticks: { color: '#64748b', font: { family: "'JetBrains Mono', monospace" } }
    }
  }
};

const h0Data = {
  labels: [],
  datasets: [
    { label: 'Guide Temp', data: [], borderColor: '#00d2ff', backgroundColor: 'rgba(0, 210, 255, 0.1)', borderWidth: 2, fill: true, tension: 0.3, pointRadius: 0, yAxisID: 'y' },
    { label: 'Target Setpoint', data: [], borderColor: '#34d399', borderDash: [4, 4], borderWidth: 1.5, fill: false, tension: 0, pointRadius: 0, yAxisID: 'y' },
    { label: 'Heater Power (%)', data: [], borderColor: '#f59e0b', backgroundColor: 'rgba(245, 158, 11, 0.08)', borderWidth: 1.5, fill: true, tension: 0.2, pointRadius: 0, yAxisID: 'y1' }
  ]
};

const h1Data = {
  labels: [],
  datasets: [
    { label: 'Main Temp', data: [], borderColor: '#c084fc', backgroundColor: 'rgba(192, 132, 252, 0.1)', borderWidth: 2, fill: true, tension: 0.3, pointRadius: 0, yAxisID: 'y' },
    { label: 'Target Setpoint', data: [], borderColor: '#34d399', borderDash: [4, 4], borderWidth: 1.5, fill: false, tension: 0, pointRadius: 0, yAxisID: 'y' },
    { label: 'Heater Power (%)', data: [], borderColor: '#f59e0b', backgroundColor: 'rgba(245, 158, 11, 0.08)', borderWidth: 1.5, fill: true, tension: 0.2, pointRadius: 0, yAxisID: 'y1' }
  ]
};

function initH0Chart() {
  if (typeof Chart === 'undefined') return;
  const ctx = document.getElementById('h0-chart');
  if (ctx) h0Chart = new Chart(ctx, { type: 'line', data: h0Data, options: commonChartOptions });
}

function initH1Chart() {
  if (typeof Chart === 'undefined') return;
  const ctx = document.getElementById('h1-chart');
  if (ctx) h1Chart = new Chart(ctx, { type: 'line', data: h1Data, options: commonChartOptions });
}

// --- LONG-TERM PID TRACKING ERROR & STABILITY CHARTS (CH0 & CH1) ---
let h0ErrorChart = null, h1ErrorChart = null;
let lastErrorSampleTime = 0;
const MAX_ERROR_POINTS = 120; // 120 samples @ 30s = 60 minutes history

function createErrorChartOptions(titleText) {
  return {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,
    plugins: {
      legend: {
        position: 'top',
        labels: {
          color: '#94a3b8',
          font: { family: "'Inter', sans-serif", size: 11 }
        }
      },
      tooltip: {
        callbacks: {
          label: function(context) {
            const val = context.parsed.y;
            if (val === null || val === undefined) return '';
            const prefix = val > 0 ? '+' : '';
            const status = Math.abs(val) <= 0.3 ? ' (On Target)' : (val > 0 ? ' (Overshoot)' : ' (Undershoot)');
            return `${context.dataset.label}: ${prefix}${val.toFixed(2)}°C${status}`;
          }
        }
      }
    },
    scales: {
      y: {
        type: 'linear',
        display: true,
        position: 'left',
        grid: {
          color: function(context) {
            if (context.tick && context.tick.value === 0) {
              return 'rgba(52, 211, 153, 0.5)'; // Distinct zero reference line
            }
            return 'rgba(255, 255, 255, 0.05)';
          },
          lineWidth: function(context) {
            return (context.tick && context.tick.value === 0) ? 2 : 1;
          }
        },
        ticks: {
          color: '#94a3b8',
          font: { family: "'JetBrains Mono', monospace" },
          stepSize: 0.5,
          callback: function(val) {
            return (val > 0 ? '+' : '') + val.toFixed(1) + '°C';
          }
        },
        title: {
          display: true,
          text: titleText || 'Δ Error (°C)',
          color: '#94a3b8'
        }
      },
      x: {
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: {
          color: '#64748b',
          font: { family: "'JetBrains Mono', monospace", size: 10 },
          maxRotation: 0,
          autoSkip: true,
          maxTicksLimit: 6
        },
        title: { display: true, text: 'Time (30s Samples)', color: '#64748b' }
      }
    }
  };
}

const h0ErrorData = {
  labels: [],
  datasets: [
    {
      label: 'CH0 Guide Error (Δ°C)',
      data: [],
      borderColor: '#00d2ff',
      backgroundColor: 'rgba(0, 210, 255, 0.08)',
      borderWidth: 2,
      pointRadius: 2,
      pointHoverRadius: 5,
      tension: 0.3,
      fill: false
    },
    {
      label: 'Target Lock (0.0°C)',
      data: [],
      borderColor: '#34d399',
      borderDash: [5, 5],
      borderWidth: 1.5,
      pointRadius: 0,
      pointHoverRadius: 0,
      tension: 0,
      fill: false
    }
  ]
};

const h1ErrorData = {
  labels: [],
  datasets: [
    {
      label: 'CH1 Main Error (Δ°C)',
      data: [],
      borderColor: '#c084fc',
      backgroundColor: 'rgba(192, 132, 252, 0.08)',
      borderWidth: 2,
      pointRadius: 2,
      pointHoverRadius: 5,
      tension: 0.3,
      fill: false
    },
    {
      label: 'Target Lock (0.0°C)',
      data: [],
      borderColor: '#34d399',
      borderDash: [5, 5],
      borderWidth: 1.5,
      pointRadius: 0,
      pointHoverRadius: 0,
      tension: 0,
      fill: false
    }
  ]
};

function initH0ErrorChart() {
  if (typeof Chart === 'undefined') return;
  const ctx = document.getElementById('h0-error-chart');
  if (ctx) h0ErrorChart = new Chart(ctx, { type: 'line', data: h0ErrorData, options: createErrorChartOptions('CH0 Error Δ [ +Over / -Under ] (°C)') });
}

function initH1ErrorChart() {
  if (typeof Chart === 'undefined') return;
  const ctx = document.getElementById('h1-error-chart');
  if (ctx) h1ErrorChart = new Chart(ctx, { type: 'line', data: h1ErrorData, options: createErrorChartOptions('CH1 Error Δ [ +Over / -Under ] (°C)') });
}

// --- PID AUTO-TUNER CONTROLLER & STEP-RESPONSE CHART ---
let tunerChart = null;
let tunerTimeCounter = 0;
const tunerData = {
  labels: [],
  datasets: [
    {
      label: 'Optics Temperature (°C)',
      data: [],
      borderColor: '#38ef7d',
      backgroundColor: 'rgba(56, 239, 125, 0.1)',
      borderWidth: 2.5,
      yAxisID: 'y',
      tension: 0.2,
      pointRadius: 0
    },
    {
      label: 'Output Step (PWM)',
      data: [],
      borderColor: '#f59e0b',
      borderDash: [5, 5],
      borderWidth: 1.5,
      yAxisID: 'y1',
      tension: 0,
      pointRadius: 0
    }
  ]
};

const tunerChartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  animation: false,
  plugins: {
    legend: {
      position: 'top',
      labels: {
        color: '#94a3b8',
        font: { family: "'Inter', sans-serif", size: 11 }
      }
    }
  },
  scales: {
    y: {
      type: 'linear',
      display: true,
      position: 'left',
      grid: { color: 'rgba(255, 255, 255, 0.05)' },
      ticks: { color: '#38ef7d', font: { family: "'JetBrains Mono', monospace" } },
      title: { display: true, text: 'Temperature (°C)', color: '#38ef7d' }
    },
    y1: {
      type: 'linear',
      display: true,
      position: 'right',
      min: 0,
      max: 255,
      grid: { drawOnChartArea: false },
      ticks: { color: '#f59e0b', font: { family: "'JetBrains Mono', monospace" } },
      title: { display: true, text: 'PWM Output (0-255)', color: '#f59e0b' }
    },
    x: {
      grid: { color: 'rgba(255, 255, 255, 0.05)' },
      ticks: { color: '#64748b', font: { family: "'JetBrains Mono', monospace" } },
      title: { display: true, text: 'Time', color: '#64748b' }
    }
  }
};

function initTunerChart() {
  if (typeof Chart === 'undefined') return;
  const ctx = document.getElementById('tuner-chart');
  if (ctx) {
    tunerChart = new Chart(ctx, { type: 'line', data: tunerData, options: tunerChartOptions });
  }
}

function resetTunerChart() {
  tunerTimeCounter = 0;
  tunerData.labels = [];
  tunerData.datasets[0].data = [];
  tunerData.datasets[1].data = [];
  if (tunerChart) tunerChart.update();
}

function updateTunerPowerDisplay() {
  const input = document.getElementById('tuner-step');
  const hint = document.getElementById('tuner-step-hint');
  if (input && hint) {
    let val = parseInt(input.value);
    if (isNaN(val)) val = 80;
    if (val < 10) val = 10;
    if (val > 100) val = 100;
    const pwm = Math.round((val / 100) * 255);
    hint.textContent = `${val}% (PWM ${pwm}/255)`;
  }
}

function updateTunerChannel() {
  const heater = document.getElementById('tuner-target').value;
  const durInput = document.getElementById('tuner-duration');
  if (durInput) {
    durInput.value = (heater === "1") ? 240 : 120;
  }
  const settleInput = document.getElementById('tuner-settle');
  if (settleInput) {
    settleInput.value = 20;
  }
  getPID();
}

function startTuner() {
  const heater = document.getElementById('tuner-target').value;
  let powerPct = parseInt(document.getElementById('tuner-step').value);
  if (isNaN(powerPct)) powerPct = 80;
  if (powerPct < 10) powerPct = 10;
  if (powerPct > 100) powerPct = 100;

  const stepPWM = Math.min(255, Math.max(10, Math.round((powerPct / 100) * 255)));
  const duration = document.getElementById('tuner-duration').value;
  const settle = document.getElementById('tuner-settle').value;
  const rule = document.getElementById('tuner-rule').value;

  if (!confirm(`Start PID Auto-Tuning on Heater CH${heater}?\n\nTest Power: ${powerPct}% (PWM ${stepPWM}/255)\nTest Duration: ${duration}s\nSettle Time: ${settle}s`)) {
    return;
  }

  resetTunerChart();
  const resCard = document.getElementById('tuner-results-card');
  if (resCard) resCard.style.display = 'none';
  const fb = document.getElementById('tuner-action-feedback');
  if (fb) fb.textContent = '';

  fetch(`/api/tuning/start?heater=${heater}&step=${stepPWM}&testTime=${duration}&settleTime=${settle}&rule=${rule}`)
    .then(r => r.json())
    .then(data => {
      pollTuningStatus();
    })
    .catch(err => {
      alert("Failed to start tuner: " + err);
    });
}

function stopTuner() {
  if (confirm("Abort active PID tuning run?")) {
    fetch('/api/tuning/stop')
      .then(r => r.json())
      .then(() => pollTuningStatus());
  }
}

function applyTuningChoice(saveToFlash) {
  const heater = document.getElementById('tuner-target').value;
  const msg = document.getElementById('tuner-action-feedback');
  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = saveToFlash ? 'Saving new tunings to flash...' : 'Applying new tunings to RAM...';
  }

  fetch(`/api/tuning/apply?heater=${heater}&save=${saveToFlash ? 1 : 0}`)
    .then(r => r.json())
    .then(data => {
      if (msg) {
        msg.style.color = '#38ef7d';
        msg.textContent = `✅ ${data.msg || 'Tunings applied successfully!'}`;
      }
      getPID();
      setTimeout(() => {
        const rc = document.getElementById('tuner-results-card');
        if (rc) rc.style.display = 'none';
      }, 4000);
    })
    .catch(err => {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to apply tunings!';
      }
    });
}

function discardTuningChoice() {
  const heater = document.getElementById('tuner-target').value;
  const msg = document.getElementById('tuner-action-feedback');
  fetch(`/api/tuning/discard?heater=${heater}`)
    .then(r => r.json())
    .then(data => {
      if (msg) {
        msg.style.color = '#e2e8f0';
        msg.textContent = 'Original settings restored.';
      }
      setTimeout(() => {
        const rc = document.getElementById('tuner-results-card');
        if (rc) rc.style.display = 'none';
      }, 2000);
    });
}

function pollTuningStatus() {
  fetch('/api/tuning/status')
    .then(r => r.json())
    .then(data => {
      const stateBadge = document.getElementById('tuner-state-badge');
      const startBtn = document.getElementById('btn-start-tune');
      const stopBtn = document.getElementById('btn-stop-tune');
      const progressBar = document.getElementById('tuner-progress-bar');
      const timeLbl = document.getElementById('tuner-time-label');
      const pctLbl = document.getElementById('tuner-percent-label');
      const msgDisp = document.getElementById('tuner-msg-display');

      if (msgDisp) msgDisp.textContent = data.msg || '';
      if (progressBar) progressBar.style.width = `${data.progress || 0}%`;
      if (pctLbl) pctLbl.textContent = `${Math.round(data.progress || 0)}%`;
      if (timeLbl) timeLbl.textContent = `${data.elapsed || 0}s / ${data.total || 0}s`;

      const stateNames = ['IDLE', 'SETTLING BASELINE', 'TESTING (STEP ACTIVE)', 'COMPLETED', 'ABORTED', 'ERROR'];
      const stateName = stateNames[data.state] || 'IDLE';

      if (stateBadge) {
        stateBadge.textContent = stateName;
        if (data.state === 1 || data.state === 2) {
          stateBadge.style.color = '#f59e0b';
        } else if (data.state === 3) {
          stateBadge.style.color = '#38ef7d';
        } else if (data.state === 4 || data.state === 5) {
          stateBadge.style.color = '#ef4444';
        } else {
          stateBadge.style.color = '#94a3b8';
        }
      }

      if (data.state === 1 || data.state === 2) {
        if (startBtn) startBtn.disabled = true;
        if (stopBtn) stopBtn.disabled = false;
        
        // Append to tuner chart in real time
        tunerTimeCounter = data.elapsed || (tunerTimeCounter + 1);
        tunerData.labels.push(tunerTimeCounter + 's');
        tunerData.datasets[0].data.push(data.temp || 0);
        tunerData.datasets[1].data.push(data.output || 0);
        if (tunerData.labels.length > 700) {
          tunerData.labels.shift();
          tunerData.datasets[0].data.shift();
          tunerData.datasets[1].data.shift();
        }
        if (tunerChart) tunerChart.update('none');
      } else {
        if (startBtn) startBtn.disabled = false;
        if (stopBtn) stopBtn.disabled = true;
      }

      if (data.state === 3) {
        // Complete! Populate results
        const resCard = document.getElementById('tuner-results-card');
        if (resCard && resCard.style.display !== 'block') {
          resCard.style.display = 'block';
        }
        
        const g = document.getElementById('res-gain');
        if (g) g.textContent = data.gain ? data.gain.toFixed(3) : '--';
        const dt = document.getElementById('res-deadtime');
        if (dt) dt.textContent = data.deadtime ? data.deadtime.toFixed(1) + 's' : '--s';
        const tau = document.getElementById('res-tau');
        if (tau) tau.textContent = data.tau ? data.tau.toFixed(1) + 's' : '--s';

        const okp = document.getElementById('table-orig-kp');
        if (okp) okp.textContent = data.orig_kp ? data.orig_kp.toFixed(3) : '--';
        const oki = document.getElementById('table-orig-ki');
        if (oki) oki.textContent = data.orig_ki ? data.orig_ki.toFixed(3) : '--';
        const okd = document.getElementById('table-orig-kd');
        if (okd) okd.textContent = data.orig_kd ? data.orig_kd.toFixed(3) : '--';

        const ckp = document.getElementById('table-calc-kp');
        if (ckp) ckp.textContent = data.calc_kp ? data.calc_kp.toFixed(3) : '--';
        const cki = document.getElementById('table-calc-ki');
        if (cki) cki.textContent = data.calc_ki ? data.calc_ki.toFixed(3) : '--';
        const ckd = document.getElementById('table-calc-kd');
        if (ckd) ckd.textContent = data.calc_kd ? data.calc_kd.toFixed(3) : '--';

        if (data.calc_kp !== undefined && data.orig_kp !== undefined) {
          const diffKp = data.calc_kp - data.orig_kp;
          const dkp = document.getElementById('table-diff-kp');
          if (dkp) dkp.textContent = `${diffKp >= 0 ? '+' : ''}${diffKp.toFixed(3)}`;
        }
        if (data.calc_ki !== undefined && data.orig_ki !== undefined) {
          const diffKi = data.calc_ki - data.orig_ki;
          const dki = document.getElementById('table-diff-ki');
          if (dki) dki.textContent = `${diffKi >= 0 ? '+' : ''}${diffKi.toFixed(3)}`;
        }
        if (data.calc_kd !== undefined && data.orig_kd !== undefined) {
          const diffKd = data.calc_kd - data.orig_kd;
          const dkd = document.getElementById('table-diff-kd');
          if (dkd) dkd.textContent = `${diffKd >= 0 ? '+' : ''}${diffKd.toFixed(3)}`;
        }
      }
    })
    .catch(() => {});
}

// --- LEAFLET MAP & LOCATION ---
let locationMap = null;
let locationMarker = null;

function handleLocationToggle(details) {
  if (details && details.open) {
    if (!locationMap) {
      initMap();
    }
    setTimeout(() => {
      if (locationMap) locationMap.invalidateSize();
    }, 200);
  }
}

function initMap() {
  fetch('/api/location')
    .then(r => r.json())
    .then(data => {
      const lat = (data.lat !== undefined && !isNaN(data.lat)) ? data.lat : 51.5074;
      const lon = (data.lon !== undefined && !isNaN(data.lon)) ? data.lon : -0.1278;
      
      const latInput = document.getElementById('loc-lat');
      const lonInput = document.getElementById('loc-lon');
      if (latInput) latInput.value = lat.toFixed(4);
      if (lonInput) lonInput.value = lon.toFixed(4);

      if (data.day_bright !== undefined) {
        const daySlider = document.getElementById('day-bright-slider');
        if (daySlider) daySlider.value = data.day_bright;
        const dayVal = document.getElementById('day-bright-val');
        if (dayVal) dayVal.textContent = data.day_bright;
      }
      if (data.night_bright !== undefined) {
        const nightSlider = document.getElementById('night-bright-slider');
        if (nightSlider) nightSlider.value = data.night_bright;
        const nightVal = document.getElementById('night-bright-val');
        if (nightVal) nightVal.textContent = data.night_bright;
      }

      if (!locationMap && document.getElementById('location-map') && typeof L !== 'undefined') {
        locationMap = L.map('location-map').setView([lat, lon], 11);
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
          maxZoom: 19,
          attribution: '© OpenStreetMap'
        }).addTo(locationMap);

        locationMarker = L.marker([lat, lon], { draggable: true }).addTo(locationMap);

        locationMarker.on('dragend', function () {
          const pos = locationMarker.getLatLng();
          if (latInput) latInput.value = pos.lat.toFixed(4);
          if (lonInput) lonInput.value = pos.lng.toFixed(4);
        });

        locationMap.on('click', function (e) {
          locationMarker.setLatLng(e.latlng);
          if (latInput) latInput.value = e.latlng.lat.toFixed(4);
          if (lonInput) lonInput.value = e.latlng.lng.toFixed(4);
        });
      }
    })
    .catch(() => {});
}

function saveLocation() {
  const lat = parseFloat(document.getElementById('loc-lat').value);
  const lon = parseFloat(document.getElementById('loc-lon').value);
  const msg = document.getElementById('loc-save-msg');

  if (isNaN(lat) || isNaN(lon)) {
    if (msg) {
      msg.style.color = '#ef4444';
      msg.textContent = '❌ Please enter valid numerical coordinates!';
    }
    return;
  }

  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = 'Saving to device flash...';
  }

  fetch(`/set_location?lat=${encodeURIComponent(lat)}&lon=${encodeURIComponent(lon)}`)
    .then(r => r.json())
    .then(() => {
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `✅ Saved! Lat: ${lat.toFixed(4)}, Lon: ${lon.toFixed(4)}`;
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    })
    .catch(() => {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to save location!';
      }
    });
}

function locateMe() {
  const msg = document.getElementById('loc-save-msg');
  if (!navigator.geolocation) {
    alert("Geolocation is not supported by your browser");
    return;
  }
  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = 'Acquiring GPS location...';
  }

  navigator.geolocation.getCurrentPosition(
    function(pos) {
      const lat = pos.coords.latitude;
      const lon = pos.coords.longitude;
      document.getElementById('loc-lat').value = lat.toFixed(4);
      document.getElementById('loc-lon').value = lon.toFixed(4);

      if (locationMarker && locationMap) {
        locationMarker.setLatLng([lat, lon]);
        locationMap.setView([lat, lon], 13);
      }
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = '📍 Location found! Click "Save to Flash" to store.';
      }
    },
    function() {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Location access denied or unavailable.';
      }
    }
  );
}

// --- SSE EVENT STREAM ---
if (!!window.EventSource) {
  const source = new EventSource('/events');
  source.addEventListener('new_readings', function(e) {
    try {
      const data = JSON.parse(e.data);
      applyReadings(data);
    } catch (err) {
      console.error("SSE parse error:", err);
    }
  }, false);

  source.addEventListener('dim_levels_update', function(e) {
    try {
      const data = JSON.parse(e.data);
      if (data.day_bright !== undefined) {
        const d = document.getElementById('day-bright-slider');
        if (d && document.activeElement !== d) d.value = data.day_bright;
        const dv = document.getElementById('day-bright-val');
        if (dv) dv.textContent = data.day_bright;
      }
      if (data.night_bright !== undefined) {
        const n = document.getElementById('night-bright-slider');
        if (n && document.activeElement !== n) n.value = data.night_bright;
        const nv = document.getElementById('night-bright-val');
        if (nv) nv.textContent = data.night_bright;
      }
    } catch (err) {}
  }, false);

  source.addEventListener('brightness_update', function(e) {
    try {
      const data = JSON.parse(e.data);
      if (data.brightness !== undefined) {
        const bDisplay = document.getElementById('screen-bright-display');
        if (bDisplay) bDisplay.textContent = data.brightness;
        const bSlider = document.getElementById('brightness-slider');
        if (bSlider && document.activeElement !== bSlider) bSlider.value = data.brightness;
        const bVal = document.getElementById('brightness-slider-value');
        if (bVal) bVal.textContent = data.brightness;
      }
      if (data.auto_dim !== undefined) {
        const toggle = document.getElementById('autodim-toggle');
        if (toggle) toggle.checked = data.auto_dim;
        const label = document.getElementById('autodim-status-label');
        if (label) {
          label.textContent = data.auto_dim ? "Active" : "Disabled";
          label.style.color = data.auto_dim ? "#34d399" : "#94a3b8";
        }
      }
    } catch (err) {}
  }, false);

  source.addEventListener('heater_state', function(e) {
    try {
      const data = JSON.parse(e.data);
      applyReadings(data);
    } catch (err) {}
  }, false);

  source.addEventListener('offsets', function(e) {
    try {
      const data = JSON.parse(e.data);
      if (data.offset1 !== undefined) {
        const s = document.getElementById('offset1-slider');
        if (s && document.activeElement !== s) s.value = data.offset1;
        const badge = document.getElementById('offset1-slider-value');
        if (badge) badge.textContent = parseFloat(data.offset1).toFixed(1);
        const cur = document.getElementById('current-offset1-display');
        if (cur) cur.textContent = parseFloat(data.offset1).toFixed(1);
      }
      if (data.offset2 !== undefined) {
        const s = document.getElementById('offset2-slider');
        if (s && document.activeElement !== s) s.value = data.offset2;
        const badge = document.getElementById('offset2-slider-value');
        if (badge) badge.textContent = parseFloat(data.offset2).toFixed(1);
        const cur = document.getElementById('current-offset2-display');
        if (cur) cur.textContent = parseFloat(data.offset2).toFixed(1);
      }
    } catch (err) {}
  }, false);
}



// --- RESET TRIGGER ---
function triggerReset() {
  if (confirm("Restart DewHeater Mk2 Controller?")) {
    fetch('/reset');
  }
}

// --- MQTT LWT AUTOMATION TESTING ---
function toggleLWT() {
  const msg = document.getElementById('lwt-action-msg');
  fetch('/api/lwt/toggle')
    .then(r => r.json())
    .then(data => {
      const badge = document.getElementById('lwt-status-badge');
      if (badge && data.lwt_online !== undefined) {
        badge.textContent = data.lwt_online ? 'Online' : 'Offline';
        badge.style.background = data.lwt_online ? 'rgba(16, 185, 129, 0.2)' : 'rgba(239, 68, 68, 0.2)';
        badge.style.color = data.lwt_online ? '#34d399' : '#fca5a5';
        badge.style.borderColor = data.lwt_online ? 'rgba(52, 211, 153, 0.4)' : 'rgba(239, 68, 68, 0.4)';
      }
      if (msg) {
        msg.style.color = data.lwt_online ? '#34d399' : '#f59e0b';
        msg.textContent = `📡 LWT Broadcast: ${data.lwt_state} (${data.topic})`;
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    })
    .catch(err => {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to toggle LWT';
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    });
}

// --- THERMAL SOFT-LANDING PREDICTIVE GOVERNOR ---
let softlandAgg0UserEditing = false;
let softlandAgg1UserEditing = false;

function onSoftlandAgg0Input(val) {
  softlandAgg0UserEditing = true;
  const v0 = document.getElementById('softland-agg0-val');
  if (v0) v0.textContent = parseFloat(val).toFixed(1);
}

function onSoftlandAgg1Input(val) {
  softlandAgg1UserEditing = true;
  const v1 = document.getElementById('softland-agg1-val');
  if (v1) v1.textContent = parseFloat(val).toFixed(1);
}

function getSoftLandingConfig() {
  fetch('/api/softland')
    .then(r => r.json())
    .then(data => {
      if (data.enabled !== undefined) {
        const toggle = document.getElementById('softland-toggle');
        if (toggle) toggle.checked = data.enabled;
      }
      if (data.agg0 !== undefined) {
        const act0 = document.getElementById('softland-agg0-active');
        if (act0) act0.textContent = parseFloat(data.agg0).toFixed(1);
        if (!softlandAgg0UserEditing) {
          const s0 = document.getElementById('softland-agg0-slider');
          if (s0) s0.value = data.agg0;
          const v0 = document.getElementById('softland-agg0-val');
          if (v0) v0.textContent = parseFloat(data.agg0).toFixed(1);
        }
      }
      if (data.agg1 !== undefined) {
        const act1 = document.getElementById('softland-agg1-active');
        if (act1) act1.textContent = parseFloat(data.agg1).toFixed(1);
        if (!softlandAgg1UserEditing) {
          const s1 = document.getElementById('softland-agg1-slider');
          if (s1) s1.value = data.agg1;
          const v1 = document.getElementById('softland-agg1-val');
          if (v1) v1.textContent = parseFloat(data.agg1).toFixed(1);
        }
      }
    })
    .catch(err => console.warn("Could not fetch softland config:", err));
}

function toggleSoftLandingMaster() {
  saveSoftLandingConfig();
}

function saveSoftLandingConfig() {
  const toggle = document.getElementById('softland-toggle');
  const s0 = document.getElementById('softland-agg0-slider');
  const s1 = document.getElementById('softland-agg1-slider');
  const msg = document.getElementById('softland-save-msg');

  const enabled = toggle ? (toggle.checked ? 1 : 0) : 1;
  const agg0 = s0 ? parseFloat(s0.value) : 1.0;
  const agg1 = s1 ? parseFloat(s1.value) : 1.0;

  fetch(`/api/softland/set?enabled=${enabled}&agg0=${agg0}&agg1=${agg1}`)
    .then(r => r.json())
    .then(data => {
      softlandAgg0UserEditing = false;
      softlandAgg1UserEditing = false;
      const act0 = document.getElementById('softland-agg0-active');
      if (act0 && data.agg0 !== undefined) act0.textContent = parseFloat(data.agg0).toFixed(1);
      const act1 = document.getElementById('softland-agg1-active');
      if (act1 && data.agg1 !== undefined) act1.textContent = parseFloat(data.agg1).toFixed(1);

      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `💾 Soft-Landing Governor settings saved to Flash! (Enabled: ${data.enabled ? 'YES' : 'NO'}, CH0: ${data.agg0.toFixed(1)}x, CH1: ${data.agg1.toFixed(1)}x)`;
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    })
    .catch(err => {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to save Soft-Landing settings';
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    });
}

// =========================================================
// ADAPTIVE SELF-LEARNING THERMAL ENGINE MODULE
// =========================================================
let conductanceChart = null;
let convergenceChart = null;
let lastKnownCycles0 = -1;
let lastKnownCycles1 = -1;

function initConductanceChart() {
  const ctx = document.getElementById('chart-conductance');
  if (!ctx || typeof Chart === 'undefined') return;

  const deltaLabels = [0, 2, 4, 6, 8, 10, 12, 14, 16];
  
  conductanceChart = new Chart(ctx.getContext('2d'), {
    type: 'line',
    data: {
      labels: deltaLabels.map(d => `${d}°C`),
      datasets: [
        {
          label: 'CH0 Guide Scope Dissipation Model',
          data: deltaLabels.map(d => Math.min(100, (d * 12.0 / 255.0) * 100)),
          borderColor: '#38bdf8',
          backgroundColor: 'rgba(56, 189, 248, 0.1)',
          borderWidth: 2.5,
          tension: 0.1,
          fill: true,
          pointRadius: 4,
          pointBackgroundColor: '#38bdf8'
        },
        {
          label: 'CH1 Main Scope Dissipation Model',
          data: deltaLabels.map(d => Math.min(100, (d * 14.0 / 255.0) * 100)),
          borderColor: '#c084fc',
          backgroundColor: 'rgba(192, 132, 252, 0.1)',
          borderWidth: 2.5,
          tension: 0.1,
          fill: true,
          pointRadius: 4,
          pointBackgroundColor: '#c084fc'
        }
      ]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: false,
      scales: {
        x: {
          title: { display: true, text: 'Optics Temperature Above Ambient (Δ°C)', color: '#94a3b8', font: { size: 11 } },
          grid: { color: 'rgba(255, 255, 255, 0.05)' },
          ticks: { color: '#94a3b8' }
        },
        y: {
          min: 0,
          max: 100,
          title: { display: true, text: 'Equilibrium Holding Power (%)', color: '#94a3b8', font: { size: 11 } },
          grid: { color: 'rgba(255, 255, 255, 0.05)' },
          ticks: { color: '#94a3b8' }
        }
      },
      plugins: {
        legend: {
          labels: { color: '#cbd5e1', font: { size: 11 } }
        }
      }
    }
  });
}

function updateConductanceCurves(kloss0, kloss1) {
  if (!conductanceChart) return;
  const k0 = (kloss0 && kloss0 > 1) ? kloss0 : 12.0;
  const k1 = (kloss1 && kloss1 > 1) ? kloss1 : 14.0;
  const deltaVals = [0, 2, 4, 6, 8, 10, 12, 14, 16];

  conductanceChart.data.datasets[0].data = deltaVals.map(d => Math.min(100, (d * k0 / 255.0) * 100));
  conductanceChart.data.datasets[1].data = deltaVals.map(d => Math.min(100, (d * k1 / 255.0) * 100));
  
  if (document.getElementById('tab-adaptive')?.classList.contains('active')) {
    conductanceChart.update('none');
  }
}

function initConvergenceChart() {
  const ctx = document.getElementById('chart-convergence');
  if (!ctx || typeof Chart === 'undefined') return;

  convergenceChart = new Chart(ctx.getContext('2d'), {
    type: 'line',
    data: {
      labels: ['Baseline'],
      datasets: [
        {
          label: 'CH0 Guide Scope Descent Error (°C)',
          data: [0.0],
          borderColor: '#38bdf8',
          backgroundColor: 'transparent',
          borderWidth: 2,
          pointRadius: 5,
          pointBackgroundColor: '#38bdf8',
          tension: 0.2
        },
        {
          label: 'CH1 Main Scope Descent Error (°C)',
          data: [0.0],
          borderColor: '#c084fc',
          backgroundColor: 'transparent',
          borderWidth: 2,
          pointRadius: 5,
          pointBackgroundColor: '#c084fc',
          tension: 0.2
        },
        {
          label: 'Target Lock (0.0°C)',
          data: [0.0],
          borderColor: 'rgba(52, 211, 153, 0.6)',
          borderDash: [5, 5],
          borderWidth: 1.5,
          pointRadius: 0,
          fill: false
        }
      ]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: false,
      scales: {
        x: {
          title: { display: true, text: 'Adaptive Descent Cycles', color: '#94a3b8', font: { size: 11 } },
          grid: { color: 'rgba(255, 255, 255, 0.05)' },
          ticks: { color: '#94a3b8' }
        },
        y: {
          suggestedMin: -0.5,
          suggestedMax: 0.5,
          title: { display: true, text: 'Landing Error Δ (Under/Overshoot °C)', color: '#94a3b8', font: { size: 11 } },
          grid: { color: 'rgba(255, 255, 255, 0.05)' },
          ticks: { color: '#94a3b8' }
        }
      },
      plugins: {
        legend: {
          labels: { color: '#cbd5e1', font: { size: 11 } }
        }
      }
    }
  });
}

function updateConvergenceHistory(cycles0, lasterr0, cycles1, lasterr1) {
  if (!convergenceChart) return;
  let updated = false;

  if (cycles0 !== undefined && cycles0 > lastKnownCycles0 && lastKnownCycles0 >= 0) {
    lastKnownCycles0 = cycles0;
    const label = `Cycle #${cycles0} (CH0)`;
    convergenceChart.data.labels.push(label);
    convergenceChart.data.datasets[0].data.push(lasterr0 !== undefined ? parseFloat(lasterr0.toFixed(2)) : 0.0);
    const lastCh1Val = convergenceChart.data.datasets[1].data.slice(-1)[0] || 0.0;
    convergenceChart.data.datasets[1].data.push(lastCh1Val);
    convergenceChart.data.datasets[2].data.push(0.0);
    if (convergenceChart.data.labels.length > 20) {
      convergenceChart.data.labels.shift();
      convergenceChart.data.datasets[0].data.shift();
      convergenceChart.data.datasets[1].data.shift();
      convergenceChart.data.datasets[2].data.shift();
    }
    updated = true;
  } else if (lastKnownCycles0 < 0 && cycles0 !== undefined) {
    lastKnownCycles0 = cycles0;
  }

  if (cycles1 !== undefined && cycles1 > lastKnownCycles1 && lastKnownCycles1 >= 0) {
    lastKnownCycles1 = cycles1;
    const label = `Cycle #${cycles1} (CH1)`;
    convergenceChart.data.labels.push(label);
    const lastCh0Val = convergenceChart.data.datasets[0].data.slice(-1)[0] || 0.0;
    convergenceChart.data.datasets[0].data.push(lastCh0Val);
    convergenceChart.data.datasets[1].data.push(lasterr1 !== undefined ? parseFloat(lasterr1.toFixed(2)) : 0.0);
    convergenceChart.data.datasets[2].data.push(0.0);
    if (convergenceChart.data.labels.length > 20) {
      convergenceChart.data.labels.shift();
      convergenceChart.data.datasets[0].data.shift();
      convergenceChart.data.datasets[1].data.shift();
      convergenceChart.data.datasets[2].data.shift();
    }
    updated = true;
  } else if (lastKnownCycles1 < 0 && cycles1 !== undefined) {
    lastKnownCycles1 = cycles1;
  }

  if (updated && document.getElementById('tab-adaptive')?.classList.contains('active')) {
    convergenceChart.update('none');
  }
}

// --- ADAPTIVE LEARNING REST API ---
function getLearningConfig() {
  fetch('/api/learning')
    .then(r => r.json())
    .then(data => {
      if (data.enabled !== undefined) {
        const toggle = document.getElementById('learning-master-toggle');
        if (toggle) toggle.checked = data.enabled;
      }
      if (data.speed !== undefined) {
        const sel = document.getElementById('learning-speed-select');
        if (sel) sel.value = data.speed;
      }
      if (data.kloss0 !== undefined || data.kloss1 !== undefined) {
        updateConductanceCurves(data.kloss0, data.kloss1);
      }
    })
    .catch(err => console.warn("Could not fetch learning config:", err));
}

function toggleLearningMaster() {
  saveLearningConfig();
}

function saveLearningConfig() {
  const toggle = document.getElementById('learning-master-toggle');
  const sel = document.getElementById('learning-speed-select');
  const msg = document.getElementById('learning-save-msg');

  const enabled = toggle ? (toggle.checked ? 1 : 0) : 1;
  const speed = sel ? parseInt(sel.value) : 1;

  fetch(`/api/learning/set?enabled=${enabled}&speed=${speed}`)
    .then(r => r.json())
    .then(data => {
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `💾 Adaptive Learning settings saved to Flash! (Active: ${data.enabled ? 'YES' : 'NO'}, Speed: ${data.speed === 0 ? 'Conservative' : (data.speed === 2 ? 'Fast' : 'Balanced')})`;
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    })
    .catch(err => {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to save Learning settings';
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    });
}

function resetLearningModel() {
  if (!confirm("Reset learned thermal model (conductance Kloss & lead dead times) to factory baseline?")) return;

  const msg = document.getElementById('learning-save-msg');
  fetch('/api/learning/reset')
    .then(r => r.json())
    .then(data => {
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = '🔄 Learned thermal parameters reset to factory defaults!';
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
      getLearningConfig();
    })
    .catch(err => {
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to reset learning model';
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    });
}

// --- WIFI & NETWORK CONNECTION MANAGER ---
function getWifiStatus() {
  fetch('/api/wifi/status')
    .then(r => r.json())
    .then(data => {
      const curSsid = document.getElementById('wifi-cur-ssid');
      const curIp = document.getElementById('wifi-cur-ip');
      const badge = document.getElementById('wifi-cur-status-badge');
      if (curSsid) curSsid.textContent = data.ssid || (data.ap_mode ? 'DewHeater-Setup' : '--');
      if (curIp) curIp.textContent = data.ip || '--';
      if (badge) {
        if (data.ap_mode) {
          badge.textContent = 'Setup AP Mode';
          badge.style.color = '#38bdf8';
          badge.style.background = 'rgba(56, 189, 248, 0.15)';
          badge.style.borderColor = 'rgba(56, 189, 248, 0.4)';
        } else if (data.connected) {
          badge.textContent = `Connected (${data.quality || 100}%)`;
          badge.style.color = '#34d399';
          badge.style.background = 'rgba(52, 211, 153, 0.15)';
          badge.style.borderColor = 'rgba(52, 211, 153, 0.4)';
        } else {
          badge.textContent = 'Disconnected';
          badge.style.color = '#ef4444';
          badge.style.background = 'rgba(239, 68, 68, 0.15)';
          badge.style.borderColor = 'rgba(239, 68, 68, 0.4)';
        }
      }
      const ssidInput = document.getElementById('wifi-cfg-ssid');
      if (ssidInput && !ssidInput.value && data.ssid) {
        ssidInput.value = data.ssid;
      }
    })
    .catch(err => console.warn('Could not fetch WiFi status:', err));
}

function scanWifiNetworks() {
  const btn = document.getElementById('btn-scan-wifi');
  const sel = document.getElementById('wifi-detected-select');
  const msg = document.getElementById('wifi-save-msg');

  if (btn) {
    btn.disabled = true;
    btn.textContent = '⏳ Scanning...';
  }
  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = '🔍 Scanning 2.4GHz WiFi networks...';
  }

  fetch('/api/wifi/scan')
    .then(r => r.json())
    .then(networks => {
      if (btn) {
        btn.disabled = false;
        btn.textContent = '🔍 Scan WiFi';
      }
      if (sel) {
        sel.innerHTML = '<option value="">-- Select Detected Network --</option>';
        if (networks && networks.length > 0) {
          networks.forEach(net => {
            const opt = document.createElement('option');
            opt.value = net.ssid;
            const lockIcon = net.secure ? '🔒' : '🔓';
            opt.textContent = `${lockIcon} ${net.ssid} (${net.rssi} dBm, ${net.quality}%)`;
            sel.appendChild(opt);
          });
          if (msg) {
            msg.style.color = '#34d399';
            msg.textContent = `✅ Found ${networks.length} WiFi network(s).`;
            setTimeout(() => { msg.textContent = ''; }, 4000);
          }
        } else {
          const opt = document.createElement('option');
          opt.value = "";
          opt.textContent = "No networks found";
          sel.appendChild(opt);
          if (msg) {
            msg.style.color = '#f59e0b';
            msg.textContent = '⚠️ No networks found. Try scanning again.';
            setTimeout(() => { msg.textContent = ''; }, 4000);
          }
        }
      }
    })
    .catch(err => {
      console.error('WiFi scan error:', err);
      if (btn) {
        btn.disabled = false;
        btn.textContent = '🔍 Scan WiFi';
      }
      if (msg) {
        msg.style.color = '#ef4444';
        msg.textContent = '❌ Failed to scan WiFi networks.';
        setTimeout(() => { msg.textContent = ''; }, 4000);
      }
    });
}

function onSelectDetectedWifi(ssid) {
  if (!ssid) return;
  const ssidInput = document.getElementById('wifi-cfg-ssid');
  if (ssidInput) {
    ssidInput.value = ssid;
  }
  const passInput = document.getElementById('wifi-cfg-pass');
  if (passInput) {
    passInput.focus();
  }
}

function togglePasswordVisibility(inputId) {
  const el = document.getElementById(inputId);
  if (el) {
    el.type = el.type === 'password' ? 'text' : 'password';
  }
}

function saveAndConnectWifi() {
  const ssid = document.getElementById('wifi-cfg-ssid')?.value?.trim();
  const pass = document.getElementById('wifi-cfg-pass')?.value || '';
  const msg = document.getElementById('wifi-save-msg');

  if (!ssid) {
    if (msg) {
      msg.style.color = '#ef4444';
      msg.textContent = '❌ Please enter or select a valid Network SSID.';
    }
    return;
  }

  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = `💾 Saving credentials & connecting to "${ssid}"... Controller may change IP address.`;
  }

  fetch(`/api/wifi/save?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`, { method: 'POST' })
    .then(r => r.json())
    .then(data => {
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `✅ Saved! Reconnecting to ${ssid}... Please wait or check router for new IP.`;
      }
      setTimeout(getWifiStatus, 6000);
    })
    .catch(err => {
      console.warn("WiFi save request sent, controller reconnecting:", err);
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = `✅ Saved! Controller is reconnecting to ${ssid}...`;
      }
    });
}

function resetWifiToAP() {
  if (!confirm("Reset WiFi settings and return controller to DewHeater-Setup Captive Portal AP mode?")) return;

  const msg = document.getElementById('wifi-save-msg');
  if (msg) {
    msg.style.color = '#38bdf8';
    msg.textContent = '🔄 Clearing WiFi credentials and launching Setup AP...';
  }

  fetch('/api/wifi/reset', { method: 'POST' })
    .then(r => r.json())
    .then(() => {
      if (msg) {
        msg.style.color = '#34d399';
        msg.textContent = '✅ Reset! Connect to "DewHeater-Setup" WiFi AP (192.168.4.1).';
      }
    })
    .catch(err => {
      if (msg) {
        msg.style.color = '#38bdf8';
        msg.textContent = '🔄 Controller resetting to AP mode. Connect to "DewHeater-Setup" (192.168.4.1).';
      }
    });
}