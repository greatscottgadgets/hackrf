# Synchronized Multi-Device Start — Design Document

**Status:** Design phase  
**Priority:** High (blocking phased-array/Direction-Finding in Mayhem)  
**Dependencies:** Clock lock (✅ verified), Phase calibration (✅ tool working)

---

## Problem Statement

Current multi-device capture is **sequential**:
```python
# hackrf_array_cal.py — current approach
subprocess.run([hackrf_transfer, "-r", ref_file, ...])      # Device 1 starts
subprocess.run([hackrf_transfer, "-r", arr_file, ...])      # Device 2 starts ~200ms later
```

This introduces **sample misalignment** that varies between captures, making phase calibration unstable for dynamic signals.

**Goal:** Sub-millisecond synchronized start across N clock-locked HackRFs.

---

## Existing Infrastructure

### HW_SYNC / Trigger System

The firmware already has a trigger mechanism:

```c
// firmware/hackrf_usb/usb_api_transceiver.c
usb_request_status_t usb_vendor_request_set_hw_sync_mode(...) {
    radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_TRIGGER, endpoint->setup.value);
}
```

This writes to `RADIO_TRIGGER`, which `radio_update()` applies via:

```c
// firmware/common/radio.c
static uint32_t radio_update_trigger(...) {
    trigger_enable(enable);
    radio->config[RADIO_BANK_APPLIED][RADIO_TRIGGER] = enable;
    return (1 << RADIO_TRIGGER);
}
```

And `trigger_enable()`:

```c
// firmware/common/clock_io.c
void trigger_enable(const bool enable) {
#ifdef IS_NOT_PRALINE
    gpio_write(sgpio_config.gpio_trigger_enable, enable);  // One: GPIO
#endif
#ifdef IS_PRALINE
    fpga_set_trigger_enable(&fpga, enable);                // Pro: FPGA reg
#endif
}
```

**Current behavior:** `hackrf_transfer -H 1` sets `RADIO_TRIGGER=1`, but this only **arms** the trigger input. It does not start sampling.

### What the Trigger Pin Actually Does

On **HackRF One**: `gpio_trigger_enable` drives the P20 pin. When enabled, the MAX5864 ADC/DAC waits for an external trigger pulse before beginning conversion.

On **HackRF Pro**: `fpga_set_trigger_enable()` sets a bit in `FPGA_STANDARD_CTRL`. The FPGA waits for a trigger before clocking SGPIO.

---

## Proposed Solution: `HACKRF_VENDOR_REQUEST_SYNC_START`

### Design Option A — Software Sync (Recommended First Step)

**Mechanism:** A new vendor request that sets both `RADIO_TRIGGER` and `RADIO_OPMODE` atomically in a single control transfer.

```c
// firmware/hackrf_usb/usb_api_sync.c
usb_request_status_t usb_vendor_request_sync_start(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage)
{
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        const transceiver_mode_t mode = endpoint->setup.value;
        
        // Atomically set trigger + mode
        nvic_disable_irq(NVIC_USB0_IRQ);
        radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_TRIGGER, 1);
        radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_OPMODE, mode);
        radio->regs_dirty |= (1 << RADIO_TRIGGER) | (1 << RADIO_OPMODE);
        nvic_enable_irq(NVIC_USB0_IRQ);
        
        // Force immediate apply (don't wait for main loop)
        radio_update(&radio);
        
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}
```

**Host API:**

```c
int ADDCALL hackrf_sync_start(hackrf_device* device, transceiver_mode_t mode);
```

**Usage:**
```python
# Python pseudocode
for dev in devices:
    hackrf_set_freq(dev, freq)
    hackrf_set_sample_rate(dev, rate)

# Broadcast start (as close to simultaneously as USB allows)
for dev in devices:
    hackrf_sync_start(dev, TRANSCEIVER_MODE_RX)
```

**Expected jitter:** ~1-5 ms (USB scheduling variance)

**Pros:** Simple, no hardware changes, works with existing clock lock  
**Cons:** USB scheduling jitter limits precision

---

### Design Option B — Hardware Trigger Sync (Higher Precision)

**Mechanism:** Use the existing P20 trigger pin wired across all devices.

