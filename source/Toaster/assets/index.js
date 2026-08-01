/**
 *   GPStar Toaster - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2026 Dustin Grau <dustin.grau@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

var websocket;
var statusInterval;
var animationSlots = []; // Cache of animation slot metadata from server
var hasAnyViableSlots = false; // Track if there are any slots with saved animations

/**
 * Animation State Machine - Single Source of Truth
 * Values must match AnimationState enum in Animation.h:
 *   ANIM_IDLE_EMPTY = 0         (no data anywhere)
 *   ANIM_RECORDING = 1          (recording active)
 *   ANIM_IDLE_PENDING_SAVE = 2  (recording stopped, unsaved)
 *   ANIM_IDLE_LOADED = 3        (animation loaded from NVS)
 *   ANIM_PLAYBACK = 4           (playback active)
 */
var currentAnimationState = "IDLE_EMPTY";

/**
 * Button State Mapping -  Maps each animation state to the buttons that should be enabled/disabled.
 */
const stateToButtons = {
  IDLE_EMPTY: {
    btnStartRec: true, btnStopRec: false,
    saveSlot: false, btnSave: false, btnCancel: false,
    playSlot: true, btnPlay: true, btnStop: false,
    showProgress: false,
  },
  RECORDING: {
    btnStartRec: false, btnStopRec: true,
    saveSlot: false, btnSave: false, btnCancel: false,
    playSlot: false, btnPlay: false, btnStop: false,
    showProgress: true,
  },
  IDLE_PENDING_SAVE: {
    btnStartRec: false, btnStopRec: false,
    saveSlot: true, btnSave: true, btnCancel: true,
    playSlot: true, btnPlay: false, btnStop: false,
    showProgress: false,
  },
  IDLE_LOADED: {
    btnStartRec: true, btnStopRec: false,
    saveSlot: false, btnSave: false, btnCancel: false,
    playSlot: true, btnPlay: true, btnStop: false,
    showProgress: false,
  },
  PLAYBACK: {
    btnStartRec: false, btnStopRec: false,
    saveSlot: false, btnSave: false, btnCancel: false,
    playSlot: false, btnPlay: false, btnStop: true,
    showProgress: true,
  },
};

function onLoad(event) {
  document.getElementsByClassName("tablinks")[0].click();
  setControlDefaults(); // Hide animation UI on startup
  getDevicePrefs(); // Get all preferences.
  getNetworkInfo(); // Get networking info.
  initWebSocket(); // Open the WebSocket.
  getStatus(updateDisplay); // Get status immediately.
}

function initWebSocket() {
  console.log("Attempting to open a WebSocket connection...");
  let gateway = "ws://" + window.location.hostname + "/ws";
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
  doHeartbeat();
}

function doHeartbeat() {
  if (websocket.readyState == websocket.OPEN) {
    websocket.send("heartbeat"); // Send a specific message.
  }
  getNetworkInfo(); // Refresh network statistics.
  setTimeout(doHeartbeat, 8000);
}

function onOpen(event) {
  console.log("WebSocket connection opened");

  // Clear the automated status interval timer.
  clearInterval(statusInterval);
}

function onClose(event) {
  console.log("WebSocket connection closed");
  setTimeout(initWebSocket, 1000);

  // Fallback for when WebSocket is unavailable.
  if (!statusInterval) {
    statusInterval = setInterval(function () {
      getStatus(updateDisplay); // Check for status every X seconds
    }, 1000);
  }
}

function onMessage(event) {
  if (isJsonString(event.data)) {
    // If JSON, use as status update.
    updateDisplay(JSON.parse(event.data));
  } else {
    // Anything else gets sent to console.
    console.log(event.data);
  }
}

function getDevicePrefs() {
  // This is updated once per page load as it is not subject to frequent changes.
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status >= 200 && this.status < 300) {
      var jObj = JSON.parse(this.responseText);
      if (jObj) {
        // Device Info
        setHtml("buildDate", "Build: " + (jObj.buildDate || ""));

        switch (jObj.audioVersion ?? 0) {
          case 0:
            setHtml("audioInfo", "No Audio Detected");
            break;
          case 100:
            setHtml("audioInfo", "GPStar Audio Firmware: v100");
            break;
          default:
            setHtml("audioInfo", "GPStar Audio Firmware: v" + (jObj.audioVersion || ""));
            break;
        }

        // microSD Warnings
        if (Boolean(jObj.audioCorrupt)) {
          alert("Corruption has been detected on the microSD card. Please reformat the card as FAT32 and reload audio files.");
        } else if (Boolean(jObj.audioOutdated)) {
          // The file count on the microSD card does not match firmware; alert the user.
          alert("Contents of microSD card do not match current firmware. Please make sure to update your microSD cards after updating firmware.");
        }
      }
    } else if (this.readyState == 4) {
      // Handle error responses
      handleStatus(this.responseText);
    }
  };
  xhttp.open("GET", "/config/device", true);
  xhttp.send();
}

