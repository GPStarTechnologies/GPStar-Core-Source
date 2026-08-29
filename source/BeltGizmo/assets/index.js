/**
 *   GPStar BeltGizmo - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2024-2026 Dustin Grau <dustin.grau@gmail.com>
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
  try {
    updateEquipment(JSON.parse(event.data));
  } catch (e) {
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
      try {
        updateNetworkInfo(JSON.parse(data));
      } catch (e) {}
    }
  }
});

window.addEventListener("load", onLoad);

function onLoad(event) {
  document.getElementsByClassName("tablinks")[0].click();
  getDevicePrefs(); // Get all preferences.
  getNetworkInfo(); // Get networking info.
  getStatus(updateEquipment); // Get status immediately.
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
    }
  });
}

function updateBars(iPower, cMode, iTheme) {
  var color = getStreamColor(cMode, iTheme);
  var powerBars = getEl("powerBars");
  if (powerBars) {
    powerBars.innerHTML = ""; // Clear previous bars if any

    if (iPower > 0) {
      for (var i = 1; i <= iPower; i++) {
        var bar = document.createElement("div");
        bar.className = "bar";
        bar.style.backgroundColor = "rgba(" + color[0] + ", " + color[1] + ", " + color[2] + ", 0." + Math.round(i * 1.8, 10) + ")";
        powerBars.appendChild(bar);
      }
    }
  }
}

function updateEquipment(jObj) {
  // Update display if we have the expected data (containing mode and theme at a minimum).
  if (jObj) {
    if (jObj.mode && jObj.theme) {
      // Current Pack Status
      setHtml("mode", jObj.mode || "...");
      setHtml("theme", jObj.theme || "...");
      setHtml("pack", jObj.pack || "...");
      setHtml("switch", jObj.switch || "...");
      setHtml("cable", jObj.cable || "...");
      setHtml("cyclotron", jObj.cyclotron || "...");
      setHtml("temperature", jObj.temperature || "...");

      // Current Wand Status
      setHtml("wandMode", jObj.wandMode || "...");
      setHtml("safety", jObj.safety || "...");
      setHtml("power", jObj.power || "...");
      setHtml("firing", jObj.firing || "...");
      updateBars(jObj.power ?? 0, jObj.wandMode || "", jObj.themeID ?? 0);
    } else {
      // If no mode/theme data, clear everything.
      setHtml("mode", "...");
      setHtml("theme", "...");
      setHtml("pack", "...");
      setHtml("switch", "...");
      setHtml("cable", "...");
      setHtml("cyclotron", "...");
      setHtml("temperature", "...");
      setHtml("wandMode", "...");
      setHtml("safety", "...");
      setHtml("power", "...");
      setHtml("firing", "...");
      updateBars(0, "", 0);
    }

    // External WiFi Status
    if (jObj.extWifiEnabled) {
      setHtml("wifiStatus", jObj.extWifiStarted ? "Connected" : jObj.extWifiPaused ? "Paused" : "Connecting...");
    } else {
      setHtml("wifiStatus", "Disabled");
    }

    // Status of remote WebSocket connection
    setHtml("wsStatus", jObj.extWebSocketState || "...");
    setHtml("wsMessage", jObj.extWebSocketMessage || "");

    // Connected Wifi Clients - Private AP vs. WebSocket
    setHtml("clientInfo", "AP Clients: " + (jObj.apClients ?? 0) + " / WebSocket Clients: " + (jObj.wsClients ?? 0));
  }
}

function testOn() {
  sendCommand("/selftest/enable?power=5");
}

function testOff() {
  sendCommand("/selftest/disable");
}
