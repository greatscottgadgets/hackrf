#!/usr/bin/env python3
"""
Decode wideband FM from HackRF raw int8 IQ capture to WAV audio.
"""

import sys
import numpy as np
from scipy import signal
from scipy.io import wavfile


def main():
    iq_path = "/tmp/fm_capture.bin"
    wav_path = "/tmp/fm_audio.wav"
    fs_rf = 2_000_000      # RF sample rate (Hz)
    fs_audio = 48_000      # Target audio rate (Hz)
    tau = 75e-6            # De-emphasis time constant (75 µs for US/World, 50 µs for EU)

    print(f"Reading IQ data from {iq_path}")
    raw = np.fromfile(iq_path, dtype=np.int8)
    print(f"  Total bytes: {raw.size}, samples: {raw.size // 2}")

    # Interleaved int8 I/Q -> complex float
    iq = raw[0::2].astype(np.float32) + 1j * raw[1::2].astype(np.float32)
    # Normalize to roughly unit amplitude
    iq = iq / 128.0

    # Optional: band-limit to ~200 kHz to reduce noise/aliasing before demod
    # Design FIR low-pass filter, decimate in stages, or just proceed
    # For simplicity we do the demod first then resample, but a pre-filter helps.
    # Let's apply a light pre-filter + decimate by 5 to 400 kHz to ease resampling.
    pre_decim = 5
    fs_intermediate = fs_rf // pre_decim
    # Polyphase FIR decimation
    iq = signal.decimate(iq, pre_decim, ftype="fir", zero_phase=True)

    # WBFM demodulation: differentiate unwrapped phase
    phase = np.unwrap(np.angle(iq))
    demod = np.diff(phase)
    # Scale to roughly audio amplitude (rads/sample -> Hz is not needed for listening)
    demod = demod.astype(np.float32)

    # Resample from fs_intermediate to fs_audio
    # fs_intermediate = 400 kHz. 400000/48000 = 125/15 = 25/3
    # Use resample_poly: up=3, down=25 -> 400k*3/25 = 48k
    audio = signal.resample_poly(demod, up=3, down=25, window=('kaiser', 5.0))

    # De-emphasis filter (first-order IIR low-pass)
    # y[n] = alpha * y[n-1] + (1-alpha) * x[n]
    alpha = np.exp(-1.0 / (fs_audio * tau))
    audio_deemph = np.zeros_like(audio)
    audio_deemph[0] = audio[0] * (1 - alpha)
    for i in range(1, audio.size):
        audio_deemph[i] = alpha * audio_deemph[i - 1] + (1 - alpha) * audio[i]

    # Remove DC offset (carrier frequency offset appears as DC after demod)
    audio_deemph = audio_deemph - np.mean(audio_deemph)

    # Normalize to 16-bit PCM range
    peak = np.max(np.abs(audio_deemph))
    print(f"  Peak audio level before norm: {peak:.4f}")
    if peak == 0:
        print("Error: audio is silent.")
        sys.exit(1)
    audio_norm = audio_deemph / peak * 0.95
    audio_int16 = (audio_norm * 32767).astype(np.int16)

    wavfile.write(wav_path, fs_audio, audio_int16)
    print(f"  Wrote {audio_int16.size} samples ({audio_int16.size/fs_audio:.2f} s) to {wav_path}")
    import os
    size_mb = os.path.getsize(wav_path) / (1024 * 1024)
    print(f"  WAV file size: {size_mb:.2f} MB")


if __name__ == "__main__":
    main()
