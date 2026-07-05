# Device API Sequences

Provides sequence diagrams for special interactions between devices.

## Handshake + Sync: Pack <-> Attenuator

```mermaid
sequenceDiagram
		participant P as Pack
		participant A as Attenuator

		loop Periodic Check-In
			P->>A: A_HANDSHAKE (d1 = PROTOCOL_SIGNATURE)
			alt Attenuator Connected
				A->>P: A_HANDSHAKE (d1 = PROTOCOL_SIGNATURE)
			else Attenuator Not Connected
				A->>P: A_SYNC_START (d1 = PROTOCOL_SIGNATURE)
				P->>A: A_SYNC_DATA (PACKET_SYNC, AttenuatorSyncData)
				P->>A: A_SYNC_END
				A->>P: A_SYNCHRONIZED
			end
		end
```

---

## Handshake + Sync: Pack <-> Wand

```mermaid
sequenceDiagram
		participant P as Pack
		participant W as Wand

		loop Periodic Handshake (Wand initiates ~3.25s)
			W->>P: A_HANDSHAKE (d1 = PROTOCOL_SIGNATURE)
		end

		opt Wand Not Responding (Pack check after ~6.5s)
			P->>W: A_HANDSHAKE (d1 = PROTOCOL_SIGNATURE)
		end

		alt Initial Connection or Resync Needed
			W->>P: A_SYNC_START (d1 = PROTOCOL_SIGNATURE)
			P->>W: A_SYNC_DATA (PACKET_SYNC, WandSyncData)
			P->>W: A_SYNC_END
			W->>P: A_SYNCHRONIZED
		end
```

---

## Preferences Exchange: Attenuator → Pack → Wand (Relay Pattern)

```mermaid
sequenceDiagram
		participant A as Attenuator
		participant P as Pack
		participant W as Wand

		A->>P: A_REQUEST_PREFERENCES_PACK
		P->>A: A_SEND_PREFERENCES_PACK (PACKET_PACK, PackPrefs)

		A->>P: A_REQUEST_PREFERENCES_WAND
		P->>W: A_SEND_PREFERENCES_WAND (command)
		W->>P: A_SEND_PREFERENCES_WAND (PACKET_WAND, WandPrefs)
		P->>A: A_SEND_PREFERENCES_WAND (PACKET_WAND, WandPrefs)

		A->>P: A_REQUEST_PREFERENCES_SMOKE
    P->>P: Get Pack smoke settings
		alt Wand Connected
			P->>W: A_SEND_PREFERENCES_SMOKE (command)
			W->>P: A_SEND_PREFERENCES_SMOKE (PACKET_SMOKE, SmokePrefs)
		end
		P->>A: A_SEND_PREFERENCES_SMOKE (PACKET_SMOKE, SmokePrefs)

		A->>A: Persist all preferences
```

---

## Preferences Storage: Attenuator → Pack → Wand (Relay Pattern)

```mermaid
sequenceDiagram
		participant A as Attenuator
		participant P as Pack
		participant W as Wand

		A->>P: A_SAVE_PREFERENCES_PACK (PACKET_PACK, updated PackPrefs)
		P->>P: Update packConfig struct
		Note over P: Audio Acknowledgment

		A->>P: A_SAVE_PREFERENCES_WAND (PACKET_WAND, updated WandPrefs)
		alt Wand Connected
      P->>W: A_SAVE_PREFERENCES_WAND (PACKET_WAND, updated WandPrefs)
      W->>W: Update wandConfig struct
		  Note over W: Audio Acknowledgment
    end

		A->>P: A_SAVE_PREFERENCES_SMOKE (PACKET_SMOKE, updated SmokePrefs)
    P->>P: Update Pack's smokeConfig struct
    Note over P: Audio Acknowledgment
		alt Wand Connected
			P->>W: A_SAVE_PREFERENCES_SMOKE (PACKET_SMOKE, updated SmokePrefs)
			W->>W: Update smokeConfig struct
		  Note over W: Audio Acknowledgment
		end

		Note over A,W: Preferences updated in RAM, not yet persisted to EEPROM
```

---

## EEPROM Persistence: Saving Preferences to Storage

```mermaid
sequenceDiagram
		participant A as Attenuator
		participant P as Pack
		participant W as Wand

		A->>P: A_SAVE_EEPROM_SETTINGS_PACK
		P->>P: saveLEDEEPROM()<br/>saveConfigEEPROM()
		Note over P: Audio Acknowledgment

		A->>P: A_SAVE_EEPROM_SETTINGS_WAND
		P->>W: A_SAVE_EEPROM_WAND
		W->>W: saveLEDEEPROM()<br/>saveConfigEEPROM()
    Note over W: Audio Acknowledgment

		Note over A,W: All preferences now persisted to EEPROM on respective devices
```
