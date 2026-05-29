# PortaPack Mayhem Platform — Unlock Analysis

**Date:** 2026-05-29  
**Source Fork:** `galic1987/hackrf` (main) → `galic1987/hackrf-portapack` (Mayhem submodule)  
**Mayhem Branch:** [`mayhem-cpld-fpga-fixes`](https://github.com/galic1987/hackrf-portapack/tree/mayhem-cpld-fpga-fixes)  
**Status:** Firmware patches cherry-picked cleanly onto Mayhem `next`

---

## 1. Mayhem HackRF Submodule Architecture

The Mayhem firmware (`portapack-mayhem/mayhem-firmware`) embeds the HackRF firmware as a Git submodule at:

```
portapack-mayhem/hackrf/  →  github.com/portapack-mayhem/hackrf.git
```

**Current submodule commit:** `f526ddf4` (branch `next`)  
**Upstream merge base:** `greatscottgadgets/hackrf@main` (merged via `bc934396`)

This means Mayhem tracks upstream `main` with a small latency. Our patches (`8d5a9c3c`) apply cleanly because they touch files that exist identically in both trees:

- `firmware/common/cpld_xc2c.c` / `.h`
- `firmware/hackrf_usb/usb_api_cpld.c`
- `firmware/hackrf_usb/usb_api_praline.c`

**Fork created:** `github.com/galic1987/hackrf-portapack`  
**Branch:** `mayhem-cpld-fpga-fixes` (cherry-pick of `8d5a9c3c` onto `f526ddf4`)

---

## 2. What Our Fixes Unlock on Mayhem

### 2.1 CPLD Hardware Diagnostics in Mayhem UI

**Current State:** Mayhem has no runtime CPLD verification. If the CPLD bitstream is corrupted (e.g., bad flash write, power glitch), the SGPIO data path fails silently and the user sees garbled RX/TX with no diagnostic feedback.

**What Our Fix Enables:**
- A **"CPLD Health Check"** screen in the Debug menu
- Display of the SRAM checksum (`0x05829db7` on a healthy HackRF One)
- Fast verification (~3 ms) without disrupting normal operation

**Mayhem Modules to Modify:**

| File | Change |
|------|--------|
| `firmware/application/apps/ui_debug.cpp` | Add `CPLDChecksumView` that calls `hackrf_cpld_checksum()` via the USB control path |
| `firmware/common/hackrf_hal.cpp` | Thin wrapper around vendor request `HACKRF_VENDOR_REQUEST_CPLD_CHECKSUM` (0x24) |
| `firmware/application/ui_navigation.cpp` | Register new view under *Debug → Hardware Tests* |

**Implementation Notes:**
- Mayhem already has JTAG infrastructure for CPLD updates (`common/cpld_update.cpp`). The SRAM-read path we added is even simpler than flash-read and can be implemented directly in Mayhem's M4 code without USB round-trips.
- The checksum value `0x05829db7` is deterministic for all HackRF One r8/r9 units with the stock CPLD bitstream. Mayhem can hardcode this as the "healthy" reference.

---

### 2.2 FPGA Bitstream Switching on HackRF Pro

**Current State:** Mayhem on Pro is limited to the single FPGA bitstream baked into firmware at compile time (`firmware/fpga/build/praline_fpga.bin`). Users cannot switch between standard, extended-precision RX, half-precision, or custom DSP modes without reflashing.

**What Our Fix Enables:**
- Runtime FPGA reconfiguration from the touchscreen
- Safe switching: the guard ensures the radio is OFF before loading a new bitstream, preventing SGPIO corruption
- Support for multiple operational profiles:
  - **Standard:** General-purpose wideband SDR (current default)
  - **Extended Precision RX:** Higher dynamic range for weak-signal reception
  - **Half Precision:** Lower latency DSP for fast-scan apps
  - **Custom DSP:** User-defined FPGA images for specific modulation schemes

**Mayhem Modules to Modify:**

| File | Change |
|------|--------|
| `firmware/application/apps/ui_settings.cpp` | Add *SDR Mode* submenu with bitstream index selector (0–3) |
| `firmware/application/receiver_model.cpp` | Before switching: `set_modulation(AM/FM/...)` → stop RX/TX → call `hackrf_set_fpga_bitstream()` → re-init baseband |
| `firmware/common/portapack_hal.cpp` | Wrapper for `HACKRF_VENDOR_REQUEST_SET_FPGA_BITSTREAM` (0x36) |
| `firmware/application/ui_navigation.cpp` | Warn user that switching modes interrupts reception |

**Critical Safety Note:**
Our guard in `usb_api_praline.c` checks `radio_reg_read(&radio, RADIO_BANK_APPLIED, RADIO_OPMODE)`. If the user tries to switch bitstreams while Mayhem is actively streaming, the device STALLs the request. Mayhem's UI should catch the `HACKRF_ERROR_LIBUSB` timeout and show: *"Stop reception before changing SDR mode."*

---

### 2.3 Hardware Clock Locking for Coherent Multi-Device

**Current State:** Mayhem operates a single HackRF. There is no UI for clock output configuration or multi-device synchronization.

**What Our Fix Enables:**
- **Pro as Clock Master:** Output 10 MHz reference on P2 (CLKOUT)
- **One as Clock Slave:** Accept external reference on CLKIN
- **Phased-array operations:** Direction finding, beam steering, passive radar

**Hardware Setup (Verified):**
```
HackRF Pro r1.2  P2_CLKOUT ──► HackRF One r9  CLKIN
         (10 MHz reference)          (phase-locked)
```

**Phase Stability Results:**
- Frequency: 2.437 GHz (WiFi ch6)
- Standard deviation: **13.6°** across 50 snapshots
- Rating: GOOD (sufficient for beamforming and coarse direction finding)

**Mayhem Modules to Modify:**

| Feature | Files | Difficulty |
|---------|-------|------------|
| Clock output toggle | `ui_settings.cpp`, `hackrf_clock.cpp` | Low |
| CLKIN status indicator | `ui_status_bar.cpp` | Low |
| Synchronized RX start | New vendor request or HW_SYNC pin | Medium |
| Direction-finding app | New `ui_direction_finding.cpp` | High |
| Beam-steering overlay | `ui_spectrum.cpp`, baseband DSP | High |

**Synchronization Options:**

1. **Software Sync (Easier):** Send `SET_TRANSCEIVER_MODE(RX)` to both devices over USB with minimal inter-packet delay. Good for ~1 ms alignment.
2. **Hardware Sync (Better):** Wire the `HW_SYNC_MODE` pin (P20) across devices. Trigger simultaneous sample capture with a single GPIO pulse. Requires a new Mayhem app to control the sync line.

**Python Tools as Reference:**

| Python Script | Mayhem Equivalent |
|---------------|-------------------|
| `hackrf_array_cal.py` | *Calibrate Array* menu item: capture 262k samples from both devices, compute FFT peak phase offset, store in persistent memory |
| `hackrf_beamform.py` | *Beamform* overlay on spectrum/waterfall: scan steering phases -180° to +180°, display power bar chart |

---

## 3. Concrete Integration Roadmap

### Phase 1: CPLD Diagnostics (1–2 days)
1. Fork `portapack-mayhem/hackrf` → `galic1987/hackrf-portapack` ✅ (done)
2. Apply cherry-pick `8d5a9c3c` → branch `mayhem-cpld-fpga-fixes` ✅ (done)
3. In Mayhem app repo: bump `hackrf` submodule to `galic1987/hackrf-portapack@mayhem-cpld-fpga-fixes`
4. Add `CPLDChecksumView` in `ui_debug.cpp`
5. Build and test on HackRF One r9 + PortaPack H2

### Phase 2: FPGA Mode Switching (1 week)
1. Add `hackrf_set_fpga_bitstream()` wrapper to Mayhem HAL
2. Create `FPGAModeSelector` UI in Settings
3. Hook into `receiver_model.cpp` stop/start lifecycle
4. Test all four FPGA modes on Pro r1.2

### Phase 3: Clock Lock UI (2–3 days)
1. Add "CLKOUT Enable" toggle for Pro in Settings
2. Add "CLKIN Detected" status icon in status bar
3. Verify phase lock with `hackrf_clock -i` equivalent

### Phase 4: Phased-Array Apps (2–4 weeks)
1. Design `ui_direction_finding.cpp` with compass visualization
2. Implement dual-device capture trigger (software or hardware sync)
3. Port `hackrf_array_cal.py` logic to M4 C++ (FFT peak, phase extraction)
4. Port `hackrf_beamform.py` scan loop to baseband DSP or M4 FPU
5. Store calibration constants in `portapack_persistent_memory`

---

## 4. Git Commands for Submodule Bump

```bash
# In the Mayhem main repo
cd portapack-mayhem/

# Point hackrf submodule to our fork
cd hackrf
git remote add galic1987-portapack https://github.com/galic1987/hackrf-portapack.git
git fetch galic1987-portapack
git checkout mayhem-cpld-fpga-fixes
cd ..

# Commit the submodule pointer change
git add hackrf
git commit -m "Bump hackrf submodule: CPLD SRAM-checksum + Pro FPGA safety guard

Changes from galic1987/hackrf-portapack@mayhem-cpld-fpga-fixes:
- CPLD checksum now reads active SRAM config (~3ms, safe)
- FPGA bitstream guard checks applied radio mode (no race)
- Dead error branch removed from CPLD vendor request handler"

# Push to your Mayhem fork
git push origin your-mayhem-branch
```

---

## 5. Summary Table

| Feature | Status in Our Fork | Mayhem Integration Difficulty | Impact |
|---------|---------------------|-------------------------------|--------|
| CPLD SRAM checksum | ✅ Merged to `main` | Low | Hardware diagnostics |
| FPGA bitstream guard | ✅ Merged to `main` | Low | Safety prerequisite |
| Host timeout fix | ✅ Merged to `main` | N/A (host-side) | Reliability |
| `hackrf_debug -k` | ✅ Merged to `main` | N/A (host-side) | Diagnostics tool |
| Clock lock validation | ✅ Verified on hardware | Low | Multi-device coherence |
| Array calibration | ✅ Python tool working | Medium | Calibration workflow |
| Beamforming | ✅ Python tool working | High | Core phased-array feature |
| Direction finding | ❌ Not implemented | High | Navigation/foxhunting app |

---

## 6. Next Steps

1. **Open upstream PR** for `greatscottgadgets/hackrf` with commits `8d5a9c3c` and `3d1de1f8`
2. **Open Mayhem PR** pointing `hackrf` submodule to `galic1987/hackrf-portapack@mayhem-cpld-fpga-fixes`
3. **Prototype `ui_debug.cpp` CPLD view** to validate the SRAM-read path in Mayhem context
4. **Design `ui_direction_finding.cpp`** using the Python tools as algorithm reference
5. **Investigate HW_SYNC pin** for sub-microsecond dual-device trigger
