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
var currentAnimationMode = "IDLE"; // Track animation mode to control UI visibility

function onLoad(event) {
  document.getElementsByClassName("tablinks")[0].click();
  disableAnimationButtons(); // Set button states by default.
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

function disableAnimationButtons() {
  // Used to just disable all the animation-related buttons initially
  // Use centralized helpers to manage disabled state consistently.
  hideEl("saveSlotSelector");
  hideEl("playSlotSelector");
  hideEl("animationProgressTab1");
  hideEl("animationProgressTab3");
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

    // Update Actuator/Relay States
    if (jObj.relays && Array.isArray(jObj.relays)) {
      for (let i = 0; i < jObj.relays.length; i++) {
        const relay = jObj.relays[i];
        const statusText = relay.active ? "ACTIVE" : "INACTIVE";
        setHtml("relay" + relay.id, statusText);
      }
    }

    // Update animation slot availability for save and play dropdowns
    if (jObj.animationSlots && Array.isArray(jObj.animationSlots)) {
      animationSlots = jObj.animationSlots; // Cache in global variable.
      updateSaveSlots();
      updatePlaySlots();
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
  
  // Hide save and play UI elements during recording - user cannot save or play while recording
  hideEl("saveSlotSelector");
  hideEl("playSlotSelector");
  
  // Show animation progress indicators so user can see recording activity in real-time
  showEl("animationProgressTab1");
  showEl("animationProgressTab3");
}

function recordingStop() {
  // Send command to backend to stop recording animation frames
  sendCommand("/animations/record/stop");
  
  // Hide progress displays after recording stops - they will reappear if save UI is shown
  hideEl("animationProgressTab1");
  hideEl("animationProgressTab3");
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
  hideEl("saveSlotSelector");
}

/** State Management **/

function playFromSlot() {
  // Get the selected animation slot from the play dropdown
  const slot = getEl("playSlot").value;
  
  // Send command to backend to play the selected animation
  sendCommand("/animations/play/" + slot);
  
  // Hide the play selector UI - it will reappear once playback completes and system returns to IDLE
  hideEl("playSlotSelector");
}

function stopPlayback() {
  // Stop playback of the current animation
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
  if (!animationSlots || !Array.isArray(animationSlots)) return;

  const playSelect = getEl("playSlot");
  if (!playSelect) return;

  // Track whether any valid (non-empty) animation slots exist
  let hasValidSlots = false;

  for (let i = 0; i < animationSlots.length; i++) {
    const slot = animationSlots[i];
    const option = playSelect.options[i];
    if (option) {
      // Show animation duration in seconds and enable only if has animation
      const label = slot.hasAnimation ? "#" + slot.id + " (" + slot.animationSeconds.toFixed(1) + "s)" : "Animation " + slot.id + " (empty)";
      option.text = label;
      option.disabled = !slot.hasAnimation;
      
      // If this slot has an animation, mark that we have valid playable content
      if (slot.hasAnimation) {
        hasValidSlots = true;
      }
    }
  }

  // Show or hide the play slot selector based on:
  // 1. Whether valid animations exist AND
  // 2. We are in IDLE mode (not recording or playing back)
  if (hasValidSlots && currentAnimationMode === "IDLE") {
    showEl("playSlotSelector");
  } else {
    hideEl("playSlotSelector");
  }
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
  let frameDisplay = "-";
  let progressValue = "-";
  let actuatorDisplay = "-";
  
  // Only populate progress info when actively recording or playing back
  if (animData.mode === "RECORDING" || animData.mode === "PLAYBACK") {
    // Calculate frame progress: current frame number and percentage completion
    const current = animData.currentFrame || 0;
    const total = animData.totalFrames || 0;  // Use totalFrames for timeline span display
    const percentage = total > 0 ? ((current / total) * 100).toFixed(1) : 0;
    frameDisplay = current + " / " + total + " (" + percentage + "%)";
    
    // Calculate elapsed time: current elapsed time vs total animation duration
    const elapsed = (animData.elapsedSeconds || 0).toFixed(1);
    const duration = (animData.totalTime || 0).toFixed(1);
    progressValue = elapsed + " / " + duration + "s";
    
    // Display which actuator was last triggered (backend provides lastActuator, fall back to frameValue if needed)
    if (animData.lastActuator && animData.lastActuator > 0 && animData.lastActuator <= 4) {
      actuatorDisplay = "Actuator " + animData.lastActuator;
    } else if (animData.frameValue && animData.frameValue > 0 && animData.frameValue <= 4) {
      actuatorDisplay = "Actuator " + animData.frameValue;
    }
  }

  // Build HTML string with frame count, time elapsed, and last actuator triggered
  const html = 
    "<p><b>Frames:</b> " + frameDisplay + "</p>" +
    "<p><b>Time:</b> " + progressValue + "</p>" +
    "<p><b>Last Actuator:</b> " + actuatorDisplay + "</p>";

  return html;
}

// Tracks the previous animation mode to detect state transitions (e.g., RECORDING -> IDLE)
// This allows us to show the save UI only when recording completes, not on every status update
var lastAnimationMode = "IDLE";

function updateAnimationDisplay(animData) {
  // Update the current animation mode for UI state management
  currentAnimationMode = animData.mode || "IDLE";
  
  // Handle IDLE mode: no active recording or playback
  if (animData.mode === "IDLE") {
    // Hide progress displays when animation is not running
    hideEl("animationProgressTab1");
    hideEl("animationProgressTab3");
    
    // Special handling when transitioning FROM recording TO idle (recording just completed)
    if (lastAnimationMode === "RECORDING") {
      if (animData.keyFrames > 0) {
        // Recording completed successfully with captured frames - show save selector so user can save
        showEl("saveSlotSelector");
      } else {
        // Recording completed but no frames captured - hide save UI (nothing to save)
        hideEl("saveSlotSelector");
      }
    }
  } else {
    // RECORDING or PLAYBACK mode: show progress indicators in both tabs
    showEl("animationProgressTab1");
    showEl("animationProgressTab3");

    // Build progress HTML once and display in both tab locations
    const progressHTML = buildAnimationProgressHTML(animData);
    setHtml("animationProgressTab1", progressHTML);
    setHtml("animationProgressTab3", progressHTML);
  }
  
  // Store current mode for next update to detect state transitions
  lastAnimationMode = animData.mode;
}

