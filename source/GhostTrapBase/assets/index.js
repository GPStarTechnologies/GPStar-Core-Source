/**
 *   GPStar Ghost Trap - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2025 Michael Rajotte <contact@gpstartechnologies.com
 *                    & Nomake Wan <nomake_wan@yahoo.co.jp>
 *                    & Dustin Grau <dustin.grau@gmail.com>
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

// Initialize connection wrappers
const wsClient = new WebSocketClient({
  onMessage: onWSMessage
});

function onWSMessage(event) {
  if (isJsonString(event.data)) {
    updateEquipment(JSON.parse(event.data));
  } else {
    console.log(event.data);
  }
}

const esManager = new EventSourceManager({
  eventHandlers: {
    'debug': (data) => {
      if (data === undefined) return;
      console.log("Debug: ", data);
    },
    'network': (data) => {
      updateNetworkInfo(data);
    }
  }
});

window.addEventListener("load", onLoad);

function onLoad(event) {
  document.getElementsByClassName("tablinks")[0].click();
  getDevicePrefs(); // Get all preferences.
  getNetworkInfo(); // Get networking info.
  getStatus(); // Get status immediately.
  wsClient.connect(); // Open the WebSocket.
  esManager.connect(); // Start EventSource connection.
  
  // Cleanup on page unload
  window.addEventListener("beforeunload", () => {
    wsClient.disconnect();
    esManager.disconnect();
  });
}



function getDevicePrefs() {
  // This is updated once per page load as it is not subject to frequent changes.
  xhrHelper.get("/config/device", (jObj) => {
    if (jObj) {
      // Device Info
      setHtml("buildDate", "Build: " + (jObj.buildDate || ""));

      switch (jObj.audioVersion ?? 0) {
        case 0:
        case 1:
          setHtml("audioInfo", "No Audio Detected");
          break;
        case 100:
          setHtml("audioInfo", "GPStar Audio Firmware: v100");
          break;
        default:
          setHtml("audioInfo", "GPStar Audio Firmware: v" + (jObj.audioVersion || ""));
          break;
      }

      // Display Preference
      switch (jObj.displayType ?? 0) {
        case 0:
          // Text-Only Display
          hideEl("equipCRT");
          showEl("equipTXT");
          break;
        case 1:
          // Graphical Display
          showEl("equipCRT");
          hideEl("equipTXT");
          break;
        case 2:
          // Both graphical and text
          showEl("equipCRT");
          showEl("equipTXT");
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
  });
}

function setButtonStates(smokeEnabled) {
  // Assume all functions are not possible, override as necessary.
  disableEl("btnSmoke2");
  disableEl("btnSmoke5");
  disableEl("btnSmokeEnable");
  disableEl("btnSmokeDisable");

  if (smokeEnabled) {
    // Enable specific buttons only when smoke is enabled.
    enableEl("btnSmoke2");
    enableEl("btnSmoke5");
    enableEl("btnSmokeDisable");
  } else {
    // Otherwise, make sure the user can re-enable smoke.
    enableEl("btnSmokeEnable");
  }
}

function updateGraphics(jObj) {
  // Update display if we have the expected data (containing door state at a minimum).
  if (jObj && jObj.doorState) {
    if (jObj.doorState == "Opened") {
      colorEl("doorOverlay", 0, 150, 0);
    } else {
      colorEl("doorOverlay", 255, 0, 0);
    }
  } else {
    // Reset all screen elements to their defaults to indicate no data available.
    colorEl("doorOverlay", 100, 100, 100);
  }
}

function updateEquipment(jObj) {
  // Update display if we have the expected data (containing door state at a minimum).
  if (jObj && jObj.doorState) {
    // Current Pack Status
    setHtml("doorState", jObj.doorState || "...");

    // Update special UI elements based on the latest data values.
    setButtonStates(jObj.smokeEnabled);

    updateGraphics(jObj);
  }
}

function getStatus() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status >= 200 && this.status < 300) {
      // Update the equipment (text) display, which will also update graphical elements.
      updateEquipment(JSON.parse(this.responseText));
    } else if (this.readyState == 4) {
      // Handle error responses
      handleStatus(this.responseText);
    }
  };
  xhttp.open("GET", "/status", true);
  xhttp.send();
}

function doRestart() {
  if (confirm("Are you sure you wish to restart the serial device?")) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status >= 200 && this.status < 300) {
        // Reload the page after 2 seconds.
        setTimeout(function () {
          window.location.reload();
        }, 2000);
      }
    };
    xhttp.open("DELETE", "/restart", true);
    xhttp.send();
  }
}

function sendCommand(apiUri) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4) {
      handleStatus(this.responseText);
    }
  };
  xhttp.open("PUT", apiUri, true);
  xhttp.send();
}

function runSmoke(msDuration) {
  sendCommand("/smoke/run?duration=" + parseInt(msDuration, 10));
}

function enableSmoke() {
  sendCommand("/smoke/enable");
}

function disableSmoke() {
  sendCommand("/smoke/disable");
}

function lightOn() {
  sendCommand("/light/on");
}

function lightOff() {
  sendCommand("/light/off");
}
