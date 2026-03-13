# Support

## Before asking for help

Check these first — your question might already be answered:

- [README](README.md) — installation, usage, and format documentation
- [CONTRIBUTING](CONTRIBUTING.md) — building from source and known limitations
- [Open issues](https://github.com/hevji/cow-audio/issues) — someone may have had the same problem

---

## Getting help

If you're stuck, open an issue on GitHub. When you do, please include:

- Your operating system and version
- What you were trying to do
- What you expected to happen
- What actually happened
- Any error messages you saw

The more detail you give, the faster we can help.

---

## Common issues

**The player opens but plays silence**
Make sure Python is installed and on your PATH. The player calls Python to decode the `.cow` file before playback.

**`cow encode` says it can't find the file**
Try providing the full path to the file instead of just the filename. The file search only looks in common folders like Music, Downloads, Desktop, Documents, and Videos.

**The file association isn't working**
Make sure you ran `cow_player.reg` as an administrator and that the path inside it matches where you put `cow_player.exe`.

**The player window is the wrong size**
This can happen if your console font size is non-standard. Try right-clicking the title bar, going to Properties, and setting the font size to a standard value like 16.

**Encoding is slow**
This is expected — LZMA compression at high presets is slow by design. Decoding is fast. If encoding speed is important to you, this is a known limitation listed in CONTRIBUTING.md.

---

## Feature requests

If you want something added, open an issue and describe what you need and why. Check the roadmap in CONTRIBUTING.md first to see if it's already planned.

---

## This is not a support channel for

- General Python or C++ questions unrelated to this project
- Help with your operating system or terminal setup
