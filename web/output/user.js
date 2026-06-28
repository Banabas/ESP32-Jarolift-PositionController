// --------------------------------------
// localization texts
// --------------------------------------
const user_translations = {
  timer: {
    de: "Timer",
    en: "Timer",
  },
  led_setup: {
    de: "LED-Setup-Mode",
    en: "LED-Setup-Mode",
  },
  learn_mode: {
    de: "neuer Anlernmodus",
    en: "new learn mode",
  },
  shutter: {
    de: "Rolladen",
    en: "Shutter",
  },
  serial_nr: {
    de: "Seriennummer",
    en: "serial number",
  },
  set_shade: {
    de: "setze Schatten",
    en: "set shade",
  },
  learn: {
    de: "anlernen",
    en: "learn",
  },
  channel: {
    de: "Kanal",
    en: "Channel",
  },
  info_leanmode: {
    de: "Der neue Lernmodus ist für Empfänger neuer als 2010, die eine Lernsequenz haben, bei der die Tasten UP+DOWN gleichzeitig gedrückt werden und dann die Taste STOP. Die alte Lernmethode für Empfänger, die vor 2010 hergestellt wurden, verwendet eine spezielle LEARN-Taste.",
    en: "new learn mode is for receivers newer than 2010, which have a learning sequence of buttons UP+DOWN pressed simultaneously, then press STOP. the old learn method for receivers manufactured before 2010 uses a special LEARN button.",
  },
  info_serial: {
    de: "Jeder Kanal benötigt eine individuelle Seriennummer. Wenn Sie hier ein neues Seriennummernpräfix eingeben, ändern sich die Seriennummern für alle Kanäle. ACHTUNG: alle Empfänger müssen danach (erneut) eingelernt werden und der Device Counter muss zurückgesetzt werden!",
    en: "Each channel requires an individual serial number. If you enter a new serial number prefix here, the serial numbers for all channels will change. ATTENTION: all receivers must then be learned (again) and the device counter must be reset!",
  },
  info_devcnt: {
    de: "Der Zähler wird zusammen mit jedem Datagramm über den Funk gesendet. Er beginnt bei null und wird bei jedem Datagramm inkrementiert. Jeder empfänger hört die empfangenen Datagramme ab und zeichnet den Zähler des Senders auf. Wenn der Zähler des Senders und des Empfängers zu sehr voneinander abweichen, müssen Sie den Empfänger neu anlernen. Um dies zu vermeiden, wenn Sie einen Dongle austauschen (oder wenn ein Update schief geht), können Sie hier einen Zähler ungleich null eingeben. ACHTUNG: lassen Sie die Finger davon, wenn Sie die Wirkung nicht verstehen, Sie riskieren, dass Sie alle Ihre Empfänger (wieder) neu anlernen müssen!",
    en: "the device counter is send together with every datagram over the radio. beginning with zero, it is incremented on every datagram. each receiver listens to received datagrams and records the sender's device counter. when the sender's and receiver's device counter differ too much, you must re-learn the receiver. to avoid this when replacing a dongle (or when an update goes wrong) you can enter a non-zero device counter here. ATTENTION: don't touch this if you don't understand the effect, you risk to re-learn all your receivers (again)!",
  },
  help: {
    de: "Hilfe",
    en: "Help",
  },
  gpio_restart_info: {
    de: "Änderungen an den GPIO Einstellungen benötigen einen Neustart!",
    en: "Changes to GPIO or Jarolift settings require a restart!",
  },
  gpio_info: {
    de: "Beispiel für einen typischen ESP32",
    en: "example for typical ESP32",
  },
  mqtt_info2: {
    de: "< ../ > ist der Platzhalter für das MQTT Topic welches in den Einstellungen vorgegeben wird.",
    en: "< ../ > is the placeholder for the MQTT topic which is specified in the settings.",
  },
  groups: {
    de: "Gruppen",
    en: "Groups",
  },
  group: {
    de: "Gruppe",
    en: "Group",
  },
  channels: {
    de: "Kanäle",
    en: "Channels",
  },
  name: {
    de: "Name",
    en: "Name",
  },
  mask: {
    de: "Bitmaske",
    en: "Bitmask",
  },
  mask_help: {
    de: "In diesem Feld wird über eine Bitmaske festgelegt, welche Kanäle zu dieser Gruppe gehören.\nDie Bitmaske ist eine 16-Bit-Zahl, wobei das niederwertigste Bit (rechts) den Kanal 1 repräsentiert. \nEin gesetztes Bit bedeutet, dass der Kanal zu dieser Gruppe gehört.\n\nBeispiel: 0000000000010101 bedeutet, dass die Kanäle 1, 3 und 5 zu dieser Gruppe gehören.",
    en: "In this field, a bitmask is used to determine which channels belong to this group.\nThe bitmask is a 16-bit number, with the least significant bit (right) representing channel 1.\nA set bit means that the channel belongs to this group.\n\nExample: 0000000000010101 means that channels 1, 3 and 5 belong to this group.",
  },
  grp_mask_help: {
    de: "Sie können auch ein generisches Gruppenkommando verwenden und die Bitmaske verwenden, um die Rolläden direkt auszuwählen. Die Bitmaske ist eine 16-Bit-Zahl, wobei das niederwertigste Bit (rechts) den Kanal 1 repräsentiert. Ein gesetztes Bit bedeutet, dass der Kanal zu dieser Gruppe gehört.\n\nBeispiel: 0000000000010101 bedeutet, dass die Kanäle 1, 3 und 5 zu dieser Gruppe gehören.\n\nAls Payload können Sie drei verschiedene Formate verwenden, um die gleiche Bitmaske darzustellen:",
    en: "You can also use a generic group command and provide the bitmask to select the shutters directly. The bitmask is a 16-bit number, with the least significant bit (on the right) representing channel 1. A set bit means that the channel belongs to this group.\n\nExample: `0000000000010101` means that channels 1, 3, and 5 belong to this group.\n\nAs payload, you can use three different formats to represent the same bitmask:",
  },
  bitmask_wizard: {
    de: "Auswahl Assistent",
    en: "Selection Wizard",
  },
  general: {
    de: "Allgemein",
    en: "General",
  },
  sunrise: {
    de: "Sonnenaufgang",
    en: "sunrise",
  },
  sundown: {
    de: "Sonnenuntergang",
    en: "sundown",
  },
  cmd_up: {
    de: "Hochfahren",
    en: "up",
  },
  cmd_down: {
    de: "Runterfahren",
    en: "down",
  },
  time: {
    de: "Uhrzeit",
    en: "Time",
  },
  day_mo: {
    de: "Mo",
    en: "Mo",
  },
  day_tu: {
    de: "Di",
    en: "Tu",
  },
  day_we: {
    de: "Mi",
    en: "We",
  },
  day_th: {
    de: "Do",
    en: "Th",
  },
  day_fr: {
    de: "Fr",
    en: "Fr",
  },
  day_sa: {
    de: "Sa",
    en: "Sa",
  },
  day_su: {
    de: "So",
    en: "Su",
  },
  geo_location: {
    de: "Geografische Lage",
    en: "Geographical location",
  },
  geo_info: {
    de: "Wird benötigt für die Timer Funktion mit Sonnenaufgang und Sonnenuntergang",
    en: "Required for the timer function with sunrise and sunset",
  },
  latitude: {
    de: "Breitengrad",
    en: "Latitude",
  },
  longitude: {
    de: "Längengrad",
    en: "Longitude",
  },
  time_info: {
    de: "Zeitinformationen",
    en: "Time information",
  },
  apply: {
    de: "Übernehmen",
    en: "apply",
  },
  cancel: {
    de: "Abbrechen",
    en: "cancel",
  },
  gpio: {
    de: "GPIO-Zuweisung",
    en: "GPIO-Settings",
  },
  remote: {
    de: "Fernbedienung",
    en: "Remote",
  },
  remotes: {
    de: "Fernbedienungen",
    en: "Remotes",
  },
  service: {
    de: "Service",
    en: "Service",
  },
  service_cmds: {
    de: "Service-Befehle",
    en: "Service-Commands",
  },
  cmd_endpoint_up: {
    de: "Endlage oben",
    en: "Endpoint up",
  },
  cmd_endpoint_down: {
    de: "Endlage unten",
    en: "Endpoint down",
  },
  set: {
    de: "setzen",
    en: "set",
  },
  delete: {
    de: "löschen",
    en: "delete",
  },
  unlearn: {
    de: "ablernen",
    en: "unlearn",
  },
  calib_title: {
    de: "Laufzeit-Kalibrierung",
    en: "Travel Time Calibration",
  },
  calib_step1: {
    de: "Schritt 1: Rollladen vollständig öffnen (Taste oben drücken bis Anschlag), dann auf Start klicken.",
    en: "Step 1: Fully open the shutter (press Up until end stop), then click Start.",
  },
  calib_step2: {
    de: "Schritt 2: Rollladen fährt runter. Warten bis vollständig geschlossen, dann Stopp drücken.",
    en: "Step 2: Shutter moves down. Wait until fully closed, then press Stop.",
  },
  calib_step3: {
    de: "Schritt 3: Rollladen fährt hoch. Warten bis vollständig geöffnet, dann Stopp drücken.",
    en: "Step 3: Shutter moves up. Wait until fully open, then press Stop.",
  },
  calib_start: {
    de: "Start Kalibrierung",
    en: "Start Calibration",
  },
  calib_finish: {
    de: "Stopp - Rollladen unten",
    en: "Stop - Shutter closed",
  },
  calib_shutter_hint: {
    de: "Kanal gemäß Auswahl oben",
    en: "Channel as selected above",
  },
  calib_step4: {
    de: "Kalibrierung abgeschlossen!",
    en: "Calibration complete!",
  },
  calib_finish_up: {
    de: "Stopp - Rollladen oben",
    en: "Stop - Shutter open",
  },
  travel_time_down: {
    de: "Laufzeit ZU (s)",
    en: "Travel time DOWN (s)",
  },
  travel_time_up: {
    de: "Laufzeit AUF (s)",
    en: "Travel time UP (s)",
  },

  astro_real: {
    de: "REAL (Sonnenauf/-untergang)",
    en: "REAL (Sunrise/Sunset)",
  },
  astro_civil: {
    de: "CIVIL (bürgerliche Dämmerung)",
    en: "CIVIL (Civil Twilight)",
  },
  astro_nautic: {
    de: "NAUTIC (nautische Dämmerung)",
    en: "NAUTIC (Nautical Twilight)",
  },
  astro_astronomic: {
    de: "ASTRONOMIC (astronomische Dämmerung)",
    en: "ASTRONOMIC (Astronomical Twilight)",
  },
  astro_horizon: {
    de: "HORIZON (eigener Winkel)",
    en: "HORIZON (custom angle)",
  },

  timer_shutters: {
    de: "Kanäle",
    en: "Channels",
  },
  timer_groups: {
    de: "Gruppen",
    en: "Groups",
  },
  offset_min: {
    de: "Minuten Versatz",
    en: "minutes offset",
  },
  weekend_override: {
    de: "Wochenende (Sa+So) andere Zeit",
    en: "Weekend (Sa+Su) different time",
  },
  timer_overview: {
    de: "Timer Übersicht",
    en: "Timer Overview",
  },
  weekdays: {
    de: "Wochentage",
    en: "Weekdays",
  },
  no_timer_active: {
    de: "Keine aktiven Timer",
    en: "No active timers",
  },
};

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

