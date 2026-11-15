#!/usr/bin/env python3
"""Mesures objectives sur des rendus WAV (§15.6) : une ligne JSON par fichier.

Installation (une fois) :
    python3 -m venv tools/.venv && tools/.venv/bin/pip install numpy scipy matplotlib
Usage :
    tools/.venv/bin/python -I tools/analyse.py renders/*.wav [--png dossier] [--grid secondes]

Code de sortie 1 si un fichier contient des NaN/Inf ou est silencieux.
"""
import argparse
import json
import os
import sys

import numpy as np
from scipy import signal
from scipy.io import wavfile

CLICK_THRESHOLD = 0.5  # dérivée seconde, en pleine échelle
SILENCE_DB = -60.0
ONSET_FLOOR_DB = -40.0
ONSET_RATIO = 4.0  # l'enveloppe doit quadrupler en 5 ms
ONSET_REFRACTORY_S = 0.03


def load(path):
    rate, data = wavfile.read(path)
    if np.issubdtype(data.dtype, np.integer):
        data = data.astype(np.float64) / float(np.iinfo(data.dtype).max)
    data = data.astype(np.float64)
    return rate, data.reshape(len(data), -1)


def db(x):
    return float(20.0 * np.log10(max(x, 1e-12)))


def onsets(mono, rate):
    hop = rate // 1000  # 1 ms
    frames = len(mono) // hop
    envelope = np.abs(mono[: frames * hop]).reshape(frames, hop).max(axis=1)
    floor = 10.0 ** (ONSET_FLOOR_DB / 20.0)
    times, last = [], -1.0
    for i in range(5, frames):
        t = i * hop / rate
        if envelope[i] > floor and envelope[i] > ONSET_RATIO * envelope[i - 5] and t - last > ONSET_REFRACTORY_S:
            times.append(t)
            last = t
    return times


def analyse(path, grid):
    rate, data = load(path)
    finite = bool(np.isfinite(data).all())
    data = np.nan_to_num(data)
    mono = data.mean(axis=1)
    peak = float(np.abs(data).max())
    rms = float(np.sqrt(np.mean(mono**2)))

    spectrum = np.abs(np.fft.rfft(mono * np.hanning(len(mono))))
    freqs = np.fft.rfftfreq(len(mono), 1.0 / rate)
    centroid = float((spectrum * freqs).sum() / max(spectrum.sum(), 1e-12))

    second = np.abs(np.diff(data, n=2, axis=0))
    times = onsets(mono, rate)
    result = {
        "file": os.path.basename(path),
        "seconds": round(len(mono) / rate, 3),
        "finite": finite,
        "peak_db": round(db(peak), 2),
        "rms_db": round(db(rms), 2),
        "crest_db": round(db(peak) - db(rms), 2),
        "dc": round(float(mono.mean()), 6),
        "centroid_hz": round(centroid, 1),
        "clicks": int((second > CLICK_THRESHOLD).sum()),
        "silent": bool(db(peak) < SILENCE_DB),
        "onsets": len(times),
    }
    if grid and times:
        # Écart de chaque attaque à la grille des pas (l'attaque du bip met environ 1 ms à sortir du seuil).
        error = [abs(t / grid - round(t / grid)) * grid for t in times]
        result["onset_grid_error_ms"] = round(1000.0 * max(error), 2)
    return result, rate, mono


def spectrogram(path, rate, mono, directory):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    f, t, power = signal.spectrogram(mono, rate, nperseg=2048, noverlap=1536)
    figure, axis = plt.subplots(figsize=(12, 4))
    axis.pcolormesh(t, f, 10.0 * np.log10(power + 1e-12), vmin=-120, vmax=-30, shading="auto", cmap="magma")
    axis.set_yscale("symlog", linthresh=200)
    axis.set_ylim(40, 8000)
    axis.set_xlabel("s")
    axis.set_ylabel("Hz")
    axis.set_title(os.path.basename(path))
    figure.tight_layout()
    os.makedirs(directory, exist_ok=True)
    figure.savefig(os.path.join(directory, os.path.splitext(os.path.basename(path))[0] + ".png"), dpi=110)
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", nargs="+")
    parser.add_argument("--png", metavar="DOSSIER", help="écrit un spectrogramme PNG par fichier")
    parser.add_argument("--grid", type=float, metavar="S", help="durée d'un pas : mesure l'écart des attaques à la grille")
    args = parser.parse_args()

    failed = False
    for path in args.files:
        result, rate, mono = analyse(path, args.grid)
        print(json.dumps(result, ensure_ascii=False))
        failed = failed or not result["finite"] or result["silent"]
        if args.png:
            spectrogram(path, rate, mono, args.png)
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