function getNetworkInfo() {
  // Fetch network configuration and statistics from dedicated endpoint.
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status >= 200 && this.status < 300) {
      var jObj = JSON.parse(this.responseText);
      if (jObj) {
        // Display local AP network name
        if (jObj.localAP && jObj.localAP.ssid) {
          setHtml("wifiName", "Private Network: " + jObj.localAP.ssid);
        }

        // Display client counts
        var clientText = "AP Clients: " + (jObj.apClients ?? 0) + " / WebSocket Clients: " + (jObj.wsClients ?? 0);
        setHtml("clientInfo", clientText);

        // Display external WiFi info if connected
        if (jObj.extWifi && jObj.extWifi.enabled && jObj.extWifi.connected) {
          var extInfo = jObj.extWifi.ssid + ": " + jObj.extWifi.address + " / " + jObj.extWifi.subnet;
          setHtml("extWifi", extInfo);
        } else {
          setHtml("extWifi", ""); // Clear if not connected
        }
      }
    } else if (this.readyState == 4) {
      // Handle error responses
      console.log("Failed to fetch network info:", this.responseText);
    }
  };
  xhttp.open("GET", "/wifi/status", true);
  xhttp.send();
}

function setControlDefaults() {
  // On page load, apply initial state (IDLE_EMPTY)
  currentAnimationState = "IDLE_EMPTY";
  applyButtonStates(currentAnimationState);
}

/**
 * Apply button enabled/disabled states based on current animation state.
 * This is the ONLY place button states are controlled for animation UI.
 * Eliminates all scattered enable/disable calls and conditional logic.
 */
function applyButtonStates(state) {
  const config = stateToButtons[state];
  if (!config) {
    console.warn("Unknown animation state: " + state);
    return;
  }

  // Apply all button states atomically
  Object.entries(config).forEach(([key, value]) => {
    if (key === "showProgress") {
      // Special handling for progress visibility - show/hide all progress divs with class
      const progressDivs = document.querySelectorAll(".animationProgress");
      progressDivs.forEach(div => {
        value ? div.style.display = "" : div.style.display = "none";
      });
    } else if (key === "playSlot" || key === "btnPlay") {
      // Special handling for play controls - only enable if there are viable slots
      // AND the state allows it
      const shouldEnable = value && hasAnyViableSlots;
      shouldEnable ? enableEl(key) : disableEl(key);
    } else {
      // Standard button enable/disable
      value ? enableEl(key) : disableEl(key);
    }
  });
}

function updateDisplay(jObj) {
  // Update display if we have the expected data (containing mode and theme at a minimum).
  if (jObj) {
    // Volume Information
    setHtml("masterVolume", (jObj.volMaster ?? 0) + "%");
    if ((jObj.volMaster ?? 0) == 0) {
      setHtml("masterVolume", "Min");
    }
    setHtml("effectsVolume", (jObj.volEffects ?? 0) + "%");
    if ((jObj.volEffects ?? 0) == 0) {
      setHtml("effectsVolume", "Min");
    }
    setHtml("musicVolume", (jObj.volMusic ?? 0) + "%");
    if ((jObj.volMusic ?? 0) == 0) {
      setHtml("musicVolume", "Min");
    }

    // Update RF Input Button States
    if (jObj.buttons && Array.isArray(jObj.buttons)) {
      for (let i = 0; i < jObj.buttons.length; i++) {
        const btn = jObj.buttons[i];
        const stateText = btn.state ? "HIGH" : "LOW";
        setHtml("button" + btn.id, stateText);
      }
    }

    // Update animation slot availability for save and play dropdowns
    if (jObj.animationSlots && Array.isArray(jObj.animationSlots)) {
      animationSlots = jObj.animationSlots; // Cache in global variable.
      updateSaveSlots();
      updatePlaySlots();
    }

    // Handle animation state from status response (ensures UI updates on page load)
    if (jObj.animation) {
      updateAnimationDisplay(jObj.animation);
    }
  }
}

