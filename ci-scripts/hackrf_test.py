#!/usr/bin/env python3

import sys
import subprocess
import time
import os, shutil
import usb
from datetime import datetime
from dataclasses import dataclass
import argparse
import numpy as np
import traceback
import re

DFU_UTIL = "/usr/bin/dfu-util"
TMP_DIR = "/tmp/"
WAVEFORM = TMP_DIR + "waveform100"
NOISE = TMP_DIR + "noise"
VENDOR_ID = 0x1d50
PRODUCT_ID = 0x6089
DFU_VENDOR_ID = 0x1fc9
DFU_PRODUCT_ID = 0x000c
ONE_MHZ = 1000000
TIMEOUT = 5
MAX_PPM = 40
MAX_BASELINE = -20
SIGNAL_THRESHOLD = -25
MAX_DC_DBC = -33
MAX_M_DBC = -27
MAX_MH2_DBC = -40
MAX_H2_DBC = -39
MAX_OTHER_DBC = -34
LP2_CORRECTION = 10
MAX_POWER = -10
IF_REQUIRED_MIN = 2300
IF_REQUIRED_MAX = 2700
IF_TEST_RANGE = 256
SLEEP_TIME = 0.05 # seconds to wait between start of TX and start of RX

@dataclass
class TestCase():
    name: str
    freq: int
    amp: bool
    direction: str
    base_ecode: int
    rx_lna_gain: int
    rx_vga_gain: int
    tx_gain: int

# Try to keep rx_lna_gain and rx_vga_gain between 8 and 32.
bp1txamp = TestCase(name="BP1-TX-amp", freq=2665, amp=True, direction="tx",
        base_ecode=320, rx_lna_gain=16, rx_vga_gain=16, tx_gain=14)
lp1txamp = TestCase(name="LP1-TX-amp", freq=915, amp=True, direction="tx",
        base_ecode=420, rx_lna_gain=16, rx_vga_gain=16, tx_gain=27)
lp2txamp = TestCase(name="LP2-TX-amp", freq=2, amp=True, direction="tx",
        base_ecode=2420, rx_lna_gain=8, rx_vga_gain=16, tx_gain=30)
hp1txamp = TestCase(name="HP1-TX-amp", freq=5995, amp=True, direction="tx",
        base_ecode=520, rx_lna_gain=32, rx_vga_gain=28, tx_gain=37)

bp1txnoamp = TestCase(name="BP1-TX-noamp", freq=2665, amp=False, direction="tx",
        base_ecode=350, rx_lna_gain=16, rx_vga_gain=16, tx_gain=23)
lp1txnoamp = TestCase(name="LP1-TX-noamp", freq=915, amp=False, direction="tx",
        base_ecode=450, rx_lna_gain=16, rx_vga_gain=16, tx_gain=39)
lp2txnoamp = TestCase(name="LP2-TX-noamp", freq=2, amp=False, direction="tx",
        base_ecode=2450, rx_lna_gain=8, rx_vga_gain=16, tx_gain=47)
hp1txnoamp = TestCase(name="HP1-TX-noamp", freq=5995, amp=False, direction="tx",
        base_ecode=550, rx_lna_gain=32, rx_vga_gain=32, tx_gain=44)

# Much of the information in the RX test cases is a duplication of information
# in the TX test cases, but it's easier to keep legacy error codes this way.
bp1rxamp = TestCase(name="BP1-RX-amp", freq=2665, amp=True, direction="rx",
        base_ecode=620, rx_lna_gain=16, rx_vga_gain=16, tx_gain=14)
lp1rxamp = TestCase(name="LP1-RX-amp", freq=915, amp=True, direction="rx",
        base_ecode=720, rx_lna_gain=16, rx_vga_gain=16, tx_gain=29)
lp2rxamp = TestCase(name="LP2-RX-amp", freq=2, amp=True, direction="rx",
        base_ecode=2720, rx_lna_gain=8, rx_vga_gain=16, tx_gain=33)
hp1rxamp = TestCase(name="HP1-RX-amp", freq=5995, amp=True, direction="rx",
        base_ecode=820, rx_lna_gain=32, rx_vga_gain=28, tx_gain=39)

bp1rxnoamp = TestCase(name="BP1-RX-noamp", freq=2665, amp=False, direction="rx",
        base_ecode=650, rx_lna_gain=16, rx_vga_gain=16, tx_gain=23)
lp1rxnoamp = TestCase(name="LP1-RX-noamp", freq=915, amp=False, direction="rx",
        base_ecode=750, rx_lna_gain=16, rx_vga_gain=16, tx_gain=39)
lp2rxnoamp = TestCase(name="LP2-RX-noamp", freq=2, amp=False, direction="rx",
        base_ecode=2750, rx_lna_gain=8, rx_vga_gain=16, tx_gain=47)
hp1rxnoamp = TestCase(name="HP1-RX-noamp", freq=5995, amp=False, direction="rx",
        base_ecode=850, rx_lna_gain=32, rx_vga_gain=32, tx_gain=44)

ifmin = TestCase(name="IF-min", freq=IF_REQUIRED_MIN, amp=None, direction=None,
        base_ecode=None, rx_lna_gain=16, rx_vga_gain=16, tx_gain=33)
ifmax = TestCase(name="IF-max", freq=IF_REQUIRED_MAX, amp=None, direction=None,
        base_ecode=None, rx_lna_gain=16, rx_vga_gain=16, tx_gain=33)

ppm = TestCase(name="PPM", freq=2707, amp=False, direction=None,
        base_ecode=None, rx_lna_gain=16, rx_vga_gain=16, tx_gain=26)

noise = TestCase(name="Noise", freq=6000, amp=True, direction=None,
        base_ecode=None, rx_lna_gain=40, rx_vga_gain=52, tx_gain=None)

all_rf_test_cases = (
        bp1txamp, lp1txamp, lp2txamp, hp1txamp,
        bp1txnoamp, lp1txnoamp, lp2txnoamp, hp1txnoamp,
        bp1rxamp, lp1rxamp, lp2rxamp, hp1rxamp,
        bp1rxnoamp, lp1rxnoamp, lp2rxnoamp, hp1rxnoamp)

r9_tester_cal = [1.1, 0.4, 0.4, 1.1, 0.8, 0.2, 0.6, 1.6, -0.4, -1.2, -1.1, 1.1, -1.4, -0.9, 0.1, 1.8]
r9_eut_cal = [-1.1, -1.6, -1.4, -1.3, -1.3, -0.9, 0.1, 1.8, 0.8, 1.1, 0.9, -0.2, 0.9, 0.2, 0.6, 1.5]
og_tester_cal = [-0.4, -0.3, -0.2, -0.4, -0.8, -0.5, -0., -0.1, 1.4, 2., 1.6, -0.1, 1.5, 0.7, 0.5, -0.3]
og_eut_cal = [1.8, 1.8, 1.6, 2., 1.3, 0.7, 0.5, -0.3, 0.2, -0.3, -0.5, 1.2, -0.8, -0.5, -0., -0.1]

