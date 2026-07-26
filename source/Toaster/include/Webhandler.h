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

#pragma once

#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

// Declare the external binary data markers for embedded files.
// common.js
extern const uint8_t _binary_assets_common_js_gz_start[];
extern const uint8_t _binary_assets_common_js_gz_end[];
// favicon.ico
extern const uint8_t _binary_assets_favicon_ico_gz_start[];
extern const uint8_t _binary_assets_favicon_ico_gz_end[];
// favicon.svg
extern const uint8_t _binary_assets_favicon_svg_gz_start[];
extern const uint8_t _binary_assets_favicon_svg_gz_end[];
// style.css
extern const uint8_t _binary_assets_style_css_gz_start[];
extern const uint8_t _binary_assets_style_css_gz_end[];
// index.html
extern const uint8_t _binary_assets_index_html_gz_start[];
extern const uint8_t _binary_assets_index_html_gz_end[];
// index.js
extern const uint8_t _binary_assets_index_js_gz_start[];
extern const uint8_t _binary_assets_index_js_gz_end[];
// device.html
extern const uint8_t _binary_assets_device_html_gz_start[];
extern const uint8_t _binary_assets_device_html_gz_end[];
// network.html
extern const uint8_t _binary_assets_network_html_gz_start[];
extern const uint8_t _binary_assets_network_html_gz_end[];
// password.html
extern const uint8_t _binary_assets_password_html_gz_start[];
extern const uint8_t _binary_assets_password_html_gz_end[];
// swaggerui.html
extern const uint8_t _binary_assets_swaggerui_html_gz_start[];
extern const uint8_t _binary_assets_swaggerui_html_gz_end[];
// help.json
extern const uint8_t _binary_assets_help_json_gz_start[];
extern const uint8_t _binary_assets_help_json_gz_end[];

// Define standard ports and URI endpoints.
const uint16_t WS_PORT = 80; // Web Server (+WebSocket) port
const char WS_URI[] = "/ws"; // WebSocket endpoint URI
bool b_httpd_started = false; // Denotes the web server has been started.

// Define an asynchronous web server at TCP port 80.
AsyncWebServer httpServer(WS_PORT);

// Define a websocket endpoint for the async web server.
AsyncWebSocket ws(WS_URI);

// Create a server-side event source on /events.
AsyncEventSource events("/events");

// Track the number of connected WebSocket clients.
uint8_t i_ws_client_count = 0;

// Track captive portal HTTP endpoint requests.
uint32_t captivePortalRequests = 0;

// Track time to refresh progress for OTA updates.
unsigned long i_progress_millis = 0;

// Create timer for WebSocket cleanup.
millisDelay ms_cleanup;
const uint16_t i_websocketCleanup = 5000;

// Forward function declarations.
void registerWebRoutes(); // From Webrouting.h
void sendDebug(const String& message); // From System.h
bool triggerActuator(ActuatorID actuatorID); // From System.h

/**
 * Scan NVS and populate animation slot cache.
 * Called once on startup and whenever a recording is saved.
 * Checks for animation blob existence and reads frameCount.
 */
void refreshAnimationSlotCache() {
  Preferences preferences;
  
  if (!preferences.begin("animations", true)) {
    // Unable to open namespace - mark all slots as empty
    for (uint8_t i = 0; i < 4; i++) {
      animationSlots[i].id = i;
      animationSlots[i].hasAnimation = false;
      animationSlots[i].frameCount = 0;
    }
    return;
  }

  // Check each animation slot
  for (uint8_t i = 0; i < 4; i++) {
    animationSlots[i].id = i;
    
    AnimationData data;
    size_t size = preferences.getBytes(ANIMATION_NAMES[i], &data, sizeof(data));
    
    if (size == sizeof(data) && data.frameCount > 0 && data.frameCount <= ANIM_MAX_FRAMES) {
      animationSlots[i].hasAnimation = true;
      animationSlots[i].frameCount = data.frameCount;
    } else {
      animationSlots[i].hasAnimation = false;
      animationSlots[i].frameCount = 0;
    }
  }
  
  preferences.end();
}

/**
 * JSON Body Helpers - Creates stringified JSON representations of device configurations
 */

String getDeviceConfig() {
  // Prepare a JSON object with information we have gleaned from the system.
  String equipSettings;
  JsonDocument jsonBody;

  // Provide current values for the device.
  jsonBody["buildDate"] = build_date;
  jsonBody["audioVersion"] = i_audio_version;
  jsonBody["audioCorrupt"] = b_microsd_corrupt;
  jsonBody["audioOutdated"] = b_microsd_outdated;

  // Serialize JSON object to string.
  serializeJson(jsonBody, equipSettings);
  return equipSettings;
}

