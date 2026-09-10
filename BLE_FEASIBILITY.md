# Bluetooth LE Feasibility

## Scope

This document evaluates whether the ESP32-S3 controllers used by the GPStar Proton Pack and Neutrona Wand can:

1. Operate Bluetooth LE (BLE) alongside Wi-Fi.
2. Use BLE as a paired wireless link between the two devices.
3. Provide performance comparable to the existing direct serial connection.

The expected distance between the Proton Pack and Neutrona Wand is within a 5–6 ft radius. This is a feasibility study only; it contains no implementation.

## Executive Summary

The proposed BLE link is technically feasible.

- Both projects target the same ESP32-S3 N16R8 class of module. The ESP32-S3 supports Bluetooth LE, but not Bluetooth Classic.
- Wi-Fi and BLE can run concurrently. They share one 2.4 GHz radio and antenna, so coexistence is implemented by time-sharing radio access rather than by two independent radios.
- BLE does not require application GPIO pins and therefore does not directly conflict with the existing UART, I2C, LED, switch, or PWM assignments.
- The eFuses currently burned to release GPIO39-44 do not disable or otherwise interfere with BLE.
- BLE can provide more throughput than the existing 9600-baud UART, but it cannot provide the same deterministic latency, jitter, connection integrity, or immunity to 2.4 GHz interference.
- A 5–6 ft operating radius is comfortably within normal BLE range and makes basic link viability favorable, subject to antenna placement, enclosure materials, body shadowing, and Wi-Fi coexistence.

The intended transport policy should be UART first. The devices would use BLE only when the physical serial connection is absent or fails to respond. The Proton Pack would remain the authoritative hub for its own Pack/Wand state, expose a stable Pack ID for selection, and permit only one paired or active Neutrona Wand at a time. Separately, each Wand could advertise and scan for nearby Wands to coordinate an optional crossing-the-streams effect without granting those Wands control of another user's Pack.

The existing UART should remain the preferred production control link whenever it is available. BLE should be treated as a fallback transport whose protocol includes explicit ordering, acknowledgements for critical commands, reconnect behavior, transport arbitration, and a safe disconnected state.

## Recommendation

BLE is viable for communication between the ESP32-S3 Proton Pack and Neutrona Wand, and the current pin assignments and eFuse settings present no obvious hardware conflict. The intended 5–6 ft range is favorable, it offers ample bandwidth for the existing protocol, and it can coexist with Wi-Fi under supported ESP32-S3 scenarios.

The recommended Pack/Wand model is a UART-first fallback system. The devices should use the wired UART whenever valid serial synchronization succeeds and enable BLE control only when UART is unavailable. The Proton Pack should remain the authoritative application hub for its own paired system, advertise a stable user-visible Pack ID, retain one authorized Wand bond, and accept only one active Wand control connection. The Pack ID should aid discovery and user selection, while BLE bonding provides trusted peer identity.

For the optional crossing-the-streams behavior, the Wand is the appropriate peer-discovery and local-coordination point because firing input originates there. This does not require one Wand to become a permanent hub for other Wands or Packs. Each opted-in Wand should advertise limited peer state, observe nearby compatible Wands, apply local freshness and proximity rules, and command only its own Pack. Non-connectable advertisements and passive scans are the preferred first prototype; a temporary authenticated Wand-to-Wand connection should be considered only if measured synchronization or security requirements cannot be met by advertisements.

BLE should not initially be treated as behaviorally equivalent to the direct serial connection. The wired UART remains preferable for deterministic control, reliability, and simple failure handling. BLE should be considered for production fallback only after measured worst-case coexistence, body-shadowing, transport-handoff, reconnection, and single-peer enforcement results satisfy explicit latency and safety requirements.

## Current Hardware and Software Environment

Both projects use an `esp32s3custom` board definition describing an ESP32-S3 with 16 MB flash and 8 MB octal PSRAM:

- [Proton Pack board definition](source/ProtonPack/boards/esp32s3custom.json)
- [Neutrona Wand board definition](source/NeutronaWand/boards/esp32s3custom.json)

Both use the Arduino framework through the same pioarduino ESP32 platform release. Neither current environment declares a BLE library or contains BLE initialization:

- [Proton Pack PlatformIO environment](source/ProtonPack/platformio.ini)
- [Neutrona Wand PlatformIO environment](source/NeutronaWand/platformio.ini)

The projects already run Wi-Fi, an asynchronous web server, OTA support, PSRAM-backed handlers, and application tasks. Adding BLE would add controller and host-stack memory, tasks, callbacks, and radio coexistence load to an already active system. Available heap, internal RAM, task stacks, and worst-case scheduling latency would need measurement on both devices.

The Proton Pack runs at 160 MHz, while the Neutrona Wand reduces its CPU to 80 MHz. The 80 MHz Wand is the more important target for combined BLE, Wi-Fi, web server, IMU processing, and LED-animation load testing.

## Implementation Roadmap

This feasibility study has identified a clear path forward divided into two phases:

### Phase 1: Core Pack/Wand BLE Link (IN PROGRESS)

**Scope:** Implement reliable UART-first BLE fallback between Proton Pack and Neutrona Wand only.

#### Current Implementation Status (as of 2026-09-08)

**COMPLETED:**
- ✅ `NimBLEServer` initialized on Proton Pack with GPStar private service
- ✅ GPStar GATT service UUID defined: `0000ffe0-0000-1000-8000-00805f9b34fb`
- ✅ Command characteristic implemented: `0000ffe1-...` (Wand → Pack, WRITE)
- ✅ Status characteristic implemented: `0000ffe2-...` (Pack → Wand, NOTIFY)
- ✅ `NimBLEClient` initialized on Neutrona Wand with scanning and connection callbacks
- ✅ Pack-side write callback (`onWrite()`) receives commands from Wand
- ✅ Wand-side notification callback (`wandNotifyCallback()`) receives Pack status
- ✅ `discoverRemoteCharacteristics()` function discovers remote service and registers notification subscriptions after pairing
- ✅ Dual-transport send infrastructure: single `txObj()` serialization sent via both UART and BLE
- ✅ `bleSendData()` functions implemented on both devices
- ✅ LE Secure Connections pairing with automatic bonding
- ✅ Advertising includes GPStar service UUID and Pack name ("GPStar-Pack-XXXX")

**IN PROGRESS:**
- 🟡 Deserialization logic: callback functions exist with TODO comments for data processing
  - `wandNotifyCallback()` on Wand: needs to deserialize Pack status into structs
  - `onWrite()` callback on Pack: needs to deserialize Wand commands and queue for processing
- 🟡 Main loop integration: callbacks fire and log data but don't queue for application processing

**NOT YET STARTED:**
- ⏳ Transport arbitration/fallback logic (UART-first policy)
- ⏳ Safe disconnected state handling
- ⏳ Message ordering and duplicate detection for BLE
- ⏳ Compile size and runtime performance measurement
- ⏳ Wi-Fi coexistence testing with live effects

**Known and ready:**
- Device identification using existing 12-bit ID from `WirelessManager` (already used by IR and WiFi)
- BLE topology (Pack as Peripheral/Server, Wand as Central/Client)
- GATT service and characteristic design
- Transport fallback logic framework (UART primary, BLE when UART unavailable)
- NimBLE library already in dependencies
- Connection interval targets (7.5–15 ms)
- Pairing/bonding approach

**Remaining implementation tasks:**
1. ✅ ~~Initialize `NimBLEServer` on Pack and `NimBLEClient` on Wand~~ **DONE**
2. ✅ ~~Define GPStar GATT service UUID and command/status characteristics~~ **DONE**
3. Implement deserialization and command queueing in callbacks
4. Implement transport arbitration logic in main loop (UART-first fallback)
5. Measure compile size impact and runtime memory/CPU usage
6. Validate latency and Wi-Fi coexistence with final enclosures

**Deliverables:** Proton Pack and Neutrona Wand can reliably communicate over BLE when UART is unavailable, with UART remaining preferred when available.

### Phase 2: Optional Peer Features and Enhancements (Future)

**Scope:** Wand-to-Wand discovery for crossing-the-streams coordination, multiple-device support, and advanced diagnostics.

**Deferred to Phase 2:**
- Wand-to-Wand peer discovery and RSSI-based proximity detection
- Crossing-the-streams automatic triggering based on proximity
- Multiple Pack support per Wand (if ever needed)
- Advanced scanning policies and rate limiting
- Channel Sounding or external ranging hardware evaluation

**Rationale:** Phase 2 features add complexity, additional BLE connections, and Wi-Fi coexistence risk. They can be developed and tested independently after Phase 1 core functionality is stable and measured. Peer discovery requires enclosure calibration and RSSI profiling that is best done with working Pack/Wand BLE established.

---

## Phase 1 Feasibility Analysis

This section documents the technical foundation that supports Phase 1 implementation.

