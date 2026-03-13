# 🐄 COW — Compressed Wave Audio Format

> A custom lossless audio format with file sizes close to MP3, built from scratch in Python and C++.

COW (**Co**mpressed **W**ave) is a lossless audio format that compresses WAV files using a delta filter + LZMA compression, achieving dramatic file size reductions while preserving every single sample — byte perfect, every time.

---

## Features

- **Truly lossless** — CRC-32 verified, byte-perfect reconstruction guaranteed
- **High compression** — LZMA preset-9 + inter-sample delta filter
- **Native Windows integration** — double-click `.cow` files to play them
- **Console player** — clean UI with progress bar, volume control, and seeking
- **CLI tool** — encode, decode, and inspect files from anywhere on your system
- **Auto file search** — finds your music in common folders automatically

---

## How it works

COW compresses audio in two stages:

1. **Delta filter** — stores the difference between consecutive samples per channel instead of raw values. Audio changes gradually, so deltas are tiny numbers that compress far better than raw PCM.
2. **LZMA preset-9** — the tightest general-purpose lossless compression available, applied to the delta-filtered data.

The result is a file that decompresses back to an identical WAV — same samples, same CRC, same everything.

---

## File format

```
Offset  Size  Field
------  ----  -----
0       4     Magic: COWv
4       1     Version: 0x01
5       2     Channels        (uint16 LE)
7       4     Sample rate     (uint32 LE)
11      2     Bit depth       (uint16 LE)
13      4     Num frames      (uint32 LE)
17      1     Compression:    0x01 = LZMA + delta
18      4     Compressed data length (uint32 LE)
22      4     CRC-32 of original PCM (uint32 LE)
26      N     Compressed payload
```

---

## Requirements

- **Windows** (the player uses Windows MCI and Win32 APIs)
- **Python 3.x** — for encoding, decoding, and file search
- **MinGW / g++** or **Visual Studio** — to compile the C++ player

---

## Installation

### Step 1 — Compile the player and CLI tool

**MinGW / g++:**
```bash
# Compile the player (with icon)
windres resource.rc -o resource.o
g++ -o cow_player.exe cow_player.cpp resource.o -lwinmm

# Compile the CLI tool
g++ -o cow.exe cow.cpp
```

**Visual Studio (Developer Command Prompt):**
```bash
cl /EHsc /O2 cow_player.cpp /link winmm.lib
cl /EHsc /O2 cow.cpp
```

### Step 2 — Set up the folder

Create `C:\COWPlayer\` and place these files in it:

```
C:\COWPlayer\
    cow_player.exe
    cow.exe
    cow_format.py
```

### Step 3 — Add to PATH

Add `C:\COWPlayer` to your system PATH so the `cow` command works from anywhere:

1. Search **"environment variables"** in Start
2. Edit **Path** under System variables
3. Add `C:\COWPlayer`

### Step 4 — Install file association

1. Open `cow_player.reg` in Notepad and verify the paths say `C:\\COWPlayer\\cow_player.exe`
2. Double-click `cow_player.reg` and click Yes

Every `.cow` file on your system will now open with COW Player.

---

## Usage

### Encode a WAV file
```bash
cow encode mysong.wav
```
Output is saved next to the original file as `mysong.cow`.

### Decode back to WAV
```bash
cow decode mysong.cow
```

### Inspect a file
```bash
cow info mysong.cow
```

### Play a file
```bash
cow play mysong.cow
```
Or just double-click any `.cow` file.

---

## COW Player controls

```
+============================================================+
|  COW Player  -  Compressed Wave Audio                      |
+------------------------------------------------------------+
|  mysong.cow                                                |
|  Duration : 3:42                                           |
|                                                            |
+------------------------------------------------------------+
|  [##################........................] 1:24/3:42   |
|  Vol [###########################..........] 80%           |
|  [ PLAYING ]                                               |
+------------------------------------------------------------+
|  SPACE Pause/Resume   LEFT/RIGHT Seek 5s                   |
|  UP/DOWN Volume       Q / ESC   Quit                       |
+============================================================+
```

| Key         | Action           |
|-------------|------------------|
| SPACE       | Pause / Resume   |
| LEFT arrow  | Rewind 5 seconds |
| RIGHT arrow | Forward 5 seconds|
| UP arrow    | Volume up        |
| DOWN arrow  | Volume down      |
| Q / ESC     | Quit             |

---

## File search

The encoder automatically searches common locations for your files so you don't need to `cd` to the right folder:

- `~/Music`
- `~/Downloads`
- `~/Desktop`
- `~/Documents`
- `~/Videos`
- `~/OneDrive/Music`
- `~/AppData/Local/Spotify/Data`
- `C:\Users\Public\Music`

If multiple files share the same name, it lists them and asks you to pick.

---

## Project structure

```
cow_format.py     — Python encoder / decoder
cow_player.cpp    — C++ console player (Win32 + MCI)
cow.cpp           — C++ CLI wrapper (encode, decode, play, info)
cow_player.reg    — Windows file association installer
resource.rc       — Icon resource file
cow_icon.ico      — Format icon
```

---

## Antivirus

Some antivirus engines flag the compiled exes as false positives due to heuristic detection. This is common for unsigned C++ programs that spawn child processes and write to temp directories — which is exactly what a media player does. The source code is fully open so you can review it and compile it yourself. Windows Defender does not flag either exe on a real machine.

| File | VirusTotal |
|------|------------|
| `cow_player.exe` | [View scan](https://www.virustotal.com/gui/file/8dfc1c79714b755fc86cba7e3fccd6e4c78087381962b6876b3b0dea1089a3ed/detection) |
| `cow.exe` | [View scan](https://www.virustotal.com/gui/file/2a720c5f026583ff4ab7c0a011ad2b53dade51feafed1b9a0aca8a20f1bc1094) |

---

## License

MIT — do whatever you want with it.