/** API Calls **/

function triggerAct1() {
  sendCommand("/device/actuator/1");
}

function triggerAct2() {
  sendCommand("/device/actuator/2");
}

function triggerAct3() {
  sendCommand("/device/actuator/3");
}

function triggerAct4() {
  sendCommand("/device/actuator/4");
}

function selectMusic1() {
  sendCommand("/music/select?track=500");
}

function selectMusic2() {
  sendCommand("/music/select?track=501");
}

function recordingStart() {
  // Send command to backend to start recording animation frames
  sendCommand("/animations/record/start");
}

function recordingStop() {
  // Send command to backend to stop recording animation frames
  sendCommand("/animations/record/stop");
}

function discardRecording() {
  // Discard unsaved recording and return to IDLE_EMPTY state
  sendCommand("/animations/record/discard");
}

function saveToSlot() {
  const slot = parseInt(getEl("saveSlot").value);

  // Check if the selected slot already has an animation
  const slotInfo = animationSlots.find((s) => s.id === slot);
  if (slotInfo && slotInfo.hasAnimation) {
    // Confirm before overwriting - display duration in seconds
    const confirmMsg = "Slot " + slot + " already has an animation (" + slotInfo.animationSeconds.toFixed(1) + " seconds). Overwrite?";
    if (!confirm(confirmMsg)) {
      return; // User cancelled
    }
  }

  sendCommand("/animations/record/save/" + slot);
}

/** State Management **/

function playFromSlot() {
  // Get the selected animation slot from the play dropdown
  const slotIndex = parseInt(getEl("playSlot").value);
  
  // Verify selected slot contains animation data
  const slotInfo = animationSlots.find((s) => s.id === slotIndex);
  if (!slotInfo || !slotInfo.hasAnimation || !slotInfo.keyFrames) {
    alert("Slot " + slotIndex + " does not contain a saved animation.");
    return;
  }
  
  // Send command to backend to play the selected animation
  sendCommand("/animations/play/" + slotIndex);
}

function stopPlayback() {
  // Stop playback of the current animation
  // Returns to IDLE_LOADED state (animation data preserved)
  sendCommand("/animations/stop");
}

function updateSaveSlots() {
  // Update save slot dropdown - all slots always enabled for save
  if (!animationSlots || !Array.isArray(animationSlots)) return;

  const saveSelect = getEl("saveSlot");
  if (!saveSelect) return;

  // Slots are always available for saving (shows duration in seconds if already has data)
  for (let i = 0; i < animationSlots.length; i++) {
    const slot = animationSlots[i];
    const option = saveSelect.options[i];
    if (option) {
      // Update option label to show animation duration in seconds if it has data
      const label = slot.hasAnimation ? "Slot " + slot.id + " (" + slot.animationSeconds.toFixed(1) + "s)" : "Slot " + slot.id;
      option.text = label;
      option.disabled = false; // All slots available for saving
    }
  }
}

function updatePlaySlots() {
  // Update play slot dropdown - only enable slots with recordings
  if (!animationSlots || !Array.isArray(animationSlots)) {
    return;
  }

  const playSelect = getEl("playSlot");
  if (!playSelect) {
    return;
  }

  // Check if any slots have viable animations
  hasAnyViableSlots = animationSlots.some((slot) => slot.hasAnimation && slot.keyFrames > 0);

  // Update option labels and disable state
  for (let i = 0; i < animationSlots.length; i++) {
    const slot = animationSlots[i];
    const option = playSelect.options[i];
    if (option) {
      // Show animation duration in seconds and enable only if has animation
      const label = "Slot " + slot.id + (slot.hasAnimation ? " (" + slot.animationSeconds.toFixed(1) + "s)" : " (empty)");
      option.text = label;
      option.disabled = !slot.hasAnimation;
    }
  }
  
  // Re-apply button states to account for new viable slots availability
  applyButtonStates(currentAnimationState);
}

/** Animation Visualizations **/

