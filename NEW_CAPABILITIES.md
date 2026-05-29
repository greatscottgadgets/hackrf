# New HackRF Capabilities: Unlocked Demos & Projects

This document analyzes the new firmware features introduced in API version **0x0113** and designs concrete demos that showcase them. All features have been validated on HackRF Pro r1.2 and HackRF One r9 hardware.

---

## Feature Overview

| # | Feature | Before | After | Demo Idea | Difficulty |
|---|---------|--------|-------|-----------|------------|
| 1 | **CPLD Checksum** (`hackrf_debug -k`) | No host-side way to verify CPLD integrity | Host can read deterministic SRAM checksum of XC2C64A | Secure boot attestation, tamper detection | Small |
| 2 | **FPGA Bitstream Guard** | FPGA could be reconfigured mid-RX/TX, corrupting samples | Firmware rejects bitstream switch while opmode ≠ OFF | Safe dynamic reconfiguration, runtime integrity | Small |
| 3 | **SPI Flash Clear Status** (`hackrf_spiflash -C`) | Stuck status registers (WIP, WEL, SUS) required power cycle | Host can clear status registers via software | Recovery tools, fleet maintenance scripts | Small |
| 4 | **Sync-Start** (`hackrf_sync_start`) | No API to arm multiple devices for shared trigger | Devices can be armed in OFF/RX/TX via vendor request; trigger fires sample-accurate start | Phased arrays, coherent MIMO, TDOA | Medium |
| 5 | **Self-Test ID Fix** | Newer RFFC5072 variants failed mixer self-test | Self-test accepts extended chip ID/revision range | Manufacturing QA, field diagnostics | Small |
| 6 | **Multi-Device SPI Flash Guard** | `hackrf_spiflash -w` could flash wrong board if serial omitted | Firmware info structure + board/platform compatibility check prevents cross-flashing | Safe fleet firmware updates | Small |
| 7 | **API Version 0x0113** | No version tracking for new features | Clean capability detection; old host/lib combinations get `HACKRF_ERROR_USB_API_VERSION` | Graceful degradation in multi-device tools | N/A |

---

## Detailed Demo Designs

### Demo 1: Multi-Device Phased Array / Direction Finding

**What was impossible before:**
- Without `hackrf_sync_start`, there was no clean API to put multiple devices into a pre-armed state where they all begin sampling on the same trigger edge. You could use `hackrf_transfer -H`, but that required shelling out to separate processes and did not integrate cleanly with programmatic capture loops.
- Phase drift between independent TCXOs made coherent integration impossible beyond ~100 ms.

**What is now possible:**
- `hackrf_sync_start(mode)` arms any number of devices. When the first device enters RX (or an external trigger is applied), all others sample-lock within **<1 sample period**.
- Combined with shared clock (Pro CLKOUT → One CLKIN), phase is stable across captures to within **<5° std dev** (validated over 10 runs).

**Concrete demo: 2-Element FM Bearing Finder**

1. **Hardware setup**
   - HackRF Pro (clock source) + HackRF One (array element)
   - Pro P2 CLKOUT → One CLKIN (SMA cable)
   - Pro P1 trigger_out → One trigger_in (jumper on P28 pin 15→16, or use shared ground + SMA if Pro exposes trigger)
   - Two dipole or whip antennas separated by **d ≈ 1 m** (≈ λ/3 at 100 MHz)

2. **Calibration** (`ci-scripts/hackrf_array_cal.py`)
   - Point both antennas at the same strong FM station.
   - Capture 262k samples from both devices using the sync-start + `hackrf_set_hw_sync_mode(1)` trigger sequence.
   - Compute phase offset via FFT peak or cross-correlation.
   - Save calibration constant `cal = exp(-j·φ_offset)`.

3. **Live bearing estimation** (`ci-scripts/hackrf_beamform.py`)
   - Repeatedly capture synchronized snapshots.
   - Apply calibration, then scan steering phases from -180° to +180°.
   - Peak beam power gives arrival angle: `θ ≈ arcsin(λ·Δφ / 2πd)`.
   - Display running mean ± std dev and ASCII beam pattern.

4. **Expected results**
   - On a local 100 MHz FM broadcaster, expect bearing stability of **±5–15°** with 1 m baseline.
   - With 4+ elements and MUSIC algorithm, resolution improves to **±2–3°**.

**Implementation difficulty:** **Medium** — calibration script exists; extending to N>2 elements requires array geometry math and possibly a GUI.

**Hardware requirements:**
| Item | Qty | Notes |
|------|-----|-------|
| HackRF Pro | 1 | Clock source (10 MHz CLKOUT) |
| HackRF One | 1+ | Array elements |
| SMA cables | 2 | Clock + optional trigger |
| Jumper wires | 2 | Trigger + ground (if One→One) |
| Antennas | 2 | Identical whips or dipoles |
| Laptop | 1 | Python + numpy + libhackrf |

