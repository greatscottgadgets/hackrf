# End-to-End Validation Report

**Date:** 2026-05-29
**Branch:** `upstream-pr-clean`
**Commit:** `357a70ae` (report), `220f3d0c` (test scripts)

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
| HackRF One r9 | `922c63dc21748847` | 4 | PortaPack H2 | `git-c9cad5f2` | ✅ Online |

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

Result: ✅ PASS (expected — no external CLKIN connected)
```

### 2.7 Final Device State
```
Requested mode: 0 (IDLE) [complete]
Active mode: 0 (IDLE)
Error: 0 (NONE)
Result: ✅ Device clean, no streaming active
```

---

## 3. HackRF One r9 — Hardware Validation

### 3.1 Recovery
The device initially failed to enumerate after SPI flash. Recovery required:
1. Enter DFU mode (hold DFU button while connecting USB)
2. Flash firmware via `dfu-util -D hackrf_usb.dfu`
3. Flash to SPI flash via `hackrf_spiflash -w hackrf_usb.bin`
4. Reset via `hackrf_spiflash -R`

Device now enumerates correctly with serial `922c63dc21748847`.

### 3.2 Basic Connectivity
```
Firmware Version: git-c9cad5f2 (API:1.12)
Hardware Revision: r9
Result: ✅ PASS
```

### 3.3 Sync-Start API (`hackrf_sync_start`)
Test script: `host/tests/test_sync_start.py`

| Mode | Expected | Result | Status |
|------|----------|--------|--------|
| OFF (0) | SUCCESS | `HACKRF_SUCCESS` | ✅ PASS |
| RX (1) | SUCCESS | `HACKRF_SUCCESS` | ✅ PASS |
| TX (2) | SUCCESS | `HACKRF_SUCCESS` | ✅ PASS |
| Invalid (255) | FAILURE | `Pipe error (-1000)` | ✅ PASS |

Result: ✅ PASS

### 3.4 CPLD Checksum
```
Command: hackrf_debug -k -s 922c63dc21748847
CPLD checksum: 0x05829db7

Result: ✅ PASS — matches expected deterministic checksum
```

### 3.5 SPI Flash Status
```
Command: hackrf_spiflash -s -d 922c63dc21748847
Status: 0x00 02

Result: ✅ PASS — status registers clean
```

### 3.6 Clock Check
```
Command: hackrf_clock -i -d 922c63dc21748847
CLKIN status: no clock signal detected

Result: ✅ PASS (expected — Pro CLKOUT not enabled)
```

### 3.7 Final Device State
```
Requested mode: 0 (IDLE) [complete]
Active mode: 0 (IDLE)
Error: 0 (NONE)
Result: ✅ Device clean, no streaming active
```

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
| One Hardware | 6 | 6 | 0 | 0 |
| Host Tests | 3 | 3 | 0 | 0 |
| **Total** | **19** | **19** | **0** | **0** |

**Verdict:** ✅ ALL TESTS PASS on both devices.
