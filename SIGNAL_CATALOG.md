# HackRF Signal Catalog Report

**Date:** 2026-05-29  
**Hardware:** HackRF One r9 (`922c63dc21748847`), HackRF Pro r1.2 (`645061de252d6613`)  
**Location:** Indoor environment  
**Method:** `hackrf_sweep` with 10 sweeps per band (~5–10 seconds each)

---

## Summary of Findings

| Band | Strongest Signal | Assessment | Decode Potential |
|------|-----------------|------------|------------------|
| VHF Airband | −60.5 dBm | Moderate | Good (AM voice) |
| NOAA Satellites | −71.8 dBm | Weak | Poor (outdoor antenna needed) |
| Pager / POCSAG | −59.6 dBm | Moderate | Good (digital decoder) |
| Marine VHF | −48.0 dBm | **Strong** | Excellent (FM voice) |
| Weather Radio | −64.6 dBm | Moderate | Good (FM voice) |
| ISM 900 MHz | −61.3 dBm | Moderate | Fair (unknown modulation) |
| GSM Downlink | −61.6 dBm | Moderate | Fair (encrypted traffic) |
| GPS L1 | −72.5 dBm | Weak / Noise | Very Hard (below noise floor) |
| 2.4 GHz ISM | −39.6 dBm | **Very Strong** | Excellent (WiFi / BT) |

---

## 1. Detected Signals by Band

### VHF Airband (118–136 MHz)

Noise floor: **−72.5 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 119.980 | −60.5 | 🟡 Moderate | ATIS / Ground Control |
| 129.733 | −62.3 | 🟡 Moderate | Aircraft / Approach |
| 127.802 | −62.7 | 🟡 Moderate | Aircraft En-route |
| 131.069 | −62.8 | 🟡 Moderate | Aircraft / Tower |
| 131.416 | −62.9 | 🟡 Moderate | Aircraft / Terminal |

**Notes:** These are typical aircraft voice communication channels, modulated in AM. Signal levels are moderate but consistent, suggesting active nearby airport or approach traffic. 119.980 MHz is close to common ATIS (Automatic Terminal Information Service) frequencies.

---

### NOAA Weather Satellites (137–138 MHz)

Noise floor: **−82.3 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 137.479 | −71.8 | 🟢 Weak | NOAA-15 / 18 / 19 APT (faint) |
| 137.000 | −72.7 | 🟢 Weak | Background / faint pass |
| 137.958 | −73.7 | 🟢 Weak | Background |
| 137.689 | −74.1 | 🟢 Weak | Background |
| 137.220 | −74.9 | 🟢 Weak | Background |

**Notes:** NOAA APT downlinks are on 137.100, 137.3125, 137.500, 137.620, and 137.9125 MHz. The detected signals are only slightly above the noise floor and are likely not valid satellite passes. APT reception requires an outdoor antenna, clear sky view, and precise timing for satellite passes.

---

### Pager / POCSAG (150–160 MHz)

Noise floor: **−72.5 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 159.950 | −59.6 | 🟡 Moderate | Pager / Business Radio |
| 156.040 | −61.3 | 🟡 Moderate | Marine VHF Ch 60 (duplex) |
| 150.545 | −61.9 | 🟡 Moderate | Land Mobile / Business |
| 154.703 | −62.6 | 🟡 Moderate | Land Mobile / Public Safety |
| 153.861 | −62.9 | 🟡 Moderate | Land Mobile / Business |

**Notes:** The 150–160 MHz range in the US is heavily used for land-mobile radio, pagers (POCSAG), and some marine duplex channels. 159.950 MHz is a known pager frequency in some regions. POCSAG decoding is straightforward with tools like `multimon-ng`.

---

### Marine VHF (156–174 MHz)

Noise floor: **−73.9 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 164.168 | −48.0 | 🔴 **Strong** | Land Mobile / Public Safety |
| 162.535 | −54.7 | 🟡 Moderate | NOAA Weather Radio (adjacent) |
| 165.703 | −56.5 | 🟡 Moderate | Land Mobile / Business Radio |
| 159.960 | −61.8 | 🟡 Moderate | Marine VHF Ch 60 / Land Mobile |
| 162.782 | −62.8 | 🟡 Moderate | Land Mobile / Interstitial |

**Notes:** The strongest signal at **164.168 MHz** is actually outside the primary marine band (156–162 MHz) and falls into the VHF high-band land-mobile allocation. This is likely a nearby public safety, taxi, or business repeater. Marine VHF itself uses FM modulation on 25 kHz channels and is easily decoded.