emessages = {
    # TODO: replace the placeholder code according to each fail instance
    1: "Placeholder error code",
    60: "Unable to program firmware via DFU",
    65: "EUT not detected after DFU programming",
    70: "Unable to program SPI flash",
    75: "EUT not detected after flashing",
    80: "Could not find EUT",
    81: "Could not find TESTER",
    82: "Found multiple candidate EUTs",
    83: "Found multiple candidate TESTERs",
    84: "EUT running from RAM instead of flash",
    85: "TESTER running from RAM instead of flash",
    86: "Couldn't parse hackrf_info output",
    88: "Incorrect EUT hardware revision",
    89: "Incorrect TESTER hardware revision",
    90: "EUT has incorrect manufacturer pin strap",
    91: "TESTER has incorrect manufacturer pin strap",
    99: "Incorrect device name",
    100: "EUT couldn't verify clkgen register",
    101: "TESTER couldn't verify clkgen register",
    120: "EUT couldn't verify xcvr register",
    121: "TESTER couldn't verify xcvr register",
    140: "EUT couldn't verify mixer register",
    141: "TESTER couldn't verify mixer register",
    150: "Failed RX for IF verification",
    152: "Couldn't parse received power",
    154: "Received power out of bounds",
    160: "Insufficient IF range",
    170: "RX operation failed",
    172: "Couldn't parse hackrf_sweep output",
    180: "Subprocess timed out",
    190: "Failed to configure LEDs",
    200: "hackrf_clock command failed",
    210: "TESTER failed to detect EUT CLKOUT",
    211: "EUT failed to detect TESTER CLKOUT",
    212: "TESTER detected spurious CLKIN",
    213: "EUT detected spurious CLKIN",
    220: "Failed TESTER CLKIN activation RX",
    221: "Failed EUT CLKIN activation RX",
    230: "Failed TESTER PLL check",
    231: "Failed EUT PLL check",
    232: "TESTER PLL failed to lock",
    233: "EUT PLL failed to lock",
    240: "Clock synchronization failure",
    250: "Failed PPM detection sweep",
    252: "Couldn't parse PPM test sweep data",
    254: "TESTER couldn't detect PPM test transmission",
    255: "EUT couldn't detect PPM test transmission",
    260: "Frequency error exceeds limit",
    300: "Failed baseline RX",
    302: "Couldn't parse baseline RX power",
    306: "Baseline RX power too high",
    310: "Failed noise RX",
    311: "Couldn't parse noise RX power",
    312: "Noise RX power out of range",
    313: "Required noise gain out of range",
    315: "Couldn't read noise RX file",
    316: "Too little variation in ADC bit",
    317: "ADC bit correlates strongly with another bit",
	321: "Excessive TX DC offset in AMP txbp1",
	322: "insufficient signal strength in AMP txbp1",
	323: "AMP txbp1 clipping",
	325: "peak found at unexpected frequency in AMP txbp1",
	330: "harmonics detected in AMP txbp1",
	335: "neighbor magnitude exceeded in AMP txbp1",
	340: "neighbor magnitude exceeded in AMP txbp1",
	345: "spectrum mirrored or inverted in AMP txbp1",
	350: "excessive power out of channel in AMP txbp1",
	351: "Excessive TX DC offset in NOAMP txbp1",
	352: "insufficient signal strength in NOAMP txbp1",
	353: "NOAMP txbp1 clipping",
	355: "peak found at unexpected frequency in NOAMP txbp1",
	360: "harmonics detected in NOAMP txbp1",
	365: "neighbor magnitude exceeded in NOAMP txbp1",
	370: "neighbor magnitude exceeded in NOAMP txbp1",
	375: "spectrum mirrored or inverted in NOAMP txbp1",
	380: "excessive power out of channel in NOAMP txbp1",
	421: "Excessive TX DC offset in AMP txlp1",
	422: "insufficient signal strength in AMP txlp1",
	423: "AMP txlp1 clipping",
	425: "peak found at unexpected frequency in AMP txlp1",
	430: "harmonics detected in AMP txlp1",
	435: "neighbor magnitude exceeded in AMP txlp1",
	440: "neighbor magnitude exceeded in AMP txlp1",
	445: "spectrum mirrored or inverted in AMP txlp1",
	450: "excessive power out of channel in AMP txlp1",
	451: "Excessive TX DC offset in NOAMP txlp1",
	452: "insufficient signal strength in NOAMP txlp1",
	453: "NOAMP txlp1 clipping",
	455: "peak found at unexpected frequency in NOAMP txlp1",
	460: "harmonics detected in NOAMP txlp1",
	465: "neighbor magnitude exceeded in NOAMP txlp1",
	470: "neighbor magnitude exceeded in NOAMP txlp1",
	475: "spectrum mirrored or inverted in NOAMP txlp1",
	480: "excessive power out of channel in NOAMP txlp1",
	2421: "Excessive TX DC offset in AMP txlp2",
	2422: "insufficient signal strength in AMP txlp2",
	2423: "AMP txlp2 clipping",
	2425: "peak found at unexpected frequency in AMP txlp2",
	2430: "harmonics detected in AMP txlp2",
	2435: "neighbor magnitude exceeded in AMP txlp2",
	2440: "neighbor magnitude exceeded in AMP txlp2",
	2445: "spectrum mirrored or inverted in AMP txlp2",
	2450: "excessive power out of channel in AMP txlp2",
	2451: "Excessive TX DC offset in NOAMP txlp2",
	2452: "insufficient signal strength in NOAMP txlp2",
	2453: "NOAMP txlp2 clipping",
	2455: "peak found at unexpected frequency in NOAMP txlp2",
	2460: "harmonics detected in NOAMP txlp2",
	2465: "neighbor magnitude exceeded in NOAMP txlp2",
	2470: "neighbor magnitude exceeded in NOAMP txlp2",
	2475: "spectrum mirrored or inverted in NOAMP txlp2",
	2480: "excessive power out of channel in NOAMP txlp2",
	521: "Excessive TX DC offset in AMP txhp1",
	522: "insufficient signal strength in AMP txhp1",
	523: "AMP txhp1 clipping",
	525: "peak found at unexpected frequency in AMP txhp1",
	530: "harmonics detected in AMP txhp1",
	535: "neighbor magnitude exceeded in AMP txhp1",
	540: "neighbor magnitude exceeded in AMP txhp1",
	545: "spectrum mirrored or inverted in AMP txhp1",
	550: "excessive power out of channel in AMP txhp1",
	551: "Excessive TX DC offset in NOAMP txhp1",
	552: "insufficient signal strength in NOAMP txhp1",
	553: "NOAMP txhp1 clipping",
	555: "peak found at unexpected frequency in NOAMP txhp1",
	560: "harmonics detected in NOAMP txhp1",
	565: "neighbor magnitude exceeded in NOAMP txhp1",
	570: "neighbor magnitude exceeded in NOAMP txhp1",
	575: "spectrum mirrored or inverted in NOAMP txhp1",
	580: "excessive power out of channel in NOAMP txhp1",
	621: "Excessive TX DC offset in AMP rxbp1",
	622: "insufficient signal strength in AMP rxbp1",
	623: "AMP rxbp1 clipping",
	625: "peak found at unexpected frequency in AMP rxbp1",
	630: "harmonics detected in AMP rxbp1",
	635: "neighbor magnitude exceeded in AMP rxbp1",
	640: "neighbor magnitude exceeded in AMP rxbp1",
	645: "spectrum mirrored or inverted in AMP rxbp1",
	650: "excessive power out of channel in AMP rxbp1",
	651: "Excessive TX DC offset in NOAMP rxbp1",
	652: "insufficient signal strength in NOAMP rxbp1",
	653: "NOAMP rxbp1 clipping",
	655: "peak found at unexpected frequency in NOAMP rxbp1",
	660: "harmonics detected in NOAMP rxbp1",
	665: "neighbor magnitude exceeded in NOAMP rxbp1",
	670: "neighbor magnitude exceeded in NOAMP rxbp1",
	675: "spectrum mirrored or inverted in NOAMP rxbp1",
	680: "excessive power out of channel in NOAMP rxbp1",
	721: "Excessive TX DC offset in AMP rxlp1",
	722: "insufficient signal strength in AMP rxlp1",
	723: "AMP rxlp1 clipping",
	725: "peak found at unexpected frequency in AMP rxlp1",
	730: "harmonics detected in AMP rxlp1",
	735: "neighbor magnitude exceeded in AMP rxlp1",
	740: "neighbor magnitude exceeded in AMP rxlp1",
	745: "spectrum mirrored or inverted in AMP rxlp1",
	750: "excessive power out of channel in AMP rxlp1",
	751: "Excessive TX DC offset in NOAMP rxlp1",
	752: "insufficient signal strength in NOAMP rxlp1",
	753: "NOAMP rxlp1 clipping",
	755: "peak found at unexpected frequency in NOAMP rxlp1",
	760: "harmonics detected in NOAMP rxlp1",
	765: "neighbor magnitude exceeded in NOAMP rxlp1",
	770: "neighbor magnitude exceeded in NOAMP rxlp1",
	775: "spectrum mirrored or inverted in NOAMP rxlp1",
	780: "excessive power out of channel in NOAMP rxlp1",
	2721: "Excessive TX DC offset in AMP rxlp2",
	2722: "insufficient signal strength in AMP rxlp2",
	2723: "AMP rxlp2 clipping",
	2725: "peak found at unexpected frequency in AMP rxlp2",
	2730: "harmonics detected in AMP rxlp2",
	2735: "neighbor magnitude exceeded in AMP rxlp2",
	2740: "neighbor magnitude exceeded in AMP rxlp2",
	2745: "spectrum mirrored or inverted in AMP rxlp2",
	2750: "excessive power out of channel in AMP rxlp2",
	2751: "Excessive TX DC offset in NOAMP rxlp2",
	2752: "insufficient signal strength in NOAMP rxlp2",
	2753: "NOAMP rxlp2 clipping",
	2755: "peak found at unexpected frequency in NOAMP rxlp2",
	2760: "harmonics detected in NOAMP rxlp2",
	2765: "neighbor magnitude exceeded in NOAMP rxlp2",
	2770: "neighbor magnitude exceeded in NOAMP rxlp2",
	2775: "spectrum mirrored or inverted in NOAMP rxlp2",
	2780: "excessive power out of channel in NOAMP rxlp2",
	821: "Excessive TX DC offset in AMP rxhp1",
	822: "insufficient signal strength in AMP rxhp1",
	823: "AMP rxhp1 clipping",
	825: "peak found at unexpected frequency in AMP rxhp1",
	830: "harmonics detected in AMP rxhp1",
	835: "neighbor magnitude exceeded in AMP rxhp1",
	840: "neighbor magnitude exceeded in AMP rxhp1",
	845: "spectrum mirrored or inverted in AMP rxhp1",
	850: "excessive power out of channel in AMP rxhp1",
	851: "Excessive TX DC offset in NOAMP rxhp1",
	852: "insufficient signal strength in NOAMP rxhp1",
	853: "NOAMP rxhp1 clipping",
	855: "peak found at unexpected frequency in NOAMP rxhp1",
	860: "harmonics detected in NOAMP rxhp1",
	865: "neighbor magnitude exceeded in NOAMP rxhp1",
	870: "neighbor magnitude exceeded in NOAMP rxhp1",
	875: "spectrum mirrored or inverted in NOAMP rxhp1",
	880: "excessive power out of channel in NOAMP rxhp1",
	3000: "BP TX failure",
	3010: "BP RX failure",
	3020: "HP TX failure",
	3030: "HP RX failure",
	3040: "LP TX failure",
	3050: "LP RX failure",
	3060: "TX AMP failure",
	3070: "RX AMP failure",
	3080: "AMP bypass failure",
	3090: "LPF failure",
	3100: "HPF failure",
	3110: "mixer bypass failure",
	3120: "mixer failure",
	3130: "TX failure",
	3140: "RX failure",
	3150: "mixer TX failure",
	3160: "mixer RX failure",
	3170: "LP2 TX failure",
	3180: "LP2 RX failure",
	3190: "LP2 TX/RX failure",
	3200: "total RF failure",
	3210: "multiple RF failures, see list above",
	3220: "multiple clipping failures"
}

