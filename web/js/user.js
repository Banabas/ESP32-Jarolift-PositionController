// ============================================================
//  Bitmask Dialog
// ============================================================
function openGrpMaskHelp(button) {
  const dialog = document.getElementById("p12_bitmask_dialog");
  if (dialog) dialog.showModal();
}

function closeGrpMaskHelp() {
  const dialog = document.getElementById("p12_bitmask_dialog");
  if (dialog) dialog.close();
}

function setupBitmaskDialog() {
  const bitmaskDialog = document.getElementById("bitmask_dialog");
  const applyButton   = document.getElementById("apply_bitmask");
  const closeButton   = document.getElementById("close_bitmask_dialog");
  if (!bitmaskDialog || !applyButton || !closeButton) return;

  let currentInput = null;

  document.querySelectorAll(".bitmask-input").forEach((input) => {
    input.addEventListener("click", () => {
      currentInput = input;
      let bitmask = parseInt(input.value || "0", 2);
      for (let i = 0; i < 16; i++) {
        const checkbox = document.getElementById(`channel-${i}`);
        if (checkbox) {
          const isVisible = checkbox.parentElement.style.display !== "none";
          checkbox.checked = isVisible && (bitmask & (1 << i)) !== 0;
          if (!isVisible) bitmask &= ~(1 << i);
        }
      }
      if (currentInput && !Object.isFrozen(currentInput)) {
        currentInput.value = bitmask.toString(2).padStart(16, "0");
      }
      bitmaskDialog.showModal();
    });
  });

  applyButton.addEventListener("click", () => {
    let bitmask = 0;
    for (let i = 0; i < 16; i++) {
      const checkbox = document.getElementById(`channel-${i}`);
      if (checkbox && checkbox.checked) bitmask |= 1 << i;
    }
    if (currentInput) currentInput.value = bitmask.toString(2).padStart(16, "0");
    bitmaskDialog.close();
  });

  closeButton.addEventListener("click", () => bitmaskDialog.close());
}

// ============================================================
//  GitHub Pages Simulation
// ============================================================
async function loadSimulatedData() {
  if (!isGitHubPages()) return;
  try {
    const response = await fetch("sim.json");
    if (!response.ok) throw new Error("Fehler");
    const simData = await response.json();
    updateJSON(simData);
  } catch (error) {
    console.error("Fehler beim Laden von sim.json:", error);
  }
}

// ============================================================
//  Timer Inputs
// ============================================================
function updateUIcallbackSelect(elementId, value) {
  const element = document.getElementById(elementId);
  if (element && element.dataset.toggle === "timeInputs") {
    toggleTimeInputs(element);
    toggleTimeInputsMinMax(element);
  }
}

function toggleTimeInputsMinMax(selectElement) {
  const matches = selectElement.id.match(/\d+/g);
  const timerId = matches[matches.length - 1];
  const el = document.getElementById(`timer${timerId}-minmaxtime-settings`);
  if (el) el.style.display = selectElement.value === "0" ? "none" : "block";
}

function toggleTimeInputs(selectElement) {
  toggleTimeInputsMinMax(selectElement);
  const matches = selectElement.id.match(/\d+/g);
  const timerId = matches[matches.length - 1];
  const timeInput   = document.getElementById(`timeInput${timerId}`);
  const offsetInput = document.getElementById(`offsetInput${timerId}`);
  if (!timeInput || !offsetInput) return;
  if (selectElement.value === "0") {
    timeInput.style.display   = "block";
    offsetInput.style.display = "none";
  } else {
    timeInput.style.display   = "none";
    offsetInput.style.display = "block";
  }
}

// ============================================================
//  Kalibrierung – globale Funktionen, aufgerufen per onclick=""
// ============================================================
let calibTimerInterval = null;
let calibStartTime     = 0;

function calibReset() {
  clearInterval(calibTimerInterval);
  calibTimerInterval = null;
  const s1 = document.getElementById("p04_calib_step1");
  const s2 = document.getElementById("p04_calib_step2");
  const s3 = document.getElementById("p04_calib_step3");
  const t  = document.getElementById("p04_calib_timer");
  if (s1) s1.style.display = "";
  if (s2) s2.style.display = "none";
  if (s3) s3.style.display = "none";
  if (t)  t.textContent    = "0.0 s";
}

function calibStart() {
  const sel = document.getElementById("p04_calib_shutter");
  const ch  = sel ? parseInt(sel.value) : 0;

  // WS-Befehl an ESP: Kanal als value, ID wie C++ erwartet
  sendData("p04_calib_start", String(ch));

  // UI: Schritt 2 anzeigen + Stoppuhr starten
  const s1 = document.getElementById("p04_calib_step1");
  const s2 = document.getElementById("p04_calib_step2");
  if (s1) s1.style.display = "none";
  if (s2) s2.style.display = "";
  calibStartTime     = Date.now();
  calibTimerInterval = setInterval(function () {
    const t = document.getElementById("p04_calib_timer");
    if (t) t.textContent = ((Date.now() - calibStartTime) / 1000).toFixed(1) + " s";
  }, 100);
}

function calibFinish() {
  clearInterval(calibTimerInterval);
  const elapsed = Date.now() - calibStartTime;
  const sel = document.getElementById("p04_calib_shutter");
  const ch  = sel ? parseInt(sel.value) : 0;

  // WS-Befehl an ESP
  sendData("p04_calib_finish", String(ch));

  // UI: Schritt 3 anzeigen
  const s2  = document.getElementById("p04_calib_step2");
  const s3  = document.getElementById("p04_calib_step3");
  const res = document.getElementById("p04_calib_result");
  if (s2)  s2.style.display  = "none";
  if (s3)  s3.style.display  = "";
  if (res) res.textContent   = (elapsed / 1000).toFixed(2) + " s (" + elapsed + " ms)";

  setTimeout(calibReset, 5000);
}

// ============================================================
//  Schieberegler: Position vom Server aktualisieren
// ============================================================
function updateSliderFromServer(ch, pos) {
  const slider = document.getElementById("p01_pos_" + ch);
  const label  = document.getElementById("pos_lbl_" + ch);
  if (slider) slider.value       = pos;
  if (label)  label.textContent  = pos + "%";
}

// ============================================================
//  Initialisierung (defer: DOM ist bereits fertig)
// ============================================================
loadSimulatedData();
setupBitmaskDialog();

// ============================================================
//  Logger (overrides logger() from lib.js)
//  The log is transferred in several packets:
//  add_log = first packet (replace view), app_log = further packets (append)
// ============================================================
function logger(data) {
  var logOutput = document.getElementById("p10_log_output");
  if (!logOutput) return;
  if (data.cmd === "add_log" || data.cmd === "app_log") {
    var html = "";
    data.entry.forEach(function (entry) {
      html += entry + "<br>";
    });
    if (data.cmd === "add_log") {
      logOutput.innerHTML = html;
    } else {
      logOutput.insertAdjacentHTML("beforeend", html);
    }
  } else if (data.cmd === "clr_log") {
    logOutput.innerHTML = "";
  }
}
