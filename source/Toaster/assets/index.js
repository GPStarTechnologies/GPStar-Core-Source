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

window.addEventListener("load", onLoad);

function onLoad(event) {
  document.getElementsByClassName("tablinks")[0].click();
  getDevicePrefs(); // Get all preferences.
  getNetworkInfo(); // Get networking info.
  initWebSocket(); // Open the WebSocket.
  getStatus(updateEquipment); // Get status immediately.

  // Initialize animation controls visibility
  hideEl("recSaveButton");
  hideEl("recPlayButton");
  hideEl("saveSlotSelector");
  hideEl("playSlotSelector");
  hideEl("animationProgress");

  // Initialize EventSource for animation updates
  initAnimationEventSource();
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
      getStatus(updateEquipment); // Check for status every X seconds
    }, 1000);
  }
}

function onMessage(event) {
  if (isJsonString(event.data)) {
    // If JSON, use as status update.
    updateEquipment(JSON.parse(event.data));
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

function updateEquipment(jObj) {
  // Update display if we have the expected data (containing mode and theme at a minimum).
  if (jObj) {
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
      animationSlots = jObj.animationSlots;
      updateSaveSlots();
      updatePlaySlots();
    }
  }
}

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
  sendCommand("/animations/record/start");
  // Show the save button after recording starts
  showEl("recSaveButton");
}

function recordingStop() {
  sendCommand("/animations/record/stop");
  // Hide the save button after recording stops
  hideEl("recSaveButton");
  // Always show the play button
  showEl("recPlayButton");
}

function showSaveSlotSelector() {
  showEl("saveSlotSelector");
  hideEl("recSaveButton");
}

function cancelSaveSlot() {
  hideEl("saveSlotSelector");
  showEl("recSaveButton");
}

function recordingSaveToSlot() {
  const slot = parseInt(getEl("saveSlot").value);

  // Check if the selected slot already has an animation
  const slotInfo = animationSlots.find((s) => s.id === slot);
  if (slotInfo && slotInfo.hasAnimation) {
    // Confirm before overwriting
    const confirmMsg = "Slot " + slot + " already has an animation (" + slotInfo.frameCount + " frames). Overwrite?";
    if (!confirm(confirmMsg)) {
      return; // User cancelled
    }
  }

  sendCommand("/animations/record/save/" + slot);
  hideEl("saveSlotSelector");
}

function showPlaySlotSelector() {
  showEl("playSlotSelector");
  hideEl("recPlayButton");
}

function cancelPlaySlot() {
  hideEl("playSlotSelector");
  showEl("recPlayButton");
}

function playAnimationFromSlot() {
  const slot = getEl("playSlot").value;
  sendCommand("/animations/play/" + slot);
  hideEl("playSlotSelector");
}

function initAnimationEventSource() {
  // Create EventSource connection to receive animation updates from server
  const eventSource = new EventSource("/events");

  // Listen for animation frame updates
  eventSource.addEventListener("animation", function (event) {
    if (isJsonString(event.data)) {
      const animData = JSON.parse(event.data);
      updateAnimationDisplay(animData);
    }
  });

  // Handle connection errors
  eventSource.onerror = function (event) {
    console.log("Animation EventSource error", event);
  };
}

function buildAnimationProgressHTML(animData) {
  // Build mode display with context based on sourceSlot
  let modeDisplay = animData.mode;
  if (animData.sourceSlot === -1) {
    modeDisplay = "Recording (unsaved)";
  } else if (animData.sourceSlot >= 0 && animData.sourceSlot <= 3) {
    if (animData.mode === "PLAYBACK") {
      modeDisplay = "Playing Slot " + animData.sourceSlot;
    } else if (animData.mode === "RECORDING") {
      modeDisplay = "Recording → Slot " + animData.sourceSlot;
    }
  }

  let progressValue = "—";
  if (animData.progress !== undefined) {
    progressValue = animData.progress.toFixed(1);
  }

  let actuatorDisplay = "—";
  if (animData.actuator && animData.actuator > 0 && animData.actuator <= 4) {
    actuatorDisplay = "Actuator " + animData.actuator;
  }

  const html = 
    "<p>Mode: " + modeDisplay + "</p>" +
    "<p>Frame: " + (animData.currentFrame || 0) + " / " + (animData.totalFrames || 0) + "</p>" +
    "<p>Progress: " + progressValue + "%</p>" +
    "<p>Elapsed: " + (animData.elapsedSeconds || 0).toFixed(1) + "s</p>" +
    "<p>Actuator: " + actuatorDisplay + "</p>";

  return html;
}

function updateAnimationDisplay(animData) {
  // Show or hide the progress display based on mode
  if (animData.mode === "IDLE") {
    hideEl("animationProgressTab1");
    hideEl("animationProgressTab3");
  } else {
    showEl("animationProgressTab1");
    showEl("animationProgressTab3");

    // Build HTML once and inject into both display locations
    const progressHTML = buildAnimationProgressHTML(animData);
    setHtml("animationProgressTab1", progressHTML);
    setHtml("animationProgressTab3", progressHTML);
  }
}

function updateSaveSlots() {
  // Update save slot dropdown - all slots always enabled for save
  if (!animationSlots || !Array.isArray(animationSlots)) return;

  const saveSelect = getEl("saveSlot");
  if (!saveSelect) return;

  // Slots are always available for saving (shows frame count if already has data)
  for (let i = 0; i < animationSlots.length; i++) {
    const slot = animationSlots[i];
    const option = saveSelect.options[i];
    if (option) {
      // Update option label to show frameCount if it has data
      const label = slot.hasAnimation ? "Slot " + slot.id + " (" + slot.frameCount + " frames)" : "Slot " + slot.id;
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

  for (let i = 0; i < animationSlots.length; i++) {
    const slot = animationSlots[i];
    const option = playSelect.options[i];
    if (option) {
      // Show frameCount and enable only if has animation
      const label = slot.hasAnimation ? "Animation " + slot.id + " (" + slot.frameCount + " frames)" : "Animation " + slot.id + " (empty)";
      option.text = label;
      option.disabled = !slot.hasAnimation;
    }
  }

  // Disable Play button if no slots have animations
  const hasAnySaved = animationSlots.some((s) => s.hasAnimation);
  const playButton = getEl("recPlayButton");
  if (playButton) {
    playButton.disabled = !hasAnySaved;
  }
}
