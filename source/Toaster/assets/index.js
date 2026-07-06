/**
 *   GPStar Toaster - Ghostbusters Props, Mods, and Kits.
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

var websocket;
var statusInterval;

window.addEventListener("load", onLoad);

function onLoad(event) {
  document.getElementsByClassName("tablinks")[0].click();
  getDevicePrefs(); // Get all preferences.
  initWebSocket(); // Open the WebSocket.
  getStatus(updateEquipment); // Get status immediately.
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

        setHtml("wifiName", "Private Network: " + jObj.wifiName || "");
        if ((jObj.wifiNameExt || "") != "" && (jObj.extAddr || "") != "" && (jObj.extMask || "") != "") {
          setHtml("extWifi", (jObj.wifiNameExt || "") + ": " + jObj.extAddr + " / " + jObj.extMask);
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

function updateEquipment(jObj) {
  // Update display if we have the expected data (containing mode and theme at a minimum).
  if (jObj) {
    // Status of remote WebSocket connection
    setHtml("wsStatus", jObj.extWebSocketState || "...");
    setHtml("wsMessage", jObj.extWebSocketMessage || "");

    // Connected Wifi Clients - Private AP vs. WebSocket
    setHtml("clientInfo", "AP Clients: " + (jObj.apClients ?? 0) + " / WebSocket Clients: " + (jObj.wsClients ?? 0));

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
