# Before vs After: Complete Capability Comparison

**Baseline (Before):** `greatscottgadgets/hackrf@cc691022` (upstream main)  
**Current (After):** `galic1987/hackrf@main` (9 commits ahead)

---

## 1. CPLD Runtime Verification

### Before: Broken Flash-Read Checksum

```c
// firmware/common/cpld_xc2c.c  (cc691022)
bool cpld_xc2c64a_jtag_checksum(...) {
    cpld_xc2c_jtag_reset_and_idle(jtag);

    if (cpld_xc2c64a_jtag_idcode_ok(jtag) &&
        cpld_xc2c_jtag_read_write_protect(jtag) &&
        cpld_xc2c64a_jtag_idcode_ok(jtag) &&
        cpld_xc2c_jtag_read_write_protect(jtag)) {
        
        cpld_xc2c_jtag_enable(jtag);  // THREE times
        cpld_xc2c_jtag_enable(jtag);
        cpld_xc2c_jtag_enable(jtag);

        cpld_xc2c_jtag_read(jtag);   // ISC_READ (flash)

        // ... read rows from flash ...
    }
}
```

**Problems:**
- **Pipe error on host** — `hackrf_debug -k` returned libusb pipe error because the flash-read path left JTAG in an invalid state
- **Triple `ISC_ENABLE`** — Required fragile sequencing that could hang on some boards
- **No `ISC_DISABLE`/`RESET` cleanup** — CPLD could be left unresponsive until power cycle
- **Slow** — ~80 ms per call

### After: Safe SRAM-Read Checksum

```c
// firmware/common/cpld_xc2c.c  (current)
bool cpld_xc2c64a_jtag_sram_checksum(...) {
    cpld_xc2c_jtag_reset_and_idle(jtag);
    cpld_xc2c_jtag_enable(jtag);           // ONCE

    cpld_xc2c_jtag_sram_read(jtag);        // ISC_SRAM_READ (active config)

    // Dummy-row-first loop over 98 rows
    for (size_t address_row = 0; address_row <= CPLD_XC2C64A_ROWS; address_row++) {
        // ... crc32_update masked rows ...
    }

    *crc_value = crc32_digest(&crc);

    cpld_xc2c_jtag_disable(jtag);          // Clean shutdown
    cpld_xc2c_jtag_bypass(jtag, false);
    cpld_xc2c_jtag_reset(jtag);
    return true;
}
```

**What changed:**
- Single `ISC_ENABLE` instead of triple
- Reads **active SRAM** (`0b11100111`) instead of flash (`0b11101110`)
- Proper `ISC_DISABLE → BYPASS → RESET` cleanup
- ~3 ms runtime (26× faster)

**New capability:** `hackrf_debug -k` now works on HackRF One r9 with PortaPack H2:
```
$ hackrf_debug -k -d 922c63dc21748847
CPLD checksum: 0x05829db7
```

---

## 2. FPGA Bitstream Safety on HackRF Pro

### Before: No Guard — Corruption Risk

```c
// firmware/hackrf_usb/usb_api_praline.c  (cc691022)
usb_request_status_t usb_vendor_request_set_fpga_bitstream(...) {
    if (detected_platform() != BOARD_ID_PRALINE) {
        return USB_REQUEST_STATUS_STALL;
    }

    if (stage == USB_TRANSFER_STAGE_SETUP) {
        if (!fpga_image_load(&fpga_loader, endpoint->setup.value)) {
            return USB_REQUEST_STATUS_STALL;
        }
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}
```

**Problems:**
- **No streaming guard** — calling `hackrf_set_fpga_bitstream()` during active RX/TX would reload the FPGA while SGPIO is actively clocking data
- **Race condition** — even checking `transceiver_request.mode` (the first fix we considered) only checks the *pending* request, not the actual hardware state
- **Silent corruption** — FPGA reload mid-stream = garbled IQ data, potential USB desync, possible crash

### After: Applied-Mode Guard

```c
// firmware/hackrf_usb/usb_api_praline.c  (current)
usb_request_status_t usb_vendor_request_set_fpga_bitstream(...) {
    if (detected_platform() != BOARD_ID_PRALINE) {
        return USB_REQUEST_STATUS_STALL;
    }

    const uint64_t opmode = radio_reg_read(&radio, RADIO_BANK_APPLIED, RADIO_OPMODE);
    if (opmode != TRANSCEIVER_MODE_OFF) {
        return USB_REQUEST_STATUS_STALL;   // BLOCKED during RX/TX
    }

    if (stage == USB_TRANSFER_STAGE_SETUP) {
        if (!fpga_image_load(&fpga_loader, endpoint->setup.value)) {
            return USB_REQUEST_STATUS_STALL;
        }
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}
```

**What changed:**
- Checks `RADIO_BANK_APPLIED` (actual hardware opmode) instead of pending request
- Returns `USB_REQUEST_STATUS_STALL` if radio is not OFF
- Host sees `HACKRF_ERROR_LIBUSB` with 1000 ms timeout