---

### Weather Radio (162.4–162.55 MHz)

Noise floor: **−81.4 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 162.549 | −64.6 | 🟡 Moderate | **NOAA Weather Radio** |

**Notes:** 162.549 MHz is extremely close to **162.550 MHz** (WX1, KIH-54 / local NOAA transmitter). This is a continuous FM broadcast of weather information. Despite the moderate signal level, the continuous nature and wide FM deviation make it easy to receive.

---

### ISM Band — 900 MHz (902–928 MHz)

Noise floor: **−72.5 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 908.683 | −61.3 | 🟡 Moderate | ISM device / LoRa / Z-Wave |
| 919.970 | −61.6 | 🟡 Moderate | Cordless phone / IoT |
| 905.465 | −61.6 | 🟡 Moderate | RFID / Telemetry |
| 922.495 | −62.0 | 🟡 Moderate | ISM data link |
| 906.455 | −62.2 | 🟡 Moderate | Smart meter / Sensor |

**Notes:** The 902–928 MHz band is license-free in the US and used by a wide variety of devices. Common signals include Z-Wave (908.42 MHz), LoRa (various sub-bands), cordless phones, baby monitors, and utility smart meters. Signals are moderate but present across the band. Identifying the exact modulation requires narrow-band analysis.

---

### GSM Downlink (935–960 MHz)

Noise floor: **−72.7 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 940.000 | −61.6 | 🟡 Moderate | GSM / UMTS / LTE downlink |
| 957.525 | −61.7 | 🟡 Moderate | Cellular tower downlink |
| 944.257 | −62.2 | 🟡 Moderate | Cellular tower downlink |
| 958.762 | −62.5 | 🟡 Moderate | Cellular tower downlink |
| 947.475 | −62.6 | 🟡 Moderate | Cellular tower downlink |

**Notes:** The relatively uniform signal level across the 935–960 MHz range indicates multiple cellular carriers operating nearby. These are tower downlink signals (phone-to-tower is 890–915 MHz). GSM can be decoded for system information (cell ID, ARFCN) with tools like `gr-gsm`, but voice and data traffic are encrypted. LTE/NR signals may also be present in this range.

---

### GPS L1 (1570–1580 MHz)

Noise floor: **−84.2 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 1576.737 | −72.5 | 🟢 Weak | Noise / local spur |
| 1572.685 | −73.3 | 🟢 Weak | Noise / local spur |
| 1571.008 | −73.3 | 🟢 Weak | Noise / local spur |
| 1573.513 | −73.8 | 🟢 Weak | Noise / local spur |
| 1579.840 | −74.2 | 🟢 Weak | Noise / local spur |

**Notes:** GPS L1 is centered at **1575.42 MHz**. Real GPS signals arrive at approximately **−130 dBm** at the antenna — far below the HackRF noise floor. Any readings here are either the receiver noise floor, local interference, or harmonic spurs. Decoding GPS requires correlation gain (GPS-SDR-SIM, gnss-sdr) and an outdoor antenna with clear sky view.

---

### 2.4 GHz ISM (2400–2500 MHz)

Noise floor: **−69.5 dBm**

| Frequency (MHz) | Strength (dBm) | Assessment | Likely Service |
|-----------------|----------------|------------|----------------|
| 2409.608 | −39.6 | 🔴 **Very Strong** | **WiFi Channel 1** |
| 2409.314 | −40.1 | 🔴 Strong | WiFi Channel 1 |
| 2408.235 | −40.9 | 🔴 Strong | WiFi Channel 1 |
| 2408.627 | −41.8 | 🔴 Strong | WiFi Channel 1 |
| 2407.255 | −43.5 | 🔴 Strong | WiFi Channel 1 |

**Notes:** The 2.4 GHz ISM band shows by far the strongest signals in the survey. The energy cluster around **2407–2410 MHz** corresponds to **WiFi Channel 1** (center frequency 2412 MHz, 22 MHz wide OFDM). This is expected in any indoor environment. Bluetooth LE and classic Bluetooth also operate in this band but use frequency-hopping spread spectrum and appear as brief spikes.

---

## 2. Recommendations for Easiest Signals to Decode

### 🥇 Tier 1 — Immediate Success

