/** Connection Overlay - Prevents interaction when device is disconnected */

function initConnectionOverlay() {
  // Create overlay element if it doesn't exist
  if (!document.getElementById('connectionOverlay')) {
    const overlay = document.createElement('div');
    overlay.id = 'connectionOverlay';
    overlay.className = 'connection-overlay';
    
    const content = document.createElement('div');
    content.className = 'connection-overlay-content';
    content.id = 'connectionOverlayContent';
    content.innerHTML = '<p>Reconnecting...</p>';

    overlay.appendChild(content);

    // Prevent any events from bubbling through and block default actions
    overlay.addEventListener('click', (e) => { e.preventDefault(); e.stopPropagation(); }, { capture: true, passive: false });
    overlay.addEventListener('touchstart', (e) => { e.preventDefault(); e.stopPropagation(); }, { capture: true, passive: true });
    overlay.addEventListener('touchend', (e) => { e.preventDefault(); e.stopPropagation(); }, { capture: true, passive: false });
    overlay.addEventListener('pointerdown', (e) => { e.preventDefault(); e.stopPropagation(); }, { capture: true, passive: false });
    overlay.addEventListener('wheel', (e) => { e.preventDefault(); e.stopPropagation(); }, { capture: true, passive: true });
    
    document.body.appendChild(overlay);
  }
}

function showConnectionOverlay(attemptNumber = null) {
  initConnectionOverlay();
  const overlay = document.getElementById('connectionOverlay');
  const content = document.getElementById('connectionOverlayContent');
  
  if (attemptNumber !== null) {
    content.innerHTML = `<p>Reconnecting...</p><p class="connection-overlay-attempt">Attempt ${attemptNumber}</p>`;
  } else {
    content.innerHTML = '<p>Reconnecting...</p>';
  }

  if (overlay) {
    overlay.classList.add('active');
  }
}

function hideConnectionOverlay() {
  const overlay = document.getElementById('connectionOverlay');
  if (overlay) {
    overlay.classList.remove('active');
  }
}

/** WebSocket Client - Handles WebSocket connections with exponential backoff and timeout */
class WebSocketClient {
  // Private fields
  #hostname;                      // Device hostname
  #wsPath;                        // WebSocket path
  #connectionTimeout;             // Connection timeout in ms
  #onOpen;                        // Callback when connection opens
  #onClose;                       // Callback when connection closes
  #onError;                       // Callback on connection error
  #onMessage;                     // Callback when message received
  #websocket = null;              // WebSocket instance
  #isConnected = false;           // True if WebSocket is open and ready
  #hasEverConnected = false;      // Track if connection ever succeeded (for overlay)
  #wsHeartbeatTimer = null;       // Periodic heartbeat send timer
  #heartbeatMissCount = 0;        // Count consecutive missed pongs (reconnect on 2nd)
  #wsConnectionAttempts = 0;      // Track attempts for exponential backoff
  #wsReconnectDelay;              // Initial delay before reconnect (ms)
  #wsReconnectDelayMax;           // Max reconnect delay cap (ms)
  #wsReconnectTimer = null;       // Timer for scheduled reconnection attempt
  #heartbeatIntervalConnected;    // Heartbeat frequency when connected
  #heartbeatIntervalDisconnected; // Heartbeat frequency when disconnected
  #heartbeatResponseTimer = null; // Timer waiting for pong response
  #heartbeatResponseWaitTime;     // Time to wait for pong before miss

