# Project rules

## Releases

- Owner instruction (2026-10-25): keep new fork releases on the `6.11.x` series. Increment the patch component for each subsequent release, for example `6.11.1`, then `6.11.2`. Do not revert to the historical 6.10 renumbering policy or derive the fork release number solely from upstream.
- Verify remote tags and releases before choosing the next unused patch version. Never move existing tags or overwrite published release assets.
- Keep the CLI version, CMake version, resource version and dependency manifest version consistent.
- Fix relevant reported defects with reproducible regression tests, and require portable CI and real-media end-to-end validation before publication.
