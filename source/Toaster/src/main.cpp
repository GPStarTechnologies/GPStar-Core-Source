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

// Required for PlatformIO
#include <Arduino.h>

// Set to 1 to enable built-in debug messages via Serial device output.
// Use with DEBUG_SEND_TO_CONSOLE and other DEBUG_'s in Configuration.h
#define GPSTAR_DEBUG 0

// Debug macros
#if GPSTAR_DEBUG == 1
  #define debug(...) Serial.print(__VA_ARGS__)
  #define debugf(...) Serial.printf(__VA_ARGS__)
  #define debugln(...) Serial.println(__VA_ARGS__)
#else
  #define debug(...)
  #define debugf(...)
  #define debugln(...)
#endif

// PROGMEM macros
#define PROGMEM_READU32(x) pgm_read_dword_near(&(x))
#define PROGMEM_READU16(x) pgm_read_word_near(&(x))
#define PROGMEM_READU8(x) pgm_read_byte_near(&(x))
#define PROGMEM_READI8(x) (int8_t)pgm_read_byte_near(&(x))

// 3rd-Party Libraries
#include <millisDelay.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <HardwareSerial.h>

// Shared Libraries
#include <DeviceState.h>
#include <WirelessManager.h>
#include <WebRouter.h>

// Global instance of DeviceState class for the overall system.
DeviceState gpstarSystem;

// Local Files
#include "Configuration.h"
#include "MusicSounds.h"
#include "Header.h"
#include "Audio.h"
#include "Wireless.h"
#include "System.h"
#include "Animation.h"
#include "Webhandler.h"
#include "Webrouting.h"

// Writes a debug message to the serial console or sends to the WebSocket.
void sendDebug(const String& message) {
  #if defined(DEBUG_SEND_TO_CONSOLE)
    debugln(message); // Print to serial console.
  #endif
  #if defined(DEBUG_SEND_TO_WEBSOCKET)
    if(b_httpd_started) {
      ws.textAll(message); // Send a copy to the WebSocket.
    }
  #endif
}

// Define the WirelessManager pointer globally (initialized to nullptr).
// This matches the extern declaration in Wireless.h
WirelessManager* wirelessMgr = nullptr;

// Task Handles
TaskHandle_t AnimationTaskHandle = NULL;
TaskHandle_t PreferencesTaskHandle = NULL;
TaskHandle_t UserInputTaskHandle = NULL;
TaskHandle_t WiFiManagementTaskHandle = NULL;
TaskHandle_t WiFiSetupTaskHandle = NULL;

// Variables for approximating CPU load
// https://www.arduino.cc/reference/en/language/variables/variable-scope-qualifiers/volatile/
volatile uint32_t idleTimeCore0 = 0;
volatile uint32_t idleTimeCore1 = 0;

// Idle task for Core 0
#if defined(DEBUG_PERFORMANCE)
void idleTaskCore0(void * parameter) {
  while(true) {
    idleTimeCore0 = idleTimeCore0 + 1;
    vTaskDelay(1);
  }
}
#endif

// Idle task for Core 1
#if defined(DEBUG_PERFORMANCE)
void idleTaskCore1(void * parameter) {
  while(true) {
    idleTimeCore1 = idleTimeCore1 + 1;
    vTaskDelay(1);
  }
}
#endif