| Signal | Tool | Why It’s Easy |
|--------|------|---------------|
| **2.4 GHz WiFi** | Wireshark + monitor-mode adapter, or `gr-ieee802-11` | Extremely strong (−39 dBm), continuous, well-documented protocol |
| **Marine VHF / 164 MHz Land Mobile** | `gqrx`, `sdr++`, any NFM demodulator | Very strong (−48 dBm), FM voice, no decryption |
| **NOAA Weather Radio (162.55 MHz)** | `gqrx`, `rtl_fm` | Continuous FM broadcast, known content, moderate signal |

### 🥈 Tier 2 — Good with Correct Tools

| Signal | Tool | Notes |
|--------|------|-------|
| **VHF Airband (AM voice)** | `gqrx` (AM mode), `dumpvdl2` for VDL | AM demodulation is straightforward; data modes need specific tools |
| **Pager / POCSAG** | `multimon-ng` | Strong signals present; 512/1200/2400 bps decodeable |
| **GSM downlink** | `gr-gsm`, `airprobe` | Can decode BCCH/SI for cell info; voice is encrypted |

### 🥉 Tier 3 — Challenging / Needs Setup

| Signal | Tool | Notes |
|--------|------|-------|
| **ISM 900 MHz devices** | `urh` (Universal Radio Hacker), `inspectrum` | Need to identify modulation first; many proprietary protocols |
| **NOAA APT satellites** | `noaa-apt`, `wxtoimg` | Need outdoor antenna, satellite pass prediction, narrow FM |
| **GPS L1** | `gnss-sdr`, GPS-SDR-SIM | Signal is below noise floor; requires correlation processing and outdoor antenna |

---

## 3. Antenna Requirements

| Band | Recommended Antenna | Current Setup Impact |
|------|---------------------|----------------------|
| 118–174 MHz (VHF) | Discone or 1/4-wave vertical (40–50 cm) | Indoor location limits range; signals are still detectable but weak |
| 900–960 MHz | 900 MHz whip or Yagi | Moderate signals suggest some energy present; better antenna would improve SNR |
| 1575 MHz (GPS) | Active GPS patch antenna (+LNA) | Passive/indoor antennas will not work; GPS is below noise floor |
| 2.4 GHz | 2.4 GHz dipole or WiFi antenna | Strongest band; even a short wire works, but directional antenna reduces noise |

### General Notes
- **Indoor vs. Outdoor:** All sweeps were performed indoors. Moving outside with the same antenna would likely improve VHF/UHF signals by 10–20 dB.
- **Amplification:** An external LNA (low-noise amplifier) such as the HackRF's internal amp (`-a 1`) or an external SPF5189Z-based LNA would help weak signals, but may overload on strong nearby signals (e.g., 2.4 GHz WiFi).
- **Filtering:** A band-pass filter for the band of interest reduces intermodulation from strong out-of-band signals (especially WiFi and GSM).

---

## 4. Notable Findings

1. **Strongest overall signal:** 2.4 GHz WiFi at **−39.6 dBm** — dominates the local RF environment.
2. **Best VHF signal:** Land mobile / public safety at **164.168 MHz** at **−48.0 dBm** — easily receivable with any FM decoder.
3. **NOAA Weather Radio detected:** Weak but present at **162.549 MHz** — likely local WX1 transmitter.
4. **GPS not detectable in sweep mode:** Expected, as GPS signals are ~60 dB below the noise floor of a wideband FFT sweep.
5. **GSM cellular towers active:** Downlink signals present across 935–960 MHz, indicating multiple nearby base stations.
6. **ISM 900 MHz is active:** Several moderate signals suggest smart home devices, sensors, or metering equipment nearby.
7. **Marine VHF (156–162 MHz) signals are weak:** The strongest signals in the 156–174 MHz scan were actually land-mobile above 162 MHz, not marine traffic.

---

## 5. Suggested Next Steps

1. **Start with NOAA Weather Radio:** Tune to **162.550 MHz** (or **162.400–162.550 MHz** depending on your region) in narrow FM mode for instant gratification.
2. **Listen to 164 MHz land mobile:** Use **NFM** mode; you will likely hear public safety, transit, or business radio traffic.
3. **Decode POCSAG pagers:** If still in use in your area, `multimon-ng` on **159.950 MHz** may yield text messages.
4. **Analyze WiFi:** Use a standard WiFi adapter in monitor mode to capture 802.11 frames rather than SDR — much easier than decoding OFDM in GNU Radio.
5. **Plan an outdoor session:** For NOAA APT satellites and GPS, schedule an outdoor scan with appropriate antennas.

---

*Report generated by `hackrf_sweep` spectrum analysis pipeline.*
