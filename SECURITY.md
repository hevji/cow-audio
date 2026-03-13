# Security Policy

## Reporting a vulnerability

If you find a security vulnerability in COW Audio, please do not open a public issue. Instead, report it privately so it can be addressed before it becomes public knowledge.

To report a vulnerability, open a [GitHub Security Advisory](https://github.com/hevji/cow-audio/security/advisories/new) on the repository. Describe the issue, how to reproduce it, and what the potential impact is.

We will acknowledge your report within a few days and work on a fix as quickly as possible. You will be credited in the release notes unless you prefer to remain anonymous.

---

## Scope

The following are considered in scope for security reports:

- Malformed `.cow` files that cause crashes, memory corruption, or arbitrary code execution in the player or CLI
- Buffer overflows or out-of-bounds reads in the C++ code
- Issues in the file format that could allow malicious files to exploit the decoder

The following are out of scope:

- Social engineering
- Issues in third-party dependencies not maintained by this project
- Theoretical vulnerabilities without a proof of concept

---

## Supported versions

Only the latest release is actively maintained. If you find a vulnerability in an older version, please check if it still exists in the latest release before reporting.

| Version | Supported |
|---------|-----------|
| Latest  | Yes       |
| Older   | No        |