String getEquipmentStatus() {
  // Prepare a JSON object with information we have gleaned from the system.
  String equipStatus;
  JsonDocument jsonBody;

  uint16_t i_music_track_min = 0;
  uint16_t i_music_track_max = 0;

  if(i_music_track_count > 0) {
    i_music_track_min = i_music_track_start; // First music track possible (eg. 500)
    i_music_track_max = i_music_track_start + i_music_track_count - 1; // 500 + N - 1 to be inclusive of the offset value.
  }

  try {
    jsonBody["musicPlaying"] = b_playing_music;
    jsonBody["musicPaused"] = b_music_paused;
    jsonBody["musicLooping"] = b_repeat_track;
    jsonBody["musicShuffled"] = b_shuffle_tracks;
    jsonBody["musicCurrent"] = i_current_music_track;
    jsonBody["musicStart"] = i_music_track_min;
    jsonBody["musicEnd"] = i_music_track_max;
    jsonBody["volMaster"] = i_volume_master_percentage;
    jsonBody["volEffects"] = i_volume_effects_percentage;
    jsonBody["volMusic"] = i_volume_music_percentage;
  }
  catch (...) {
  }

  // Report on the state of each relay/actuator
  JsonArray relayArray = jsonBody["relays"].to<JsonArray>();

  JsonObject relay1Obj = relayArray.add<JsonObject>();
  relay1Obj["id"] = 1;
  relay1Obj["pin"] = devices.relay1.pin;
  relay1Obj["active"] = devices.relay1.state.relayActive;

  JsonObject relay2Obj = relayArray.add<JsonObject>();
  relay2Obj["id"] = 2;
  relay2Obj["pin"] = devices.relay2.pin;
  relay2Obj["active"] = devices.relay2.state.relayActive;

  JsonObject relay3Obj = relayArray.add<JsonObject>();
  relay3Obj["id"] = 3;
  relay3Obj["pin"] = devices.relay3.pin;
  relay3Obj["active"] = devices.relay3.state.relayActive;

  JsonObject relay4Obj = relayArray.add<JsonObject>();
  relay4Obj["id"] = 4;
  relay4Obj["pin"] = devices.relay4.pin;
  relay4Obj["active"] = devices.relay4.state.relayActive;

  // Report on the state of each RF input button
  JsonArray buttonArray = jsonBody["buttons"].to<JsonArray>();

  JsonObject button1Obj = buttonArray.add<JsonObject>();
  button1Obj["id"] = 1;
  button1Obj["pin"] = devices.button1.pin;
  button1Obj["state"] = devices.button1.state.currentState;

  JsonObject button2Obj = buttonArray.add<JsonObject>();
  button2Obj["id"] = 2;
  button2Obj["pin"] = devices.button2.pin;
  button2Obj["state"] = devices.button2.state.currentState;

  JsonObject button3Obj = buttonArray.add<JsonObject>();
  button3Obj["id"] = 3;
  button3Obj["pin"] = devices.button3.pin;
  button3Obj["state"] = devices.button3.state.currentState;

  JsonObject button4Obj = buttonArray.add<JsonObject>();
  button4Obj["id"] = 4;
  button4Obj["pin"] = devices.button4.pin;
  button4Obj["state"] = devices.button4.state.currentState;

  // Report on animation slot availability (for playback selection)
  JsonArray slotArray = jsonBody["animationSlots"].to<JsonArray>();
  for (uint8_t i = 0; i < 4; i++) {
    JsonObject slotObj = slotArray.add<JsonObject>();
    slotObj["id"] = animationSlots[i].id;
    slotObj["hasAnimation"] = animationSlots[i].hasAnimation;
    slotObj["frameCount"] = animationSlots[i].frameCount;
  }

  // Serialize JSON object to string.
  serializeJson(jsonBody, equipStatus);
  return equipStatus;
}

String getWifiSettings() {
  // Prepare a JSON object with information stored in preferences (or a blank default).
  String wifiSettings;
  JsonDocument jsonBody;

  // Modern ArduinoJson: assign nested object for "active"
  JsonObject active = jsonBody["active"].to<JsonObject>();
  wirelessMgr->getExtWifiNetworkAsJson(active);

  // Modern ArduinoJson: assign nested array for "saved"
  JsonArray saved = jsonBody["saved"].to<JsonArray>();
  String savedNetworks = wirelessMgr->getPreferredNetworks();

  // Parse the saved networks JSON string into a temporary document
  JsonDocument tmpDoc;
  DeserializationError err = deserializeJson(tmpDoc, savedNetworks);
  if(!err && tmpDoc.is<JsonArray>()) {
    for(JsonVariant v : tmpDoc.as<JsonArray>()) {
      saved.add(v);
    }
  }

  // Serialize JSON object to string.
  serializeJson(jsonBody, wifiSettings);
  return wifiSettings;
}

/*
 * Web Handler Functions - Performs actions or returns data for web UI
 */

// Send notification to all websocket clients.
void notifyWSClients() {
  if(b_httpd_started) {
    // Send latest status to all connected clients.
    ws.textAll(getEquipmentStatus());
  }
}

void onWebSocketEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch(type) {
    case WS_EVT_CONNECT:
      #if defined(DEBUG_SEND_TO_CONSOLE)
        debugf("WebSocket[%s][%lu] Connect\n", server->url(), client->id());
      #endif
      i_ws_client_count++;
      notifyWSClients();
    break;

    case WS_EVT_DISCONNECT:
      #if defined(DEBUG_SEND_TO_CONSOLE)
        debugf("WebSocket[%s][C:%lu] Disconnect\n", server->url(), client->id());
      #endif
      if(i_ws_client_count > 0) {
        i_ws_client_count--;
        notifyWSClients();
      }
    break;

    case WS_EVT_ERROR:
      #if defined(DEBUG_SEND_TO_CONSOLE)
        debugf("WebSocket[%s][C:%lu] Error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
      #endif
    break;

    case WS_EVT_PONG:
      #if defined(DEBUG_SEND_TO_CONSOLE)
        debugf("WebSocket[%s][C:%lu] Pong[L:%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
      #endif
    break;

    case WS_EVT_DATA:
      #if defined(DEBUG_SEND_TO_CONSOLE)
        debugf("WebSocket[%s][C:%lu] Data[L:%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
      #endif
    break;
  }
}

void onOTAStart() {
  // Log when OTA has started
  debugln(F("OTA update started"));
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if(millis() - i_progress_millis > 1000) {
    i_progress_millis = millis();
    debugf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if(success) {
    debugln(F("OTA update finished successfully!"));
  }
  else {
    debugln(F("There was an error during OTA update!"));
  }
}

void startWebServer() {
  // Initialize animation slot cache from NVS
  refreshAnimationSlotCache();

  // Register all routes and handlers for the web server.
  registerWebRoutes();

  // Set the MDNS name (get it from your wireless manager)
  setDeviceMdnsName(wirelessMgr->getMdnsName());

  // Set the private IP address for OpenAPI spec (set unique per device)
  setDeviceIpAddress(wirelessMgr->getLocalAddress().toString());

  // Set callback to dynamically retrieve external IP for OpenAPI spec
  setExternalIpCallback([]() -> String {
    return wirelessMgr->getExtWifiAddress().toString();
  });

  // Configures all URI endpoints using registered routes.
  setupRouting(httpServer);

  // Configure the WebSocket endpoint.
  ws.onEvent(onWebSocketEventHandler);
  httpServer.addHandler(&ws);

  // Handle web server Events for telemetry data.
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      debugf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
  });
  httpServer.addHandler(&events);

  // Configure the OTA firmware endpoint handler.
  ElegantOTA.begin(&httpServer);

  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  // Start the web server.
  httpServer.begin();

  // Denote that the web server should be started.
  b_httpd_started = true;

  #if defined(DEBUG_SEND_TO_CONSOLE)
    debugln(F("Async HTTP Server Started"));
  #endif
}

// Perform management if the AP and web server are started.
void webLoops() {
  if(b_local_ap_started && b_httpd_started) {
    // Process DNS requests for captive portal detection via WirelessManager.
    // This must be called frequently to handle incoming DNS queries.
    wirelessMgr->handleDNS();

    if(ms_cleanup.remaining() < 1) {
      // Clean up oldest WebSocket connections.
      ws.cleanupClients();

      // Restart timer for next cleanup action.
      ms_cleanup.start(i_websocketCleanup);
    }

    if(ms_apclient.remaining() < 1) {
      // Update the current count of AP clients.
      i_ap_client_count = WiFi.softAPgetStationNum();

      // Restart timer for next count.
      ms_apclient.start(i_apClientDelay);
    }

    if(ms_otacheck.remaining() < 1) {
      // Handles device reboot after an OTA update.
      ElegantOTA.loop();

      // Restart timer for next check.
      ms_otacheck.start(i_otaCheck);
    }
  }
}

// Send a debug event to connected clients via Server-Sent Events (SSE).
void sendDebugEvent(const char* message) {
  events.send(message, "debug", millis());
}

/**
 * Prepare a JSON object with current animation frame and progress data.
 * Sends real-time updates during recording and playback sessions.
 * 
 * RAW measurements:
 * - mode: Current animation state (IDLE, RECORDING, PLAYBACK)
 * - sourceSlot: Context indicator: -1=fresh recording, 0-3=loaded from slot, 0xFF=idle
 * - totalFrames: Total frame capacity for this animation
 * - currentFrame: Current frame index (0-based)
 * 
 * DERIVED values:
 * - elapsedSeconds: Human-readable elapsed time (currentFrame × 0.1s per frame)
 * - progress: Percentage completion for progress bars (currentFrame / totalFrames × 100)
 */
String getAnimationFrame() {
  String frameData;
  JsonDocument jsonFrame;
  
  const char* modeNames[] = {"IDLE", "RECORDING", "PLAYBACK"};
  jsonFrame["mode"] = modeNames[currentAnimation.mode];
  jsonFrame["sourceSlot"] = currentAnimation.sourceSlot;
  jsonFrame["totalFrames"] = currentAnimation.frameCount;
  
  uint32_t elapsed = millis() - currentAnimation.startTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;
  jsonFrame["currentFrame"] = currentFrame;
  
  // Derive elapsed time in seconds (ANIM_TIME_UNIT_MS = 100ms, so 0.1s per frame)
  jsonFrame["elapsedSeconds"] = (float)currentFrame * 0.1f;
  
  // Derive progress percentage
  if(currentAnimation.frameCount > 0) {
    jsonFrame["progress"] = (float)currentFrame / currentAnimation.frameCount * 100.0f;
  }
  
  // Include which actuator (if any) is firing at current frame
  // Value: 0 = no action, 1-4 = actuator ID
  uint8_t currentActuator = (currentFrame < currentAnimation.frameCount) ? currentAnimation.buffer[currentFrame] : 0;
  jsonFrame["actuator"] = currentActuator;
  
  // Serialize JSON object to string.
  serializeJson(jsonFrame, frameData);
  return frameData;
}

void sendAnimationFrameData() {
  if(b_httpd_started && (currentAnimation.mode == ANIM_RECORDING || currentAnimation.mode == ANIM_PLAYBACK)) {
    // Gather the latest animation frame data, serialize it to a JSON string,
    // and send it to all connected EventSource (SSE) clients as an "animation"
    // event name (using the current ms time as a unique event identifier).
    events.send(getAnimationFrame().c_str(), "animation", millis());
  }
}

/**
 * Standard Page Handlers - Delivers the main web pages and common content
 */

void handleRoot(AsyncWebServerRequest *request) {
  // Used for the root page (/ = index.html) from the web server.
  debugln(F("Sending -> Index HTML"));
  size_t i_file_len = embeddedFileSize(_binary_assets_index_html_gz_start, _binary_assets_index_html_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_HTML, _binary_assets_index_html_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

void handleRootJS(AsyncWebServerRequest *request) {
  // Used for the root page (/ = index.js) from the web server.
  debugln(F("Sending -> Index JavaScript"));
  size_t i_file_len = embeddedFileSize(_binary_assets_index_js_gz_start, _binary_assets_index_js_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JAVASCRIPT, _binary_assets_index_js_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

void handleCommonJS(AsyncWebServerRequest *request) {
  // Used for all pages (common.js) from the web server.
  debugln(F("Sending -> Common JavaScript"));
  size_t i_file_len = embeddedFileSize(_binary_assets_common_js_gz_start, _binary_assets_common_js_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JAVASCRIPT, _binary_assets_common_js_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

void handleStylesheet(AsyncWebServerRequest *request) {
  // Used for the common stylesheet of the web server.
  debugln(F("Sending -> Main StyleSheet"));
  size_t i_file_len = embeddedFileSize(_binary_assets_style_css_gz_start, _binary_assets_style_css_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_CSS, _binary_assets_style_css_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

void handleFavIco(AsyncWebServerRequest *request) {
  // Used for the favicon of the web server.
  debugln(F("Sending -> Favicon"));
  size_t i_file_len = embeddedFileSize(_binary_assets_favicon_ico_gz_start, _binary_assets_favicon_ico_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_ICON, _binary_assets_favicon_ico_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve gzipped .ico file.
}

void handleFavSvg(AsyncWebServerRequest *request) {
  // Used for the favicon of the web server.
  debugln(F("Sending -> Favicon"));
  size_t i_file_len = embeddedFileSize(_binary_assets_favicon_svg_gz_start, _binary_assets_favicon_svg_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_SVG, _binary_assets_favicon_svg_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve gzipped .svg file.
}

void handleContextHelp(AsyncWebServerRequest *request) {
  // Serves the contextual help JSON file for web UI field descriptions.
  debugln(F("Sending -> Help JSON"));
  size_t i_file_len = embeddedFileSize(_binary_assets_help_json_gz_start, _binary_assets_help_json_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JSON, _binary_assets_help_json_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response);
}

void handleNetwork(AsyncWebServerRequest *request) {
  // Used for the network page from the web server.
  debugln(F("Sending -> Network HTML"));
  size_t i_file_len = embeddedFileSize(_binary_assets_network_html_gz_start, _binary_assets_network_html_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_HTML, _binary_assets_network_html_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

void handlePassword(AsyncWebServerRequest *request) {
  // Used for the password page from the web server.
  debugln(F("Sending -> Password HTML"));
  size_t i_file_len = embeddedFileSize(_binary_assets_password_html_gz_start, _binary_assets_password_html_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_HTML, _binary_assets_password_html_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

void handleDeviceSettings(AsyncWebServerRequest *request) {
  // Used for the device page from the web server.
  debugln(F("Sending -> Device Settings HTML"));
  size_t i_file_len = embeddedFileSize(_binary_assets_device_html_gz_start, _binary_assets_device_html_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_HTML, _binary_assets_device_html_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

void handleSwagger(AsyncWebServerRequest *request) {
  // Used for the SwaggerUI page (/ = swaggerui.html) from the web server.
  debugln(F("Sending -> SwaggerUI HTML"));
  size_t i_file_len = embeddedFileSize(_binary_assets_swaggerui_html_gz_start, _binary_assets_swaggerui_html_gz_end);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_HTML, _binary_assets_swaggerui_html_gz_start, i_file_len);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  response->addHeader(HEADER_CONTENT_ENCODING, ENCODING_GZIP); // Tell the client this is gzipped content.
  request->send(response); // Serve page content.
}

/**
 * Peripheral Page Handlers - Delivers the preference pages for available peripherals
 */

void handleGetDeviceConfig(AsyncWebServerRequest *request) {
  // Return current device settings as a stringified JSON object.
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JSON, getDeviceConfig());
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  request->send(response);
}

void handleGetStatus(AsyncWebServerRequest *request) {
  // Return current system status as a stringified JSON object.
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JSON, getEquipmentStatus());
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  request->send(response);
}

void handleGetWifi(AsyncWebServerRequest *request) {
  // Return current system status as a stringified JSON object.
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JSON, getWifiSettings());
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  request->send(response);
}

void handleGetSSIDs(AsyncWebServerRequest *request) {
  // Prepare a JSON object with an array of in-range 2.4 GHz WiFi networks.
  String wifiNetworks;
  String ssidList[40];
  JsonDocument jsonBody;

  // Return available SSIDs (up to 40) as a String array.
  uint8_t i_found = wirelessMgr->scanForSSIDs(ssidList, 40);

  // Make a single array property and add each discovered SSID.
  JsonArray arr = jsonBody["networks"].to<JsonArray>();
  for(uint8_t i = 0; i < i_found; ++i) {
    arr.add(ssidList[i]);
  }

  // Serialize JSON object to string.
  serializeJson(jsonBody, wifiNetworks);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JSON, wifiNetworks);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  request->send(response);
}

void handleGetNetworkStatus(AsyncWebServerRequest *request) {
  // Return network status and statistics including DNS request count and connected clients.
  String statusJson;
  JsonDocument jsonBody;
  JsonObject statusObj = jsonBody.to<JsonObject>();

  // Populate with current network configuration and statistics.
  wirelessMgr->getNetworkStatus(statusObj);

  // Add device-specific client connection counts.
  statusObj["apClients"] = i_ap_client_count;  // WiFi AP clients
  statusObj["wsClients"] = i_ws_client_count;  // WebSocket clients
  statusObj["captivePortalRequests"] = captivePortalRequests;  // HTTP captive portal endpoint hits

  // Serialize JSON object to string.
  serializeJson(jsonBody, statusJson);
  AsyncWebServerResponse *response = request->beginResponse(HTTP_STATUS_200, MIME_JSON, statusJson);
  response->addHeader(HEADER_CACHE_CONTROL, CACHE_NO_CACHE);
  request->send(response);
}

// Handles DELETE /wifi/network/{index} to remove a saved WiFi network by index.
void handleDeleteNetwork(AsyncWebServerRequest *request) {
  int networkIndex = -1;
  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);
      if(segment.length() == 0) {
        request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Missing network index."));
        return;
      }
      networkIndex = segment.toInt();
    }
  }

  int count = wirelessMgr->getPreferredNetworkCount();
  if(networkIndex < 0 || networkIndex >= count) {
    request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Invalid network index."));
    return;
  }

  bool removed = wirelessMgr->removePreferredNetwork((uint8_t)networkIndex);
  if(removed) {
    request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus("Saved network successfully removed."));
  } else {
    request->send(HTTP_STATUS_404, MIME_JSON, returnJsonStatus("Network not found or could not be removed."));
  }
}

void handleRestart(AsyncWebServerRequest *request) {
  // Performs a restart of the device.
  request->send(HTTP_STATUS_204, MIME_JSON, returnJsonStatus());
  delay(1000);
  ESP.restart();
}

/**
 * Connectivity Check Handler
 * Purpose: Intercept OS connectivity checks to signal this is a captive portal without internet.
 * This prevents mobile devices (especially Android) from thinking they have internet access,
 * allowing them to fall back to cellular data for actual internet while staying connected to
 * the device's WiFi for local configuration.
 */
void handleConnectivityCheck(AsyncWebServerRequest *request) {
  // Handle OS-specific connectivity checks.
  // Return exact responses that tell the OS "internet works, dismiss captive portal".
  captivePortalRequests++;

  String path = request->url();

  // Android expects 204 No Content for /generate_204 and /gen_204
  if (path.indexOf("/generate_204") >= 0 || path.indexOf("/gen_204") >= 0) {
    debugln(F("Sending -> 204 No Content (Android connectivity check)"));
    request->send(204);
    return;
  }

  // iOS expects 200 with EXACT HTML format that Apple's server returns
  // This signals "captive portal authenticated, dismiss the view"
  if (path.indexOf("hotspot-detect") >= 0 || path.indexOf("success.html") >= 0) {
    debugln(F("Sending -> Apple Success HTML (iOS connectivity check)"));
    request->send(HTTP_STATUS_200, MIME_HTML,
      F("<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"));
    return;
  }

  // Windows and other endpoints - return Microsoft's expected format
  debugln(F("Sending -> Microsoft Success (Generic connectivity check)"));
  request->send(HTTP_STATUS_200, MIME_PLAIN, F("Microsoft Connect Test"));
}

/**
 * Action Handlers - Perform specific actions via web requests
 */

void handleActuator(AsyncWebServerRequest *request) {
  debugln(F("Web: Actuator Control"));

  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);

      // Check if segment is a valid number (0 is valid, or toInt() returns non-zero)
      if(segment == "0" || segment.toInt() != 0) {
        uint8_t actuatorNum = abs(segment.toInt());

        // Convert 1-4 to ActuatorID enum (0-3)
        if(actuatorNum >= 1 && actuatorNum <= 4) {
          ActuatorID actuator = (ActuatorID)(actuatorNum - 1);
          // Trigger the specified actuator (handles checking the value given).
          triggerActuator(actuator);

          // If currently recording an animation, record this relay trigger at current frame
          if(currentAnimation.mode == ANIM_RECORDING) {
            recordRelayAtCurrentFrame(actuatorNum);  // 1-4
          }

          notifyWSClients();
        }
        else {
          debugln(F("Invalid Actuator (must be 1-4)"));
          request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Invalid Actuator (1-4)"));
          return;
        }
      }
    }
  }
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
}

void handleRestartWiFi(AsyncWebServerRequest *request) {
  // Performs a restart of the external WiFi.

  // Disconnect from the WiFi network and re-apply any changes.
  WiFi.disconnect();
  b_ext_wifi_started = false;
  notifyWSClients();

  delay(100); // Delay needed.

  b_ext_wifi_started = startExternalWifi(); // Restart and set global flag.
  if(b_ext_wifi_started) {
    request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus("WiFi connection restarted successfully."));
  }
  else {
      request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus("WiFi connection was not successful."));
  }
}

void handleToggleMute(AsyncWebServerRequest *request) {
  debugln(F("Web: Toggle Mute"));

  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);
      if(segment == "mute") {
        toggleMute(true);
        notifyWSClients();
        request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
        return;
      }
      else if(segment == "unmute") {
        toggleMute(false);
        notifyWSClients();
        request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
        return;
      }
    }
  }

  debugln(F("Invalid Action"));
  request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Invalid Action")); // 400 Bad Request
}

