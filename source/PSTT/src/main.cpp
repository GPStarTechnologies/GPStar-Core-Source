/**
 *   GPStar Proton Stream Target Trainer
 *   Copyright (C) 2023-2026 GPStar Technologies <contact@gpstartechnologies.com>
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
#include <CRC32.h>
#include <digitalWriteFast.h>
#include <millisDelay.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <ESP32Servo.h>
#include <ezButton.h>

// Forward declaration for use in all includes.
void sendDebug(const String& message);

// Shared Libraries
#include <WirelessManager.h>
#include <WebRouter.h>

// Include the InfraredManager class and define a global pointer for it.
#define IR_LED_PIN 17
#define IR_RECEIVER_PIN 12
#include <InfraredManager.hpp>

// Define the InfraredManager pointer globally (initialized to nullptr).
// This matches the extern declaration in InfraredManager.h
InfraredManager* irManager = nullptr;

// Local Files
#include "LightConfig.h"
#include "Configuration.h"
#include "Header.h"
#include "PreferencesESP.h"
#include "GPStarServo.h"
#include "Wireless.h"
#include "Webhandler.h"
#include "Webrouting.h"
#include "System.h"

// Writes a debug message to the serial console or sends to the WebSocket or Events stream.
void sendDebug(const String& message) {
  #if defined(DEBUG_SEND_TO_CONSOLE)
    debugln(message); // Print to serial console.
  #endif
  #if defined(DEBUG_SEND_TO_WEBSOCKET) and defined(ESP32)
    if(b_httpd_started) {
      ws.textAll(message); // Send a copy to the WebSocket.
    }
  #endif
  #if defined(DEBUG_SEND_TO_EVENTS) and defined(ESP32)
    sendDebugEvent(message.c_str()); // Send message to the events stream.
  #endif
}

// Define the WirelessManager pointer globally (initialized to nullptr).
// This matches the extern declaration in Wireless.h
WirelessManager* wirelessMgr = nullptr;

// Task Handles
TaskHandle_t AnimationTaskHandle = NULL;
TaskHandle_t InputsTaskHandle = NULL;
TaskHandle_t PreferencesTaskHandle = NULL;
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

    // Update the addressable LEDs to reflect any changes.
    LightingManager::getInstance().show();

    vTaskDelay(16 / portTICK_PERIOD_MS); // 16ms delay (60fps)
  }
}

// Inputs Task (Loop)
void InputsTask(void *parameter) {
  while(true) {
    #if defined(DEBUG_TASK_TO_CONSOLE)
      // Confirm the core in use for this task, and when it runs.
      debug(F("Executing InputsTask in core"));
      debug(xPortGetCoreID());
      // Get the stack high water mark for optimizing bytes allocated.
      debug(F(" | Stack HWM: "));
      debugln(uxTaskGetStackHighWaterMark(NULL));
    #endif

    // Get the current state of any input devices (toggles, buttons, and switches).
    switch_pstt.loop();

    // Check the target health and do appropriate actions if required.
    checkTargetHealth();

    // Update the colour of the LED indicators.
    updateHealthIndicators();

    // Manual button press detection for ezButton (similar to Attenuator pattern).
    static bool was_pressed = false;
    static unsigned long press_start = 0;
    const unsigned long LONG_PRESS_TIME = 600; // 600ms for long press

    if(switch_pstt.isPressed()) {
      was_pressed = true;
      press_start = millis();
    }

    if(switch_pstt.isReleased() && was_pressed) {
      unsigned long press_duration = millis() - press_start;

      if(press_duration >= LONG_PRESS_TIME) {
        // Long press - reset target
        setTargetAsReady();
      }
      else {
        // Short press - retract target
        setTargetDefeated();
      }

      was_pressed = false;
    }

    // Check for the latest IR data and handle as needed.
    checkInfraredData();

    vTaskDelay(50 / portTICK_PERIOD_MS); // 50ms delay (20Hz)
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

    // Proceed with management if the AP and web server are started.
    if(b_local_ap_started) {
      // Perform periodic checks for WiFi clients and OTA updates.
      webLoops();

      // (Re-)Start WiFi if the web server is not running.
      if(!wirelessMgr->isWifiActive()) {
        restartWireless();
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
    wirelessMgr = new WirelessManager(WirelessDeviceType::PSTT, "192.168.2.2");

    // Initialize the Infrared handler with the device type and ID.
    if(irManager == nullptr) {
      irManager = new InfraredManager(IR_DEVICE_PSTT, wirelessMgr->getDeviceID());
    }

    #if defined(RESET_AP_SETTINGS)
      // Reset the WiFi password to the expected default on every startup.
      wirelessMgr->resetWifiPassword();
      debugln(F("WARNING: Firmware forced a reset of the local WiFi password!"));
    #endif
  }

  // Begin by setting up WiFi as a prerequisite to all else.
  if(startWiFi()) {
    if(b_local_ap_started) {
      // TODO: Indicate we've established the private network.
    }

    // Start the local web server.
    startWebServer();
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
  // Initialize the LED driver first
  LightingManager::getInstance().initializeDriver();

  // Reduce CPU frequency to 160 MHz to save ~33% power compared to 240 MHz.
  // Alternatively set CPU to 80 MHz to save ~50% power compared to 240 MHz.
  // Do not set below 80 MHz as it will affect WiFi and other peripherals.
  setCpuFrequencyMhz(80);
  #if defined(DEBUG_SEND_TO_CONSOLE)
    debug(F("CPU Freq (MHz): "));
    debugln(getCpuFrequencyMhz());
  #endif

  // This is required in order to make sure the board boots successfully.
  Serial.begin(115200);

#if GPSTAR_DEBUG == 1
  // When debugging is enabled, wait for Serial to be ready (max 3 seconds).
  unsigned long startMillis = millis();
  while (!Serial && millis() - startMillis < 3000) {
    delay(10);
  }
  Serial.flush(); // Ensure buffer is clear.
  Serial.setTxTimeoutMs(0); // Optional: reduce USB-CDC transmission delay.
  Serial.println(F("Serial is Ready")); // Should appear after Serial is ready.
#endif

  /* This loop changes GPIO40~GPIO42 to Function 1, which is GPIO.
   * PIN_FUNC_SELECT sets the IOMUX function register appropriately.
   * IO_MUX_GPIO0_REG is the register for GPIO0, which we then seek from.
   * PIN_FUNC_GPIO is a define for Function 1, which sets the pins to GPIO mode.
   */
  for(uint8_t gpio_pin = 40; gpio_pin < 43; gpio_pin++) {
    PIN_FUNC_SELECT(IO_MUX_GPIO0_REG + (gpio_pin * 4), PIN_FUNC_GPIO);
  }

  // Get all special device preferences from NVS which may be needed for sensors.
  // This also loads the target configuration settings.
  getSpecialPreferences();

  // Attach the servo motor.
  pstt_servo.attach(PSTT_SERVO_PIN);

  // The status indicator LED on the Proton Stream Target Trainer board.
  pinModeFast(PSTT_STATUS_LED_PIN, OUTPUT);
  digitalWriteFast(PSTT_STATUS_LED_PIN, HIGH);

  // Make sure all LEDs are off and set the default target state.
  LightingManager::getInstance().lightsOff();

  // Set target to ready state automatically on boot.
  setTargetAsReady();

  delay(200); // Delay before configuring and running tasks.

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
  xTaskCreatePinnedToCore(PreferencesTask, "PreferencesTask", 4096, NULL, 4, &PreferencesTaskHandle, 1);

  // Delay all lower priority tasks until Preferences are loaded.
  vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100ms to avoid competition.

  // Create a single-run setup task with the highest priority for WiFi/WebServer startup.
  xTaskCreatePinnedToCore(WiFiSetupTask, "WiFiSetupTask", 4096, NULL, 3, &WiFiSetupTaskHandle, 1);

  // Delay all lower priority tasks until WiFi and WebServer setup is done.
  vTaskDelay(200 / portTICK_PERIOD_MS); // Delay for 200ms to avoid competition.

  // Create tasks which utilize a loop for continuous operation (prioritized highest to lowest).
  xTaskCreatePinnedToCore(AnimationTask, "AnimationTask", 4096, NULL, 2, &AnimationTaskHandle, 1);
  xTaskCreatePinnedToCore(InputsTask, "InputsTask", 4096, NULL, 2, &InputsTaskHandle, 1);
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
  if(AnimationTaskHandle != NULL) {
    debug(F("|--Animation: "));
    debug(formatBytesWithCommas(uxTaskGetStackHighWaterMark(AnimationTaskHandle)));
    debugln(F(" / 2,048 bytes"));
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