## Wi-Fi and BLE Coexistence

### Supported Configuration

The ESP32-S3 supports concurrent Wi-Fi and BLE operation. This is not truly simultaneous RF transmission: Wi-Fi and BLE share a single 2.4 GHz RF module. Espressif's coexistence controller arbitrates access using priorities and time division. One protocol cannot transmit or receive while the other owns the RF resource.

Espressif identifies Wi-Fi station-connected plus BLE-connected operation as a supported, stable combination. Some SoftAP scenarios are supported with less stable performance, particularly while the AP is connecting clients or serving active traffic.

This distinction matters because the shared WirelessManager configures `WIFI_AP_STA`, allowing each controller to operate as a station and a local SoftAP. BLE should continue to function while the web interface is enabled, but BLE latency and throughput can vary during:

- Wi-Fi scanning or association.
- SoftAP client connection and disconnection.
- Active web requests or WebSocket/event traffic.
- OTA uploads.
- High Wi-Fi retry rates in a congested 2.4 GHz environment.

Coexistence can also reduce Wi-Fi performance while BLE is active. Neither side should assume that enabling BLE has no effect on existing web and OTA behavior.

### Software Configuration Considerations

Espressif recommends enabling software coexistence when both protocols are used. It also recommends placing the Wi-Fi stack and Bluetooth controller/host tasks on different CPU cores for better coexistence performance. The exact configuration controls available must be confirmed against the ESP-IDF and Arduino core versions bundled by the pinned pioarduino platform.

#### NimBLE Library Status

[NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) (`h2zero/NimBLE-Arduino@^2.5.1`) is the preferred host-stack candidate and is **already added to both project dependencies** (Proton Pack and Neutrona Wand platformio.ini files). NimBLE is chosen because the ESP32-S3 only provides BLE (not Bluetooth Classic) and NimBLE has a smaller memory footprint than Bluedroid. The library headers are included in both projects' `Wireless.h` files, but active BLE implementation (server/client setup, advertising, scanning, and GATT operations) remains pending.

#### NimBLE-Arduino Implementation Model

**Implementation Reference (Phase 1 as implemented):**

Actual UUIDs and callback patterns used in current codebase:
- **Service UUID:** `0000ffe0-0000-1000-8000-00805f9b34fb` (private GPStar service)
- **Command Characteristic UUID:** `0000ffe1-0000-1000-8000-00805f9b34fb` (Wand → Pack)
- **Status Characteristic UUID:** `0000ffe2-0000-1000-8000-00805f9b34fb` (Pack → Wand)
- **Callback Pattern (Pack):** Class-based `GPStarPackCharacteristicCallbacks` inheriting from `NimBLECharacteristicCallbacks` with `onWrite()` method
- **Callback Pattern (Wand):** Function-based `wandNotifyCallback()` registered via `registerForNotify()`
- **Characteristic Discovery (Wand):** `discoverRemoteCharacteristics()` called after pairing completion; discovers service and characteristics, then registers for notifications
- **Connection Parameters:** 7.5–15 ms connection interval, LE Secure Connections with automatic bonding

NimBLE separates two distinct layers:

| Layer | Purpose | Pack Implementation | Wand Implementation |
| --- | --- | --- | --- |
| **GATT** (Generic Attribute Profile) | Data model: services and characteristics | `NimBLEServer` — owns and exposes characteristics | `NimBLEClient` — discovers and accesses Pack characteristics |
| **GAP** (Generic Access Profile) | Connection management: advertising and scanning | `NimBLEAdvertising` — advertises the server | `NimBLEScan` — scans for the Pack's advertisement |

**NimBLEServer** (Pack):
- Creates services and characteristics using `NimBLEDevice::createServer()`.
- Sets characteristic values that the Wand can read/write.
- Starts advertising with `NimBLEAdvertising::start()` so the Wand can discover it.
- Receives writes and provides notifications/indications to connected clients.

**NimBLEClient** (Wand):
- Scans for the Pack's advertised service using `NimBLEDevice::getScan()`.
- Connects to the Pack with `NimBLEClient::connect()`.
- Discovers the Pack's services and characteristics.
- Reads and writes Pack characteristics, triggering Pack-side callbacks.

This topology aligns with NimBLE's design: one device is the authoritative server (Pack) that advertises and owns state; the other (Wand) is the client that discovers and communicates with that state. This is documented further in the "Pairing the Proton Pack and Wand" section below.

### Scanning for Other BLE Devices

Yes, the Wand can scan for BLE devices on demand or according to a timer. Scanning is an asynchronous controller/host operation rather than a blocking application delay, so properly written scan code does not inherently stop UART processing, I2C service, LED animation, or the main loop. Scan results arrive through callbacks and should be copied into a bounded queue rather than extensively processed inside the BLE callback.

An ESP32-S3 can generally maintain an existing BLE connection while scanning. The controller schedules connection events and scan windows on the same radio. This does not create a GPIO or peripheral conflict, but it does create radio contention:

- A scan window listens on BLE advertising channels and cannot simultaneously carry a connected data exchange or Wi-Fi packet.
- The BLE controller may preserve a connection event by shortening or skipping part of a requested scan window, making discovery slower.
- Aggressive scanning may defer a connection event or increase command latency by one or more 7.5 ms connection intervals.
- Active scanning adds transmitted scan requests and received scan responses, using more airtime than passive scanning.
- Wi-Fi shares the same RF hardware, so SoftAP/web/OTA performance and scan completeness can also vary during a scan.
- A large number of advertisements can produce many host callbacks and allocations; duplicate filtering and bounded result storage are important.

Scanning is not required to maintain or monitor an established Pack/Wand connection. BLE connection events and the supervision timeout already provide link health. In this design, routine periodic scans while connected provide little value and should normally be disabled.

#### Recommended Scan Policy

| Device state | Suggested scan behavior | Example scan window / interval | Nominal BLE scan duty | Suggested scan duration | Expected effect |
| --- | --- | ---: | ---: | ---: | --- |
| UART connected | Do not scan automatically | N/A | 0% | N/A | No BLE RF impact; UART remains authoritative |
| BLE connected to selected Pack | Normally do not scan | N/A | 0% | N/A | Lowest command jitter and Wi-Fi impact |
| User opens “Find Packs” or starts pairing | Passive scan first; active only if required data is absent | 50 ms / 100 ms | 50% | 3–5 seconds | Fast discovery, but temporary BLE/Wi-Fi latency impact is expected |
| UART unavailable and selected Pack is disconnected | Short passive scan burst followed by backoff | 30 ms / 100 ms | 30% during burst | 2–3 seconds, then wait 2–5 seconds before retrying | Responsive reconnection without continuous scanning |
| Optional connected diagnostic refresh | Low-duty passive scan only | 20 ms / 500 ms | 4% | 2–3 seconds | Usually modest impact, but command jitter must still be measured |

These are starting values for testing rather than guaranteed optimal settings. A user-initiated 3–5 second pairing scan is preferable to a continuous background scan because the temporary performance cost is bounded and visible in context. If scanning must occur while effects are active, a 4% low-duty scan is less disruptive but may miss advertisers with long advertising intervals and may require a longer total scan duration.

The Pack advertisement should place the private GPStar service UUID and short Pack ID in the primary advertising packet whenever they fit. That permits passive discovery and avoids active scan request/response traffic. Legacy BLE advertisements allow 31 bytes of advertising data, so the UUID and identity encoding must be budgeted carefully; nonessential human-readable text can remain in the scan response or be read after connection.

#### Does a Scan Interrupt Current Behavior?

It should not functionally interrupt current behavior in the sense of stopping animations, serial handling, or an established BLE connection. It can temporarily degrade timing because it consumes radio opportunities and generates callbacks. With a 7.5 ms Pack/Wand connection interval, the desired acceptance criterion should be that a scan does not cause disconnects and does not push critical command latency beyond the chosen limit. That must be measured while both devices run normal effects and Wi-Fi traffic.

For the single-Pack design, the simplest policy is:

1. Scan aggressively only while disconnected or during an explicit user pairing/search action.
2. Stop scanning immediately after the selected Pack is found and a connection attempt begins.
3. Do not scan for unrelated devices during normal connected operation.
4. If a user explicitly requests a scan while connected, temporarily use a bounded low-duty scan and expose that discovery may add command jitter.
5. Cancel or defer optional scans during OTA, Wi-Fi association, and timing-sensitive Pack/Wand state transitions.

## Wand-to-Wand Discovery and Crossing the Streams

### Central Role Versus Application Hub

Making the Wand the BLE central does not require making it the authoritative application hub. These terms describe different layers:

- The BLE **central** is the device that initiates a connection. The proposed Wand already has this role when it scans for and connects to its Pack.
- The GATT **client** reads or writes attributes exposed by a GATT server. The proposed Wand already has this role for Pack communication.
- The application **hub** is the device that owns authoritative state and resolves commands. That should remain the Pack for Pack effects because the Pack owns its own audio, lighting, alarm, overheat, and safety behavior.
- A BLE device can expose different behavior for different relationships. A Wand can initiate its Pack connection while also transmitting non-connectable advertisements that other Wands observe.

The fact that firing controls originate at the Wand is a good reason for the Wand to detect and coordinate the peer effect. It is not, by itself, a reason to route all Pack/Wand state through a new multi-Wand hub. Inverting the existing authority would increase failure coupling: loss or reset of one coordinating Wand could affect another user's Pack, and nearby unpaired Wands would need a new authorization model before being allowed to influence Pack state.

The existing shared API already contains Wand-originated crossing-the-streams commands and firing states, including `A_CROSS_THE_STREAMS`, `A_FIRING_CTS`, and their stop and mix variants. A peer detector can therefore select an established local firing behavior; it does not need to replace the normal Pack/Wand command model.

### Recommended Topology

Use two logically separate BLE relationships:

1. **Trusted Pack control:** one Wand communicates only with its selected Pack over UART, or over the bonded BLE connection when UART is unavailable. The Pack remains authoritative for that pair.
2. **Untrusted peer discovery:** every opted-in Wand periodically sends a small GPStar peer advertisement and performs bounded passive scans for advertisements from nearby Wands.
3. **Local coordination:** each Wand decides whether its own firing state qualifies, then asks only its own Pack to enter or leave the crossing-the-streams behavior through the existing authoritative transport.

This is decentralized discovery rather than one Wand controlling the others. It avoids electing a permanent leader and continues to work when either Wand uses UART to its own Pack. It also preserves the one-bond/one-Wand rule on each Pack.

The peer advertisement should be non-connectable unless experiments prove that a connected handshake is necessary. A compact legacy advertisement can include a private peer-service UUID plus fields such as protocol version, ephemeral Wand identifier, opt-in status, firing eligibility, firing session counter, and a changing freshness value. It should not contain a command that directly controls a foreign Pack.

### Proximity Is an Estimate

BLE can identify another participating Wand in the area, but it cannot reliably prove that two physical stream props are touching or aimed at one another. Received Signal Strength Indicator (RSSI) is strongly affected by body position, antenna orientation, enclosure materials, transmit power, reflections, and nearby 2.4 GHz traffic. Two Wands at the same distance can report substantially different RSSI values, while a reflected signal from a farther Wand may briefly appear stronger.

The ESP32-S3 BLE scan result includes RSSI for each received advertisement. Espressif represents it as received power in dBm, so a reading such as -45 dBm is stronger than -70 dBm. This directly supports threshold, averaging, and hysteresis logic without first establishing a Wand-to-Wand connection.

An idealized logarithmic path-loss model can convert RSSI into an estimated distance:

$$
d = 10^{\frac{P_{1\text{m}}-RSSI}{10n}}
$$

where $d$ is estimated distance in meters, $P_{1\text{m}}$ is the calibrated RSSI from that Wand design at 1 m, and $n$ is an environmental path-loss exponent. Free space is often approximated near $n=2$; indoor, body-shadowed, and reflective environments vary and can require substantially different values. Because the result is exponential, a modest RSSI error produces a large distance error. A 6 dB change corresponds to approximately a factor of two in distance only when $n=2$ and the propagation model actually holds.

For this use case, calculating and displaying feet from that equation would imply more accuracy than the radio provides. The better control input is a filtered RSSI score compared with thresholds calibrated from complete Wands. Including a fixed calibrated reference level or actual transmit-power field in the peer advertisement can improve consistency between units, but it cannot remove body and multipath effects.

RSSI should therefore be treated as a coarse proximity hint rather than a distance measurement. A robust trigger would require all of the following:

- The feature is explicitly enabled on both Wands.
- Both advertisements report a compatible protocol and an eligible active-firing state.
- RSSI remains above an experimentally selected enter threshold for several observations or a minimum dwell time.
- A lower exit threshold and timeout provide hysteresis so the effect does not rapidly toggle near the boundary.
- Stale session counters, duplicate advertisements, and observations older than the freshness timeout are ignored.
- Each Wand immediately exits the peer effect if its own trigger is released, its Pack link is lost, or peer observations expire.

The intended 5–6 ft Pack/Wand operating radius does not define an appropriate Wand-to-Wand threshold. That threshold must be measured with completed Wand enclosures in realistic poses. RSSI alone is unlikely to distinguish, for example, 1 ft from 3 ft consistently enough for a precise physical crossing gesture.

### Time-of-Flight Ranging

Ordinary BLE advertisement or GATT timing cannot be converted into useful time-of-flight distance on the ESP32-S3. Radio propagation takes approximately 1 ns per foot one way. Packet callbacks occur after controller scheduling, radio processing, operating-system scheduling, and possible retransmission delays measured in microseconds or milliseconds. Those variable delays are many orders of magnitude larger than the propagation-time difference of interest.

Bluetooth Channel Sounding is the Bluetooth feature intended for precise ranging. It combines phase-based ranging and round-trip-time measurements and requires support in both devices' Bluetooth controllers and physical layers. The ESP32-S3 is a Bluetooth 5 LE device that predates Channel Sounding hardware. Newer ESP-IDF API documentation may contain common host definitions for Channel Sounding and newer targets, but the presence of those declarations does not add the required capability to the ESP32-S3 controller. This feature cannot be enabled on the existing boards by a firmware update alone.

Therefore:

- Use RSSI for a coarse automatic “close enough” effect on the existing hardware.
- Do not describe RSSI-derived output as measured distance or time of flight.
- If repeatable physical distance is mandatory, use hardware designed for ranging, such as a Channel-Sounding-capable BLE replacement or an external ultra-wideband transceiver.

### Candidate Automatic CTS Gate

An initial research state model could require both firing state and filtered proximity:

1. Remain inactive unless the local Wand has the feature enabled and is actively firing in a compatible mode.
2. Enter a candidate state after receiving a fresh compatible peer advertisement that also reports active firing and feature opt-in.
3. Collect at least three peer observations over approximately 300–500 ms and calculate a median or similarly outlier-resistant filtered RSSI.
4. Engage CTS only while filtered RSSI is at or above a calibrated `RSSI_ENTER` threshold and both firing states remain current.
5. Exit CTS immediately when the local trigger is released or a fresh peer advertisement reports firing stopped.
6. Also exit when no fresh peer advertisement arrives for approximately 500–750 ms, or when filtered RSSI remains below `RSSI_EXIT` for approximately 500–1000 ms.
7. Set `RSSI_EXIT` approximately 6–10 dB weaker than `RSSI_ENTER` as an initial hysteresis test range, then tune both from enclosure measurements.

No universal value such as -50 dBm or -60 dBm should be assigned to a distance before measurement. The calibration should record RSSI distributions, not just averages, at the desired engage distance and at distances that must not engage. A threshold is acceptable only if those distributions separate sufficiently across users, orientations, final enclosures, Wi-Fi activity, and representative venues.

When more than two Wands are present, each Wand should select the strongest qualifying peer and bind the effect to that peer's short-lived firing-session identifier. An already-engaged CTS advertisement must not cause a chain reaction by itself: every participant must still report its own active trigger and independently satisfy the proximity gate. If both Wands must always agree on the same instant and peer, advertisements alone are insufficient and a short acknowledged session handshake is required.

### Discovery-Only Versus Connected Coordination

| Approach | Advantages | Limitations | Suitability |
| --- | --- | --- | --- |
| Non-connectable advertisements plus passive scans | No pairing, no leader, works while each Wand uses UART or BLE to its Pack, low protocol overhead | Advertisements are lossy and unauthenticated; RSSI is noisy; state changes are eventually observed rather than acknowledged | Best initial feasibility path for an imitative effect |
| Short Wand-to-Wand BLE connection | Bidirectional handshake, acknowledgement, negotiated start, and stronger freshness semantics | Adds another connection, role/state complexity, RF scheduling load, disconnect handling, and peer authorization | Consider only if measured advertisement timing is inadequate |
| One Wand as permanent multi-device application hub | Centralized decisions and explicit synchronization | Single point of failure, unclear ownership, foreign-Pack security risk, more bonds and connections, and behavior coupled to leader availability | Not recommended for this feature |
| BLE Mesh | Designed for larger many-to-many networks | Substantially more complexity and overhead than a two-Wand proximity effect requires | Not recommended |

Advertisements cannot guarantee simultaneous starts because they are one-way and may collide or be missed. For a theatrical imitative effect, a bounded difference of one or two advertisement periods may be acceptable. If synchronized audio or lighting requires a tighter and measured start bound, the Wands need a brief acknowledged handshake with a future start timestamp or another synchronization mechanism; RSSI discovery alone is insufficient.

### Radio Scheduling Policy