void handleMasterVolumeUp(AsyncWebServerRequest *request) {
  debugln(F("Web: Master Volume Up"));
  increaseVolume();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleMasterVolumeDown(AsyncWebServerRequest *request) {
  debugln(F("Web: Master Volume Down"));
  decreaseVolume();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleMasterVolumeSet(AsyncWebServerRequest *request) {
  debugln(F("Web: Master Volume Set"));

  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);

      // Check if segment is a valid number (0 is valid, or toInt() returns non-zero)
      if(segment == "0" || segment.toInt() != 0) {
        uint8_t volume = abs(segment.toInt());

        // Validate and constrain to 0-100 range
        if(volume <= 100) {
          // Set volume directly to the specified level
          if(setMasterVolumePercentage(volume)) {
            request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
            notifyWSClients();
            return;
          }
          else {
            debugln(F("Failed to set volume"));
            request->send(HTTP_STATUS_500, MIME_JSON, returnJsonStatus("Failed to set volume"));
            return;
          }
        }
      }
    }
  }

  debugln(F("Invalid Volume Level"));
  request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Invalid Volume Level (0-100)"));
}

void handleEffectsVolumeUp(AsyncWebServerRequest *request) {
  debugln(F("Web: Effects Volume Up"));
  increaseVolumeEffects();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleEffectsVolumeDown(AsyncWebServerRequest *request) {
  debugln(F("Web: Effects Volume Down"));
  decreaseVolumeEffects();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleMusicVolumeUp(AsyncWebServerRequest *request) {
  debugln(F("Web: Music Volume Up"));
  increaseVolumeMusic();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleMusicVolumeDown(AsyncWebServerRequest *request) {
  debugln(F("Web: Music Volume Down"));
  decreaseVolumeMusic();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleMusicStartStop(AsyncWebServerRequest *request) {
  debugln(F("Web: Music Start/Stop"));
  if(b_playing_music) {
    stopMusic();
  }
  else {
    playMusic();
  }
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
}

void handleMusicPauseResume(AsyncWebServerRequest *request) {
  debugln(F("Web: Music Pause/Resume"));
  if(b_playing_music) {
    // If last playing music, either pause or resume.
    if(!b_music_paused) {
      pauseMusic();
    }
    else {
      resumeMusic();
    }
  }
  else {
    // if not playing music, start playing the current track.
    playMusic();
  }
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleNextMusicTrack(AsyncWebServerRequest *request) {
  debugln(F("Web: Next Music Track"));
  musicNextTrack();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handlePrevMusicTrack(AsyncWebServerRequest *request) {
  debugln(F("Web: Prev Music Track"));
  musicPrevTrack();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  notifyWSClients();
}

void handleLoopMusicTrack(AsyncWebServerRequest *request) {
  debugln(F("Web: Toggle Music Track Loop"));

  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);
      if(segment == "one") {
        toggleMusicLoop(true);
        notifyWSClients();
        request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
        return;
      }
      else if(segment == "all") {
        toggleMusicLoop(false);
        notifyWSClients();
        request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
        return;
      }
    }
  }

  debugln(F("Invalid Looping Option"));
  request->send(HTTP_STATUS_400, MIME_JSON, "Invalid Looping Option"); // 400 Bad Request
}

void handleShuffleMusicTracks(AsyncWebServerRequest *request) {
  debugln(F("Web: Toggle Music Track Shuffling"));

  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);
      if(segment == "on") {
        toggleMusicShuffle(true);
        notifyWSClients();
        request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
        return;
      }
      else if(segment == "off") {
        toggleMusicShuffle(false);
        notifyWSClients();
        request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
        return;
      }
    }
  }

  debugln(F("Invalid Shuffle Option"));
  request->send(HTTP_STATUS_400, MIME_JSON, "Invalid Shuffle Option"); // 400 Bad Request
}

