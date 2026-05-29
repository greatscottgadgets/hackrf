# Upstream PR Preparation Notes

**Target:** `greatscottgadgets/hackrf`  
**Source:** `galic1987/hackrf#main`  
**Commits to include:** `faa71ec7` through `4decb680` (9 commits)

---

## Commit Breakdown

| Commit | Files | Description | Upstream Value |
|--------|-------|-------------|----------------|
| `faa71ec7` | `hackrf_debug.c`, `hackrf_spiflash.c` | Fix `-C` flag conflict, add `-k` CPLD checksum | High — bugfix + feature |
| `7400217b` | `hackrf_array_cal.py` | Dual-device phase calibration tool | Medium — new tool |
| `9161856e` | `hackrf_array_cal.py` | Multi-run stability stats | Low — enhancement |
| `97c162e1` | `hackrf_beamform.py` | Live beamforming tool | Medium — new tool |
| `3d1de1f8` | `hackrf.c` | FPGA bitstream timeout 100→1000ms | High — bugfix |
| `8d5a9c3c` | `cpld_xc2c.c`, `cpld_xc2c.h`, `usb_api_cpld.c`, `usb_api_praline.c` | CPLD SRAM-read + FPGA guard | High — core fixes |
| `7307883f` | `ACHIEVEMENTS_REPORT.md`, `MAYHEM_UNLOCK_ANALYSIS.md` | Documentation | N/A for upstream |
| `4decb680` | `BEFORE_AFTER_COMPARISON.md` | Documentation | N/A for upstream |

## Recommended PR Split

**PR 1 — Bugfixes (high priority, easy review):**
- `faa71ec7`: spiflash `-C` fix + debug `-k` flag
- `3d1de1f8`: FPGA timeout fix

**PR 2 — Firmware improvements (needs careful review):**
- `8d5a9c3c`: CPLD SRAM-read + FPGA guard

**PR 3 — Tools (optional, community value):**
- `7400217b` + `9161856e` + `97c162e1`: Python tools

## Pre-Submission Checklist

- [ ] Run `clang-format` on all modified C files
- [ ] Verify builds on both HACKRF_ONE and PRALINE targets
- [ ] Verify `hackrf_debug -k` on HackRF One r8 (not just r9)
- [ ] Check API version bump necessity for `hackrf_cpld_checksum`
- [ ] Verify no unrelated files in commits (docs should be separate)
- [ ] Confirm Doxygen comments for new public API

## Known Concerns for Upstream Review

1. **CPLD checksum API version** — Currently requires `0x0103`. If upstream already reports `0x0103+` without the handler, users get STALL instead of clean error. Consider bumping to `0x010A`.

2. **FPGA guard race** — `radio_reg_read(&radio, RADIO_BANK_APPLIED, RADIO_OPMODE)` is solid, but the maintainers may want both request-mode AND applied-mode checks for defense in depth.

3. **`cpld_xc2c64a_jtag_sram_checksum()` always returns `true`** — Reviewers may ask for meaningful error paths (e.g., JTAG enable failure detection).

## Draft PR Title

```
firmware: safe CPLD SRAM-read checksum and FPGA bitstream guard

- Replace flash-read CPLD checksum with SRAM-read (~3ms, safe cleanup)
- Guard SET_FPGA_BITSTREAM against active RX/TX by checking applied opmode
- Host: increase FPGA bitstream timeout to 1000ms
- Tools: add hackrf_debug -k, fix hackrf_spiflash -C
```

## Draft PR Body

```markdown
## Summary

This PR addresses three long-standing issues:
1. **CPLD checksum crashes** — The existing `cpld_xc2c64a_jtag_checksum()` uses a fragile flash-read path that leaves JTAG in an invalid state, causing pipe errors when accessed via USB. This replaces it with an SRAM-read path that is fast (~3ms) and has safe cleanup.
2. **FPGA bitstream corruption risk** — `SET_FPGA_BITSTREAM` had no guard against active streaming. Reloading the FPGA during RX/TX can corrupt SGPIO state. This adds a check against `RADIO_BANK_APPLIED` opmode.
3. **Host timeout too short** — The 100ms default timeout is shorter than `fpga_image_load()` needs.

## Testing

- [x] HackRF One r9 + PortaPack H2: CPLD checksum returns `0x05829db7`
- [x] HackRF Pro r1.2: FPGA switch succeeds when idle, STALLs during RX
- [x] Both devices: `hackrf_spiflash -C` clears status correctly

## API Changes

- New: `hackrf_cpld_checksum(hackrf_device*, uint32_t*)` — requires API 0x0103
- Modified: `hackrf_set_fpga_bitstream()` — timeout increased to 1000ms
```