Peer discovery differs from a user-initiated Pack search because it may need to run while firing. The Wand can scan while maintaining its BLE Pack connection, but scanning, peer advertising, the Pack connection, and Wi-Fi all share one radio. A low-duty passive scan and a moderate peer advertising interval should be tested first. The discovery latency is approximately governed by the advertising interval, scan interval, packet loss, and whether their radio windows overlap; it is not guaranteed by any one setting.

A practical research target is to advertise peer state every 100–250 ms only while the feature is enabled, and use short passive scan windows during eligible firing states rather than scanning aggressively at all times. These values are initial test points, not guarantees. Whether the exact pinned Arduino/NimBLE combination permits advertising and scanning as concurrent GAP procedures must be verified. If it does not, short alternating advertise and scan phases provide the same decentralized design with greater discovery latency. Optional peer discovery should be suspended during OTA and should never be allowed to starve the bonded Pack connection or delay a local stop-firing command.

### Security and User Experience Boundaries

Nearby-device discovery should be opt-in and distinct from Pack pairing. A detected peer must never be accepted as a replacement Pack controller, added to the Pack's bond list, or allowed to send general GPStar commands. At minimum, peer advertisements should use a private service UUID, protocol version, short-lived session identity, freshness checks, and a feature-enabled flag on both devices.

Advertisements are observable and spoofable unless an application authentication scheme is added. If false triggering is merely an undesirable theatrical effect, mutual opt-in plus temporal and RSSI filtering may be proportionate. If the feature can trigger hazardous hardware behavior, heat, high current, lockout changes, or safety-critical state, unauthenticated advertisements are not sufficient and the behavior must require an authenticated handshake with strict local safety interlocks.

## Transport Selection and Fallback

### UART-First Policy

The physical serial connection should have priority. BLE should be considered only when the expected UART request/response or synchronization exchange does not succeed. Merely seeing idle RX/TX lines is not sufficient evidence that no cable is attached because a healthy link may be quiet between messages.

A future transport policy should follow this sequence:

1. On startup, attempt the existing UART synchronization for a defined discovery window.
2. If a valid UART peer responds, use UART and keep BLE control inactive.
3. If UART synchronization times out, activate BLE discovery and attempt a connection to the configured Pack ID.
4. If BLE connects, perform protocol compatibility and full-state synchronization checks before accepting control traffic.
5. Continue probing UART conservatively while on BLE, or probe only at defined safe times.
6. If a valid UART connection later appears, perform an orderly state synchronization and hand back to UART rather than allowing both transports to issue commands concurrently.
7. If neither transport is valid, enter the existing disconnected or standalone behavior rather than assuming the last state remains safe.

Only one transport should be authoritative at a time. Receiving equivalent commands over both UART and BLE could otherwise duplicate state transitions, sounds, firing actions, or preference updates. Transport ownership should therefore be an explicit state, not an incidental consequence of whichever callback runs first.

The existing 750 ms UART synchronization retry and 8 second Pack-side disconnect timeout provide useful context, but a fallback design may require a separate initial UART discovery window, BLE connection timeout, and shorter transport-loss detection for safety-critical state.

### Return to UART

Returning from BLE to a newly detected cable should not happen in the middle of a critical operation without defined behavior. The devices should first confirm protocol compatibility, exchange complete current state, choose an authority transition point, and then stop accepting commands from BLE. Disconnecting BLE after the handoff is the simplest way to preserve the one-link model.

## Pairing the Proton Pack and Wand (Phase 1)

### Feasibility and NimBLE Implementation Pattern (IMPLEMENTED)

The two devices establish a persistent BLE relationship using NimBLE's server/client model:

**Proton Pack** acts as the `NimBLEServer` (implemented in `ProtonPack/include/Bluetooth.h`):
- Creates and hosts a private GPStar service (UUID: `0000ffe0-0000-1000-8000-00805f9b34fb`)
- Defines two characteristics:
  - **Command** (UUID: `0000ffe1-...`): Write property, receives Wand commands via `GPStarPackCharacteristicCallbacks::onWrite()`
  - **Status** (UUID: `0000ffe2-...`): Notify property, sends Pack state via `notify()`
- Starts advertising service UUID and Pack name ("GPStar-Pack-XXXX") via `NimBLEAdvertising`
- Listens for incoming client connections and handles write callbacks
- Uses LE Secure Connections with automatic bonding for pairing
- Enforces single-Wand bonding and one active connection at the application layer

**Neutrona Wand** acts as the `NimBLEClient` (implemented in `NeutronaWand/include/Bluetooth.h`):
- Scans for the Pack's advertised GPStar service using `NimBLEScan`
- Discovered Packs presented via `GPStarWandScanCallbacks::onResult()` callback
- Initiates connection to selected Pack via `NimBLEClient::connect()` with `GPStarWandClientCallbacks` for lifecycle events
- Performs LE Secure Connections pairing automatically
- After pairing completes (`onAuthenticationComplete()`), calls `discoverRemoteCharacteristics()` to:
  - Discover remote GPStar service via `getService()`
  - Discover command and status characteristics via `getCharacteristic()`
  - Register `wandNotifyCallback()` for status notifications via `registerForNotify()`
- Writes commands to Pack via `g_pRemoteCommandChar->writeValue()`
- Receives Pack state via `wandNotifyCallback()` notifications

**Dual-transport sending** (implemented in both `Serial.h` files):
- Single serialization: `i_send_size = coms.txObj(object)`
- Both UART and BLE paths: `coms.sendData(i_send_size, PACKET_TYPE)` + `bleSendData(coms.packet.txBuff, i_send_size)`
- Same serialized bytes sent both ways; BLE bypasses SerialTransfer framing

The Wand is the client because firing control originates there—it must initiate the discovery and connection. The Pack is the server because it is the authoritative hub that owns its own state and controls power, audio, lighting, alarms, and safety features. This topology is correct and aligns with how NimBLE-Arduino is designed to be used.

### Pack ID and User Selection

The user needs a stable, human-manageable identifier to select the intended Proton Pack. The Pack ID should identify a device but should not itself be treated as a password or cryptographic secret.

**Current Implementation:** The Pack advertises its ID in the device name as "GPStar-Pack-XXXX" (where XXXX is extracted from the 12-bit WirelessManager device ID in hexadecimal). The Wand's scan callback parses this name and can store the Pack ID for persistent bonding and reconnection.

A future design could derive a short display ID from the ESP32-S3's factory-programmed unique base MAC address or store a generated identifier in NVS. A shortened value is easier to enter, but the complete underlying identity should remain available internally to avoid collisions. The advertised local name includes the short ID (e.g., `GPSTAR-PACK-A1B2C3`), while service data carries the full identifier.

The Pack ID should be available through at least one dependable user-facing path:

- The Proton Pack web interface and API.
- A label or QR code attached to the controller or enclosure.
- Serial diagnostics during setup.
- A pairing mode that lists nearby Pack IDs in the Wand's web interface.

The web interfaces are likely the clearest management path because both devices already expose Wi-Fi configuration surfaces. A physical label or QR code remains useful after settings are reset or when no prior network configuration is known.

### Pairing Workflow

**Infrastructure Status (as of 2026-09-08):** Pairing and bonding are automatically handled by NimBLE's LE Secure Connections. Packing numeric comparison for PIN confirmation occurs in the Pack's serial debug output and must be confirmed in the initiating Wand's BLE stack. After bonding, reconnection uses the stored bond instead of repeating pairing.

**Application Integration:** The following steps remain pending:
1. ~~The user obtains the Pack ID from the Pack web interface, label, QR code, or setup diagnostics.~~ **Infrastructure ready** (Pack advertises as "GPStar-Pack-XXXX")
2. ~~The user enters or selects that Pack ID in the Wand configuration.~~ **Web interface integration needed**
3. ~~The Pack is placed into an explicit, time-limited pairing mode through a physical action or authenticated Pack web interface.~~ **Pairing mode UI needed**
4. ~~With UART unavailable, the Wand scans for the matching service and Pack ID.~~ **Scan infrastructure ready**, needs UART-first fallback logic
5. ~~The devices perform authenticated pairing and store a bond.~~ **Automatic via LE Secure Connections** ✅
6. ~~The Pack records that Wand as its sole permitted Wand peer and rejects other control connections.~~ **Bond storage ready**, needs application-level enforcement
7. ~~Pairing mode ends after success or timeout.~~ **Timeout logic needed**

Normal advertising should not allow an arbitrary nearby Wand to replace the current bond. Replacing a Wand should require an explicit unpair/reset action on the Pack, and preferably confirmation from both devices. If either side has a stale or different bond, the UI should expose that state rather than repeatedly failing without explanation.

### Security and Identity Requirements (IMPLEMENTED)

Automatic reconnection should not be based only on a human-readable device name. The current design addresses:

- ✅ **LE Secure Connections and bonding via NimBLE's pairing APIs** — Implemented with automatic numeric comparison pairing
- ✅ **Bonding enforcement** — NimBLE stores bond keys after pairing; reconnection attempts to use existing bond
- 🟡 **Whitelisting or resolving-list use** — NimBLE supports this, but application-level selection logic not yet integrated
- 🟡 **Deliberate first-pairing procedure** — Infrastructure ready, needs application mode (auto-pairing on first UART-unavailable connection vs. explicit pairing mode)
- ⏳ **Physical or authenticated method to erase and replace a bond** — Needs factory-reset or admin endpoint
- ⏳ **Protection against replayed or stale control messages** — Needs sequence numbering in application protocol
- ⏳ **Behavior when stored bonds disagree** — Needs conflict resolution logic

For this use case, the Pack should store one active Wand bond, permit one active Wand BLE connection, and require an explicit replacement procedure before another Wand can take control. Automatic reconnection should use the bonded identity or BLE resolving list rather than relying only on the advertised name or short Pack ID. The short ID helps the user select the Pack; cryptographic bond identity determines whether the peer is trusted.

Pairing is not synonymous with application reliability. Bonding authenticates the peer and assists reconnection, but the GPStar protocol still needs message ordering, duplicate handling, synchronization, and safety semantics.

## Phase 1 Summary and Implementation Status

The BLE feasibility study confirmed that Phase 1 is technically viable. **As of 2026-09-08, the core BLE infrastructure is now IMPLEMENTED:**

**Confirmed Technical Foundation:**
1. ✅ **Technical viability**: Wi-Fi and BLE coexistence is supported on the ESP32-S3.
2. ✅ **GPIO compatibility**: BLE requires no additional GPIO pins or peripheral modifications.
3. ✅ **Device identification**: Unified 12-bit device ID from `WirelessManager` (reuses IR and WiFi infrastructure).
4. ✅ **Performance**: BLE meets latency requirements for Pack/Wand control (7.5–15 ms connection intervals).
5. ✅ **Transport hierarchy**: UART remains primary; BLE is a fallback when UART is unavailable.
6. ✅ **Library readiness**: NimBLE-Arduino is already integrated into both projects with headers included.
7. ✅ **NimBLE API alignment**: The documented server/client pattern (Pack as server, Wand as client) matches the intended architecture.

**Phase 1 Implementation Status:**
- ✅ Initialize Pack as `NimBLEServer` with a private GPStar service and command/status characteristics
- ✅ Initialize Wand as `NimBLEClient` that scans for and connects to the Pack
- ✅ Implement characteristic write callbacks on the Pack to receive Wand commands
- ✅ Implement client notification subscriptions on the Wand to receive Pack state
- ✅ Dual-transport sending (UART + BLE) implemented in both `Serial.h` files
- ✅ LE Secure Connections pairing with automatic bonding
- 🟡 Deserialization logic exists with TODO comments (callbacks fire, data needs parsing and queueing)
- ⏳ Transport selection logic (UART-first fallback) not yet integrated into main application loop
- ⏳ Compile size and runtime performance measurement (primary unknown)

**Code Locations:**
- **Pack (Server):** `source/ProtonPack/include/Bluetooth.h`
- **Wand (Client):** `source/NeutronaWand/include/Bluetooth.h`
- **Dual-transport sends:** `source/ProtonPack/include/Serial.h`, `source/NeutronaWand/include/Serial.h`