void handleSelectMusicTrack(AsyncWebServerRequest *request) {
  String c_music_track = "";

  if(request->hasParam("track")) {
    // Get the parameter "track" if it exists (will be a String).
    c_music_track = request->getParam("track")->value();
  }

  if(c_music_track.toInt() != 0 && c_music_track.toInt() >= i_music_track_start) {
    uint16_t i_music_track = c_music_track.toInt();
    debugln(String(F("Web: Selected Music Track: ")) + String(i_music_track));
    if(i_music_track_count > 0 && i_music_track >= i_music_track_start) {
      if(b_playing_music) {
        stopMusic(); // Stops current track before change.

        // Only update after the music is stopped.
        i_current_music_track = i_music_track;

        // Play the appropriate track on pack and wand, and notify the Attenuator.
        playMusic();
      }
      else {
        i_current_music_track = i_music_track;
      }
    }
    request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus());
  }
  else {
    // Tell the user why the requested action failed.
    request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Invalid track number requested")); // 400 Bad Request
  }
}

/**
 * Animation Control Handlers - Manage recording and playback of relay sequences
 */

void handleRecordStart(AsyncWebServerRequest *request) {
  debugln(F("Web: Animation Record Start"));
  startRecording();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus("Recording started"));
}

void handleRecordStop(AsyncWebServerRequest *request) {
  debugln(F("Web: Animation Record Stop"));

  if (currentAnimation.mode != ANIM_RECORDING) {
    request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Not currently recording"));
    return;
  }

  uint16_t frameCount = stopRecording();

  JsonDocument jsonResponse;
  jsonResponse["status"] = "success";
  jsonResponse["message"] = "Recording stopped";
  jsonResponse["frameCount"] = frameCount;

  String response;
  serializeJson(jsonResponse, response);
  request->send(HTTP_STATUS_200, MIME_JSON, response);
}

