# Project rules

## Releases

- Release numbering is currently paused for owner clarification. The owner subsequently said the earlier `6.11.x` instruction may have been a typo; official upstream remains `6.10`. The proposed next fork version is `6.10.3` because historical `6.10.1-ci` and `6.10.2-ci` tags exist, but this proposal has not been approved. Continue code and CI work without creating new release tags or publishing until the owner confirms the version line.
- Owner instruction (2026-10-25): keep new fork releases on the `6.11.x` series. Increment the patch component for each subsequent release, for example `6.11.1`, then `6.11.2`. Do not revert to the historical 6.10 renumbering policy or derive the fork release number solely from upstream.
- Verify remote tags and releases before choosing the next unused patch version. Never move existing tags or overwrite published release assets.
- Keep the CLI version, CMake version, resource version and dependency manifest version consistent.
- Fix relevant reported defects with reproducible regression tests, and require portable CI and real-media end-to-end validation before publication.
