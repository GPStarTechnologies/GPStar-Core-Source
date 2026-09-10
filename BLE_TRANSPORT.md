# BLE SerialData Transport Architecture

## Purpose

Provide a Bluetooth Low Energy (BLE) transport between a Ghostbusters proton pack and neutrona wand that carries the same logical packets already used by the existing SerialTransfer/UART communication path.

BLE is an additional transport, not a replacement for the existing command protocol. Application logic should continue to create and process the same commands, data packets, and structs regardless of whether they travel over UART or BLE.

## Device Roles

### Proton Pack

The proton pack is the **HomeBase** device.

- Advertises its presence over BLE.
- Exposes the BLE `SerialData` service.
- A wand is associated with one specific pack.

### Neutrona Wand

The wand:

- Scans for its associated pack.
- Connects to that pack.
- Exchanges bidirectional SerialData messages with it.

Future functionality may allow wands to advertise and communicate with other wands, but that is separate from the Pack/Wand SerialData transport described here.

## SerialData Service

The Pack exposes one BLE service dedicated to reproducing the existing serial communication behavior:

```text
SerialData Service
├── PackTX    Pack → Wand
└── PackRX    Wand → Pack
```

The characteristic names are always defined from the Pack's perspective.

- `PackTX` carries messages originating from the Pack and destined for the Wand.
- `PackRX` carries messages originating from the Wand and destined for the Pack.

The characteristics are transport endpoints. They are **not queues** and do not represent individual commands or application state.

Other BLE functionality should use separate services and characteristics so that `SerialData` remains solely responsible for transporting the existing command protocol.

## Message Structure

Each BLE transmission carries **one complete logical message**.

A BLE message consists conceptually of:

```text
[ Packet Type ][ Sequence Number ][ Original Payload ]
```

### Packet Type

This is the same packet identifier already used by the existing serial protocol. It identifies how the payload should be interpreted, such as:

- a normal command packet;
- a command containing additional data; or
- a distinct serialized struct/object.

### Sequence Number

Each transmitted message receives a sequence number.

Use an unsigned 8-bit value (`0–255`) and allow normal wraparound:

```text
253 → 254 → 255 → 0 → 1 → 2
```

The sequence number identifies and orders nearby messages. It is not intended to be globally unique.

### Original Payload

The payload is preserved exactly as produced for the existing serial transport.

For normal command packets, this may already contain:

```text
[start][command][optional data][end]
```

For other packet types, the payload may be the complete serialized struct.

The BLE transport does not reinterpret or rebuild this payload. It wraps and transports it intact.

## Queues

Each device maintains two application-side BLE queues:

```text
TX Queue = complete messages waiting to be transmitted
RX Queue = complete received messages waiting to be processed
```

Therefore:

```text
PACK                                      WAND

Pack TX Queue  ─── PackTX ───────────►  Wand RX Queue
Pack RX Queue  ◄── PackRX ────────────  Wand TX Queue
```

Use fixed-size circular/ring buffers.

Initial size:

```text
TX queue: 16 messages
RX queue: 16 messages
```

A full queue must **never silently overwrite an existing message**. Queue overflow should be detectable through an error, status flag, diagnostic counter, or logging mechanism.

## Transmit Behavior

Application logic may generate several commands during a single Arduino `loop()` iteration.

For example:

```text
A
B
C
D
```

These commands are immediately placed into the device's BLE TX queue:

```text
[A][B][C][D]
```

They are **not all immediately submitted to BLE**.

The BLE transport handles only the message at the head of the queue:

```text
[A][B][C][D]
 ↑
 current message
```

Only one reliable BLE operation should be outstanding at a time for a given direction.

For Pack → Wand communication, `PackTX` should use BLE indications so that the Pack receives confirmation of delivery from the BLE stack.

Conceptually:

```text
Pack TX Queue
[A][B][C]
 ↑

       A
PACK ─────────► WAND

PACK ◄───────── WAND
     confirmation

[A] is removed from the queue

[B][C]
 ↑
 next message
```

A message remains at the head of the TX queue until the transport has received the appropriate confirmation that allows it to advance.

The exact reliable mechanism used for Wand → Pack through `PackRX` should provide equivalent queue discipline while respecting the BLE/GATT behavior appropriate to that direction.

## Receive Behavior

When BLE receives a complete SerialData message, the BLE handling layer places that message into the local RX queue.

It does not immediately execute the command from within BLE callbacks or BLE background processing.

Conceptually:

```text
BLE receives message
        ↓
validate/copy complete message
        ↓
RX queue
        ↓
normal application processing
```

At the beginning of normal `loop()` processing, the application checks the BLE RX queue for pending messages and processes them through the same command-handling logic used for packets received through SerialTransfer.

## Relationship to Existing UART Communication

The existing application should continue to think in terms of complete logical packets.

When application logic generates a command:

```text
                    ┌──► SerialTransfer / UART
Application command ┤
                    └──► BLE TX queue
```

When commands are received:

```text
SerialTransfer RX ──┐
                    ├──► existing command processing
BLE RX queue ───────┘
```

The BLE layer is responsible for BLE-specific concerns such as:

- connection state;
- queue management;
- transmission pacing;
- determining when another message may be submitted;
- indication/confirmation handling;
- sequence numbers; and
- queue overflow detection.

The normal Pack/Wand application logic should not need to understand those details.

## Arduino Loop Model

BLE radio and protocol activity are primarily handled by the BLE stack in the background. Normal Arduino `loop()` processing advances the application-side queues and reacts to completed BLE events.

Conceptually:

```text
loop()
│
├── Check/process pending BLE RX messages
│
├── Existing SerialTransfer receive processing
│
├── Normal application logic
│     └── generated messages may be added to BLE TX queue
│
└── Advance BLE TX processing when the transport is ready
```

Many loop iterations may have no BLE work to perform.

## Core Design Rules

1. BLE `SerialData` transports the existing logical packet protocol; it does not redefine it.
2. `PackTX` always means Pack → Wand.
3. `PackRX` always means Wand → Pack.
4. One queue entry represents one complete logical message.
5. Characteristics are transport endpoints, not message queues.
6. Multiple commands generated rapidly are queued rather than immediately submitted to BLE.
7. Only the current message is handed to the BLE transport; subsequent messages wait their turn.
8. A reliably transmitted message is removed from the TX queue only after the appropriate BLE completion/confirmation event.
9. Received BLE messages are queued and later processed by normal application logic.
10. Queue overflow must be detectable and must never silently discard an older queued message.
11. BLE-specific implementation details remain isolated from the existing command-processing logic.
12. Future BLE features should use separate services rather than expanding `SerialData` into a general application-state service.
