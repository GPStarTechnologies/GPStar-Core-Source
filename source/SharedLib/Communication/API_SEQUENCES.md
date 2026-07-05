# Device API Sequences

Provides sequence diagrams for special interactions between devices.

## Handshake + Sync: Attenuator <-> Pack

```mermaid
sequenceDiagram
		participant A as Attenuator
		participant P as Pack

		A->>P: A_HANDSHAKE
		P->>A: A_SYNC_START (d1 = PROTOCOL_SIGNATURE)
		P->>A: A_SYNC_DATA (PACKET_PACK, AttenuatorSyncData)
		P->>A: A_SYNC_END
		A->>P: A_SYNCHRONIZED
```

---

## Handshake + Sync: Wand <-> Pack

```mermaid
sequenceDiagram
		participant W as Wand
		participant P as Pack

		W->>P: A_HANDSHAKE
		P->>W: A_SYNC_START (d1 = PROTOCOL_SIGNATURE)
		P->>W: A_SYNC_DATA (PACKET_WAND, WandSyncData)
		P->>W: A_SYNC_END
		W->>P: A_SYNCHRONIZED
```

---

## Preferences Exchange: Attenuator <-> Pack

```mermaid
sequenceDiagram
		participant A as Attenuator
		participant P as Pack

		A->>P: A_REQUEST_PREFERENCES_PACK
		P->>A: A_SEND_PREFERENCES_PACK (PACKET_PACK, PackPrefs)
		A->>P: A_SEND_PREFERENCES_WAND (PACKET_WAND, WandPrefs)
		A->>P: A_SEND_PREFERENCES_SMOKE (PACKET_SMOKE, SmokePrefs)
		P->>P: Persist imported data
```

---

## Preferences Exchange: Wand <-> Pack

```mermaid
sequenceDiagram
		participant W as Wand
		participant P as Pack

		W->>P: A_REQUEST_PREFERENCES_PACK
		P->>W: A_SEND_PREFERENCES_PACK (PACKET_PACK, PackPrefs)

		W->>P: A_REQUEST_PREFERENCES_WAND
		P->>W: A_SEND_PREFERENCES_WAND (PACKET_WAND, WandPrefs)

		W->>P: A_REQUEST_PREFERENCES_SMOKE
		P->>W: A_SEND_PREFERENCES_SMOKE (PACKET_SMOKE, SmokePrefs)
```

---

## Stream Mode + Power Propagation

```mermaid
sequenceDiagram
		participant W as Wand
		participant P as Pack
		participant A as Attenuator

		W->>P: A_SET_STREAM_MODE (d1 = STREAM_MODES)
		P->>P: Apply stream mode
		P->>A: A_SET_STREAM_MODE (d1 = STREAM_MODES)

		W->>P: A_SET_POWER_LEVEL (d1 = 1..5)
		P->>P: Apply power level
		P->>A: A_SET_POWER_LEVEL (d1 = 1..5)

		P->>W: A_STREAM_FLAGS (d1 = bitfield)
		P->>A: A_STREAM_FLAGS (d1 = bitfield)
```

---

## Music Control + Playback Status Propagation

```mermaid
sequenceDiagram
		participant A as Attenuator
		participant P as Pack
		participant W as Wand

		A->>P: A_MUSIC_PLAY_TRACK (d1 = track)
		P->>P: Start selected track
		P->>A: A_MUSIC_IS_PLAYING (d1 = current track)
		P->>W: A_MUSIC_STATUS (d1 = 2 playing / 1 stopped)

		A->>P: A_MUSIC_PAUSE_RESUME
		P->>W: A_MUSIC_STATUS (d1 = 4 paused / 3 resumed)

		P->>A: A_MUSIC_IS_NOT_PLAYING (d1 = current track)
		P->>W: A_MUSIC_STATUS (d1 = 1 stopped)
```

---

## Loop/Shuffle/Mute + Volume Control Propagation

```mermaid
sequenceDiagram
		participant A as Attenuator
		participant P as Pack
		participant W as Wand

		A->>P: A_MUSIC_TRACK_LOOP_TOGGLE (d1 = 1/2)
		P->>W: A_MUSIC_TRACK_LOOP_STATUS (d1 = 1/2)

		A->>P: A_MUSIC_TRACK_SHUFFLE_TOGGLE (d1 = 1/2)
		P->>W: A_MUSIC_TRACK_SHUFFLE_STATUS (d1 = 1/2)

		A->>P: A_TOGGLE_MUTE (d1 = 1/2)
		P->>W: A_MASTER_AUDIO_STATUS (d1 = 1/2)

		A->>P: A_VOLUME_SET (d1 = volume)
		P->>P: Apply master volume
```

---

## Alarm + Lockout + Telemetry Status Flows

```mermaid
sequenceDiagram
    participant W as Wand
    participant P as Pack
    participant A as Attenuator

    W->>P: A_BUTTON_MASHING (d1 = timeout)
    P->>A: A_SYSTEM_LOCKOUT (d1 = timeout)

    P->>W: A_ALARM_ON (d1 = ribbon 0/1)
    P->>A: A_ALARM_ON (d1 = ribbon 0/1)
    P->>W: A_ALARM_OFF (d1 = ribbon 0/1)
    P->>A: A_ALARM_OFF (d1 = ribbon 0/1)

    loop periodic telemetry
        P->>A: A_WAND_POWER_AMPS (d1 = amps value)
        P->>A: A_BATTERY_VOLTAGE_PACK (d1 = volts x100)
        P->>A: A_TEMPERATURE_PACK (d1 = celsius x100)
    end
```