void handleRecordSave(AsyncWebServerRequest *request) {
  debugln(F("Web: Animation Record Save"));

  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);

      // Check if segment is a valid number (0 is valid, or toInt() returns non-zero)
      if(segment == "0" || segment.toInt() != 0) {
        uint8_t animIndex = abs(segment.toInt());

        if (animIndex >= ANIM_MAX_STORED) {
          request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Invalid animation index (0-3)"));
          return;
        }

        if (saveRecordingToNVS(animIndex)) {
          JsonDocument jsonResponse;
          jsonResponse["status"] = "success";
          jsonResponse["message"] = "Animation saved";
          jsonResponse["slot"] = animIndex;
          jsonResponse["name"] = ANIMATION_NAMES[animIndex];

          String response;
          serializeJson(jsonResponse, response);
          request->send(HTTP_STATUS_200, MIME_JSON, response);
        } else {
          request->send(HTTP_STATUS_500, MIME_JSON, returnJsonStatus("Failed to save animation to NVS"));
        }
        return;
      }
    }
  }
  request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Missing or invalid path parameter: id (0-3)"));
}

void handlePlayAnimation(AsyncWebServerRequest *request) {
  debugln(F("Web: Animation Play"));

  String s_path = request->url();
  if(s_path.length() > 0) {
    int lastSlash = s_path.lastIndexOf('/');
    if(lastSlash >= 0 && lastSlash < s_path.length() - 1) {
      String segment = s_path.substring(lastSlash + 1);

      // Check if segment is a valid number (0 is valid, or toInt() returns non-zero)
      if(segment == "0" || segment.toInt() != 0) {
        uint8_t animIndex = abs(segment.toInt());

        if (animIndex >= ANIM_MAX_STORED) {
          request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Invalid animation index (0-3)"));
          return;
        }

        if (startPlayback(animIndex)) {
          JsonDocument jsonResponse;
          jsonResponse["status"] = "success";
          jsonResponse["message"] = "Playback started";
          jsonResponse["slot"] = animIndex;
          jsonResponse["frameCount"] = currentAnimation.frameCount;

          String response;
          serializeJson(jsonResponse, response);
          request->send(HTTP_STATUS_200, MIME_JSON, response);
        } else {
          request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Failed to load animation or invalid animation slot"));
        }
        return;
      }
    }
  }
  request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Missing or invalid path parameter: id (0-3)"));
}

