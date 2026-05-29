# Hardware Tests

This directory contains **hardware-in-the-loop** tests for libhackrf and HackRF
devices.  These tests require physical hardware and are **not** run in CI.

## Test Inventory

| Test | Hardware Required | Description |
|------|-------------------|-------------|
| `test_coherence.py` | 2 HackRFs, trigger wire, signal splitter | Validates coherent multi-device operation by capturing ambient RF simultaneously and checking that the phase difference is stable across repeated captures. |
| `test_sync_start.py` | 1–2 HackRFs | Validates the `hackrf_sync_start()` control transfer on physical hardware without streaming samples. |
| `test_fpga_bitstream_guard.py` | HackRF Pro only | Verifies that the Pro firmware rejects FPGA bitstream switches while the transceiver is in RX or TX mode. |

## Environment Variables

The Python tests use the following environment variables for configuration:

| Variable | Description | Example |
|----------|-------------|---------|
| `HACKRF_LIB` | Absolute path to `libhackrf.dylib` or `libhackrf.so` | `/Users/ivo/hackrf/host/build/libhackrf/src/libhackrf.dylib` |
| `HACKRF_SERIAL_PRO` | Serial number of the HackRF Pro under test | `645061de252d6613` |
| `HACKRF_SERIAL_ONE` | Serial number of the HackRF One under test | `922c63dc21748847` |

If a variable is not set, the test scripts fall back to hard-coded defaults
that match the original developer's setup.

## C Tests (CMake)

The following tests are built by CMake and link against `hackrf_static`.
Some use a minimal mock libusb for hardware-free testing:

- `test_hackrf_init`
- `test_cpld_checksum`
- `test_fpga_bitstream`

These are run via `ctest` in the normal CMake build workflow.