// Animation Task (Loop)
void AnimationTask(void *parameter) {
  while(true) {
    #if defined(DEBUG_TASK_TO_CONSOLE)
      // Confirm the core in use for this task, and when it runs.
      debug(F("Executing AnimationTask in core"));
      debug(xPortGetCoreID());
      // Get the stack high water mark for optimizing bytes allocated.
      debug(F(" | Stack HWM: "));
      debugln(uxTaskGetStackHighWaterMark(NULL));
    #endif

    // Track whether at least one relay remains active, affecting the status LED.
    bool b_relay_active = false;

    // Process each actuator relay and check if it should be turned off
    RelayChannel* relays[] = {&devices.relay1, &devices.relay2, &devices.relay3, &devices.relay4};
    uint8_t relayPins[] = {devices.relay1.pin, devices.relay2.pin, devices.relay3.pin, devices.relay4.pin};

    for (uint8_t i = 0; i < 4; i++) {
      // If this relay is active and its configured on-time has expired,
      // mark it as inactive (LOW) and turn off the associated relay output.
      bool b_expired = ((int32_t)(millis() - relays[i]->state.relayOffTime) >= 0);
      if(relays[i]->state.relayActive && b_expired){
        relays[i]->state.relayActive = false;
        digitalWrite(relayPins[i], LOW);
      }

      // If this relay is still active, ensure the relay output remains
      // on (HIGH) and record that at least one relay is currently active.
      if(relays[i]->state.relayActive){
        b_relay_active = true;
        digitalWrite(relayPins[i], HIGH);
      }
    }

    // Use the built-in LED to indicate if any relays are active.
    digitalWrite(BUILT_IN_LED, b_relay_active ? HIGH : LOW);

    // Update animation playback if currently playing
    updatePlayback();

    // Send animation frame data to connected clients via SSE
    sendAnimationFrameData();

    updateAudio(); // Update the state of the available sound board.
    checkMusic(); // Perform music control as necessary.

    vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms delay
  }
}

// Preferences Task (Single-Run)
void PreferencesTask(void *parameter) {
  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Confirm the core in use for this task, and when it runs.
    debug(F("Executing PreferencesTask in core"));
    debugln(xPortGetCoreID());
  #endif

  // Print partition information to verify NVS availability
  #if defined(DEBUG_SEND_TO_CONSOLE)
  printPartitions();
  #endif

  // Initialize the NVS flash partition and throw any errors as necessary.
  esp_err_t err = nvs_flash_init();
  if(err != ESP_OK) {
    #if defined(DEBUG_SEND_TO_CONSOLE)
    debug(F("NVS initialization failed with error: "));
    debugln(esp_err_to_name(err));
    #endif

    // If initialization fails, erase and reinitialize NVS.
    debugln(F("Erasing and reinitializing NVS..."));
    nvs_flash_erase();

    err = nvs_flash_init();
    if(err != ESP_OK) {
      #if defined(DEBUG_SEND_TO_CONSOLE)
      debug(F("Failed to reinitialize NVS: "));
      debugln(esp_err_to_name(err));
      #endif
    }
    else {
      debugln(F("NVS reinitialized successfully"));
    }
  }
  else {
    debugln(F("NVS initialized successfully"));
  }

  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Get the stack high water mark for optimizing bytes allocated.
    debug(F("PreferencesTask Stack HWM: "));
    debugln(uxTaskGetStackHighWaterMark(NULL));
  #endif

  // Task ends after setup is complete and MUST be removed from scheduling.
  // Failure to do this can cause an error within the watchdog timer!
  vTaskDelete(NULL);
}

