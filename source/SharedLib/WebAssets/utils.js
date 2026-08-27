/** General Utility Functions - Device-agnostic helper functions **/

function isJsonString(str) {
  try {
    JSON.parse(str);
  } catch (e) {
    return false;
  }
  return true;
}

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

function openTab(evt, tabName) {
  // Hide all tab contents
  var tabs = document.getElementsByClassName("tab");
  for (var i = 0; i < tabs.length; i++) {
    tabs[i].style.display = "none";
  }

  // Remove the active class from all tab links
  var tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // Show the current tab and add an "active" class to the button that opened the tab
  showEl(tabName);
  evt.currentTarget.className += " active";
}

function getStreamColor(cMode, iTheme, iCustomVal = 200, iCustomSat = 254) {
  var color = [0, 0, 0];

  // Use this to do our colour-change for spectral streams.
  var tickSeconds = new Date().getSeconds();

  switch (cMode) {
    case "Plasm System":
      if (iTheme == 3) {
        // Pink
        color[0] = 200;
        color[2] = 180;
      } else {
        // Dark Green
        color[1] = 80;
      }
      break;
    case "Dark Matter Gen.":
      // Light Blue
      color[1] = 60;
      color[2] = 255;
      break;
    case "Particle System":
      // Orange
      color[0] = 255;
      color[1] = 140;
      break;
    case "Settings":
      // Gray
      color[0] = 40;
      color[1] = 40;
      color[2] = 40;
      break;
    case "Halloween":
      if (tickSeconds % 2) {
        // Orange
        color[0] = 255;
        color[1] = 140;
      } else {
        // Purple
        color[0] = 200;
        color[2] = 240;
      }
      break;
    case "Christmas":
      if (tickSeconds % 2) {
        // Red
        color[0] = 180;
      } else {
        // Green
        color[1] = 180;
      }
      break;
    case "Spectral Stream":
      switch (tickSeconds % 8) {
        case 0:
        default:
          // Red
          color[0] = 180;
          break;
        case 1:
          // Orange
          color[0] = 255;
          color[1] = 140;
          break;
        case 2:
          // Yellow
          color[0] = 240;
          color[1] = 220;
          break;
        case 3:
          // Green
          color[1] = 180;
          break;
        case 4:
          // Light Blue
          color[1] = 60;
          color[2] = 255;
          break;
        case 5:
          // Blue
          color[2] = 180;
          break;
        case 6:
          // Indigo
          color[0] = 90;
          color[2] = 240;
          break;
        case 7:
          // Purple
          color[0] = 200;
          color[2] = 240;
          break;
      }
      break;
    case "Custom Stream":
    default:
      // Proton Stream(s) as Red
      color[0] = 180;
      break;
  }

  return color;
}
