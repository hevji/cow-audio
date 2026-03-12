"""
.COW — Compressed Wave Format  (Python encoder/decoder)
========================================================
Encodes WAV → .cow and decodes .cow → WAV.
Lossless: LZMA preset-9 + inter-sample delta filter.

File Layout (26-byte header)
-----------------------------
Offset  Size  Field
------  ----  -----
0       4     Magic: b'COWv'
4       1     Version: 0x01
5       2     Channels        (uint16 LE)
7       4     Sample rate     (uint32 LE)
11      2     Bit depth       (uint16 LE)
13      4     Num frames      (uint32 LE)
17      1     Compression: 0x01 = LZMA+delta
18      4     Compressed data length (uint32 LE)
22      4     CRC-32 of original PCM (uint32 LE)
26      N     Compressed payload
"""

import wave
import struct
import zlib
import lzma
import os
import sys
import tempfile


# ── File search ───────────────────────────────────────────────────────────────

SEARCH_DIRS = [
    os.path.expanduser("~\\Music"),
    os.path.expanduser("~\\Downloads"),
    os.path.expanduser("~\\Desktop"),
    os.path.expanduser("~\\Documents"),
    os.path.expanduser("~\\Videos"),
    os.path.expanduser("~\\OneDrive\\Music"),
    os.path.expanduser("~\\OneDrive\\Downloads"),
    os.path.expanduser("~\\AppData\\Local\\Spotify\\Data"),
    "C:\\Users\\Public\\Music",
]


def _find_file(filename, extensions):
    """Search common directories for a file by name (no path needed).
    extensions: list of extensions to try e.g. ['.wav', '.WAV']
    Returns the full path, or raises FileNotFoundError."""

    # If it's already a valid path, just return it
    if os.path.isfile(filename):
        return filename

    # Strip extension from search name so we can try multiple extensions
    base, ext = os.path.splitext(filename)
    if not ext:
        names_to_try = [filename + e for e in extensions]
    else:
        names_to_try = [filename]

    matches = []
    for search_dir in SEARCH_DIRS:
        if not os.path.isdir(search_dir):
            continue
        for root, dirs, files in os.walk(search_dir):
            # Limit depth to 3 levels to avoid being slow
            depth = root[len(search_dir):].count(os.sep)
            if depth >= 3:
                dirs.clear()
                continue
            for name_to_try in names_to_try:
                target = os.path.basename(name_to_try).lower()
                for f in files:
                    if f.lower() == target:
                        matches.append(os.path.join(root, f))

    if not matches:
        raise FileNotFoundError(
            f"Could not find '{filename}' in common music folders.\n"
            f"Searched: {', '.join(d for d in SEARCH_DIRS if os.path.isdir(d))}\n"
            f"Tip: provide the full path instead.")

    if len(matches) == 1:
        print(f"Found: {matches[0]}")
        return matches[0]

    # Multiple matches — ask user to pick
    print(f"\nFound {len(matches)} files named '{os.path.basename(names_to_try[0])}':")
    for i, m in enumerate(matches, 1):
        print(f"  [{i}] {m}")
    while True:
        try:
            choice = int(input("\nPick a number: "))
            if 1 <= choice <= len(matches):
                return matches[choice - 1]
        except (ValueError, KeyboardInterrupt):
            pass
        print("Invalid choice, try again.")

MAGIC   = b"COWv"
VERSION = 0x01
COMP_LZMA_DELTA = 0x01

HEADER_FMT  = "<4sBHIHIBII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)   # 26 bytes


# ── Delta filter ─────────────────────────────────────────────────────────────

def _apply_delta(samples, channels):
    out  = list(samples)
    prev = [0] * channels
    for i, s in enumerate(samples):
        ch       = i % channels
        delta    = (s - prev[ch]) & 0xFFFF
        if delta >= 0x8000:
            delta -= 0x10000
        out[i]   = delta
        prev[ch] = s
    return out


def _undo_delta(deltas, channels):
    out = list(deltas)
    cur = [0] * channels
    for i, d in enumerate(deltas):
        ch     = i % channels
        cur[ch] = (cur[ch] + d) & 0xFFFF
        val    = cur[ch]
        if val >= 0x8000:
            val -= 0x10000
        out[i] = val
    return out


# ── Encode ────────────────────────────────────────────────────────────────────