// User Input Task (Loop)
void UserInputTask(void *parameter) {
  // Track which animation is currently playing (-1 = no animation playing)
  static int8_t currentPlayingAnim = -1;

  while(true) {
    #if defined(DEBUG_TASK_TO_CONSOLE)
      // Confirm the core in use for this task, and when it runs.
      debug(F("Executing UserInputTask in core"));
      debug(xPortGetCoreID());
      // Get the stack high water mark for optimizing bytes allocated.
      debug(F(" | Stack HWM: "));
      debugln(uxTaskGetStackHighWaterMark(NULL));
    #endif

    // Check each RF input pin for triggers.
    RFButtonChannel* buttons[] = {&devices.button1, &devices.button2, &devices.button3, &devices.button4};
    uint8_t buttonPins[] = {devices.button1.pin, devices.button2.pin, devices.button3.pin, devices.button4.pin};

    for (uint8_t i = 0; i < 4; i++) {
      bool rfState = digitalRead(buttonPins[i]) == HIGH;
      bool stateChanged = false;

      // Debounce: accumulate when pin state DIFFERS from current debounced state
      if (rfState != buttons[i]->state.currentState) {
        // Pin differs from debounced state - accumulate transition count
        if (buttons[i]->state.debounceCount < RF_DEBOUNCE_MAX) {
          buttons[i]->state.debounceCount++;
        }

        // Once we've accumulated enough consistent reads of the DIFFERENT state, accept transition
        if (buttons[i]->state.debounceCount >= RF_DEBOUNCE_THRESHOLD) {
          buttons[i]->state.previousState = buttons[i]->state.currentState;
          buttons[i]->state.currentState = rfState;
          buttons[i]->state.debounceCount = 0;  // Reset for next transition
          stateChanged = true;
        }
      } else {
        // Pin matches debounced state - reset transition counter
        buttons[i]->state.debounceCount = 0;
      }

      // Detect rising edge ONLY on the iteration where state actually changed
      // RF buttons control animation playback
      if (stateChanged && buttons[i]->state.currentState && !buttons[i]->state.previousState) {
        uint8_t buttonIndex = i;  // 0-3 maps to animation slots 0-3

        // Handle animation playback control based on current state
        if (currentAnimation.mode == ANIM_IDLE || currentAnimation.mode == ANIM_RECORDING) {
          // Not playing - start this animation
          if (startPlayback(buttonIndex)) {
            currentPlayingAnim = buttonIndex;

            #if defined(DEBUG_SEND_TO_CONSOLE)
              debug(F("RF"));
              debug(buttonIndex + 1);
              debugln(F(" started animation playback"));
            #endif

            notifyWSClients();
          }
        } else if (currentAnimation.mode == ANIM_PLAYBACK) {
          // Currently playing - handle same or different button
          if (buttonIndex == currentPlayingAnim) {
            // Same button pressed - stop playback
            stopPlayback();
            currentPlayingAnim = -1;

            #if defined(DEBUG_SEND_TO_CONSOLE)
              debug(F("RF"));
              debug(buttonIndex + 1);
              debugln(F(" stopped animation playback"));
            #endif

            notifyWSClients();
          } else {
            // Different button - stop current, play new animation
            stopPlayback();
            if (startPlayback(buttonIndex)) {
              currentPlayingAnim = buttonIndex;

              #if defined(DEBUG_SEND_TO_CONSOLE)
                debug(F("RF"));
                debug(buttonIndex + 1);
                debugln(F(" switched to new animation"));
              #endif

              notifyWSClients();
            }
          }
        }
      }
    }

    vTaskDelay(14 / portTICK_PERIOD_MS); // 14ms delay
  }
}