void handleStopAnimation(AsyncWebServerRequest *request) {
  debugln(F("Web: Animation Stop"));

  if (currentAnimation.mode != ANIM_PLAYBACK) {
    request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("No animation currently playing"));
    return;
  }

  stopPlayback();
  request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus("Playback stopped"));
}

void handleAnimationStatus(AsyncWebServerRequest *request) {
  debugln(F("Web: Animation Status"));

  const char* modeNames[] = {"IDLE", "RECORDING", "PLAYBACK"};

  JsonDocument jsonResponse;
  jsonResponse["mode"] = modeNames[currentAnimation.mode];
  jsonResponse["frameCount"] = currentAnimation.frameCount;

  if (currentAnimation.mode == ANIM_PLAYBACK) {
    uint32_t elapsed = millis() - currentAnimation.startTime;
    uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;
    jsonResponse["currentFrame"] = currentFrame;
    jsonResponse["progress"] = (currentAnimation.frameCount > 0) ?
      (float)currentFrame / currentAnimation.frameCount * 100.0 : 0.0;
  }

  String response;
  serializeJson(jsonResponse, response);
  request->send(HTTP_STATUS_200, MIME_JSON, response);
}

/**
 * Body Handler Methods - These handlers process JSON body content from POST requests
 */

// Handles the JSON body for the device settings save request.
AsyncCallbackJsonWebHandler *handleSaveDeviceConfig = new AsyncCallbackJsonWebHandler("/config/device/save", [](AsyncWebServerRequest *request, JsonVariant &json) {
  JsonDocument jsonBody;
  if(json.is<JsonObject>()) {
    jsonBody = json.as<JsonObject>();
  } else {
    debugln(F("Body was not a JSON object"));
  }

  try {
    // First check if a new private WiFi network name has been chosen.
    String newSSID = jsonBody["wifiName"].as<String>();
    newSSID = sanitizeSSID(newSSID); // Jacques, clean him!
    bool b_ssid_changed = false;

    // Update the private network name ONLY if the new value differs from the current SSID.
    if(newSSID != "" && newSSID != wirelessMgr->getLocalNetworkName()){
      if(newSSID.length() >= 8 && newSSID.length() <= 32) {
        // Create Preferences object to handle non-volatile storage (NVS).
        Preferences preferences;

        // Accesses namespace in read/write mode.
        if(preferences.begin("credentials", false)) {
          #if defined(DEBUG_SEND_TO_CONSOLE)
            debugln(F("New Private SSID: "));
            debugln(newSSID);
          #endif
          preferences.putString("ssid", newSSID); // Store SSID in case this was altered.
          preferences.end();
        }

        b_ssid_changed = true; // This will cause a reboot of the device after saving.
      }
      else {
        // Immediately return an error if the network name was invalid.
        request->send(HTTP_STATUS_400, MIME_JSON, returnJsonStatus("Error: Network name must be between 8 and 32 characters in length.")); // 400 Bad Request
      }
    }

    // Create Preferences object to handle non-volatile storage (NVS).
    Preferences preferences;

    // Accesses namespace in read/write mode.
    // if(preferences.begin("device", false)) {
    //   preferences.end();
    // }

    if(b_ssid_changed) {
      request->send(HTTP_STATUS_201, MIME_JSON, returnJsonStatus("Settings updated, restart required. Please use the new network name to connect to your device."));
    }
    else {
      request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus("Settings updated."));
    }
  }
  catch (...) {
    request->send(HTTP_STATUS_500, MIME_JSON, returnJsonStatus("An error was encountered while saving settings.")); // 500 Server Error
  }
}); // handleSaveDeviceConfig

// Handles the JSON body for the password change request.
AsyncCallbackJsonWebHandler *passwordChangeHandler = new AsyncCallbackJsonWebHandler("/password/update", [](AsyncWebServerRequest *request, JsonVariant &json) {
  JsonDocument jsonBody;
  if(json.is<JsonObject>()) {
    jsonBody = json.as<JsonObject>();
  } else {
    debugln(F("Body was not a JSON object"));
  }

  if(jsonBody["password"].is<const char*>()) {
    String newPasswd = jsonBody["password"].as<String>();

    // Password is used for the built-in Access Point ability, which will be used when a preferred network is not available.
    if(newPasswd.length() >= 8) {
      // Create Preferences object to handle non-volatile storage (NVS).
      Preferences preferences;

      // Accesses namespace in read/write mode.
      if(preferences.begin("credentials", false)) {
        #if defined(DEBUG_SEND_TO_CONSOLE)
          debug(F("New Private WiFi Password: "));
          debugln(newPasswd);
        #endif
        preferences.putString("password", newPasswd); // Store user-provided password.
        preferences.end();
      }

      request->send(HTTP_STATUS_201, MIME_JSON, returnJsonStatus("Password updated, restart required. Please enter your new WiFi password when prompted by your device."));
    }
    else {
      // Password must be at least 8 characters in length.
      request->send(HTTP_STATUS_417, MIME_JSON, returnJsonStatus("Password must be a minimum of 8 characters to meet WPA2 requirements.")); // 417 Expectation Failed
    }
  }
  else {
    debugln(F("No password in JSON body"));
    request->send(HTTP_STATUS_500, MIME_JSON, returnJsonStatus("Unable to update password.")); // 500 Server Error
  }
}); // passwordChangeHandler

