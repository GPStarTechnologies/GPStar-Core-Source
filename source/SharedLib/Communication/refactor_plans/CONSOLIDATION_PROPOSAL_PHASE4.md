# Phase 4 Consolidation Proposal: ENABLED/DISABLED → A_SET_* Enums

## Summary
Consolidate **16 ENABLED/DISABLED pairs** from API_COMMAND into A_SET_* enums in API_DATA.
- **Current Cost**: 16 pairs = 32 enum slots in API_COMMAND
- **After Consolidation**: 16 new A_SET_* entries in API_DATA (compact, fits uint8_t)
- **Net Savings**: ~16 slots in API_COMMAND (brings it from ~288 to ~272)

---

## Consolidation Candidates

### Category 1: ENABLED/DISABLED Configuration Pairs
These are clearly feature toggles/preferences with binary enabled/disabled states.

| Current Enums | Proposed A_SET_* | d1 Values | Slots Saved |
|---|---|---|---|
| `A_QUICK_VENT_ENABLED`<br/>`A_QUICK_VENT_DISABLED` | `A_SET_QUICK_VENT` | 0=DISABLED, 1=ENABLED | **1** |
| `A_BOOTUP_ERRORS_ENABLED`<br/>`A_BOOTUP_ERRORS_DISABLED` | `A_SET_BOOTUP_ERRORS` | 0=DISABLED, 1=ENABLED | **1** |
| `A_SMOKE_DISABLED`<br/>`A_SMOKE_ENABLED` | `A_SET_SMOKE` | 0=DISABLED, 1=ENABLED | **1** |
| `A_BARGRAPH_OVERHEAT_BLINK_ENABLED`<br/>`A_BARGRAPH_OVERHEAT_BLINK_DISABLED` | `A_SET_BARGRAPH_OVERHEAT_BLINK` | 0=DISABLED, 1=ENABLED | **1** |
| `A_MODE_BEEP_LOOP_ENABLED`<br/>`A_MODE_BEEP_LOOP_DISABLED` | `A_SET_MODE_BEEP_LOOP` | 0=DISABLED, 1=ENABLED | **1** |
| `A_RGB_VENT_DISABLED`<br/>`A_RGB_VENT_ENABLED` | `A_SET_RGB_VENT` | 0=DISABLED, 1=ENABLED | **1** |
| `A_AUTO_VENT_INTENSITY_DISABLED`<br/>`A_AUTO_VENT_INTENSITY_ENABLED` | `A_SET_AUTO_VENT_INTENSITY` | 0=DISABLED, 1=ENABLED | **1** |
| `A_VENT_LIGHT_COLOURS_DISABLED`<br/>`A_VENT_LIGHT_COLOURS_ENABLED` | `A_SET_VENT_LIGHT_COLOURS` | 0=DISABLED, 1=ENABLED | **1** |
| `A_WAND_WIFI_DISABLED`<br/>`A_WAND_WIFI_ENABLED` | `A_SET_WAND_WIFI` | 0=DISABLED, 1=ENABLED | **1** |
| `A_VIDEO_GAME_MODE_COLOURS_ENABLED`<br/>`A_VIDEO_GAME_MODE_COLOURS_DISABLED` | `A_SET_VIDEO_GAME_MODE_COLOURS` | 0=DISABLED, 1=ENABLED | **1** |
| `A_SPECTRAL_MODES_ENABLED`<br/>`A_SPECTRAL_MODES_DISABLED` | `A_SET_SPECTRAL_MODES` | 0=DISABLED, 1=ENABLED | **1** |
| `A_VOICE_NEUTRONA_WAND_SOUNDS_ENABLED`<br/>`A_VOICE_NEUTRONA_WAND_SOUNDS_DISABLED` | `A_SET_VOICE_NEUTRONA_WAND_SOUNDS` | 0=DISABLED, 1=ENABLED | **1** |

### Category 2: ON/OFF Configuration Pairs (Similar Pattern)
These follow ON/OFF logic (can be treated as 0=OFF, 1=ON).

| Current Enums | Proposed A_SET_* | d1 Values | Slots Saved |
|---|---|---|---|
| `A_SPECTRAL_LIGHTS_OFF`<br/>`A_SPECTRAL_LIGHTS_ON` | `A_SET_SPECTRAL_LIGHTS` | 0=OFF, 1=ON | **1** |
| `A_OVERHEATING_DISABLED`<br/>`A_OVERHEATING_ENABLED` | `A_SET_OVERHEATING` | 0=DISABLED, 1=ENABLED | **1** |

### Category 3: INVERTED/NOT_INVERTED Pairs
These follow invert logic (can be treated as 0=NOT_INVERTED, 1=INVERTED).

| Current Enums | Proposed A_SET_* | d1 Values | Slots Saved |
|---|---|---|---|
| `A_BARGRAPH_NOT_INVERTED`<br/>`A_BARGRAPH_INVERTED` | `A_SET_BARGRAPH_INVERT` | 0=NOT_INVERTED, 1=INVERTED | **1** |
| `A_POWERCELL_NOT_INVERTED`<br/>`A_POWERCELL_INVERTED` | `A_SET_POWERCELL_INVERT` | 0=NOT_INVERTED, 1=INVERTED | **1** |

**Subtotal Savings: 16 slots**

---