**Implementation references from NimBLE-Arduino documentation:**
- [Pack (Server) API](https://github.com/h2zero/NimBLE-Arduino/blob/master/docs/New_user_guide.md#creating-a-server): `NimBLEDevice::createServer()`, `NimBLEServer::createService()`, characteristic creation and callbacks.
- [Wand (Client) API](https://github.com/h2zero/NimBLE-Arduino/blob/master/docs/New_user_guide.md#creating-a-client): `NimBLEDevice::getScan()`, `NimBLEClient::connect()`, service/characteristic discovery.

**Phase 1 does NOT include:**
- Wand-to-Wand peer discovery or crossing-the-streams coordination
- Multiple Pack support
- Advanced scanning or diagnostics

**Next Steps for Production Readiness:**
1. Implement deserialization in `wandNotifyCallback()` and Pack's `onWrite()` callback
2. Integrate transport fallback logic (UART-first, BLE when UART unavailable)
3. Measure compile size impact and runtime memory/CPU usage
4. Test Wi-Fi coexistence during live effects
5. Validate connection reliability and reconnection behavior
6. Implement application-level bond management (single Wand per Pack)

---

## Testing and Validation with Mobile Apps

### iOS and Android BLE Scanners

You can use a smartphone BLE scanner app to validate Pack/Wand BLE initialization, advertising, and GATT discovery without waiting for full deserialization code. This is useful for rapid debugging and verifying UUID configuration.

#### Recommended Apps

| App | Platform | Use case | Notes |
| --- | --- | --- | --- |
| **nRF Connect** | iOS, Android | Professional BLE debugging | Full GATT explorer, read/write characteristics, connection parameters, real-time packet inspection |
| **LightBlue** | iOS, Android | User-friendly discovery | Good for seeing service/characteristic structure, simpler than nRF Connect |
| **BLE Scanner** | Android | Quick scanning | Shows RSSI, device info, basic characteristic read/write |
| **Core Bluetooth** (Xcode) | macOS | Hardware validation | Built into Xcode for Mac Bluetooth debugging; works with iOS simulators on M-series Macs |

**For Phase 1 testing, nRF Connect is recommended** because it provides complete GATT visibility, allows writing to characteristics without implementing a full client, and shows real-time connection parameters.

**Status: These tests are now immediately available** with the implemented Pack server and Wand client infrastructure.

### Typical Testing Workflow

#### 1. Validate Pack Advertisement (NOW AVAILABLE)

**Setup:** Power on the Proton Pack with BLE enabled (infrastructure complete as of 2026-09-08).

**Using nRF Connect (iOS/Android):**
1. Open nRF Connect.
2. Tap **"Scan"**.
3. Look for a device named `GPStar-Pack-XXXX` (where XXXX is the Pack ID in hex).
4. Tap the device to expand details.
5. Verify you see the GPStar service UUID: `0000ffe0-0000-1000-8000-00805f9b34fb`.
6. Verify RSSI varies as you move the phone away/toward the Pack (typical range: -40 dBm at 1 ft, -60 dBm at 6 ft, very variable).

**If the Pack does not appear:**
- Check that `startBluetooth()` succeeded (look for debug output: `"========== BLE Server (Pack) RUNNING =========="`).
- Verify the Pack's `DEBUG_BLUETOOTH` flag is enabled and check the serial output for initialization errors.
- Confirm `NimBLEDevice::init()` and `NimBLEAdvertising::start()` executed without exceptions.
- Verify the service UUID string matches exactly in both `Bluetooth.h` files.

#### 2. Discover Pack Services and Characteristics

**Continuing with nRF Connect (connected to the Pack):**
1. Tap **"Connect"** to establish a BLE connection.
2. After connection, nRF Connect automatically discovers services and characteristics.
3. Expand the **"GPStar Service"** (UUID `0000ffe0-...`).
4. You should see two characteristics:
   - **Command characteristic** (UUID `0000ffe1-...`): write permission (Wand sends commands here)
   - **Status characteristic** (UUID `0000ffe2-...`): notify permission (Pack broadcasts state here)
5. If characteristics are missing, check `startBluetooth()` for characteristic creation failures.

#### 3. Test Command Characteristic Write

**Simulating a Wand command:**
1. In nRF Connect, tap the **Command characteristic** (`0000ffe1-...`).
2. Select **"Write"** mode and choose an encoding (e.g., **Hex** or **Text**).
3. Enter a test hex value (e.g., `01` for a simple byte).
4. Tap **"Send"**.
5. Check the Pack's serial output for the write callback: `"[BLE-Pack] Command received from Wand (X bytes, addr: ...)"`
6. If the callback does not fire, verify the characteristic callbacks are set correctly: `g_pCommandCharacteristic->setCallbacks(new GPStarPackCharacteristicCallbacks())`.

#### 4. Validate Status Characteristic Subscription

**Simulating a Wand notification listener:**
1. In nRF Connect, tap the **Status characteristic** (`0000ffe2-...`).
2. Tap **"Notify"** (enable notifications from the Pack).
3. nRF Connect should show **"Notifications enabled"**.
4. On the Pack side, you can manually write a test value to the characteristic (from an internal Pack function or debug command).
5. The notification should appear in nRF Connect's display in real time.
6. If notifications don't arrive, verify:
   - `g_pStatusCharacteristic->setValue()` is called when Pack state changes.
   - `g_pStatusCharacteristic->notify()` is called to transmit to subscribed clients.

#### 5. Connection Parameters and RSSI

**Monitor link quality:**
- In nRF Connect, after connecting, you can see:
  - **Connection interval**: Should be approximately 7.5–15 ms (80–120 in x1.25ms units).
  - **RSSI**: Real-time signal strength as you move the phone around.
  - **TX Power**: Current transmit level.
  - **MTU**: Data Length Extension negotiation status.

### Wand Testing Without Full Client Implementation (INFRASTRUCTURE NOW IMPLEMENTED)

The Wand BLE client is now implemented as of 2026-09-08, so testing can occur with both devices:

**Wand-to-Pack Connection Test:**
1. Power the Pack with BLE enabled.
2. Power the Wand with BLE enabled.
3. Monitor Wand serial output for scan callback debug messages: `"[BLE-Wand] Found device:"`, `"[BLE-Wand] *** PACK FOUND! ***"`.
4. Verify pairing sequence: Wand should show `"[BLE-Wand] *** PAIRING COMPLETE ***"`.
5. Verify characteristic discovery: Wand should log `"[BLE-Wand] *** Found remote GPStar service ***"`, `"[BLE-Wand] *** BLE READY FOR DATA EXCHANGE ***"`.

**Alternatively, use nRF Connect as a simulated Wand** to test the Pack's server half independently:

1. **nRF Connect on iPhone/iPad** acts as the Wand (GATT client).
2. Power the Pack normally.
3. Open nRF Connect, scan, and connect to `GPStar-Pack-XXXX`.
4. Manually write test commands to the Command characteristic.
5. Verify the Pack's write callback logs the expected debug output.
6. Subscribe to Status and trigger test notifications from the Pack.

This validates the Pack BLE server implementation independently, useful for debugging Pack behavior before requiring Wand cooperation.

### Bonding and Pairing State

If the Pack is configured to use LE Secure Connections pairing:

1. **First connection attempt**: nRF Connect will prompt for pairing.
2. **PIN confirmation**: Both devices show a numeric comparison (e.g., "123456"). Confirm it matches on both sides (the Pack serial console should print the PIN; nRF Connect displays it in the app).
3. **Bonding**: After confirmation, the devices bond and store a persistent bond key.
4. **Reconnection**: Subsequent connections skip the pairing dialog and use the stored bond.

**To clear a bond on the Pack:**
- Erase NVS (non-volatile storage) during a setup or debug mode, or implement a factory-reset command in the web interface.
- Reconnecting the app then re-pairs and bonds afresh.

### Android Alternatives

**nRF Connect (Android)** provides the same functionality as iOS. **BLE Scanner** is lighter-weight if you only need to verify advertisement and basic read/write.

### Debugging Coexistence

If you have both Wi-Fi and BLE enabled:

1. Enable Wi-Fi on the Pack (e.g., connect to a local network or start the SoftAP).
2. Simultaneously use nRF Connect to scan and connect.
3. Observe RSSI and connection stability as you browse the web interface or upload files over OTA.
4. Look for:
   - Sudden RSSI drops during Wi-Fi activity (expected due to shared radio).
   - BLE connection drops (should be rare unless severe coexistence issues).
   - Notification delays when Wi-Fi traffic is heavy.

This provides early warning of coexistence problems before deploying the Wand client.

---

## Phase 2: Future Enhancements

### Wand-to-Wand Discovery and Crossing the Streams

The following sections describe potential Phase 2 enhancements for Wand-to-Wand peer discovery and crossing-the-streams coordination. These are deferred until Phase 1 core Pack/Wand BLE functionality is stable and measured.

#### Central Role Versus Application Hub

### Connection Setup Versus Per-Message Delay

BLE has both a one-time connection cost and a smaller recurring scheduling delay on every send.

The one-time path includes advertising, scanning, connection establishment, optional pairing/encryption, service discovery, characteristic subscription, and application synchronization. Its duration depends heavily on advertising and scan intervals. With purposefully responsive settings and a previously bonded peer, a practical design target is roughly 100–500 ms to reconnect and synchronize, but occasional longer results should be expected under Wi-Fi contention or missed advertisements. First-time pairing and service discovery can take several hundred milliseconds to multiple seconds. These are design targets to verify, not guaranteed ESP32-S3 timings.

After connection, there is no new pairing or service discovery for every message. However, data is still transmitted only during connection events. If a message is queued at a random point in a 7.5 ms connection interval, its scheduling wait is nominally 0–7.5 ms, averaging about 3.75 ms. At a 15 ms interval, the nominal wait is 0–15 ms, averaging about 7.5 ms. Stack dispatch, application callbacks, Wi-Fi coexistence, encryption, and retransmissions add time beyond that wait.

A reasonable initial target for this short-range control link would be a 7.5 or 15 ms connection interval with no intentional peripheral event skipping while active. Under clean RF conditions, a small unacknowledged application message would reasonably be expected to reach the peer callback in approximately 3–12 ms at a 7.5 ms interval or 5–20 ms at a 15 ms interval. Wi-Fi contention or a lost packet can push delivery into one or more later connection events. These ranges are engineering estimates and require measurement on the installed hardware.

An acknowledged request takes longer because confirmation must travel back. A GATT write request or application-level command/acknowledgement would commonly complete in approximately one to two connection intervals when the receiver can respond promptly: roughly 8–20 ms with a 7.5 ms interval or 15–35 ms with a 15 ms interval under clean conditions. A delayed application callback or Wi-Fi arbitration can increase this further.

### Concrete UART and BLE Comparison

The existing shared communication header documents the actual UART frame sizes. SerialTransfer adds 6 framing bytes to each payload. The BLE estimates below assume a one-byte GPStar packet-type envelope, a negotiated ATT MTU of at least 64, Data Length Extension sufficient to carry each value without link-layer fragmentation, no intentional peripheral event skipping, and an already-established connection. They do not include initial connection or pairing time.

**Direct conclusion:** yes, after BLE is already connected, a common command can often arrive faster over BLE at a 7.5 ms connection interval than over the current 9600-baud UART. UART needs about 12.5 ms just to serialize the complete framed command. BLE normally waits no more than one 7.5 ms connection interval, then sends the small command in well under 1 ms of radio airtime; stack and callback processing add additional time. A reasonable clean-link expectation is therefore approximately 3–12 ms for BLE versus at least 12.5 ms plus receiver polling for UART.

This is a typical-latency advantage, not a guaranteed-latency advantage. A BLE command that misses an event, loses RF arbitration to Wi-Fi, or requires retransmission can take 15 ms, 22.5 ms, or longer because it moves into later connection events. The UART frame normally remains close to its fixed 12.5 ms serialization time. BLE can therefore be faster in the normal case while UART remains more deterministic and may have the better worst case.

| Existing packet | Struct payload | UART bytes with SerialTransfer | UART serialization at 9600 baud | BLE value with 1-byte type | Approximate BLE data-PDU airtime at 1M / 2M PHY | Estimated BLE one-way delivery at 7.5 ms interval | Estimated BLE one-way delivery at 15 ms interval |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `CommandPacket` | 6 bytes | 12 bytes | 12.5 ms | 7 bytes | 0.19 / 0.10 ms | 3–12 ms | 5–20 ms |
| `DataPacket` | 7 bytes | 13 bytes | 13.5 ms | 8 bytes | 0.20 / 0.10 ms | 3–12 ms | 5–20 ms |
| `WandSyncData` | 9 bytes | 15 bytes | 15.6 ms | 10 bytes | 0.22 / 0.11 ms | 3–12 ms | 5–20 ms |
| `SmokePrefs` | 12 bytes | 18 bytes | 18.8 ms | 13 bytes | 0.24 / 0.12 ms | 3–12 ms | 5–20 ms |
| `WandPrefs` | 16 bytes | 22 bytes | 22.9 ms | 17 bytes | 0.27 / 0.14 ms | 3–12 ms | 5–20 ms |
| `PackPrefs` | 26 bytes | 32 bytes | 33.3 ms | 27 bytes | 0.35 / 0.18 ms | 3–12 ms | 5–20 ms |

The BLE PDU-airtime column approximates only the data packet on the radio, including normal link/L2CAP/ATT headers but excluding inter-frame spacing, the peer's link-layer acknowledgement packet, encryption MIC, stack processing, and Wi-Fi arbitration. Encryption adds approximately 0.03 ms at 1M PHY or 0.02 ms at 2M PHY to this data PDU. These sub-millisecond differences explain why all rows have similar end-to-end BLE estimates: waiting for the next connection event dominates the actual payload transmission time.

The estimated BLE ranges are queued-to-peer-callback engineering targets under clean RF conditions, not Bluetooth guarantees. A message queued just before an event may arrive near the bottom of the range; one queued just after an event waits nearly a full interval. Wi-Fi arbitration, task scheduling, or a retransmission can add one or more connection intervals. At 7.5 ms, plausible delayed results therefore progress roughly into the 15–25 ms range and beyond; at 15 ms, they can progress into the 25–40 ms range and beyond.

With the default ATT MTU of 23, a write command or notification has a 20-byte value limit. The 7, 8, 10, 13, and 17-byte enveloped values fit in one operation. The 27-byte enveloped `PackPrefs` does not: it needs application fragmentation or a negotiated ATT MTU of at least 30. Negotiating an ATT MTU of 64 or greater comfortably fits every current Pack/Wand payload in one GATT operation. ATT MTU controls operation size; the separate BLE Data Length Extension controls whether the lower layers can carry that operation efficiently in one link-layer data packet.

For the common 6-byte command, UART occupies the wire for about 12.5 ms. Connected BLE at a 7.5 ms interval will often deliver it in less time, but with more jitter: approximately 3–12 ms is a reasonable clean-link expectation for unacknowledged delivery and approximately 8–20 ms for an acknowledged application exchange. At a 15 ms interval, BLE is more likely to be comparable to or slower than UART for a single command. The BLE connection interval is not added after a long transmission; it is the maximum nominal wait for the next scheduled exchange opportunity. UART remains more deterministic even where BLE's average is lower.

BLE radio time is much shorter than the connection interval. For example, the raw transmission portion of a small packet is measured in hundreds of microseconds on 1M PHY and approximately half as long on 2M PHY. Most perceived latency comes from waiting for the next connection event, protocol processing, coexistence, and any retry rather than from shifting the payload bits over the air.

### Acknowledged Exchange Timing

One-way delivery and confirmed processing are different comparisons. The table below uses a small command and assumes the receiver can generate its response immediately. “Application acknowledgement” means the receiving GPStar code sends a sequence-numbered acknowledgement or resulting-state message, not merely that the BLE stack received the bytes.

| Exchange | UART at 9600 baud | BLE at 7.5 ms interval | BLE at 15 ms interval | What it proves |
| --- | ---: | ---: | ---: | --- |
| One-way command, no application acknowledgement | At least 12.5 ms plus receive polling | Approximately 3–12 ms | Approximately 5–20 ms | Bytes reached the receiving parser/callback under normal conditions |
| GATT-confirmed write request or indication | Not directly applicable | Approximately 8–20 ms | Approximately 15–35 ms | Remote GATT stack confirmed the operation, not that GPStar completed it |
| Command plus GPStar application acknowledgement | At least about 25 ms for two 12-byte UART frames, plus processing/polling | Approximately 8–20 ms when both directions fit promptly into connection events | Approximately 15–35 ms | Remote GPStar application accepted or completed the defined action |

The BLE acknowledgement estimates overlap because a response may fit into the same connection event or the next one, depending on direction, stack timing, and when the application prepares it. Under contention or retransmission, add one or more connection intervals. UART can also send a response immediately after receiving the full command, but at 9600 baud two 12-byte frames require approximately 25 ms of aggregate serialization even before application and polling delays.

BLE introduces characteristics absent from the wire:

- Data is scheduled around connection events, with a minimum nominal BLE connection interval of 7.5 ms.
- Wi-Fi coexistence can defer radio access and add jitter.
- Lost RF packets may be retried below the application layer, increasing delivery latency.
- GATT notifications are not acknowledged by the receiving application.
- GATT indications are acknowledged but allow only one outstanding indication and reduce throughput.
- Connection establishment and recovery take substantially longer than sending a UART packet.
- Body placement, Pack materials, antenna orientation, and crowded RF environments affect behavior even within 5–6 ft.

The BLE 2M PHY may improve airtime efficiency at close range, but it does not make the link deterministic and may reduce usable range. LE Coded PHY improves range at the cost of airtime and can have a larger effect on concurrent Wi-Fi; it appears unnecessary for the stated operating radius unless enclosure testing shows an unusual link-budget problem.

### Bidirectional Communication

BLE connections are bidirectional. During a connection event, the central sends first and the peripheral can send packets back in the same event. GATT presents asymmetric APIs, but not a one-way radio:

- Wand to Pack: the Wand, as GATT client, writes to a Pack characteristic using a write request or write without response.
- Pack to Wand: the Pack, as GATT server, sends notifications or indications on a characteristic to which the Wand subscribed.

This supports the same logical two-way command and status flow as the UART. It is not byte-stream full duplex in the UART sense; it is scheduled, packet-oriented bidirectional communication.

### Retries and Delivery Assurance

BLE provides automatic link-layer acknowledgements, CRC checking, and retransmission while a connection remains alive. If a radio packet is corrupted or not acknowledged, the controller can retry it during the current or a later connection event. This is transparent to the application and is one reason a transient collision usually appears as added latency rather than a corrupt application payload.

That mechanism does not guarantee that the remote application accepted or acted on a command. Delivery choices provide different levels of assurance:

| Operation | Direction in proposed topology | GATT confirmation | Appropriate use |
| --- | --- | --- | --- |
| Write without response | Wand to Pack | No | Frequent replaceable status or commands with an application sequence/ack scheme |
| Write request | Wand to Pack | Yes, server ATT response | Infrequent writes where protocol confirmation is useful |
| Notification | Pack to Wand | No | Frequent status updates and replaceable telemetry |
| Indication | Pack to Wand | Yes, client confirmation | Infrequent critical updates; only one outstanding |

Even unconfirmed operations still receive link-layer retransmission while connected. A GATT confirmation proves receipt by the remote GATT stack, not necessarily completion of the requested GPStar action. Critical commands should therefore carry a sequence number and receive an application-level acknowledgement or resulting-state confirmation. If the connection drops before confirmation, the sender must decide whether to retry, resynchronize, or discard the command; blindly replaying actions such as toggles or sound triggers can duplicate effects.

### Overall Assessment

| Property | 9600-baud UART | BLE connection |
| --- | --- | --- |
| Application bandwidth | Low but adequate | Potentially much higher |
| Typical command latency | Low | Usually several milliseconds or more |
| Latency jitter | Very low | Variable, especially with Wi-Fi |
| Link establishment | Immediate physical link | Advertising, scanning, connecting, security |
| Interference sensitivity | Low | Subject to 2.4 GHz RF conditions |
| Disconnect detection | Protocol heartbeat/physical failure | Controller event plus application timeout |
| Security/peer identity | Physical wiring | Requires pairing/bonding policy |
| Wiring requirement | Yes | No |
| Failure complexity | Low | Higher |

BLE may be faster in bulk throughput while still being less performant for real-time control. For firing, shutdown, alarm, overheat, and synchronization events, bounded latency and predictable failure behavior matter more than peak bytes per second.

## Protocol Considerations for a Future Prototype

The current API packets and protocol signature could remain conceptually common to both transports, but SerialTransfer is coupled to Arduino `Stream`/serial behavior and should not be assumed to work directly over a GATT characteristic. A future investigation would need to define transport-independent framing.

### Reusing the Existing Struct Payloads

Yes, the existing packed structs can be sent as BLE characteristic byte values, preserving the command enums, packet structs, preference structs, synchronization structs, protocol signature, and existing command handlers. BLE accepts arbitrary binary values; it does not require JSON, strings, or a redesigned command API.

The clean boundary is:

1. Existing application code creates a `CommandPacket`, `DataPacket`, preferences struct, or synchronization struct.
2. A transport-neutral envelope identifies the GPStar packet type and payload length.
3. The active transport sends those bytes through either SerialTransfer/UART or a BLE characteristic.
4. The receiver validates the envelope and protocol signature, copies the bytes into the corresponding packed struct, and invokes the same existing handler.

The application API can therefore remain substantially unchanged even though the transport adapter changes. `packSerialSend()` and `wandSerialSend()` are currently named and implemented around SerialTransfer, so some routing code would eventually need transport-neutral naming or delegation, but the underlying command definitions and behavioral handlers do not need redesign.

SerialTransfer's six framing bytes are useful on a UART byte stream because they mark packet boundaries, escape bytes, identify a packet, and detect corruption. GATT already preserves characteristic-value boundaries and BLE already provides link CRC and retransmission. Sending the entire SerialTransfer wire frame over BLE is possible, especially through a stream-emulation adapter, but it adds redundant framing and makes MTU handling less clear. Sending a small GPStar envelope plus the existing raw struct is simpler.

The structs are explicitly packed and the supported ESP32 and ATmega devices are little-endian, which is why raw struct transfer already works. That remains safe only while both peers use identical field widths, enum representations, bitfield layout, packing, and protocol versions. The existing protocol signature catches size/count mismatches but does not fully define a portable wire format. Since the proposed BLE path is ESP32-S3 to ESP32-S3 built from matching source, direct packed-struct payloads are a practical compatibility choice. Explicit field serialization would be more portable for future non-ESP32 peers but is not required merely to introduce BLE.

Important properties include:

- Message type, payload length, and protocol version/signature.
- Monotonic sequence numbers.
- Duplicate rejection and ordered processing.
- Application acknowledgements for critical state-changing commands.
- Optional unacknowledged delivery for replaceable status telemetry.
- Fragmentation and reassembly for payloads larger than the negotiated GATT payload.
- Bounded queues and explicit overflow behavior.
- State resynchronization after reconnect rather than replaying an old queue.
- Heartbeats and disconnect timeouts appropriate to BLE.
- Explicit UART/BLE transport ownership and handoff behavior.
- A fail-safe state that does not depend on receiving a final wireless command.

Bulk preferences and synchronization data can tolerate retries. Immediate control events should be compact, prioritized, and independently acknowledged where loss would create an unsafe or visibly incorrect state.

## Risks and Unknowns

The following cannot be resolved from source inspection alone:

1. Worst-case BLE command latency while each device serves active SoftAP/web traffic.
2. Heap and internal-RAM headroom after enabling the BLE controller and host.
3. Scheduling impact on the 6 ms LED task, UART receive servicing, audio control, and Wand motion processing.
4. RF performance at 5–6 ft when the controllers are installed in their final enclosures and worn on opposite sides of the user's body.
5. Reconnection time after one device resets or experiences temporary interference.
6. Whether both SoftAPs operating near one another produce acceptable combined Wi-Fi and BLE behavior.
7. Power consumption and thermal impact with Wi-Fi and BLE continuously active.
8. Exact coexistence and task-core defaults supplied by the pinned pioarduino platform release.
9. The safest UART discovery timeout and BLE-to-UART handoff point for the existing state machine.
10. The preferred user interface for displaying, entering, and resetting Pack IDs and bonds.
11. Whether the pinned BLE host/controller combination supports the required Pack connection, peer advertising, and peer scanning procedures concurrently or requires alternating advertise/scan phases.
12. RSSI distributions at candidate crossing distances in realistic two-user poses, and whether thresholds can avoid unacceptable false entry and exit events.
13. Acceptable peer-effect discovery and cancellation latency when advertisements collide, are missed, or become stale.

## Research Validation Plan

Before considering BLE an acceptable UART fallback, a prototype should be evaluated against measurable acceptance criteria. At minimum, testing should record:

- One-way and round-trip latency distributions, including median, 95th, 99th, and worst observed values.
- Packet loss, retransmission, duplicate, and out-of-order rates.
- Connection and bonded-reconnection times.
- Correct UART-first selection when a cable is present, BLE fallback when it is absent, and orderly return to UART when it becomes available.
- Rejection of a second Wand while one Wand is bonded or connected.
- Replacement-pairing and stale-bond recovery behavior.
- Minimum free heap, largest free block, PSRAM use, and task stack high-water marks.
- LED animation deadline misses and sensor/audio processing delays.
- Wi-Fi page, WebSocket, and OTA behavior while BLE traffic is active.
- BLE behavior during Wi-Fi scanning, AP client association, and sustained web transfers.
- Pack/Wand command latency and connection stability during 4%, 30%, and 50% BLE scan duty cycles.
- Discovery time and missed-device rate for passive versus active user-initiated scans.
- Peer discovery latency at 100, 150, and 250 ms advertising intervals with both Wands firing, including missed and colliding advertisements.
- False-positive and false-negative proximity classifications across distance, body orientation, antenna orientation, and crowded 2.4 GHz environments.
- Per-unit 1 m RSSI calibration, inter-unit variation, and comparison of raw, moving-median, and exponentially filtered RSSI.
- Overlap between engage-distance and reject-distance RSSI distributions; do not approve automatic CTS if a threshold cannot separate them reliably.
- Entry hysteresis, exit hysteresis, stale-peer timeout, and immediate local trigger-release behavior for the crossing-the-streams effect.
- Three-or-more-Wand tests proving that CTS does not propagate through an out-of-range chain and that the strongest qualifying peer is selected consistently.
- Pack command latency and Wi-Fi behavior while a Wand maintains its Pack link and alternates or concurrently performs peer advertising and passive scanning.
- Isolation tests proving that a peer Wand cannot control, pair with, or replace the authorized Wand on another user's Pack.
- Operation at 80 MHz on the Wand and 160 MHz on the Pack.
- Range and body-shadowing tests at the expected 5–6 ft maximum in final enclosures and normal worn positions.
- Failure injection: abrupt reset, power loss, RF loss, stale bond, queue overflow, malformed packet, mismatched protocol version, and transport changes during active effects.

Testing should compare BLE directly against captured UART behavior using the same command sequences. Averages alone are insufficient; worst-case latency and recovery behavior determine suitability for the control path.

---

## References

- [ESP32-S3 RF Coexistence](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/coexist.html)
- [ESP32-S3 Bluetooth API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/bluetooth/index.html)
- [ESP32-S3 BLE Advertising and Scanning](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/ble/get-started/ble-device-discovery.html)
- [ESP32-S3 BLE GAP API and RSSI](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/bluetooth/esp_gap_ble.html)
- [ESP32-S3 BLE Connection Parameters](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/ble/get-started/ble-connection.html)
- [ESP32-S3 BLE GATT Data Exchange](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/ble/get-started/ble-data-exchange.html)
- [Bluetooth Channel Sounding](https://www.bluetooth.com/learn-about-bluetooth/feature-enhancements/channel-sounding/)
- [Arduino ESP32 BLE API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ble.html)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Shared communication definitions](source/SharedLib/Communication/include/Communication.h)


## BLE Terminology

- **GAP (Generic Access Profile):** The BLE layer responsible for advertising, scanning, establishing connections, connection parameters, and security. GAP determines how the Wand discovers and connects to a Pack.
- **ATT (Attribute Protocol):** The protocol that moves typed values called attributes across an established BLE connection.
- **GATT (Generic Attribute Profile):** The data model built on ATT. A GATT server exposes services containing characteristics; a GATT client discovers and accesses them. In the proposed topology, the Pack is the GATT server and the Wand is the GATT client.
- **Characteristic:** A named byte value within a GATT service, identified by a UUID and assigned properties such as read, write, notify, or indicate. GPStar could use one characteristic for Wand-to-Pack writes and another for Pack-to-Wand notifications or indications.
- **ATT MTU (Attribute Protocol Maximum Transmission Unit):** The maximum size of one ATT packet. The default ATT MTU is 23 bytes. After the 3-byte ATT operation header, a notification or write command can carry up to 20 bytes of application value. A larger negotiated MTU permits a larger value, subject to lower-layer limits; an ATT MTU of 247 commonly permits up to 244 bytes for these operations.
- **PHY (Physical Layer):** The radio modulation and symbol rate. BLE 1M PHY transmits at a raw 1 megabit per second and is the normal compatibility/range choice. BLE 2M PHY transmits at a raw 2 megabits per second, reducing radio airtime for the same packet but usually with somewhat less link margin. These raw rates are not application throughput because every packet also has link, L2CAP, ATT, acknowledgement, scheduling, and possible encryption overhead.
- **Connection interval:** The scheduled period between opportunities for the two connected devices to exchange packets. BLE permits 7.5 ms through 4 seconds in 1.25 ms steps. A short interval lowers command latency but consumes more radio airtime and power.
- **Connection event:** The exchange window at each connection interval. The central transmits first, the peripheral can reply, and multiple link-layer packets may fit in one event.
- **Notification:** An unconfirmed GATT server-to-client value update. It is fast and benefits from BLE link-layer reliability, but it does not prove that the remote application processed the value.
- **Indication:** A GATT server-to-client value update requiring a GATT confirmation. It provides stronger delivery knowledge but only one indication may be outstanding at a time and it is slower.
- **Write request:** A client-to-server write requiring a GATT response.
- **Write without response (write command):** A faster client-to-server write without a GATT-level response.
- **Pairing and bonding:** Pairing establishes security keys; bonding stores them so the same devices can authenticate and reconnect after power cycles.
- **Passive scan:** The scanner listens for advertising packets but does not transmit scan requests. It uses less radio airtime and is sufficient when the Pack includes its service UUID and Pack ID in the primary advertisement.
- **Active scan:** After receiving a scannable advertisement, the scanner transmits a scan request and waits for a scan response containing additional data. It discovers more information but uses more shared radio airtime.
- **Scan window:** The amount of time the radio listens for advertisements during each scan interval.
- **Scan interval:** The time from the beginning of one scan window to the beginning of the next. Scan duty cycle is approximately `scan window / scan interval`; for example, a 20 ms window every 500 ms is a 4% nominal scan duty cycle.
- **RSSI (Received Signal Strength Indicator):** An estimate of received radio power reported in dBm. Values are normally negative; a value closer to zero is stronger. RSSI can classify coarse proximity after calibration and filtering, but it is not a direct distance measurement.
- **dB versus dBm:** dB expresses a relative gain or loss, while dBm expresses absolute power relative to 1 milliwatt. A scan result is RSSI in dBm; subtracting received power from known transmitted power produces an approximate path loss in dB.
