# End-to-End Validation Report

**Date:** 2026-05-29
**Branch:** `upstream-pr-clean`
**Commit:** `220f3d0c`

---

## Test Environment

- **OS:** macOS (Apple Silicon)
- **ARM Toolchain:** GNU 13.2.1 (`arm-gnu-toolchain-13.2.rel1-darwin-arm64-arm-none-eabi`)
- **Build System:** CMake + Make
- **libhackrf:** Built from source at `host/build/libhackrf/src/libhackrf.dylib`

---

## Devices Under Test

| Device | Serial | Board ID | Hardware | Firmware | Status |
|--------|--------|----------|----------|----------|--------|
| HackRF Pro r1.2 | `645061de252d6613` | 5 | GSG manufactured | `git-c9cad5f2` | ✅ Online |
| HackRF One r9 | `922c63dc21748847` | 4 | PortaPack H2 | `git-c9cad5f2` | ❌ Offline (recovery needed) |

---

## 1. Build Validation

### Firmware (HACKRF_ONE)
```
Build target: hackrf_usb.elf, hackrf_usb.bin, hackrf_usb.dfu
Result: ✅ SUCCESS
```

### Firmware (PRALINE)
```
Build target: hackrf_usb.elf, hackrf_usb.bin, hackrf_usb.dfu
Result: ✅ SUCCESS
```

### Host Library
```
Build target: libhackrf.dylib + all hackrf-tools
Result: ✅ SUCCESS
```

### Unit Tests
```
test_cpld_checksum     3/3 passed ✅
test_fpga_bitstream    2/2 passed ✅
test_hackrf_init       4/4 passed ✅
```

---

## 2. HackRF Pro r1.2 — Hardware Validation

### 2.1 Basic Connectivity
```
Firmware Version: git-c9cad5f2 (API:1.12)
Hardware Revision: r1.2
Result: ✅ PASS
```

### 2.2 Sync-Start API (`hackrf_sync_start`)
Test script: `host/tests/test_sync_start.py`

| Mode | Expected | Result | Status |
|------|----------|--------|--------|
| OFF (0) | SUCCESS | `HACKRF_SUCCESS` | ✅ PASS |
| RX (1) | SUCCESS | `HACKRF_SUCCESS` | ✅ PASS |
| TX (2) | SUCCESS | `HACKRF_SUCCESS` | ✅ PASS |
| Invalid (255) | FAILURE | `Pipe error (-1000)` | ✅ PASS |

**Note:** Invalid mode returns `Pipe error` (USB STALL) rather than `HACKRF_ERROR_INVALID_PARAM`. This is correct behavior — the firmware STALLs invalid requests.

### 2.3 FPGA Bitstream Guard
Test script: `host/tests/test_fpga_bitstream_guard.py`

```
Steps:
1. Device in OFF mode
2. sync_start(RX) → SUCCESS
3. set_fpga_bitstream(0) → Pipe error (-1000)
Result: ✅ PASS — guard correctly rejects bitstream switch in RX mode
```

### 2.4 SPI Flash — Clear Status (`-C`)
```
Command: hackrf_spiflash -C -d 645061de252d6613
Exit code: 0

Status after clear:
  SRP0=0, SEC=0, TB=0, BP=0, WEL=0, Busy=0
  SUS=0, CMP=0, LB=0, Res=0, SRP1=0
  QE=1 (persistent config bit, expected)
Result: ✅ PASS
```

### 2.5 Self-Test
```
Command: hackrf_debug -t -d 645061de252d6613

Mixer: RFFC5072 — PASS
Clock: Si5351 — PASS
Transceiver: MAX2831 — PASS
FPGA configuration — PASS
FPGA SPI — PASS
SGPIO RX test — PASS
Loopback test — PASS

Result: ✅ PASS
```

### 2.6 Clock Check
```
Command: hackrf_clock -i -d 645061de252d6613
CLKIN status: no clock signal detected

Result: ✅ PASS (expected — One is offline, no CLKIN connected)
```

### 2.7 Final Device State
```
Requested mode: 0 (IDLE) [complete]
Active mode: 0 (IDLE)
Error: 0 (NONE)
Result: ✅ Device clean, no streaming active
```

---

## 3. HackRF One r9 — Recovery Status

**Issue:** Device not enumerating on USB after firmware flash + reset.

**Investigation:**
- ❌ Not visible in `hackrf_info`
- ❌ Not visible in `dfu-util -l`
- ❌ Not visible in `ioreg` / `system_profiler`
- ❌ Software reset (`hackrf_spiflash -R`) does not restore USB

**Root Cause:** Likely PortaPack H2 interference. The PortaPack may have taken control of the LCD/UI and prevented USB enumeration.

**Recovery Steps Required (physical):**
1. Check PortaPack screen — select "HackRF mode" if available
2. If unavailable, hold **DFU button** while connecting USB to enter ROM bootloader
3. Re-flash via `dfu-util` then `hackrf_spiflash`
4. If DFU also fails, check USB cable (charge-only cables lack data lines)

---

## 4. API Symbol Verification

| Symbol | Status | Location |
|--------|--------|----------|
| `hackrf_sync_start` | ✅ Exported | `libhackrf.dylib` |
| `hackrf_set_hw_sync_mode` | ✅ Exported | `libhackrf.dylib` |
| `hackrf_cpld_checksum` | ✅ Exported | `libhackrf.dylib` |
| `hackrf_spiflash_clear_status` | ✅ Exported | `libhackrf.dylib` |

---

## 5. CLI Tool Verification

| Tool | Flag | Status |
|------|------|--------|
| `hackrf_debug` | `-k, --cpld-checksum` | ✅ Present |
| `hackrf_spiflash` | `-C, --clear` | ✅ Present |

---

## Summary

| Category | Tests | Passed | Failed | Blocked |
|----------|-------|--------|--------|---------|
| Build | 4 | 4 | 0 | 0 |
| Pro Hardware | 6 | 6 | 0 | 0 |
| One Hardware | 6 | 0 | 0 | 6 |
| Host Tests | 3 | 3 | 0 | 0 |
| **Total** | **19** | **13** | **0** | **6** |

**Verdict:** All online devices pass all tests. The HackRF One requires physical recovery before its tests can be run.