  /**
   * Initialize WebSocket client
   * @param {Object} config - Configuration object
   * @param {string} config.hostname - Device hostname (default: window.location.hostname)
   * @param {string} config.wsPath - WebSocket path (default: '/ws')
   * @param {number} config.connectionTimeout - Connection timeout in ms
   * @param {number} config.heartbeatIntervalConnected - Heartbeat interval when connected (ms)
   * @param {number} config.heartbeatIntervalDisconnected - Heartbeat interval when disconnected (ms)
   * @param {number} config.wsReconnectDelay - Initial reconnect delay in ms
   * @param {number} config.wsReconnectDelayMax - Max reconnect delay in ms
   * @param {number} config.heartbeatResponseWaitTime - Time to wait for heartbeat response in ms
   * @param {Function} config.onOpen - Called when connection opens
   * @param {Function} config.onClose - Called when connection closes
   * @param {Function} config.onError - Called when connection error occurs
   * @param {Function} config.onMessage - Called when message received (receives event)
   */
  constructor(config = {}) {
    this.#hostname = config.hostname || window.location.hostname;
    this.#wsPath = config.wsPath || '/ws';
    this.#onOpen = config.onOpen || (() => {});
    this.#onClose = config.onClose || (() => {});
    this.#onError = config.onError || (() => {});
    this.#onMessage = config.onMessage || (() => {});
    this.#connectionTimeout = config.connectionTimeout || 5000; // Default 5s
    this.#wsReconnectDelay = config.wsReconnectDelay || 1000; // Default 1s
    this.#wsReconnectDelayMax = config.wsReconnectDelayMax || 30000; // Default 30s
    this.#heartbeatIntervalConnected = config.heartbeatIntervalConnected || 4000; // Default s
    this.#heartbeatIntervalDisconnected = config.heartbeatIntervalDisconnected || 30000; // Default 30s
    this.#heartbeatResponseWaitTime = config.heartbeatResponseWaitTime || 2000; // Default 2s
  }
  
  /**
   * Get current connection state
   * @returns {boolean} True if WebSocket is connected
   */
  get isConnected() {
    return this.#isConnected;
  }
  