// Handles the JSON body for the wifi network info.
AsyncCallbackJsonWebHandler *wifiChangeHandler = new AsyncCallbackJsonWebHandler("/wifi/update", [](AsyncWebServerRequest *request, JsonVariant &json) {
  JsonDocument jsonBody;
  if(json.is<JsonObject>()) {
    jsonBody = json.as<JsonObject>();
  } else {
    debugln(F("Body was not a JSON object"));
  }

  // Check for 'active' property (object) and use it if present, else use top-level
  JsonObject activeObj;
  if(jsonBody["active"].is<JsonObject>()) {
    activeObj = jsonBody["active"].as<JsonObject>();
  } else {
    debugln(F("No 'active' object in JSON body"));
    request->send(HTTP_STATUS_204, MIME_JSON, returnJsonStatus("Unable to find expected network information in JSON body.")); // 204 No Content
    return;
  }

  if(activeObj["ssid"].is<const char*>() && activeObj["password"].is<const char*>()) {
    bool b_errors = false; // Assume false until otherwise indicated.
    bool b_enabled = wirelessMgr->isExtWifiEnabled(); // Default to the current state.
    updateJsonBool(b_enabled, activeObj, "enabled"); // Update var from JSON if present.
    String wifiNetwork = activeObj["ssid"].as<String>();
    String wifiPasswd = activeObj["password"].as<String>();

    // Handle staticIP logic: if false, blank the fields; if true, use provided string values if present
    bool b_static_ip = false;
    String localAddr = "";
    String subnetMask = "";
    String gatewayIP = "";

    if(activeObj["staticIP"].is<bool>()) {
      b_static_ip = activeObj["staticIP"].as<bool>();
    }

    if(b_static_ip) {
      if(activeObj["address"].is<const char*>()) {
        localAddr = activeObj["address"].as<String>();
      }
      if(activeObj["subnet"].is<const char*>()) {
        subnetMask = activeObj["subnet"].as<String>();
      }
      if(activeObj["gateway"].is<const char*>()) {
        gatewayIP = activeObj["gateway"].as<String>();
      }
    }

    if(!b_enabled) {
      // If disabled, update the stored preference immediately.
      wirelessMgr->disableExtWiFi();
    } else {
      // Check validity of provided values.
      if(wifiNetwork.length() >= 2 && wifiPasswd.length() >= 8) {
        // Clear old network IP info if SSID or password have been changed.
        String old_ssid = wirelessMgr->getExtWifiNetworkName();
        String old_passwd = wirelessMgr->getExtWifiPassword();
        if(old_ssid == "" || old_ssid != wifiNetwork || old_passwd == "" || old_passwd != wifiPasswd) {
          localAddr = "";
          subnetMask = "";
          gatewayIP = "";
        }

        // Continue saving static IP info only if network values are 7 characters or more (eg. N.N.N.N)
        bool b_valid_ip = true;
        if(!(localAddr.length() >= 7 && localAddr != wirelessMgr->getExtWifiAddress().toString())) {
          b_valid_ip = false;
        }
        if(!(subnetMask.length() >= 7 && subnetMask != wirelessMgr->getExtWifiSubnet().toString())) {
          b_valid_ip = false;
        }
        if(!(gatewayIP.length() >= 7 && gatewayIP != wirelessMgr->getExtWifiGateway().toString())) {
          b_valid_ip = false;
        }

        if(!b_valid_ip) {
          // If any of the above values were invalid, clear all three fields.
          localAddr = "";
          subnetMask = "";
          gatewayIP = "";
        }

        // Save and apply the new values as the current external network.
        if(wirelessMgr->savePreferredNetwork(wifiNetwork, wifiPasswd, b_static_ip, localAddr, subnetMask, gatewayIP)) {
          int8_t idx = wirelessMgr->getPreferredNetworkIndex(wifiNetwork);
          if(idx >= 0) {
            if(!wirelessMgr->applyPreferredNetwork((uint8_t)idx)) {
              request->send(HTTP_STATUS_500, MIME_JSON, returnJsonStatus("Unable to apply settings for the current network."));
              return;
            }
          }
          else {
            request->send(HTTP_STATUS_500, MIME_JSON, returnJsonStatus("Unable to locate the preferred network information."));
            return;
          }
        }
        else {
          request->send(HTTP_STATUS_500, MIME_JSON, returnJsonStatus("Unable to save preferred network, check total saved networks (must be 5 or less)."));
          return;
        }
      }
      else {
        b_errors = true; // General error for invalid SSID or password length.
      }
    }

    if(!b_errors) {
      // Disconnect from the WiFi network and re-apply any changes.
      WiFi.disconnect();
      b_ext_wifi_started = false;
      notifyWSClients();

      delay(100); // Delay needed.

      String s_reason = "";
      if(b_enabled) {
        b_ext_wifi_started = startExternalWifi(); // Restart and set global flag.

        if(b_ext_wifi_started) {
          s_reason = "Settings updated, WiFi connection restarted successfully.";
        }
        else {
          s_reason = "Settings updated, but WiFi connection was not successful.";
        }
      }
      else {
        s_reason = "Settings updated, and external WiFi has been disconnected.";
      }

      request->send(HTTP_STATUS_201, MIME_JSON, returnJsonStatus(s_reason));
    }
    else {
      request->send(HTTP_STATUS_200, MIME_JSON, returnJsonStatus("Errors encountered while processing data. Please re-check submitted values and try again."));
    }
  }
  else {
    debugln(F("No password in JSON body"));
    request->send(HTTP_STATUS_204, MIME_JSON, returnJsonStatus("Unable to update password.")); // 204 No Content
  }
}); // wifiChangeHandler