def fail_rf(errors):
    # check for combinations of RF errors
    ecode = 0
    eset = set(errors)

    # error affects only bp tx
    if ((322 in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset)):
        ecode = 3000
        errors.append(ecode)

    # error affects only bp rx
    if ((322 not in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset)):
        ecode = 3010
        errors.append(ecode)

    # error affects only hp tx
    if ((322 not in eset) and (422 not in eset) and (2422 not in eset) and (522 in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset)):
        ecode = 3020
        errors.append(ecode)

    # error affects only hp rx
    if ((322 not in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset) and (822 in eset)):
        ecode = 3030
        errors.append(ecode)

    # error affects only lp tx (both lp1 and lp2)
    if ((322 not in eset) and (422 in eset) and (2422 in eset) and (522 not in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset)):
        ecode = 3040
        errors.append(ecode)

    # error affects only lp rx (both lp1 and lp2)
    if ((322 not in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 not in eset) and (722 in eset) and (2722 in eset) and (822 not in eset)):
        ecode = 3050
        errors.append(ecode)

    # error in tx amp path
    if ((2422 in eset) and (352 not in eset) and (452 not in eset) and (2452 not in eset)):
        ecode = 3060
        errors.append(ecode)

    # error in rx amp path
    if ((2722 in eset) and (652 not in eset) and (752 not in eset) and (2752 not in eset)):
        ecode = 3070
        errors.append(ecode)

    # error in amp bypass path
    if ((352 in eset) and (452 in eset) and (2452 in eset) and (652 in eset) and (752 in eset) and (2752 in eset) and (322 not in eset) and (422 not in eset) and (2422 not in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset)):
        ecode = 3080
        errors.append(ecode)

    # error in lpf path
    if (((422 in eset) and (2422 in eset)) and ((722 in eset) and (2722 in eset)) and ((322 not in eset) and (622 not in eset)) and ((522 not in eset) and (822 not in eset))):
        ecode = 3090
        errors.append(ecode)

    # error in hpf path
    if (((522 in eset) and (822 in eset)) and ((322 not in eset) and (622 not in eset)) and ((422 not in eset) and (2422 not in eset) and (722 not in eset) and (2722 not in eset))):
        ecode = 3100
        errors.append(ecode)

    # error affects both mixer bypass paths
    if ((322 in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset)):
        ecode = 3110
        errors.append(ecode)

    # error in mixer tx/rx
    if ((322 not in eset) and ((422 in eset) or (2422 in eset)) and (522 in eset) and (622 not in eset) and ((722 in eset) or (2722 in eset) or (822 in eset))):
        ecode = 3120
        errors.append(ecode)

    if ((322 not in eset) and ((422 in eset) or (2422 in eset) or (522 in eset)) and (622 not in eset) and ((722 in eset) and (2722 in eset) and (822 in eset))):
        ecode = 3120
        errors.append(ecode)

    # error in all tx, no rx
    if ((322 in eset) and (422 in eset) and (2422 in eset) and (522 in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset) and (352 in eset) and (452 in eset) and (2452 in eset) and (552 in eset)):
        ecode = 3130
        errors.append(ecode)

    # error in all rx, no tx
    if ((322 not in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 in eset) and (722 in eset) and (2722 in eset) and (822 in eset) and (652 in eset) and (752 in eset) and (2752 in eset) and (852 in eset)):
        ecode = 3140
        errors.append(ecode)

    # error in all mixer tx
    if ((322 not in eset) and (422 in eset) and (2422 in eset) and (522 in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset)):
        ecode = 3150
        errors.append(ecode)

    # error in all mixer rx
    if ((322 not in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 not in eset) and (722 in eset) and (2722 in eset) and (822 in eset)):
        ecode = 3160
        errors.append(ecode)

    # error affecting only lp2 tx
    if ((322 not in eset) and (422 not in eset) and (2422 in eset) and (522 not in eset) and (622 not in eset) and (722 not in eset) and (2722 not in eset) and (822 not in eset) and (2752 in eset)):
        ecode = 3170
        errors.append(ecode)

    # error affecting only lp2 rx
    if ((322 not in eset) and (422 not in eset) and (2422 not in eset) and (522 not in eset) and (622 not in eset) and (722 not in eset) and (2722 in eset) and (822 not in eset) and (2752 in eset)):
        ecode = 3180
        errors.append(ecode)

    # error affecting only lp2 rx/tx
    if ((322 not in eset) and (422 not in eset) and (2422 in eset) and (522 not in eset) and (622 not in eset) and (722 not in eset) and (2722 in eset) and (822 not in eset)):
        ecode = 3190
        errors.append(ecode)

    # error in all rx/tx
    if ((322 in eset) and (422 in eset) and (2422 in eset) and (522 in eset) and (622 in eset) and (722 in eset) and (2722 in eset) and (822 in eset)):
        ecode = 3200
        errors.append(ecode)

    clipping_count = 0
    for code in (323, 423, 2423, 523, 353, 453, 2453, 553, 623, 723, 2723, 823, 653, 753, 2753, 853):
        if code in eset:
            clipping_count += 1
    if clipping_count > 2:
        ecode = 3220
        errors.append(ecode)
    elif (not ecode) and (len(errors) > 1):
        # multiple failures without a logical grouping
        ecode = 3210
        errors.append(ecode)

    for error in errors[:-1]:
        out(f"RF failure {error}: {emessages[error]}")
    fail(errors[-1])

