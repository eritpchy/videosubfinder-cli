# Project rules

## Releases

- Owner confirmation on 2026-10-25: keep fork releases in the `6.10.x` series. The next planned release is `6.10.6`, followed by `6.10.7`, and so on. This supersedes the earlier mistaken `6.11.x` instruction.
- Check current remote tags and releases before selecting the next unused patch number. Never move existing tags or overwrite published release assets.
- Keep CLI, CMake, resource, and dependency manifest versions consistent. Run `scripts/check-version.py` before publication.
- Require portable CI and real-media end-to-end verification. Distinguish CPU execution of CUDA builds from real GPU execution; disclose unverified GPU behavior explicitly and never claim that a GPU test passed without evidence.
- Preserve runtime licenses and audit the complete extracted distribution size, not only its archive or executable size.