def encode(wav_path, cow_path=None):
    wav_path = _find_file(wav_path, [".wav", ".WAV"])
    if cow_path is None:
        cow_path = os.path.splitext(wav_path)[0] + ".cow"

    with wave.open(wav_path, "rb") as wf:
        channels  = wf.getnchannels()
        sampwidth = wf.getsampwidth()
        framerate = wf.getframerate()
        nframes   = wf.getnframes()
        raw_bytes = wf.readframes(nframes)

    bits = sampwidth * 8
    crc  = zlib.crc32(raw_bytes) & 0xFFFFFFFF

    if bits == 16:
        total   = nframes * channels
        samples = list(struct.unpack_from(f"<{total}h", raw_bytes))
        deltas  = _apply_delta(samples, channels)
        pcm_to_compress = struct.pack(f"<{total}h", *deltas)
    else:
        pcm_to_compress = raw_bytes

    compressed = lzma.compress(pcm_to_compress,
                               format=lzma.FORMAT_XZ,
                               preset=9 | lzma.PRESET_EXTREME)

    header = struct.pack(HEADER_FMT,
                         MAGIC, VERSION,
                         channels, framerate, bits, nframes,
                         COMP_LZMA_DELTA,
                         len(compressed), crc)

    with open(cow_path, "wb") as f:
        f.write(header)
        f.write(compressed)

    raw_size = os.path.getsize(wav_path)
    cow_size = os.path.getsize(cow_path)
    print(f"Encoded  → {cow_path}")
    print(f"WAV: {raw_size:,} bytes   COW: {cow_size:,} bytes  ({cow_size/raw_size*100:.1f}%)")
    return cow_path


# ── Decode ────────────────────────────────────────────────────────────────────

def decode(cow_path, wav_path=None):
    cow_path = _find_file(cow_path, [".cow"])
    if wav_path is None:
        wav_path = os.path.splitext(cow_path)[0] + "_decoded.wav"

    with open(cow_path, "rb") as f:
        hdr = f.read(HEADER_SIZE)
        (magic, version, channels, framerate, bits,
         nframes, comp, clen, stored_crc) = struct.unpack(HEADER_FMT, hdr)
        if magic != MAGIC:
            raise ValueError("Not a .cow file")
        compressed = f.read(clen)

    pcm_filtered = lzma.decompress(compressed, format=lzma.FORMAT_XZ)
    sampwidth    = bits // 8

    if bits == 16:
        total   = nframes * channels
        deltas  = list(struct.unpack_from(f"<{total}h", pcm_filtered))
        samples = _undo_delta(deltas, channels)
        raw_bytes = struct.pack(f"<{total}h", *samples)
    else:
        raw_bytes = pcm_filtered

    actual_crc = zlib.crc32(raw_bytes) & 0xFFFFFFFF
    if actual_crc != stored_crc:
        raise RuntimeError("CRC mismatch — file may be corrupted")

    with wave.open(wav_path, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sampwidth)
        wf.setframerate(framerate)
        wf.writeframes(raw_bytes)

    return wav_path


# ── Read header (used by C++ player via subprocess or standalone) ─────────────

def info(cow_path):
    cow_path = _find_file(cow_path, [".cow"])
    with open(cow_path, "rb") as f:
        hdr = f.read(HEADER_SIZE)
    (magic, version, channels, framerate, bits,
     nframes, comp, clen, stored_crc) = struct.unpack(HEADER_FMT, hdr)
    if magic != MAGIC:
        raise ValueError("Not a .cow file")
    duration = nframes / framerate if framerate else 0
    raw_est  = nframes * channels * (bits // 8)
    return {
        "channels":    channels,
        "sample_rate": framerate,
        "bit_depth":   bits,
        "num_frames":  nframes,
        "duration":    duration,
        "compressed":  clen,
        "raw_est":     raw_est,
        "crc32":       stored_crc,
    }


# ── CLI ───────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        print("Usage: cow_format.py encode <in.wav> [out.cow]")
        print("       cow_format.py decode <in.cow> [out.wav]")
        print("       cow_format.py info   <in.cow>")
        sys.exit(0)

    cmd, *rest = args
    if cmd == "encode":
        encode(rest[0], rest[1] if len(rest) > 1 else None)
    elif cmd == "decode":
        wav = decode(rest[0], rest[1] if len(rest) > 1 else None)
        print(f"Decoded → {wav}")
    elif cmd == "info":
        d = info(rest[0])
        for k, v in d.items():
            print(f"  {k:<14} {v}")
    else:
        print(f"Unknown command: {cmd}")