## Status/State Messages (Recommended to KEEP as pairs)
These appear to be **state reporting messages** (device status, not configuration commands).
Should remain in API_COMMAND as discrete status indicators:

| Enum Pair | Reason |
|---|---|
| `A_PACK_ON` / `A_PACK_OFF` | Device status (Pack power state) |
| `A_WAND_ON` / `A_WAND_OFF` | Device status (Wand power state) |
| `A_CYCLOTRON_LID_ON` / `A_CYCLOTRON_LID_OFF` | Sensor state (Lid position) |
| `A_ALARM_ON` / `A_ALARM_OFF` | Alert status |
| `A_ION_ARM_SWITCH_ON` / `A_ION_ARM_SWITCH_OFF` | Arm switch position |
| `A_TURN_PACK_ON` / `A_TURN_PACK_OFF` | Power command (related to startup) |

---

## Incomplete Pairs (Clarification Needed)
These entries only have **_ENABLED but no _DISABLED variant**. Need clarification:

| Current Enum | Status | Question |
|---|---|---|
| `A_PACK_MOTORIZED_CYCLOTRON_ENABLED` | Incomplete pair | Is there a _DISABLED variant, or is this just config? |
| `A_VIDEO_GAME_MODE_POWER_CELL_ENABLED` | Incomplete pair | Is there a _DISABLED variant, or is this just config? |
| `A_VIDEO_GAME_MODE_CYCLOTRON_ENABLED` | Incomplete pair | Is there a _DISABLED variant, or is this just config? |

---

## Proposed Changes to API_DATA
Add these 16 new entries to API_DATA enum (after existing d1-parameter entries):

```cpp
enum API_DATA : uint8_t {
  // ... existing entries (A_HANDSHAKE through A_SET_OVERHEAT_LEVEL_1) ...
  
  // Phase 4: Configuration toggle consolidations
  A_SET_QUICK_VENT,                      // d1: 0=DISABLED, 1=ENABLED
  A_SET_BOOTUP_ERRORS,                   // d1: 0=DISABLED, 1=ENABLED
  A_SET_SMOKE,                           // d1: 0=DISABLED, 1=ENABLED
  A_SET_BARGRAPH_OVERHEAT_BLINK,         // d1: 0=DISABLED, 1=ENABLED
  A_SET_MODE_BEEP_LOOP,                  // d1: 0=DISABLED, 1=ENABLED
  A_SET_RGB_VENT,                        // d1: 0=DISABLED, 1=ENABLED
  A_SET_AUTO_VENT_INTENSITY,             // d1: 0=DISABLED, 1=ENABLED
  A_SET_VENT_LIGHT_COLOURS,              // d1: 0=DISABLED, 1=ENABLED
  A_SET_WAND_WIFI,                       // d1: 0=DISABLED, 1=ENABLED
  A_SET_VIDEO_GAME_MODE_COLOURS,         // d1: 0=DISABLED, 1=ENABLED
  A_SET_SPECTRAL_MODES,                  // d1: 0=DISABLED, 1=ENABLED
  A_SET_VOICE_NEUTRONA_WAND_SOUNDS,      // d1: 0=DISABLED, 1=ENABLED
  A_SET_SPECTRAL_LIGHTS,                 // d1: 0=OFF, 1=ON
  A_SET_OVERHEATING,                     // d1: 0=DISABLED, 1=ENABLED
  A_SET_BARGRAPH_INVERT,                 // d1: 0=NOT_INVERTED, 1=INVERTED
  A_SET_POWERCELL_INVERT,                // d1: 0=NOT_INVERTED, 1=INVERTED
  
  A_DATA_NO_OP
};
```

**Result**: API_DATA grows to 50 entries (still well under uint8_t limit of 255)
API_COMMAND shrinks to ~272 entries (still needs splitting, but 16 slots freed)

---

## Questions for Review

1. **Status vs. Configuration**: Are `A_PACK_ON/OFF`, `A_WAND_ON/OFF`, `A_CYCLOTRON_LID_ON/OFF`, `A_ALARM_ON/OFF`, `A_ION_ARM_SWITCH_ON/OFF` truly status messages, or should they also be consolidated?

2. **Incomplete Pairs**: What should we do with the three incomplete pairs (MOTORIZED_CYCLOTRON, VIDEO_GAME_MODE_POWER_CELL, VIDEO_GAME_MODE_CYCLOTRON)? Should they have _DISABLED variants added, or are they intentionally single-state?

3. **Implementation Order**: Should this be Phase 4, or would you prefer to batch this with other enhancements?

4. **Overheat Semantics**: `A_OVERHEATING_DISABLED` and `A_OVERHEATING_ENABLED` appear to be state messages. Should these stay paired, or move to A_SET_OVERHEATING with d1 values?

---

## Files Affected by Consolidation

If approved, these files will need updates:

1. **Communication.h** - Remove old pairs from API_COMMAND, add new A_SET_* to API_DATA
2. **ProtonPack/include/Serial.h** - Handler update for incoming messages
3. **ProtonPack/include/Command.h** - Sender updates for button/menu press handlers
4. **NeutronaWand/include/Serial.h** - Handler update for incoming messages
5. **NeutronaWand/include/Command.h** - Sender updates
6. **Attenuator/include/Serial.h** - Handler update
7. **Attenuator/include/Webhandler.h** - Web UI control updates