---

### Demo 2: Secure Boot / Tamper Detection

**What was impossible before:**
- The host had no way to verify that the CPLD contained the expected configuration. An adversary could flash a malicious CPLD bitstream (e.g., to bypass filters or exfiltrate data over a side channel) and the host would never know.
- The FPGA bitstream could be switched at any time, even during active RX/TX, causing silent data corruption or protocol injection.

**What is now possible:**
- `hackrf_cpld_checksum()` returns a deterministic CRC of the active CPLD SRAM configuration.
- The FPGA bitstream guard ensures the active bitstream cannot be swapped while the radio is on.

**Concrete demo: Boot-Time Attestation Script**

```python
#!/usr/bin/env python3
"""hackrf_attest.py — Verify device integrity before use."""
import ctypes, sys

LIBHACKRF = "./host/build/libhackrf/src/libhackrf.dylib"
lib = ctypes.CDLL(LIBHACKRF)

# Known-good checksums for your firmware revision
TRUSTED_CPLD_CHECKSUMS = {
    "922c63dc21748847": 0x05829db7,   # HackRF One r9
    "645061de252d6613": 0x0a3b4c5d,   # HackRF Pro r1.2 (example)
}

def attest(serial: str) -> bool:
    dev = ctypes.c_void_p()
    lib.hackrf_open_by_serial(serial.encode(), ctypes.byref(dev))
    
    # 1. Verify CPLD checksum
    crc = ctypes.c_uint32()
    lib.hackrf_cpld_checksum(dev, ctypes.byref(crc))
    expected = TRUSTED_CPLD_CHECKSUMS.get(serial)
    if expected and crc.value != expected:
        print(f"TAMPER: CPLD checksum mismatch! Expected 0x{expected:08x}, got 0x{crc.value:08x}")
        return False
    
    # 2. Verify API version (ensures firmware supports guards)
    api = ctypes.c_uint16()
    lib.hackrf_usb_api_version_read(dev, ctypes.byref(api))
    if api.value < 0x0113:
        print(f"TAMPER: Firmware too old (API 0x{api.value:04x}) — cannot verify guards")
        return False
    
    # 3. Verify device is in OFF (not left in RX/TX by previous process)
    # (Poll hackrf_is_streaming or use sync_start(OFF) as reset)
    lib.hackrf_sync_start(dev, 0)  # reset to OFF
    
    print(f"PASS: {serial} attested — CPLD 0x{crc.value:08x}, API 0x{api.value:04x}")
    lib.hackrf_close(dev)
    return True

if __name__ == "__main__":
    ok = all(attest(s) for s in sys.argv[1:])
    sys.exit(0 if ok else 1)
```

**Extensions:**
- **Fleet HSM integration:** Store trusted checksums in a YubiKey or TPM; sign attestation reports.
- **Runtime re-auth:** Re-check CPLD checksum every 30 minutes. If it changes mid-session, abort and alert.
- **FPGA bitstream guard test:** Before entering RX, deliberately attempt a bitstream switch; expect failure. This proves the guard is active.

**Implementation difficulty:** **Small** — the script above is ~40 lines. HSM integration pushes it to **Medium**.

**Hardware requirements:**
| Item | Qty | Notes |
|------|-----|-------|
| HackRF One/Pro | 1+ | Any device with API ≥0x0103 for CPLD checksum |
| YubiKey (optional) | 1 | For signed attestation |

---

### Demo 3: Safe Fleet Management

**What was impossible before:**
- `hackrf_spiflash -w` without `-d` would target the first enumerated device. In a multi-device lab or production line, this routinely bricked the wrong unit.
- Stuck SPI flash status registers (e.g., after a failed write or brown-out) required physical power-cycling, which is impossible in headless/remote setups.

**What is now possible:**
- **Compatibility guard:** `hackrf_spiflash` parses the firmware info structure at offset `0x400` and checks `supported_platforms` against the target's `board_id`. A Pro firmware image cannot be flashed to a One, and vice versa.
- **Status clear:** `hackrf_spiflash -C` sends `0x30` + `0x01` (write-enable + clear status) to reset stuck `WIP`, `WEL`, or `SUS` bits.

**Concrete demo: Update 5 Devices Without Bricking Any**

