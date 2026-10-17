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
  if (!response) {
    // No response provided - typically indicates a network or server error
    console.warn("handleStatus: No response data");
    return; // Error already logged by XHRHelper
  }

  // If a response is not JSON then the original text is displayed.
  if (isJsonString(response || "")) {
    const jObj = JSON.parse(response || "");
    if (jObj.status && jObj.status != "success") {
      alert(jObj.status); // Report non-success status.
    }
  } else {
    alert(response); // Display plain text message.
  }
}

function openTab(evt, tabName) {
  // Hide all tab contents
  const tabs = document.getElementsByClassName("tab");
  for (let i = 0; i < tabs.length; i++) {
    tabs[i].style.display = "none";
  }

  // Remove the active class from all tab links
  const tablinks = document.getElementsByClassName("tablinks");
  for (let i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // Show the current tab and add an "active" class to the button that opened the tab
  showEl(tabName);
  evt.currentTarget.className += " active";
}

function getStreamColor(cMode, iTheme) {
  let color = [0, 0, 0];

  switch (cMode) {
    case "Plasm System":
      if (iTheme == 3) {
        // Pink-Magenta (GB2 Theme)
        color[0] = 230;
        color[1] = 40;
        color[2] = 170;
      } else {
        // Deep Teal-Green
        color[0] = 50;
        color[1] = 120;
        color[2] = 90;
      }
      break;
    case "Dark Matter Gen.":
      // Bright Light Blue
      color[0] = 0;
      color[1] = 100;
      color[2] = 255;
      break;
    case "Particle System":
      // Vibrant Orange
      color[0] = 255;
      color[1] = 165;
      color[2] = 0;
      break;
    case "Settings":
      // Gray
      color[0] = 40;
      color[1] = 40;
      color[2] = 40;
      break;
    case "Halloween":
      // Solid Purple
      color[0] = 220;
      color[1] = 0;
      color[2] = 255;
      break;
    case "Christmas":
      // Bright Red
      color[0] = 255;
      color[1] = 0;
      color[2] = 0;
      break;
    case "Spectral Stream":
      // Default to Cyan
      color[0] = 100;
      color[1] = 200;
      color[2] = 255;
      break;
    case "Custom Stream":
      // Bright White
      color[0] = 255;
      color[1] = 255;
      color[2] = 255;
      break;
    case "Proton Stream":
    default:
      // Default Red
      color[0] = 180;
      color[1] = 0;
      color[2] = 0;
      break;
  }

  return color;
}