def log(message=""):
    if args.log:
        with open(args.log, "a") as log_file:
            print(message, file=log_file)
    return

def out(message=""):
    log(message)
    print(message)

def fail(error_code):
    out(f"FAIL {error_code}: {emessages[error_code]}")
    sys.exit(2)

class HackRF:
    def __init__(self, name, bin_dir, serial, mf_check, revision=None, unique_bin=False):
        self.name = name
        self.bin_dir = bin_dir
        self.unique_bin = unique_bin
        self.revision = None
        self.serial = None
        self.clkout_connected = False
        self.tester_cal = [0] * 16
        self.eut_cal = [0] * 16
        self.errors = []
        self.unit_number = 0
        self.partner = None

        if self.name == "TESTER":
            self.unit_number = 1
        elif self.name != "EUT":
            fail(99)

        info = subprocess.run([bin_dir + "hackrf_info"], capture_output=True,
                encoding="utf-8", timeout=TIMEOUT)
        log(info.stdout + info.stderr)
        if info.returncode != 0 or "failed" in info.stderr:
            fail(80 + self.unit_number)

        # parse the hackrf_info output
        count = info.stdout.count("Found HackRF")
        if count < 1:
            fail(80 + self.unit_number)
        if count > 1 and self.name == "TESTER" and self.unique_bin:
            fail(83)
        if mf_check and "does not appear to have been manufactured by Great Scott Gadgets" in info.stdout:
            fail(90 + self.unit_number)
        try:
            devices_list = re.findall(r'Found HackRF\n.+\n.+\n.+\n.+\n.+\n.+', info.stdout)
            for device in devices_list:
                sn = re.search(r'Serial number: \w+', device).group().split(': ')[1]
                rev = re.search(r'Hardware Revision: .+', device).group().split(': ')[1]
                if sn == serial:
                    self.serial = sn
                    self.revision = rev
                    break
            out(f"{self.name} serial number: {self.serial}")
            out(f"{self.name} revision: {self.revision}")
            out(f"{self.name} binary directory {self.bin_dir}")
        except:
            log(traceback.format_exc())
            fail(1)

        if revision and self.revision != revision:
            fail(88 + self.unit_number)

        if self.revision == "r9":
            self.tester_cal = r9_tester_cal
            self.eut_cal = r9_eut_cal
        else:
            self.tester_cal = og_tester_cal
            self.eut_cal = og_eut_cal

    def clkin(self):
        command = subprocess.run([self.bin_dir + "hackrf_clock", "-i", "-d",
            self.serial], capture_output=True, encoding="utf-8",
            timeout=TIMEOUT)
        if command.returncode != 0:
            log(command.stdout + command.stderr)
            out(f"{self.name} CLKIN request failed")
            fail(200)
        if "no clock signal detected" in command.stdout:
            log(f"{self.name}: no clock signal detected")
            return False
        elif "clock signal detected" in command.stdout:
            log(f"{self.name}: clock signal detected")
            return True
        else:
            log(command.stdout + command.stderr)
            out(f"Couldn't parse {self.name} CLKIN status")
            fail(200)

    # Warning: Avoid clock loops by disabling your partner's CLKOUT before
    # enabling your own.
    def clkout(self, enable):
        command = subprocess.run([self.bin_dir + "hackrf_clock", "-o",
            "1" if enable else "0", "-d", self.serial], capture_output=True,
            encoding="utf-8", timeout=TIMEOUT)
        if command.returncode != 0:
            log(command.stdout + command.stderr)
            out(f"{self.name} CLKOUT control failed")
            fail(200)

        if self.partner and self.clkout_connected:
            timeout = time.time() + 1
            while time.time() < timeout:
                if self.partner.clkin() == enable:
                    break
            if self.partner.clkin() != enable:
                if enable:
                    fail(210 + self.unit_number)
                else:
                    if self.name == "EUT":
                        out("Note: use --tcxo to use TCXO or external reference on TESTER")
                    fail(212 + self.unit_number)

            # transceiver operation to switch clock source
            source = "CLKIN" if enable else "crystal"
            log(f"switching {self.partner.name} to {source}")
            receive = subprocess.run([self.partner.bin_dir + "hackrf_transfer", "-d",
                self.partner.serial, "-n1", "-r", "/dev/null", "-f", "2500000000",
                "-a", "0", "-l", "0", "-g", "0"], capture_output=True,
                encoding="utf-8", timeout=TIMEOUT)
            if receive.returncode != 0:
                log(receive.stdout + receive.stderr)
                fail(220 + self.unit_number)

            # confirm that PLL locked to new source
            expected_value = "0x01" if (enable and self.partner.revision != "r9") else "0x51"
            timeout = time.time() + 1
            while time.time() < timeout:
                debug = subprocess.run([self.partner.bin_dir + "hackrf_debug", "-d",
                    self.partner.serial, "-srn0"], capture_output=True,
                    encoding="utf-8", timeout=TIMEOUT)
                log(debug.stdout)
                if debug.returncode != 0:
                    log(debug.stderr)
                    fail(230 + self.unit_number)
                if expected_value in debug.stdout:
                    break
            if expected_value not in debug.stdout:
                fail(232 + self.unit_number)
            time.sleep(0.1)
        else:
            time.sleep(0.2)

    def test_clkout(self):
        assert self.clkout_connected
        self.partner.clkout(False)
        self.clkout(True)
        frequency_error_ppm = self.clock_error_ppm()
        log(f"{self.name}->{self.partner.name} CLK: {frequency_error_ppm} ppm")
        if abs(frequency_error_ppm) > 1:
            out(f"{self.partner.name} CLKIN: unexpected frequency error while synchronized")
            fail(240)

    def test_clkin(self):
        self.partner.test_clkout()

    def clock_error_ppm(self):
        # ppm.freq is in MHz, but we use it as Hz for the FFT bin width,
        # resulting in bins with a width of 1 ppm of the transmission
        # frequency.
        bin_width = str(ppm.freq)
        tx_frequency = str(ppm.freq * ONE_MHZ)
        sweep_range = f"{ppm.freq-1}:{ppm.freq+19}"
        expected_bin = round(ONE_MHZ/ppm.freq)

        log(f"{self.name} transmitting to {self.partner.name} for PPM measurement")
        transmit = subprocess.Popen([self.bin_dir + "hackrf_transfer", "-d",
            self.serial, "-c", "127", "-a", "0", "-x", f"{ppm.tx_gain}", "-f",
            tx_frequency], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            encoding="utf-8")

        time.sleep(SLEEP_TIME)
        sweep = subprocess.run([self.partner.bin_dir + "hackrf_sweep", "-d",
            self.partner.serial, "-N", "13", "-w", bin_width, "-f", sweep_range, "-a",
            "0", "-l", f"{ppm.rx_lna_gain}", "-g", f"{ppm.rx_vga_gain}"],
            capture_output=True, encoding="utf-8", timeout=TIMEOUT)
        if sweep.returncode != 0:
            log(sweep.stdout + sweep.stderr)
            fail(250)

        # parse the hackrf_sweep output
        try:
            rows = sweep.stdout.split("\n")
            # Note: skipping data from the first twelve sweeps until issue #1230 is resolved.
            row = rows[48].split(", ")
            bins = [float(bin) for bin in row[6:]]
            peak_bin = bins.index(max(bins))
            frequency_error_ppm = peak_bin - expected_bin
        except:
            log(sweep.stdout + sweep.stderr)
            log(traceback.format_exc())
            fail(252)

        log(f"PPM max bin {peak_bin} power: {bins[peak_bin]}")
        for row in rows[0:49:4]:
            bins = [float(bin) for bin in row.split(", ")[6:]]
            peak_bin = bins.index(max(bins))
            log(f"peak bin {peak_bin}: {bins[peak_bin]}")
        if max(bins) < SIGNAL_THRESHOLD:
            fail(254 + self.unit_number)
        transmit.terminate()
        tx_out, tx_err = transmit.communicate(timeout=TIMEOUT)
        if transmit.returncode != 0:
            log(tx_out + tx_err)
            out(f"Warning: {self.name} PPM test transmit failed")
        return frequency_error_ppm

    def test_xtal(self):
        self.clkout(False)
        self.partner.clkout(False)
        frequency_error_ppm = self.clock_error_ppm()
        out(f"{self.name} crystal frequency error: {frequency_error_ppm} ppm")
        if abs(frequency_error_ppm) > MAX_PPM:
            fail(260)

    def verify_register(self, target, address, value):
        command = subprocess.run([self.bin_dir + "hackrf_debug",
            f"--{target}", "--register", address, "--read", "--device",
            self.serial], capture_output=True, encoding="UTF-8")
        log(f"Checking {target} register {address}")
        if value in command.stdout:
            return True
        else:
            log("Unexpected result:")
            log(command.stdout + command.stderr)
            return False

    def test_clkgen(self):
        if not self.verify_register("si5351", "2", "0x03"):
            fail(100 + self.unit_number)

    def test_mixer(self):
        if not self.verify_register("rffc5072", "2", "0x9055"):
            fail(140 + self.unit_number)

    def test_xcvr(self):
        if not self.verify_register("max283", "28", "0x0c0"):
            fail(120 + self.unit_number)

    def test_serial(self):
        log(f"Testing {self.name} serial communications")
        self.test_clkgen()
        self.test_mixer()
        self.test_xcvr()

    # Ensure that hackrf_transfer works and that the received power is within
    # the expected range with no transmission active.
    def baseline_rx(self, test_case):
        # Test at the TX tuning frequency of measure_rf()
        center_freq = (test_case.freq * ONE_MHZ) + 1500000
        receive = subprocess.run([self.bin_dir + "hackrf_transfer", "-d",
            self.serial, "-n10000", "-r", "/dev/null", "-f",
            f"{center_freq}", "-a", "0", "-l", f"{test_case.rx_lna_gain}",
            "-g", f"{test_case.rx_vga_gain}", "-b",
            "1750000"], capture_output=True, encoding="utf-8", timeout=TIMEOUT)
        if receive.returncode != 0:
            log(receive.stdout + receive.stderr)
            log(f"Failed {self.name} {test_case.name} baseline RX")
            fail(300)

        output = str(receive.stderr).split()
        try:
            power = float(output[output.index("power") + 1])
        except:
            log(receive.stdout + receive.stderr)
            log(traceback.format_exc())
            log(f"Couldn't parse {self.name} {test_case.name} baseline RX power")
            fail(302)
        # The theoretical minimum power is about -48 dB. The expected power is
        # about -30 dB.
        log(f"{self.name} {test_case.name} baseline power: {power}")
        if power > MAX_BASELINE:
            fail(306)

    # Check the ADC for shorts or other unexpected behavior by sampling
    # amplified noise.
    def noise_rx(self, test_case):
        num_samples = 1000
        log(f"Testing {self.name} {test_case.name}")
        center_freq = test_case.freq * ONE_MHZ

        low_gain = 0
        high_gain = 16
        while high_gain > low_gain:
            gain = (high_gain + low_gain) // 2
            receive = subprocess.run([self.bin_dir + "hackrf_transfer", "-d",
                self.serial, "-n", f"{num_samples*10}", "-r", f"{NOISE}", "-f",
                f"{center_freq}", "-a", "1" if test_case.amp else "0", "-l",
                f"{test_case.rx_lna_gain}", "-g", f"{gain*2 + 32}", "-b",
                "28000000"], capture_output=True, encoding="utf-8", timeout=TIMEOUT)
            if receive.returncode != 0:
                log(receive.stdout + receive.stderr)
                fail(310)
            output = str(receive.stderr).split()
            try:
                power = float(output[output.index("power") + 1])
            except:
                log(receive.stdout + receive.stderr)
                log(traceback.format_exc())
                fail(311)
            log(f"{self.name} {test_case.name} RX power at -g {gain*2 + 32}: {power}")
            if power > -2:
                high_gain = gain
            elif power > -5:
                break
            else:
                low_gain = gain + 1

        # We want the power to be between -1 and -6 dB to ensure that all eight
        # bits are flipped without much clipping.
        if power < -6 or power > -1:
            fail(312)
        elif gain > 14 or gain < 4:
            fail(313)

        bit_count = [0] * 8
        matches = []
        for bit in range(8):
            matches.append([0] * 8)

        try:
            file = open(f"{NOISE}", "rb")
            data = file.read()
            assert len(data) == num_samples * 20
            data = data[-num_samples*2:]
            for byte in data:
                for bit in range(8):
                    bit_count[bit] += (byte>>bit)&1
                    for match_bit in range(bit + 1, 8):
                        if (byte>>bit)&1 == (byte>>match_bit)&1:
                            matches[bit][match_bit] += 1
        except:
            fail(315)
        log(f"{self.name} ADC bit counts: {bit_count}")
        log(f"{self.name} ADC bit matches: {matches}")
        for bit in range(7):
            if bit_count[bit] < (0.2 * len(data)) or bit_count[bit] > (0.8 * len(data)):
                log(f"{self.name} ADC bit {bit} set in {bit_count[bit]} out of {len(data)} bytes")
                fail(316)
            # Check for correlations between adjacent bits except between 6 and 7 (highest).
            #for match_bit in range(bit + 1, 7):
            for match_bit in range(bit + 1, 8):
                if matches[bit][match_bit] > (0.999 * len(data)):
                    out(f"{self.name} ADC bit {bit} correlates with bit {match_bit}")
                    fail(317)
        # DC bias can cause bit 7 to be predominantly unchanged, so only
        # error if there is less than 1% variation.
        if bit_count[7] < (0.001 * len(data)) or bit_count[bit] > (0.999 * len(data)):
                out(f"{self.name} ADC bit 7 set in {bit_count[7]} out of {len(data)} bytes")
                fail(316)

    def analyze_transfer(self, transfer, test_case):
        # parse the hackrf_transfer output
        output = str(transfer.stderr).split()
        try:
            power = float(output[output.index("power") + 1])
        except:
            log(transfer.stdout + transfer.stderr)
            log(traceback.format_exc())
            fail(152)
        log(f"{test_case.name} power at {test_case.freq} MHz: {power}")

        if test_case.base_ecode:
            if power < SIGNAL_THRESHOLD:
                log(f"{test_case.name} signal not strong enough")
                self.errors.append(test_case.base_ecode + 2)
            if power > -3:
                log(f"{test_case.name} signal clipping")
                self.errors.append(test_case.base_ecode + 3)
        elif power < -48 or power > 0:
            fail(154)
        return power

    # Verify that signal power at a given IF in MHz is detected.
    def verify_if_tuning(self, test_case):
        transmit = subprocess.Popen([self.bin_dir + "hackrf_transfer", "-d",
            self.serial, "-c", "127", "-a", "0", "-x", f"{test_case.tx_gain}",
            "-i", f"{test_case.freq*ONE_MHZ}", "-m", "0", "-F"],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")

        time.sleep(SLEEP_TIME)
        receive = subprocess.run([self.partner.bin_dir + "hackrf_transfer", "-d",
            self.partner.serial, "-n10000", "-r", "/dev/null", "-f",
            f"{(test_case.freq+1)*ONE_MHZ}", "-a", "0", "-l",
            f"{test_case.rx_lna_gain}", "-g", f"{test_case.rx_vga_gain}", "-b",
            "2500000"], capture_output=True, encoding="utf-8", timeout=TIMEOUT)
        transmit.terminate()
        if receive.returncode != 0:
            log(receive.stdout + receive.stderr)
            fail(150)

        power = self.analyze_transfer(receive, test_case)

        tx_out, tx_err = transmit.communicate(timeout=TIMEOUT)
        if transmit.returncode != 0:
            log(tx_out + tx_err)
            out(f"Warning: {self.name} IF transmit failed")
        return power > SIGNAL_THRESHOLD

    def test_if_range(self, min_case, max_case):
        low_end = IF_REQUIRED_MIN - IF_TEST_RANGE + 1
        min_freq = IF_REQUIRED_MIN + 1
        while min_freq > low_end:
            min_case.freq = (min_freq + low_end) // 2
            if self.verify_if_tuning(min_case):
                min_freq = min_case.freq
            else:
                low_end = min_case.freq + 1

        high_end = IF_REQUIRED_MAX + IF_TEST_RANGE - 1
        max_freq = IF_REQUIRED_MAX - 1
        while max_freq < high_end:
            max_case.freq = (max_freq + high_end + 1) // 2
            if self.verify_if_tuning(max_case):
                max_freq = max_case.freq
            else:
                high_end = max_case.freq - 1

        out(f"{self.name} IF range: {min_freq}-{max_freq} MHz")
        if min_freq > IF_REQUIRED_MIN or max_freq < IF_REQUIRED_MAX:
            fail(160)

    def analyze_sweep(self, sweep, test_case):
        eut_cal = self.eut_cal[all_rf_test_cases.index(test_case)]
        tester_cal = self.partner.tester_cal[all_rf_test_cases.index(test_case)]
        correction = LP2_CORRECTION if test_case.freq < 30 else 0

        # parse the hackrf_sweep output
        try:
            rows = sweep.stdout.split("\n")
            # Note: skipping data from the first twelve sweeps until issue #1230 is resolved.
            bins = np.zeros(225)
            # average 10 sweeps
            row_count = 0
            for row in  rows[48:88:4]:
                bins += np.array([float(x) for x in row.split(", ")[6:231]])
                row_count += 1
            assert row_count == 10
            bins /= 10
            # log the raw hackrf_sweep data before calibration
            log(f"{', '.join(rows[12].split(', ')[:6])}, {np.array2string(bins, separator=', ', max_line_width=3000)[1:-1]}")
            # apply calibration
            bins -= eut_cal
            bins -= tester_cal
            peak_bin = bins.argmax()
        except:
            log(sweep.stdout + sweep.stderr)
            log(traceback.format_exc())
            out(f"Couldn't parse {test_case.name} sweep data")
            fail(172)

        # ensure acceptable performance
        log(f"peak ({peak_bin}) {test_case.name}: {bins[peak_bin]:.2f}")
        if bins[peak_bin] < SIGNAL_THRESHOLD:
            for row in rows[0:88:4]:
                bins = [float(bin) for bin in row.split(", ")[6:]]
                peak_bin = bins.index(max(bins))
                log(f"peak bin {peak_bin}: {bins[peak_bin]}")
            log(f"{test_case.name} signal not strong enough")
            self.errors.append(test_case.base_ecode + 2)
            return bins[peak_bin]
        elif bins[peak_bin] > MAX_POWER:
            log(f"{test_case.name} signal clipping")
            self.errors.append(test_case.base_ecode + 3)
            return bins[peak_bin]
        tx_dc_power_dbc = bins[112] - bins[peak_bin]
        if tx_dc_power_dbc > MAX_DC_DBC + correction:
            log(f"{test_case.name} excessive TX DC offset {tx_dc_power_dbc:.1f} dBc")
            self.errors.append(test_case.base_ecode + 1)
        if peak_bin == 157:
            log(f"{test_case.name} TX spectrum inverted")
            self.errors.append(test_case.base_ecode + 25)
            return bins[peak_bin]
        elif peak_bin == 112:
            log(f"{test_case.name} peak found at DC")
            self.errors.append(test_case.base_ecode + 1)
            return bins[peak_bin]
        elif peak_bin >= 52 and peak_bin < 67:
            log(f"{test_case.name} peak found at adjacent frequency")
            self.errors.append(test_case.base_ecode + 15)
            return bins[peak_bin]
        elif peak_bin > 67 and peak_bin <= 82:
            log(f"{test_case.name} peak found at adjacent frequency")
            self.errors.append(test_case.base_ecode + 20)
            return bins[peak_bin]
        elif peak_bin != 67:
            log(f"{test_case.name} peak found at unexpected frequency")
            self.errors.append(test_case.base_ecode + 5)
            return bins[peak_bin]

        if bins[157] - bins[67] > MAX_M_DBC + correction:
            log(f"{test_case.name} TX spectrum mirrored")
            self.errors.append(test_case.base_ecode + 25)
        if (bins[22] - bins[67]) > MAX_H2_DBC + correction:
            log(f"{test_case.name} excessive TX harmonic (bin 22): {bins[22]}")
            self.errors.append(test_case.base_ecode + 10)
        if (bins[202] - bins[67]) > MAX_MH2_DBC + correction:
            log(f"{test_case.name} excessive TX harmonic (bin 202): {bins[202]}")
            self.errors.append(test_case.base_ecode + 10)
        worst_other_dbc = -999
        worst_other_bin = 999
        for i in range(225):
            # Skip various frequencies where spurs are expected (most of which have their own tests above).
            if (i < 52 and i != 22) or (i > 82 and i < 111) or (i > 113 and i < 156) or (i > 158 and i != 202):
                other_dbc = bins[i] - bins[67]
                if other_dbc > worst_other_dbc:
                    worst_other_dbc = other_dbc
                    worst_other_bin = i
        if worst_other_dbc > MAX_OTHER_DBC + correction:
            log(f"{test_case.name} excessive power at unexpected frequency ({worst_other_bin}: {bins[worst_other_bin]})")
            self.errors.append(test_case.base_ecode + 30)
        log(f"h2 (22) {test_case.name}: {(bins[22] - bins[67]):.2f}")
        log(f"DC (112) {test_case.name}: {(bins[112] - bins[67]):.2f}")
        log(f"m (157) {test_case.name}: {(bins[157] - bins[67]):.2f}")
        log(f"mh2 (202) {test_case.name}: {(bins[202] - bins[67]):.2f}")
        log(f"other ({worst_other_bin}) {test_case.name}: {worst_other_dbc:.2f}")
        #FIXME look in another row for RX inversion?
        return bins[peak_bin]

    def measure_rf(self, test_case, sweep=True):
        log(f"Executing RF test case {test_case.name}, sweep={sweep}")
        if test_case.direction == "tx":
            transmitter = self
            tx_amp = "1" if test_case.amp else "0"
            receiver = self.partner
            rx_amp = "0"
        elif test_case.direction == "rx":
            transmitter = self.partner
            tx_amp = "0"
            receiver = self
            rx_amp = "1" if test_case.amp else "0"
        else:
            out(f"Error: Invalid direction in {test_case.name}")
            sys.exit(1)

        # The nominal test frequency (as specified in test_case) is 1 MHz above
        # the low end of the sweep range. The transmitted signal is 500 kHz
        # above the nominal frequency, and the tuning (center) frequency of the
        # transmitting device is 1 MHz above that.
        tx_center_freq = (test_case.freq * ONE_MHZ) + 1500000
        sweep_range = f"{test_case.freq-1}:{test_case.freq+19}"

        transmit = subprocess.Popen([transmitter.bin_dir + "hackrf_transfer", "-d",
            transmitter.serial, "-R", "-t", WAVEFORM, "-a", tx_amp, "-x",
            f"{test_case.tx_gain}", "-f", str(tx_center_freq)],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")

        time.sleep(SLEEP_TIME)
        if sweep:
            receive = subprocess.run([receiver.bin_dir + "hackrf_sweep",
                "-d", receiver.serial, "-N", "22", "-w", "22222", "-f",
                sweep_range, "-a", rx_amp, "-l", f"{test_case.rx_lna_gain}",
                "-g", f"{test_case.rx_vga_gain}"], capture_output=True,
                encoding="utf-8", timeout=TIMEOUT)
        else:
            receive = subprocess.run([receiver.bin_dir + "hackrf_transfer",
                "-d", receiver.serial, "-n10000", "-r", "/dev/null", "-f",
                f"{(test_case.freq)*ONE_MHZ}", "-a", rx_amp, "-l",
                f"{test_case.rx_lna_gain}", "-g", f"{test_case.rx_vga_gain}",
                "-b", "2500000"], capture_output=True, encoding="utf-8",
                timeout=TIMEOUT)
        transmit.terminate()
        if receive.returncode != 0:
            log(receive.stdout + receive.stderr)
            log(f"{receiver.name} Failed RX, sweep={sweep}")
            fail(170)

        if sweep:
            rssi = self.analyze_sweep(receive, test_case)
        else:
            rssi = self.analyze_transfer(receive, test_case)

        tx_out, tx_err = transmit.communicate(timeout=TIMEOUT)
        if transmit.returncode != 0:
            log(tx_out + tx_err)
            out(f"Warning: {transmitter.name} transmit failed")
        return rssi

    def test_rf_preliminary(self):
        results = []

        for test_case in (all_rf_test_cases):
            if test_case.amp == False:
                if test_case.direction == "rx":
                    self.baseline_rx(test_case)
                elif self.partner:
                    self.partner.baseline_rx(test_case)

        self.noise_rx(noise)
        # FIXME test for RX DC offset?

        if not self.partner:
            out("Skipping RF tests")
            return

        for test_case in (all_rf_test_cases):
            results.append(int(round(self.measure_rf(test_case, sweep=False) - (SIGNAL_THRESHOLD + 11))))

        log(f"Preliminary results: {results}")
        if self.errors:
            fail_rf(self.errors)

    def test_rf(self):
        assert self.partner
        results = []

        for test_case in (all_rf_test_cases):
            results.append(int(round(self.measure_rf(test_case, sweep=True) - (SIGNAL_THRESHOLD + 5))))

        out(f"RF results summary: {results}")
        if self.errors:
            fail_rf(self.errors)

    def activate_leds(self, enable):
        if enable:
            out(f"Activating {self.name} LEDs")
            state = 7
        else:
            state = 1
        command = subprocess.run([self.bin_dir + "hackrf_debug", "-d", self.serial, "--leds",
            f"{state}"], capture_output=True, encoding="utf-8", timeout=TIMEOUT)
        if command.returncode != 0:
            log(command.stdout + command.stderr)
            log(f"Failed to configure {self.name} LEDs ({state})")
            fail(190)


# This waveform is a full-scale complex exponential with a frequency of -1/10 the
# sample rate. When transmitted at 10 Msps, the frequency is 1 MHz below the
# center frequency.

def write_waveform():
    tx_bytes = b'\x7f\x00\x67\xb5\x27\x87\xd9\x87\x99\xb5\x81\x00\x99\x4b\xd9\x79\x27\x79\x67\x4b'
    # defective waveform with DC offset:
    #tx_bytes = b'\x7f\x10\x67\xa5\x27\x97\xd9\x97\x99\xc5\x81\x10\x99\x5b\xd9\x7f\x27\x7f\x67\x5b'
    # defective waveform with missing Q channel:
    #tx_bytes = b'\x7f\x00\x67\x00\x27\x00\xd9\x00\x99\x00\x81\x00\x99\x00\xd9\x00\x27\x00\x67\x00'
    with open(WAVEFORM, "wb") as bin_file:
        for i in range(50000):  # 1MB file size
            bin_file.write(tx_bytes)


def program(bin_dir, fw_dir, serial, unattended=False):
    if "build" in fw_dir:
        binary = "hackrf_usb.bin"
        dfu_stub = "hackrf_usb.dfu"
    else:
        binary = "hackrf_one_usb.bin"
        dfu_stub = "hackrf_one_usb.dfu"

    # EUTs in unattended (including Jenkins CI) setups have pins jumped to always boot into DFU mode, so resetting is undesirable
    if unattended:
        reset_write = "-w"
    else:
        reset_write = "-Rw"

    if not usb.core.find(idVendor=DFU_VENDOR_ID, idProduct=DFU_PRODUCT_ID):
        print("ACTION REQUIRED: Press and release EUT RESET button while holding DFU button.")
        while not usb.core.find(idVendor=DFU_VENDOR_ID, idProduct=DFU_PRODUCT_ID):
            time.sleep(0.1)

    out("Programming EUT in DFU mode")
    dfu = subprocess.run([f"{DFU_UTIL}", "--device",
        f"{DFU_VENDOR_ID}:{DFU_PRODUCT_ID}", "--alt", "0", "--download",
        f"{fw_dir}{dfu_stub}"], capture_output=True, encoding="utf-8",
        timeout=TIMEOUT)

    #dfu-util: unable to read DFU status after completion (LIBUSB_ERROR_IO)
    # despite successful download; present as of at least dfu-util 0.11
    if "Download done." not in dfu.stdout:
        fail(60)

    # Wait for device to boot from DFU.
    then = time.time()
    dfu_device_found = False
    while time.time() < (then + 50):
        dfu_info = subprocess.run([bin_dir + "hackrf_info"], capture_output=True,
            encoding="utf-8", timeout=TIMEOUT)

        if "RunningFromRAM" in dfu_info.stdout:
            if serial in dfu_info.stdout and serial != "RunningFromRAM":
                out("wrong device in DFU mode")
                fail(1)
            else:
                dfu_device_found = True
                break
        time.sleep(0.1)
    if not dfu_device_found:
        out("DFU device not found")
        fail(1)

    out("Programming EUT SPI flash")
    spiflash = subprocess.run([bin_dir + "hackrf_spiflash", "-d",
                    "RunningFromRAM", reset_write, f"{fw_dir}{binary}"],
                    capture_output=True, encoding="utf-8", timeout=TIMEOUT)
    if spiflash.returncode != 0:
        log(spiflash.stdout + spiflash.stderr)
        fail(70)

    then = time.time()
    device_found = False
    while time.time() < (then + 5):
        flash_info = subprocess.run([bin_dir + "hackrf_info"], capture_output=True,
            timeout=TIMEOUT)

        if serial in flash_info.stdout.decode('utf-8', errors='ignore'):
            device_found = True
            break
        time.sleep(0.1)
    if not device_found:
        fail(75)


def find_sn(name, bin_dir, claimed_sns=[]):
    while True:
        sn_list = []
        info = subprocess.run([bin_dir + "hackrf_info"], capture_output=True,
                    encoding="utf-8", timeout=TIMEOUT)
        devices_list = re.findall(r'Found HackRF\n.+\n.+\n.+\n.+\n.+\n.+', info.stdout)

        out(f"\nPotential {name}(s) found:\n")
        for device in devices_list:
            new_sn = re.search(r'Serial number: \w+', device).group().split(': ')[1]
            if new_sn not in claimed_sns:
                sn_list.append(new_sn)
        if len(sn_list) > 1:
            for i in range(len(sn_list)):
                out(f"{i}: {sn_list[i]}")
            choice = int(input(f"\nEnter the index of your target {name}s serial number: "))
            if 0 <= choice < len(sn_list):
                sn = sn_list[choice]
                break
            else:
                out("Incorrect choice, try again.")
        elif len(sn_list) == 1:
            sn = sn_list[0]
            out(f"{sn}\n")
            break
        else:
            out("Failed to parse a serial number from hackrf_info")
            fail(1)
    return sn


def get_version():
    version = subprocess.run(["git", "log", "-n" "1", "--format=%h"],
                capture_output=True, encoding="utf-8", timeout=TIMEOUT)
    if version.returncode != 0:
        return "2026.01.3+"
    elif version.returncode == 0:
        dirty = subprocess.run(["git", "status", "-s", "--untracked-files=no"],
                capture_output=True, encoding="utf-8", timeout=TIMEOUT)
        if dirty.stdout != "":
            dirty_flag = "*"
        else:
            dirty_flag = ""
        return f"git-{version.stdout.strip()}{dirty_flag}"


def main():

    write_waveform()

    parser = argparse.ArgumentParser(description="Utility for testing HackRF One")
    parser.add_argument("-c", "--noclk", action="store_true", help="skip CLKIN, CLKOUT, and PPM tests")
    parser.add_argument("-i", "--noif", action="store_true", help="skip IF tuning range test")
    parser.add_argument("-l", "--loop", action="store_true", help="repeat the test until terminated with ctrl-c")
    parser.add_argument("-m", "--manufacturer", action="store_true", help="check the manufacturer of target devices")
    parser.add_argument("-r", "--rev", metavar="<hardware rev> or \"any\"", type=str,
            help="check EUT pin straps for specific hardware revision (default: \"r9\")",
            default="r9")
    parser.add_argument("-s", "--solo", action="store_true", help="perform limited tests on a single EUT with no TESTER")
    parser.add_argument("-t", "--tcxo", action="store_true", help="skip CLKIN/CLKOUT tests and use TCXO instead")
    parser.add_argument("-E", "--eut", metavar="<EUT SN>", type=str,
            help="EUT serial number")
    parser.add_argument("-T", "--tester", metavar="<TESTER SN>", type=str,
            help="TESTER serial number")
    parser.add_argument("-f", "--fwupdate", metavar="<path to firmware binaries>", type=str,
            help="update firmware with binaries from specified directory")
    parser.add_argument("-H", "--hostdir", metavar="<path to installed host tools>", type=str,
            help="location of installed hackrf host tools, will attempt to find them if omitted")
    parser.add_argument("-b", "--testerdir", metavar="<separate path to TESTER host tools>", type=str,
            help="necessary only if EUT/TESTER have separate host binaries")
    parser.add_argument("-C", "--ci", action="store_true", help="For use with Jenkins CI user")
    parser.add_argument("-u", "--unattended", action="store_true", help="For use with unattended hardware")
    parser.add_argument("-L", "--log", metavar="<log file>", type=str,
            help="log file location")
    global args
    args = parser.parse_args()

    if args.ci:
        user = "jenkins"
    else:
        user = os.getlogin()

    if "any" in args.rev.lower():
        args.rev = None
    elif args.rev.lower() == "r1" or args.rev.lower() == "r4":
        args.rev = "older than r6"

    if args.hostdir:
        eut_host_dir = args.hostdir
    else:
        try:
            eut_host_dir = os.path.dirname(shutil.which('hackrf_info')) + "/"
        except:
            out("No path to hackrf host tools found. Please provide a directory via --hostdir")
            fail(1)

    if args.testerdir:
        if args.testerdir == eut_host_dir:
            out("Specified TESTER bin directory must be different from the found EUT bin directory")
            out("If shared TESTER/EUT bin directory is intended, omit --testerdir")
            fail(1)
        else:
            tester_host_dir = args.testerdir
    else:
        tester_host_dir = eut_host_dir

    unique_bin = (tester_host_dir != eut_host_dir)
    count = 0

    while True:
        out()
        out("================================")
        out(datetime.now())
        out(sys.argv)
        out(f"user: {user}")
        out(f"version: {get_version()}")
        out(f"pid: {os.getpid()}")
        if args.loop:
            out(f"repeat count: {count}")
        out("================================")
        out()

        if args.eut:
            eut_sn = args.eut
        else:
            if args.tester:
                eut_sn = find_sn("EUT", eut_host_dir, [args.tester])
            else:
                eut_sn = find_sn("EUT", eut_host_dir)
        if args.tester:
            tester_sn = args.tester
        else:
            tester_sn = find_sn("TESTER", tester_host_dir, [eut_sn])

        if eut_sn == tester_sn:
            out("TESTER and EUT cannot be the same device")
            fail(1)

        if args.fwupdate:
            program(eut_host_dir, args.fwupdate, eut_sn, args.unattended)

        if not args.rev:
            out("Skipping EUT hardware revision check")
        eut = HackRF("EUT", eut_host_dir, eut_sn, args.manufacturer, revision=args.rev)

        if args.solo:
            tester = None
        else:
            tester = HackRF("TESTER", tester_host_dir, tester_sn, args.manufacturer, unique_bin=unique_bin)
            tester.test_serial()
            tester.partner = eut
            eut.partner = tester

        out("Testing EUT")

        if args.solo:
            eut.clkout_connected = False
        elif args.noclk:
            eut.clkout_connected = False
            tester.clkout_connected = False
        elif args.tcxo:
            eut.clkout_connected = False
            tester.clkout_connected = True
        else:
            eut.clkout_connected = True
            tester.clkout_connected = True

        if count > 0 and not args.fw_update:
            eut.activate_leds(False)

        eut.test_serial()

        # limited RF test without clock synchronization
        eut.test_rf_preliminary()

        if args.noclk or args.solo:
            out("Skipping CLKIN, CLKOUT, and PPM tests")
        else:
            eut.test_xtal()
            if not args.tcxo:
                eut.test_clkout()
            eut.test_clkin()

        eut.clkout(False)

        if tester:
            tester.clkout(True)

        if args.tcxo:
            if args.solo:
                out("Error: --solo is incompatible with --tcxo")
                sys.exit(1)
            if tester.clkin():
                out("Using TCXO on TESTER")
            else:
                out("Warning: No TCXO or external clock detected on TESTER")
            out("Skipping CLKOUT test")
            if not args.noclk and not eut.clkin():
                fail(211)

        # full RF test with clock synchronization
        if tester:
            eut.test_rf()

        if args.noif or args.solo:
            out("Skipping IF tuning range test")
        else:
            eut.test_if_range(ifmin, ifmax)

        eut.activate_leds(True)

        out()
        if (args.solo or args.noif or args.noclk or args.tcxo or not args.rev):
            out("Limited test complete")
        else:
            out("PASS if all 6 LEDs active")

        count += 1
        if not args.loop:
            break


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        out("KeyboardInterrupt")
        sys.exit(0)
    except subprocess.TimeoutExpired:
        log(traceback.format_exc())
        fail(180)