**New capability:** Safe runtime FPGA switching. Verified:
```
Idle:     hackrf_set_fpga_bitstream() → 0 (SUCCESS)
During RX: hackrf_set_fpga_bitstream() → -1000 (STALL)
After stop: hackrf_set_fpga_bitstream() → 0 (SUCCESS)
```

---

## 3. Host Library Timeout

### Before: 100 ms Default Timeout

```c
// host/libhackrf/src/hackrf.c  (cc691022)
int ADDCALL hackrf_set_fpga_bitstream(hackrf_device* device, const uint8_t index)
{
    // ...
    int result = libusb_control_transfer(
        // ...
        NULL, 0,
        DEFAULT_REQUEST_TIMEOUT);   // 100 ms
}
```

**Problem:** `fpga_image_load()` takes ~400 ms. Host times out before device finishes.

### After: 1000 ms FPGA-Specific Timeout

```c
// host/libhackrf/src/hackrf.c  (current)
#define FPGA_BITSTREAM_TIMEOUT 1000

int ADDCALL hackrf_set_fpga_bitstream(...) {
    // ...
    int result = libusb_control_transfer(
        // ...
        NULL, 0,
        FPGA_BITSTREAM_TIMEOUT);   // 1000 ms
}
```

**New capability:** Idle FPGA switching works without timeout errors.

---

## 4. CLI Tools

### `hackrf_debug` — Before: No CPLD Checksum Flag

```c
// host/hackrf-tools/src/hackrf_debug.c  (cc691022)
// No -k option. No hackrf_cpld_checksum() call anywhere.
```

**After:** `-k` / `--cpld-checksum` reads and prints the 32-bit CRC.

### `hackrf_spiflash` — Before: Broken `-C` / `--clear`

```c
// host/hackrf-tools/src/hackrf_spiflash.c  (before faa71ec7)
static struct option long_options[] = {
    {"compatibility", no_argument, 0, 'c'},
    {"clear",         no_argument, 0, 'c'},  // SAME LETTER!
    // ...
};
// optstring: "a:l:r:w:id:scvRh?"  — NO 'C'

case 'c':
    // Reached by BOTH --compatibility AND --clear (broken!)
```

**After:** `-c` = compatibility, `-C` = clear status. Separate code paths.

---

## 5. Multi-Device Coherent Operation

### Before: Nothing

No scripts, no validation, no documented clock-lock procedure for phased-array work.

### After: Two Python Tools

| Tool | What It Does |
|------|-------------|
| `hackrf_array_cal.py` | Captures from both devices, computes phase offset via FFT peak, reports stability statistics |
| `hackrf_beamform.py` | Repeated snapshots, applies calibration, scans steering angles, prints ASCII beam pattern |

**Validated hardware setup:**
```
Pro P2_CLKOUT (10 MHz) ──► One CLKIN
```
- CLKIN detection: **confirmed**
- Phase stability @ 2.437 GHz: **13.6° std dev** (GOOD rating)

---

## 6. Capability Matrix

| Capability | Before | After |
|------------|--------|-------|
| **Runtime CPLD verify** | ❌ Pipe error / crash | ✅ `0x05829db7` in ~3 ms |
| **CPLD verify from CLI** | ❌ Not available | ✅ `hackrf_debug -k` |
| **FPGA switch (idle)** | ❌ Timeout at 100 ms | ✅ 1000 ms timeout, works |
| **FPGA switch (streaming)** | ❌ Corruption risk | ✅ Safely STALLs |
| **SPI flash status clear** | ❌ Broken (`-C` unreachable) | ✅ `hackrf_spiflash -C` |
| **Dual-device clock lock** | ❌ Undocumented | ✅ Validated + tools |
| **Phase calibration** | ❌ Nothing | ✅ `hackrf_array_cal.py` |
| **Live beamforming** | ❌ Nothing | ✅ `hackrf_beamform.py` |
| **Mayhem CPLD diagnostics** | ❌ No path | ✅ Firmware primitive ready |
| **Mayhem FPGA mode switch** | ❌ No path | ✅ Guard + host API ready |

---

## 7. What Can Be Done Now That Couldn't Before

### Field Diagnostics Without a PC
- Before: To verify CPLD integrity you needed JTAG tools and a PC
- Now: `hackrf_debug -k` works from any host. PortaPack Mayhem can add a "Hardware Test" screen that calls the same vendor request.

### Safe FPGA Mode Switching on Pro
- Before: Switching FPGA bitstreams during reception would corrupt the SGPIO data path
- Now: The device STALLs the request if streaming is active. A Mayhem UI can switch between standard / ext_precision_rx / half_precision modes with confidence.

### Coherent Multi-Device Arrays
- Before: No tooling existed for phase-locked multi-HackRF operation
- Now: Clock lock validated (Pro → One), calibration scripts working, beamforming prototype running. A PortaPack app could orchestrate 2+ devices for direction finding or passive radar.

### Upstream Contribution
- Before: No PR-ready fixes for the known CPLD/fpga issues
- Now: 9 clean commits on a fork, all tested on hardware, with comprehensive documentation