  /**
   * Connect to WebSocket with automatic backoff on failure
   */
  connect() {
    // Clear any pending reconnection attempt
    if (this.#wsReconnectTimer) {
      clearTimeout(this.#wsReconnectTimer);
    }
    
    console.log("WebSocket: Attempting connection... (attempt " + (this.#wsConnectionAttempts + 1) + ")");
    const gateway = "ws://" + this.#hostname + this.#wsPath;
    
    try {
      this.#websocket = new WebSocket(gateway);
      this.#websocket.onopen = (e) => this._handleOpen(e);
      this.#websocket.onclose = (e) => this._handleClose(e);
      this.#websocket.onerror = (e) => this._handleError(e);
      this.#websocket.onmessage = (e) => this._handleMessage(e);
      
      this.#wsConnectionAttempts++;
      
      // Timeout connection attempt after configured duration
      this.#wsReconnectTimer = setTimeout(() => {
        if (this.#websocket && this.#websocket.readyState === WebSocket.CONNECTING) {
          console.log("WebSocket: Connection timeout - closing attempt");
          this.#websocket.close();
        }
      }, this.#connectionTimeout);
    } catch (error) {
      console.error("WebSocket: Failed to create:", error);
      this._scheduleReconnect();
    }
  }
  
  /**
   * Send message over WebSocket if connected
   * @param {any} data - Data to send
   * @returns {boolean} True if message was queued
   */
  send(data) {
    if (this.#websocket && this.#websocket.readyState === WebSocket.OPEN) {
      this.#websocket.send(data);
      return true;
    }
    return false;
  }
  
  /**
   * Disconnect WebSocket
   */
  disconnect() {
    if (this.#websocket) {
      this.#websocket.close();
      this.#websocket = null;
    }
    if (this.#wsReconnectTimer) {
      clearTimeout(this.#wsReconnectTimer);
      this.#wsReconnectTimer = null;
    }
    if (this.#wsHeartbeatTimer) {
      clearTimeout(this.#wsHeartbeatTimer);
      this.#wsHeartbeatTimer = null;
    }
    if (this.#heartbeatResponseTimer) {
      clearTimeout(this.#heartbeatResponseTimer);
      this.#heartbeatResponseTimer = null;
    }
  }
  
  /**
   * Internal: Handle connection open
   */
  _handleOpen(event) {
    console.log("WebSocket: Connection opened");
    if (this.#wsReconnectTimer) {
      clearTimeout(this.#wsReconnectTimer);
      this.#wsReconnectTimer = null;
    }
    this.#isConnected = true;
    this.#hasEverConnected = true;
    this.#wsConnectionAttempts = 0;
    this.#wsReconnectDelay = 1000;
    this.#heartbeatMissCount = 0;
    hideConnectionOverlay();
    this._startHeartbeat();
    this.#onOpen(event);
  }
  
  /**
   * Internal: Handle connection close
   */
  _handleClose(event) {
    console.log("WebSocket: Connection closed");
    this.#isConnected = false;
    if (this.#wsReconnectTimer) {
      clearTimeout(this.#wsReconnectTimer);
    }
    if (this.#hasEverConnected) {
      showConnectionOverlay(this.#wsConnectionAttempts + 1);
    }
    this._scheduleReconnect();
    this.#onClose(event);
  }
  
  /**
   * Internal: Handle connection error
   */
  _handleError(event) {
    console.error("WebSocket: Error:", event);
    this.#onError(event);
  }
  
  /**
   * Internal: Handle incoming message
   */
  _handleMessage(event) {
    // Clear heartbeat response timer if we receive a pong
    if (event.data === "pong") {
      if (this.#heartbeatResponseTimer) {
        clearTimeout(this.#heartbeatResponseTimer);
        this.#heartbeatResponseTimer = null;
      }
      this.#heartbeatMissCount = 0; // Reset miss counter on successful pong
      //console.log("WebSocket: Heartbeat acknowledged");
      return; // Don't pass pong to application handlers
    }
    this.#onMessage(event);
  }
  
  /**
   * Internal: Schedule reconnection with exponential backoff
   */
  _scheduleReconnect() {
    if (this.#wsReconnectTimer) {
      clearTimeout(this.#wsReconnectTimer);
    }
    
    const delayMs = Math.min(
      this.#wsReconnectDelay * Math.pow(1.5, Math.min(this.#wsConnectionAttempts, 5)),
      this.#wsReconnectDelayMax
    );
    const jitter = Math.random() * 1000;
    const totalDelay = Math.floor(delayMs + jitter);
    
    console.log("WebSocket: Reconnect scheduled in " + totalDelay + "ms (attempt " + (this.#wsConnectionAttempts + 1) + ")");
    this.#wsReconnectTimer = setTimeout(() => this.connect(), totalDelay);
  }
  
  /**
   * Internal: Start heartbeat timers (called on connection open)
   * Starts WebSocket heartbeat to keep connection alive
   */
  _startHeartbeat() {
    if (this.#wsHeartbeatTimer) {
      clearTimeout(this.#wsHeartbeatTimer);
    }
    this._doWsHeartbeat();
  }
  
  /**
   * Internal: Send periodic WebSocket heartbeat to keep connection alive
   */
  _doWsHeartbeat() {
    if (this.send("heartbeat")) {
      // Heartbeat sent successfully, start timer waiting for response
      if (this.#heartbeatResponseTimer) {
        clearTimeout(this.#heartbeatResponseTimer);
      }
      this.#heartbeatResponseTimer = setTimeout(() => this._checkHeartbeatResponse(), this.#heartbeatResponseWaitTime);
    }
    const interval = this.#isConnected ? this.#heartbeatIntervalConnected : this.#heartbeatIntervalDisconnected;
    this.#wsHeartbeatTimer = setTimeout(() => this._doWsHeartbeat(), interval);
  }
  
  /**
   * Internal: Check if heartbeat response was received (called on timeout)
   * Increments miss counter; closes connection and reconnects on 2nd miss
   */
  _checkHeartbeatResponse() {
    this.#heartbeatMissCount++;
    console.log("WebSocket: Heartbeat response timeout (miss #" + this.#heartbeatMissCount + ")");
    
    if (this.#heartbeatMissCount >= 2) {
      console.log("WebSocket: 2nd heartbeat miss - closing connection and reconnecting");
      this.#heartbeatResponseTimer = null;
      if (this.#websocket) {
        this.#websocket.close();
      }
    } else {
      this.#heartbeatResponseTimer = null;
    }
  }
}

/** EventSource Manager - Handles Server-Sent Events with flexible event handlers **/
class EventSourceManager {
  // Private fields
  #eventSource = null;
  
  /**
   * Initialize EventSource manager
   * @param {Object} config - Configuration object
   * @param {Object} config.eventHandlers - Map of event names to handler functions
   *                                        e.g., { 'debug': (data) => {...}, 'telemetry': (data) => {...} }
   */
  constructor(config = {}) {
    this.eventHandlers = config.eventHandlers || {};
  }
  
  /**
   * Connect to EventSource and register event handlers
   */
  connect() {
    if (!window.EventSource) {
      console.warn("EventSource: Not supported in this browser");
      return;
    }
    
    console.log("EventSource: Connecting...");
    this.#eventSource = new EventSource("/events");
    
    // Standard open handler
    this.#eventSource.addEventListener("open", () => {
      console.log("EventSource: Connected");
    }, false);
    
    // Standard error handler
    this.#eventSource.addEventListener("error", (e) => {
      if (e.target.readyState !== EventSource.OPEN) {
        console.log("EventSource: Disconnected");
      }
    }, false);
    
    // Register custom event handlers
    Object.keys(this.eventHandlers).forEach((eventName) => {
      this.#eventSource.addEventListener(eventName, (e) => {
        try {
          if (e.data && e.data !== "") {
            this.eventHandlers[eventName](e.data); // Pass through the original data.
          }
        } catch (error) {
          console.error("EventSource: Handler error for '" + eventName + "':", error);
        }
      }, false);
    });
  }
  
  /**
   * Register an event handler dynamically
   * @param {string} eventName - Name of the event
   * @param {Function} handler - Handler function
   */
  on(eventName, handler) {
    if (!this.#eventSource) {
      console.warn("EventSource: Not connected, caching handler for '" + eventName + "'");
    }
    this.eventHandlers[eventName] = handler;
    if (this.#eventSource) {
      this.#eventSource.addEventListener(eventName, (e) => {
        try {
          if (e.data && e.data !== "") {
            handler(e.data); // Pass through the original data.
          }
        } catch (error) {
          console.error("EventSource: Handler error for '" + eventName + "':", error);
        }
      }, false);
    }
  }
  
  /**
   * Close EventSource connection
   */
  disconnect() {
    if (this.#eventSource) {
      this.#eventSource.close();
      this.#eventSource = null;
    }
  }
}

/** XHR Helper - Consistent request handling with timeout and error management **/
class XHRHelper {
  /**
   * Initialize XHR helper
   * @param {Object} config - Configuration object
   * @param {number} config.timeout - Request timeout in ms (default: 5000)
   * @param {Function} config.onError - Called on network error
   */
  constructor(config = {}) {
    this.timeout = config.timeout || 5000;
    this.onError = config.onError || (() => {});
  }
  
  /**
   * Send GET request
   * @param {string} url - URL to request
   * @param {Function} callback - Called with parsed JSON response
   */
  get(url, callback) {
    var xhttp = new XMLHttpRequest();
    xhttp.timeout = this.timeout;
    xhttp.onreadystatechange = () => {
      if (xhttp.readyState == 4) {
        if (xhttp.status >= 200 && xhttp.status < 300) {
          if (callback && typeof callback === "function") {
            try {
              callback(JSON.parse(xhttp.responseText));
            } catch (e) {
              console.error("XHR: JSON parse error:", e);
            }
          }
        } else {
          console.warn("XHR: GET " + url + " failed with status " + xhttp.status);
          this.onError(xhttp);
        }
      }
    };
    xhttp.onerror = () => {
      console.error("XHR: GET " + url + " network error");
      this.onError(xhttp);
    };
    xhttp.open("GET", url, true);
    xhttp.send();
    return true;
  }
  
  /**
   * Send PUT request
   * @param {string} url - URL to request
   * @param {any} data - Data to send (optional)
   * @param {Function} callback - Called with response (optional)
   */
  put(url, data, callback) {
    var xhttp = new XMLHttpRequest();
    xhttp.timeout = this.timeout;
    xhttp.onreadystatechange = () => {
      if (xhttp.readyState == 4) {
        if (xhttp.status >= 200 && xhttp.status < 300) {
          if (callback && typeof callback === "function") {
            callback(xhttp.responseText);
          }
        } else {
          console.warn("XHR: PUT " + url + " failed with status " + xhttp.status);
          this.onError(xhttp);
        }
      }
    };
    xhttp.onerror = () => {
      console.error("XHR: PUT " + url + " network error");
      this.onError(xhttp);
    };
    xhttp.open("PUT", url, true);
    xhttp.send(data || null);
    return true;
  }
  
  /**
   * Send POST request
   * @param {string} url - URL to request
   * @param {any} data - Data to send
   * @param {Function} callback - Called with response (optional)
   */
  post(url, data, callback) {
    var xhttp = new XMLHttpRequest();
    xhttp.timeout = this.timeout;
    xhttp.onreadystatechange = () => {
      if (xhttp.readyState == 4) {
        if (xhttp.status >= 200 && xhttp.status < 300) {
          if (callback && typeof callback === "function") {
            callback(xhttp.responseText);
          }
        } else {
          console.warn("XHR: POST " + url + " failed with status " + xhttp.status);
          this.onError(xhttp);
        }
      }
    };
    xhttp.onerror = () => {
      console.error("XHR: POST " + url + " network error");
      this.onError(xhttp);
    };
    xhttp.open("POST", url, true);
    xhttp.send(data || null);
    return true;
  }
  
  /**
   * Send DELETE request
   * @param {string} url - URL to request
   * @param {Function} callback - Called with response (optional)
   */
  delete(url, callback) {
    var xhttp = new XMLHttpRequest();
    xhttp.timeout = this.timeout;
    xhttp.onreadystatechange = () => {
      if (xhttp.readyState == 4) {
        if (xhttp.status >= 200 && xhttp.status < 300) {
          if (callback && typeof callback === "function") {
            callback(xhttp.responseText);
          }
        } else {
          console.warn("XHR: DELETE " + url + " failed with status " + xhttp.status);
          this.onError(xhttp);
        }
      }
    };
    xhttp.onerror = () => {
      console.error("XHR: DELETE " + url + " network error");
      this.onError(xhttp);
    };
    xhttp.open("DELETE", url, true);
    xhttp.send();
    return true;
  }
}

/** Global XHR Helper - Available to all projects for consistent request handling **/
const xhrHelper = new XHRHelper();

/** General API Callbacks - Common functions for data responses **/

function updateNetworkInfo(jObj) {
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
}

/** General API Helpers - Common functions for device control **/

function sendCommand(apiUri) {
  // Sends an action command to the server (device) using a PUT request.
  // These commands have no response data, so we just handle the status.
  xhrHelper.put(apiUri, null, (response) => {
    handleStatus(response);
  });
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

function toggleMute(el) {
  handleToggle(el, "/volume/mute", "/volume/unmute");
}

function musicLoop(el) {
  handleToggle(el, "/music/loop/one", "/music/loop/all");
}

function musicShuffle(el) {
  handleToggle(el, "/music/shuffle/on", "/music/shuffle/off");
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

function getStatus(callbackFunc) {
  // This function expects a JSON response from the server which must be parsed and sent to the callback function.
  xhrHelper.get("/status", (data) => {
    if (callbackFunc && typeof callbackFunc === "function") {
      callbackFunc(data);
    } else {
      console.warn("No callback function provided for getStatus response.");
    }
  });
}

function getNetworkInfo() {
  // Fetch network configuration and statistics from dedicated endpoint.
  xhrHelper.get("/wifi/status", updateNetworkInfo);
}

function doRestart() {
  // A special command which requires user confirmation before proceeding.
  if (confirm("Are you sure you wish to restart the serial device?")) {
    xhrHelper.delete("/restart", (response) => {
      // Reload the page after 2 seconds.
      setTimeout(function () {
        window.location.reload();
      }, 2000);
    });
  }
}