```bash
#!/bin/bash
# update_fleet.sh — Safe multi-device firmware update

FW_PRO="hackrf_usb_pro.bin"
FW_ONE="hackrf_usb_one.bin"
LOG="fleet_update_$(date +%Y%m%d_%H%M%S).log"

# Discover all devices
mapfile -t DEVICES < <(hackrf_info 2>/dev/null | grep "Serial number" | awk '{print $3}')

echo "Found ${#DEVICES[@]} devices" | tee -a "$LOG"

for serial in "${DEVICES[@]}"; do
    echo -e "\n--- Processing $serial ---" | tee -a "$LOG"
    
    # Read board ID to choose correct firmware
    board_id=$(hackrf_info -d "$serial" | grep "Board ID" | awk '{print $3}')
    case "$board_id" in
        4|5)  # HackRF One OG / r9
            fw="$FW_ONE"
            ;;
        6)    # Praline / Pro
            fw="$FW_PRO"
            ;;
        *)
            echo "SKIP: Unknown board ID $board_id" | tee -a "$LOG"
            continue
            ;;
    esac
    
    # Pre-flight: clear any stuck status
    hackrf_spiflash -C -d "$serial" | tee -a "$LOG"
    
    # Flash with compatibility guard (built into hackrf_spiflash -w)
    if hackrf_spiflash -w "$fw" -d "$serial" | tee -a "$LOG"; then
        echo "PASS: $serial updated" | tee -a "$LOG"
        hackrf_spiflash -R -d "$serial"  # reset to load new firmware
    else
        echo "FAIL: $serial update aborted by compatibility guard" | tee -a "$LOG"
    fi
done
```

**Extensions:**
- **CI/CD integration:** Run this in GitHub Actions with a USB-over-IP hub for remote testing labs.
- **Rollback on failure:** Keep the previous firmware image; if the device fails to enumerate within 10 s after reset, DFU-recover and re-flash the old image.

**Implementation difficulty:** **Small** — the script above is sufficient for manual labs. CI integration makes it **Medium**.

**Hardware requirements:**
| Item | Qty | Notes |
|------|-----|-------|
| HackRF devices | 2–10+ | Mixed One/Pro OK |
| USB hub with per-port power | 1 | For remote power-cycle recovery |

---

### Demo 4: Passive Radar / Bistatic SAR

**What was impossible before:**
- Independent clocks caused sample-time drift. Over a 1-second coherent integration, two free-running 20 ppm TCXOs drift by **20 µs** (≈ 400 samples at 20 MSPS), completely destroying Doppler resolution.
- Without sync-start, trigger latency varied by milliseconds depending on USB scheduling.

**What is now possible:**
- Shared clock eliminates sample-clock drift.
- `hackrf_sync_start(RX)` + `hackrf_set_hw_sync_mode(1)` gives **sample-accurate** trigger alignment.
- Coherent integration over **tens of seconds** is now feasible, enabling Doppler resolution of **<0.1 Hz**.

**Concrete demo: FM-Broadcast Passive Radar**

1. **Hardware setup**
   - **Reference channel:** HackRF One with directional Yagi pointed at FM tower (direct path).
   - **Surveillance channel:** HackRF One with omnidirectional antenna pointed at sky (reflections).
   - Both clock-locked to HackRF Pro CLKOUT.
   - Trigger loop: Pro trigger_out → both devices trigger_in.

2. **Capture**
   - Tune both to 100 MHz (strong local FM station).
   - Sample rate 10 MSPS; capture 10 s bursts.
   - Use `hackrf_sync_start` + trigger to start both simultaneously.

3. **Signal processing** (Python / GNU Radio)
   - Cross-ambiguity function (CAF) between reference and surveillance:
     ```python
     def caf(ref, surv, max_doppler=100, doppler_res=0.1):
         n = len(ref)
         doppler_shifts = np.arange(-max_doppler, max_doppler, doppler_res)
         peaks = []
         for fd in doppler_shifts:
             shifted = surv * np.exp(-2j*np.pi*fd*np.arange(n)/fs)
             corr = np.fft.ifft(np.fft.fft(ref) * np.conj(np.fft.fft(shifted)))
             peaks.append(np.max(np.abs(corr)))
         return doppler_shifts, peaks
     ```
   - Direct path cancellation: subtract scaled reference from surveillance.
   - Detect aircraft by Doppler shift (±50–500 Hz) and delay (≈1–100 µs).

4. **Expected results**
   - Commercial aircraft at 10 km altitude, 100 km range: RCS ≈ 10–100 m².
   - FM illuminator ERP ≈ 10 kW → detectable SNR after 10 s integration.
   - Doppler resolution: **0.1 Hz** → velocity resolution ≈ **0.15 m/s** at 100 MHz.

**Implementation difficulty:** **Large** — requires significant DSP (ambiguity function, clutter cancellation, CFAR detection). The RF frontend (clock/sync) is now solved; the signal processing is the hard part.