if (!!window.EventSource) {
  // Create events for one-way communication.
  var source = new EventSource("/events");

  source.addEventListener(
    "open",
    function (e) {
      console.log("Server-Side Events connected");
    },
    false,
  );

  source.addEventListener(
    "error",
    function (e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Server-Side Events disconnected");
      }
    },
    false,
  );

  source.addEventListener(
    "debug",
    function (e) {
      if (e.data === undefined) return;
      console.log("Debug: ", e.data);
    },
    false,
  );

  source.addEventListener(
    "animation",
    function (e) {
      if (e.data === undefined) return;

      var animData = {}; // Always begin with an empty object.
      try {
        animData = JSON.parse(e.data); // JSON with frame position, timings
      } catch (e) {}

      // Listen for and update on animation frame information.
      updateAnimationDisplay(animData);
    },
    false
  );
}

// Delay the onLoad event until all necessary JS has been loaded
window.addEventListener("load", onLoad);

function buildAnimationProgressHTML(animData) {
  // Initialize display strings with default values for IDLE mode
  let progressBar = "-";
  let elapsedTime = "-";
  let lastActuator = "-";
  
  // Only populate progress info when actively recording or playing back
  if (animData.state === "RECORDING" || animData.state === "PLAYBACK") {
    // Calculate frame progress: percentage completion with visual progress bar
    const current = animData.currentFrame || 0;
    const total = animData.totalFrames || 0;
    const percentage = total > 0 ? ((current / total) * 100).toFixed(1) : 0;
    progressBar = percentage + "% <progress class=\"animationProgressBar\" value=\"" + percentage + "\" max=\"100\"></progress>";
    
    // Calculate elapsed time: current elapsed time vs total animation duration
    const elapsed = (animData.elapsedSeconds || 0).toFixed(1);
    const duration = (animData.totalTime || 0).toFixed(1);
    elapsedTime = elapsed + " / " + duration + "s";
    
    // Display which actuator was last triggered (backend provides lastActuator, fall back to frameValue if needed)
    if (animData.lastActuator && animData.lastActuator > 0 && animData.lastActuator <= 4) {
      lastActuator = "Actuator " + animData.lastActuator;
    } else if (animData.frameValue && animData.frameValue > 0 && animData.frameValue <= 4) {
      lastActuator = "Actuator " + animData.frameValue;
    }
  }

  // Build HTML string with progress bar, time elapsed, and last actuator triggered
  const html = 
    "<p><span class=\"infoLabel\">Progress:</span> <span class=\"infoState\">" + progressBar + "</span></p>" +
    "<p><span class=\"infoLabel\">Elapsed Time:</span> <span class=\"infoState\">" + elapsedTime + "</span></p>" +
    "<p><span class=\"infoLabel\">Last Actuator:</span> <span class=\"infoState\">" + lastActuator + "</span></p>";

  return html;
}

function updateAnimationDisplay(animData) {
  // Map server's explicit state name string directly
  // Valid states: IDLE_EMPTY, RECORDING, IDLE_PENDING_SAVE, IDLE_LOADED, PLAYBACK
  const validStates = ["IDLE_EMPTY", "RECORDING", "IDLE_PENDING_SAVE", "IDLE_LOADED", "PLAYBACK"];
  
  if (animData.state !== undefined && typeof animData.state === "string" && validStates.includes(animData.state)) {
    currentAnimationState = animData.state;
  } else {
    // Safe default - if state is invalid or missing, treat as IDLE_EMPTY
    currentAnimationState = "IDLE_EMPTY";
  }

  // Update slot options and recalculate viability FIRST (before applying button states)
  // This ensures hasAnyViableSlots is populated for button state logic
  // On first page load, animationSlots may be empty, but updatePlaySlots() will recalculate
  // when WebSocket delivers the slot data
  updatePlaySlots();
  
  // Apply all button states based on current state and newly calculated slot viability
  applyButtonStates(currentAnimationState);
  
  // Update progress display ONLY during active operations (RECORDING or PLAYBACK)
  // These states will have currentFrame, elapsedSeconds, progress fields populated
  const progressDivs = document.querySelectorAll(".animationProgress");
  if ((currentAnimationState === "RECORDING" || currentAnimationState === "PLAYBACK") && 
       animData.currentFrame !== undefined && animData.elapsedSeconds !== undefined) {
    const progressHTML = buildAnimationProgressHTML(animData);
    progressDivs.forEach(div => {
      div.innerHTML = progressHTML;
      div.style.display = "";
    });
  } else {
    // Hide progress during idle states
    progressDivs.forEach(div => {
      div.style.display = "none";
    });
  }
}

