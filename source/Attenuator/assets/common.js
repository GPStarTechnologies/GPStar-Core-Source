/**
 *   GPStar Attenuator - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2026 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *                         & Dustin Grau <dustin.grau@gmail.com>
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

/** NOTICE: Some required functions exist in shared JS files dom.js and utils.js **/

/** Common Data Handling **/

function handleStatus(response) {
  // Generic handler for a JSON response with a "status" field.
  // If a response is not JSON then the full text is displayed.
  if (isJsonString(response || "")) {
    var jObj = JSON.parse(response || "");
    if (jObj.status && jObj.status != "success") {
      alert(jObj.status); // Report non-success status.
    }
  } else {
    alert(response); // Display plain text message.
  }
}

/** Common API Commands **/

function sendCommand(apiUri) {
  // Sends an action command to the server (device) using a PUT request.
  // These commands have no response data, so we just handle the status.
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4) {
      handleStatus(this.responseText);
    }
  };
  xhttp.open("PUT", apiUri, true);
  xhttp.send();
}

function packOn() {
  sendCommand("/pack/on");
}

function packOff() {
  sendCommand("/pack/off");
}

function packAttenuate() {
  sendCommand("/pack/attenuate");
}

function packVent() {
  sendCommand("/pack/vent");
}

function cableOn() {
  sendCommand("/pack/cable/on");
}

function cableOff() {
  sendCommand("/pack/cable/off");
}

function volSysUp() {
  sendCommand("/volume/master/up");
}

function volSysDown() {
  sendCommand("/volume/master/down");
}

function volFxUp() {
  sendCommand("/volume/effects/up");
}

function volFxDown() {
  sendCommand("/volume/effects/down");
}

function volMusicUp() {
  sendCommand("/volume/music/up");
}

function volMusicDown() {
  sendCommand("/volume/music/down");
}

function musicStartStop() {
  sendCommand("/music/startstop");
}

function musicPauseResume() {
  sendCommand("/music/pauseresume");
}

function musicSelect(caller) {
  // Change the music track by selected option: /music/select?track=<#>
  sendCommand("/music/select?track=" + caller.value);
}

function musicPrev() {
  sendCommand("/music/prev");
}

function musicNext() {
  sendCommand("/music/next");
}

function handleToggle(el, apiOn, apiOff) {
  if (el._lockout) return;
  el._lockout = true;

  const switchEl = el.parentElement.querySelector(".switch");

  function onTransitionEnd(e) {
    if (e.propertyName === "right") {
      sendCommand(el.checked ? apiOn : apiOff);
      el._lockout = false;
      switchEl.removeEventListener("transitionend", onTransitionEnd);
    }
  }

  switchEl.addEventListener("transitionend", onTransitionEnd);
}

function toggleMute(el) {
  handleToggle(el, "/volume/mute", "/volume/unmute");
}

function musicLoop(el) {
  handleToggle(el, "/music/loop/one", "/music/loop/all");
}

function musicShuffle(el) {
  handleToggle(el, "/music/shuffle/on", "/music/shuffle/off");
}

function toggleSmoke(el) {
  handleToggle(el, "/pack/smoke/on", "/pack/smoke/off");
}

function toggleVibration(el) {
  handleToggle(el, "/pack/vibration/on", "/pack/vibration/off");
}

function cyclotronDirection(el) {
  handleToggle(el, "/pack/cyclotron/clockwise", "/pack/cyclotron/counterclockwise");
}

function themeSelect(caller) {
  // Change the theme via selected option: /pack/theme/<year>
  sendCommand("/pack/theme/" + caller.value);
}

function streamModeSelect(caller) {
  // Change the stream mode via selected option: /pack/stream/<stream_mode>
  sendCommand("/pack/stream/" + caller.value);
}

function getStatus(callbackFunc) {
  // This function expects a JSON response from the server which must be parsed and sent to the callback function.
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status >= 200 && this.status < 300) {
      if (callbackFunc && typeof callbackFunc === "function") {
        // If a callback function is provided, call it with the JSON response.
        callbackFunc(JSON.parse(this.responseText));
      } else {
        // Otherwise display a message that no callback function was provided.
        console.warn("No callback function provided for getStatus response.");
      }
    }
  };
  xhttp.open("GET", "/status", true);
  xhttp.send();
}

function doRestart() {
  // A special command which requires user confirmation before proceeding.
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