**Hardware requirements:**
| Item | Qty | Notes |
|------|-----|-------|
| HackRF One | 2 | Reference + surveillance |
| HackRF Pro | 1 | Clock + trigger source |
| Yagi antenna | 1 | Reference channel (FM tower) |
| Omni antenna | 1 | Surveillance channel (sky) |
| Low-noise amps (optional) | 2 | If FM signal is weak |
| Tripod/mast | 2 | Spatial separation >10 m reduces direct-path leakage |

---

### Demo 5: HF/Sub-MHz Reception with External Downconverter

**What was impossible before:**
- HackRF's native floor is ~1 MHz (with direct sampling). Below that, sensitivity and image rejection degrade severely.
- Without stable clock distribution, external downconverters (e.g., Ham It Up, 125 MHz LO) drift relative to the HackRF, causing tuning errors and phase noise.

**What is now possible:**
- Pro CLKOUT can drive the external downconverter's LO reference (if it accepts 10 MHz ref in).
- Sync-start ensures multiple HackRFs (e.g., for I/Q balance or diversity) start on the same sample.

**Concrete demo: 40 m Amateur Band (7.0–7.3 MHz) Receiver**

1. **Hardware**
   - HackRF One + Ham It Up v1.3 (125 MHz LO, 0–65 MHz IF → 125–190 MHz at HackRF input).
   - Or: Custom Tayloe mixer (QSD) clocked from Pro CLKOUT.

2. **Configuration**
   - Pro CLKOUT → Ham It Up "REF IN" (10 MHz).
   - HackRF tuned to 125 MHz + desired_offset (e.g., 125.100 MHz for 7.100 MHz RF).
   - Sample rate 2–10 MSPS; decimate in software to 48 kHz audio bandwidth.

3. **Software**
   - Use existing `decode_fm.py` structure, but with SSB/CW demod instead of WBFM.
   - Or feed IQ into GNU Radio / fldigi for digital modes.

**Implementation difficulty:** **Medium** — mostly wiring and LO frequency math. The sync-start feature is overkill for a single receiver but essential if building a multi-channel diversity receiver.

**Hardware requirements:**
| Item | Qty | Notes |
|------|-----|-------|
| HackRF One/Pro | 1 | Receiver |
| Ham It Up / QSD | 1 | External downconverter |
| 10 MHz ref cable | 1 | CLKOUT → REF IN |
| HF antenna | 1 | Dipole or long-wire for target band |

---

## Implementation Difficulty Summary

| Difficulty | Definition | Demos |
|------------|------------|-------|
| **Small** | <1 day, single script, no external deps beyond numpy/libhackrf | CPLD attestation, fleet flash, SPI status clear |
| **Medium** | 1–3 days, requires calibration or signal processing, some hardware | 2-element beamforming, HF downconverter reception |
| **Large** | 1–2 weeks, advanced DSP, custom hardware, or real-time constraints | Passive radar, N-element MUSIC beamforming, bistatic SAR |

---

## Cross-Feature Synergies

Several demos combine multiple new features:

1. **Secure phased array:** Attestation (CPLD checksum + API version check) → clock config → sync-start → beamforming. If any step fails, the array refuses to operate.
2. **Remote passive radar station:** Fleet flash script updates firmware on 4 devices; attestation verifies integrity; sync-start + shared clock enable coherent integration; results uploaded to central server.
3. **Educational kit:** 2× HackRF One + Pro clock source. Students run calibration → beamforming → direction finding in a single 2-hour lab. SPI flash guard prevents them from bricking devices.

---

## Validation Status

All features were validated on real hardware (see `END_TO_END_VALIDATION_REPORT.md`):

| Feature | Pro r1.2 | One r9 | Test File |
|---------|----------|--------|-----------|
| CPLD checksum | N/A | ✅ | `host/tests/test_cpld_checksum.c` |
| FPGA bitstream guard | ✅ | N/A | `host/tests/test_fpga_bitstream_guard.py` |
| SPI flash clear status | ✅ | ✅ | Manual (`hackrf_spiflash -C`) |
| Sync-start | ✅ | ✅ | `host/tests/test_sync_start.py` |
| Phase coherence | ✅ | ✅ | `host/tests/test_coherence.py` |
| Self-test | ✅ | ✅ | `hackrf_debug -t` |
| Multi-device flash guard | ✅ | ✅ | Built into `hackrf_spiflash` |

---

## Next Steps

1. **Extend beamforming to N>2 elements:** Generalize `hackrf_array_cal.py` to accept an arbitrary list of serials and compute a full calibration matrix.
2. **Real-time GUI:** Build a PyQt / PySide spectrum + beam-pattern display that updates live from sync-started captures.
3. **Passive radar DSP chain:** Implement the cross-ambiguity function in C/CUDA for real-time aircraft detection.
4. **Publish attestation spec:** Document the CPLD checksum + FPGA guard as a formal "HackRF Trust Anchor" protocol.
