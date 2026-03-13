# Contributing to COW Audio

First off, thanks for taking the time to contribute. Every bit helps, whether it's fixing a typo, reporting a bug, or building out a whole new feature.

---

## Ways to contribute

You don't have to write code to contribute. Here are some ways to help:

- **Report bugs** — open an issue describing what went wrong, what you expected, and what actually happened
- **Suggest features** — open an issue with your idea before writing any code so we can discuss it first
- **Improve documentation** — clearer docs help everyone
- **Write tests** — try encoding and decoding different WAV files and report anything unexpected
- **Fix bugs** — check open issues for things marked `bug` or `good first issue`
- **Build new features** — check the roadmap section below for ideas

---

## Getting started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/your-username/cow-audio`
3. Create a branch: `git checkout -b my-change`
4. Make your changes
5. Test thoroughly
6. Open a pull request

---

## Code style

- Keep it readable over clever
- Comment anything non-obvious
- Match the style of the file you're editing
- No external dependencies without a good reason and prior discussion

---

## File format compatibility

This is the most important rule in the project:

**Never break existing .cow files.**

If you need to change the format:
- Bump the version byte (`0x01` → `0x02`)
- Keep the decoder able to read all previous versions
- Document the change clearly in your PR

The format spec is in the README. Read it before touching the encoder or decoder.

---

## Roadmap

These are things the project needs. If you want to work on one, open an issue first so we can coordinate.

**Cross-platform support**
The player currently uses Win32 and Windows MCI which are Windows only. Rewriting it with SDL2 would make it work on Linux and macOS with minimal code changes.

**Pure C++ encoder**
The encoder currently requires Python. A C++ encoder using zlib or a bundled compression library would make the project fully self-contained.

**Metadata support**
Artist, title, album, and cover art could be stored as optional chunks in the file header without breaking backwards compatibility.

**Faster decoding**
Currently decodes to a temp WAV before playback. Streaming decode directly to the audio device would start playback instantly.

**Package managers**
Getting COW into Homebrew, AUR, or winget would make it much easier to install.

---

## Reporting bugs

When opening a bug report, please include:

- Your operating system and version
- What you were trying to do
- What you expected to happen
- What actually happened
- The WAV file details if relevant (sample rate, bit depth, channels)

---

## Pull request checklist

Before submitting a PR, make sure:

- [ ] Your change works on at least one platform
- [ ] You haven't broken existing `.cow` files
- [ ] Your code follows the style of the rest of the project
- [ ] You've described what you changed and why in the PR description

---

## Questions

Open an issue and we'll get back to you.