// WiFi Management Task (Loop)
void WiFiManagementTask(void *parameter) {
  while(true) {
    #if defined(DEBUG_TASK_TO_CONSOLE)
      // Confirm the core in use for this task, and when it runs.
      debug(F("Executing WiFiManagementTask in core"));
      debug(xPortGetCoreID());
      // Get the stack high water mark for optimizing bytes allocated.
      debug(F(" | Stack HWM: "));
      debugln(uxTaskGetStackHighWaterMark(NULL));
    #endif

    // Handle reconnection to external WiFi when necessary.
    if(b_local_ap_started) {
      if(b_httpd_started && ms_cleanup.remaining() < 1) {
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

      if(WiFi.status() == WL_CONNECTED && b_ext_wifi_started) {
        b_ext_wifi_paused = false; // Resume WiFi retries when needed.
      }
    }

    // Proceed with management if the AP and web server are started.
    if(b_local_ap_started) {
      // Perform periodic checks for WiFi clients and OTA updates.
      webLoops();

      // Try to start the external WiFi.
      if(!b_ext_wifi_started && !b_ext_wifi_paused) {
        notifyWSClients(); // Notify clients of this device of a change of data.
        b_ext_wifi_started = startExternalWifi();
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1000ms delay
  }
}

// WiFi Setup Task (Single-Run)
void WiFiSetupTask(void *parameter) {
  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Confirm the core in use for this task, and when it runs.
    debug(F("Executing WiFiSetupTask in core"));
    debugln(xPortGetCoreID());
  #endif

  // Define the WirelessManager object only after NVS/Preferences are initialized.
  if(wirelessMgr == nullptr) {
    wirelessMgr = new WirelessManager(WirelessDeviceType::TOASTER, "192.168.2.2");

    #if defined(RESET_AP_SETTINGS)
      // Reset the WiFi password to the expected default on every startup.
      wirelessMgr->resetWifiPassword();
      debugln(F("WARNING: Firmware forced a reset of the local WiFi password!"));
    #endif
  }

  // Begin by setting up WiFi as a prerequisite to all else.
  if(startWiFi()) {
    if(b_local_ap_started) {
      // Indicate we've established the private network.
      debugln(F("SoftAP Started Successfully!"));
    }

    // Start the local web server.
    startWebServer();

    // Begin timer for remote client events.
    ms_cleanup.start(i_websocketCleanup);
    ms_apclient.start(i_apClientDelay);
    ms_otacheck.start(i_otaCheck);
  }

  vTaskDelay(200 / portTICK_PERIOD_MS); // 200ms delay

  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Get the stack high water mark for optimizing bytes allocated.
    debug(F("WiFiSetupTask Stack HWM: "));
    debugln(uxTaskGetStackHighWaterMark(NULL));
  #endif

  // Task ends after setup is complete and MUST be removed from scheduling.
  // Failure to do this can cause an error within the watchdog timer!
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200); // Serial monitor via USB connection.

  pinMode(BUILT_IN_LED, OUTPUT); // On-board LED for testing.

  // Read states from the RF receiver module.
  pinMode(RF1_PIN, INPUT); // 34
  pinMode(RF2_PIN, INPUT); // 33
  pinMode(RF3_PIN, INPUT); // 35
  pinMode(RF4_PIN, INPUT); // 39

  // Output signal pins for the relay module.
  pinMode(RELAY1_PIN, OUTPUT); // 25
  pinMode(RELAY2_PIN, OUTPUT); // 26
  pinMode(RELAY3_PIN, OUTPUT); // 27
  pinMode(RELAY4_PIN, OUTPUT); // 32

  // Initialize all outputs as low.
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);

  // Initialize RF input button states (LOW = idle, HIGH = pressed)
  devices.button1.state.currentState = false;  // LOW (idle)
  devices.button1.state.previousState = false;
  devices.button1.state.debounceCount = 0;

  devices.button2.state.currentState = false;
  devices.button2.state.previousState = false;
  devices.button2.state.debounceCount = 0;

  devices.button3.state.currentState = false;
  devices.button3.state.previousState = false;
  devices.button3.state.debounceCount = 0;

  devices.button4.state.currentState = false;
  devices.button4.state.previousState = false;
  devices.button4.state.debounceCount = 0;

#if GPSTAR_DEBUG == 1
  // When debugging is enabled, wait for Serial to be ready (max 3 seconds).
  unsigned long startMillis = millis();
  while (!Serial && millis() - startMillis < 3000) {
    delay(10);
  }
  Serial.flush(); // Ensure buffer is clear.
  Serial.println(F("Serial is Ready")); // Should appear after Serial is ready.
#endif

  // Provide an opportunity to set the CPU Frequency MHz: 80, 160, 240 [Default = 240]
  // Lower frequency means less power consumption, but slower performance (obviously).
  setCpuFrequencyMhz(80);
  #if defined(DEBUG_SEND_TO_CONSOLE)
    debug(F("CPU Freq (MHz): "));
    debugln(getCpuFrequencyMhz());
  #endif

  btStop(); // Disable Bluetooth which is not needed for this hardware.

  // Setup the audio device for this controller.
  setupAudioDevice();

  delay(200); // Delay before configuring and running tasks.

  setMasterVolumePercentage(100); // Set master volume to 100%.

  /**
   * By default the WiFi will run on core0, while the standard loop() runs on core1.
   * We can make efficient use of the available cores by "pinning" a task to a core.
   * The ESP32 platform comes with FreeRTOS implemented internally and exposed even
   * to the Arduino platform (meaning: no need for using the ESP-IDF exclusively).
   * In theory this allows for improved parallel processing with prioritization and
   * granting of dedicated memory stacks to each task (which can be monitored).
   *
   * Parameters:
   *  Task Function Name,
   *  User-Friendly Task Name,
   *  Stack Size (in bytes),
   *  Input Parameter,
   *  Priority (use higher #),
   *  Task Handle Reference,
   *  Pinned Core (0 or 1)
   */

  // Create a single-run setup task with the highest priority for WiFi/WebServer startup.
  xTaskCreatePinnedToCore(PreferencesTask, "PreferencesTask", 4096, NULL, 5, &PreferencesTaskHandle, 1);

  // Delay all lower priority tasks until Preferences are loaded.
  vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100ms to avoid competition.

  // Create a single-run setup task with the highest priority for WiFi/WebServer startup.
  xTaskCreatePinnedToCore(WiFiSetupTask, "WiFiSetupTask", 4096, NULL, 4, &WiFiSetupTaskHandle, 1);

  // Delay all lower priority tasks until WiFi and WebServer setup is done.
  vTaskDelay(200 / portTICK_PERIOD_MS); // Delay for 200ms to avoid competition.

  // Create tasks which utilize a loop for continuous operation (prioritized highest to lowest).
  xTaskCreatePinnedToCore(UserInputTask, "UserInputTask", 4096, NULL, 3, &UserInputTaskHandle, 1);
  xTaskCreatePinnedToCore(AnimationTask, "AnimationTask", 4096, NULL, 2, &AnimationTaskHandle, 1);
  xTaskCreatePinnedToCore(WiFiManagementTask, "WiFiManagementTask", 4096, NULL, 1, &WiFiManagementTaskHandle, 0);

  // Create idle tasks for each core, used to estimate % busy for core.
  #if defined(DEBUG_PERFORMANCE)
  xTaskCreatePinnedToCore(idleTaskCore0, "Idle Task Core 0", 1000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(idleTaskCore1, "Idle Task Core 1", 1000, NULL, 1, NULL, 1);
  #endif
}

// Helper function to format bytes with a comma separator
String formatBytesWithCommas(uint32_t bytes) {
    String result = String(bytes);
    int insertPosition = result.length() - 3;
    while(insertPosition > 0) {
        result = result.substring(0, insertPosition) + "," + result.substring(insertPosition);
        insertPosition -= 3;
    }
    return result;
}

// Function to calculate and print CPU load
void printCPULoad() {
  uint32_t idle0 = idleTimeCore0;
  uint32_t idle1 = idleTimeCore1;

  // Calculate CPU load as (total time - idle time) / total time
  float cpuLoadCore0 = 100.0 - ((float)idle0 / (float)(idle0 + idle1)) * 100.0;
  float cpuLoadCore1 = 100.0 - ((float)idle1 / (float)(idle0 + idle1)) * 100.0;

  debug(F("CPU Load Core0: "));
  debug(cpuLoadCore0);
  debugln(F("%"));

  debug(F("CPU Load Core1: "));
  debug(cpuLoadCore1);
  debugln(F("%"));

  // Reset idle times after calculation
  idleTimeCore0 = 0;
  idleTimeCore1 = 0;
}

void printMemoryStats() {
  debugln(F("Memory Usage Stats:"));

  // Heap memory
  debug(F("|-Total Free Heap: "));
  debug(formatBytesWithCommas(esp_get_free_heap_size()));
  debugln(F(" bytes"));

  debug(F("|-Minimum Free Heap Ever: "));
  debug(formatBytesWithCommas(esp_get_minimum_free_heap_size()));
  debugln(F(" bytes"));

  debug(F("|-Maximum Allocatable Block: "));
  debug(formatBytesWithCommas(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
  debugln(F(" bytes"));

  // Stack memory (for the main task)
  debug(F("|-Tasks Stack High Water Mark:"));
  debug(F("|--Main Task: "));
  debug(formatBytesWithCommas(uxTaskGetStackHighWaterMark(NULL)));
  debugln(F(" bytes"));

  // Stack memory (for other tasks)
  if(UserInputTaskHandle != NULL) {
    debug(F("|--User Input: "));
    debug(formatBytesWithCommas(uxTaskGetStackHighWaterMark(UserInputTaskHandle)));
    debugln(F(" / 4,096 bytes"));
  }
  if(AnimationTaskHandle != NULL) {
    debug(F("|--Animation: "));
    debug(formatBytesWithCommas(uxTaskGetStackHighWaterMark(AnimationTaskHandle)));
    debugln(F(" / 4,096 bytes"));
  }
  if(WiFiManagementTaskHandle != NULL) {
    debug(F("|--WiFi Mgmt.: "));
    debug(formatBytesWithCommas(uxTaskGetStackHighWaterMark(WiFiManagementTaskHandle)));
    debugln(F(" / 2,048 bytes"));
  }
}

void loop() {
  // No work done here, only in the tasks!

  #if defined(DEBUG_PERFORMANCE)
  debugln(F("=================================================="));
  printCPULoad();      // Print CPU load
  printMemoryStats();  // Print memory usage
  delay(3000);         // Wait 3 seconds before printing again
  #endif
}