**Hardware setup:**
```
Pro P20 (trigger_out) ──► One 1 P20 (trigger_in)
                      ──► One 2 P20 (trigger_in)
```

**Firmware change:** Add a vendor request to pulse the trigger output:

```c
// New: pulse trigger for exactly N microseconds
usb_request_status_t usb_vendor_request_trigger_pulse(...) {
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        const uint16_t us = endpoint->setup.value;
        
        // On master device: pulse P20 high then low
        gpio_set(gpio_trigger);
        delay_us_at_mhz(us, 204);
        gpio_clear(gpio_trigger);
        
        usb_transfer_schedule_ack(endpoint->in);
    }
    return USB_REQUEST_STATUS_OK;
}
```

**Usage flow:**
1. All devices: `hackrf_set_hw_sync_mode(1)` → arms trigger input
2. All devices: set freq, sample rate, but DO NOT start RX
3. Master device: `hackrf_trigger_pulse(10)` → all devices start simultaneously

**Expected jitter:** <1 µs (GPIO propagation delay)

**Pros:** Sub-microsecond precision, deterministic  
**Cons:** Requires physical wiring, master/slave role assignment

---

### Design Option C — USB Broadcast Sync (Best of Both)

**Mechanism:** Use libusb's ability to send control transfers to multiple devices from the same host with minimal inter-call delay.

```c
// Host-side: single-threaded rapid-fire
struct timeval tv;
gettimeofday(&tv, NULL);
const uint64_t target_us = tv.tv_usec + 50000;  // 50ms from now

for (int i = 0; i < n_devices; i++) {
    // Send SYNC_ARM with target timestamp
    hackrf_sync_arm(devices[i], target_us, TRANSCEIVER_MODE_RX);
}

// Device-side: SCTimer counts down to target timestamp, then starts
```

**Firmware change:** Use the LPC43xx SCTimer (State Configurable Timer) to schedule the start:

```c
// In usb_vendor_request_sync_arm handler:
// 1. Parse target timestamp from wValue/wIndex
// 2. Configure SCTimer to match
// 3. On match interrupt: call request_transceiver_mode(mode)
```

**Expected jitter:** ~10-100 µs (SCTimer resolution)

**Pros:** No extra wiring, better than pure software  
**Cons:** Requires SCTimer programming, host clock sync

---

## Recommendation

**Phase 1:** Implement **Option A** (Software Sync) as a new vendor request. It requires minimal firmware changes and provides ~1-5ms alignment — sufficient for beamforming and direction-finding at VHF/UHF frequencies where wavelength >> jitter distance.

**Phase 2:** Implement **Option B** (Hardware Trigger) for applications needing <1µs precision (e.g., radar, interferometry).

---

## Files to Modify

### Firmware

| File | Change |
|------|--------|
| `firmware/hackrf_usb/usb_api_sync.c` (new) | `usb_vendor_request_sync_start()` handler |
| `firmware/hackrf_usb/usb_api_sync.h` (new) | Header with request enum |
| `firmware/hackrf_usb/hackrf_usb.c` | Register vendor request in dispatch table |
| `firmware/common/radio.c` | Ensure `radio_update()` can be called from USB context safely |

### Host Library

| File | Change |
|------|--------|
| `host/libhackrf/src/hackrf.c` | Add `hackrf_sync_start()` function |
| `host/libhackrf/src/hackrf.h` | Declare function, document API version requirement |

### Tools

| File | Change |
|------|--------|
| `host/hackrf-tools/src/hackrf_transfer.c` | Add `--sync` flag for synchronized start |
| `ci-scripts/hackrf_array_cal.py` | Use `hackrf_sync_start()` instead of sequential subprocess |

---

## Vendor Request Allocation

The next available vendor request number after `HACKRF_VENDOR_REQUEST_READ_SELFTEST = 56` is **57**.

```c
// host/libhackrf/src/hackrf.c
HACKRF_VENDOR_REQUEST_SYNC_START = 57,
```

---

## Testing Plan

1. **Two-device loopback:** Connect Pro TX to One RX with cable. Verify sample alignment by checking phase consistency across 100 captures.
2. **Clock-lock + sync:** Pro as clock master + sync master, One as slave. Verify combined jitter <5ms.
3. **Mayhem integration:** Add sync trigger to `receiver_model.cpp` start path.
