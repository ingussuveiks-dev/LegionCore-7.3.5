# Closing the Eye entry

War Councilor Victoria's options `20778/6` and `20778/7` both used the same native `229196` teleport for scenario `1204` on map `1673`. The paid option checked the 100 Nethershard balance. The spell has no currency reagent, and the world data had no producer for the free-attempt marker `45635`. Blizzard's 7.3.5 announcement confirms the first entry is free and later entries use Nethershards. [Blizzard's June 2018 announcement](https://news.blizzard.com/en-gb/article/21901728/its-time-heed-the-speakers-call).

Victoria uses the original SmartAI and delegates the actual selection to it after checking the displayed option, current gossip conditions and the player's transfer state. The native server-accepted far transfer must match the `229196` destination before the paid route deducts exactly 100 currency `1226` or the free route records the existing hidden marker. A failed native transfer restores the previous scenario selection when no transfer is pending. The marker uses the native persisted quest-flag setter. Both native chains and the five specialization gates remain in the database.

## Verification

- Installed DB2 data confirmed effect 227, LFG `1406` and the absent built-in currency and first-attempt effects. The source-extracted ASAN check exercised the real admission callback with the first free entry, repeat selection during a transfer, post-marker re-entry, exact 100-shard deduction and failed and unexpected transfers. It also exercised current gossip conditions and delegation of a sibling menu choice.
- Two isolated MariaDB applications retained the two native scenario chains, positions, live conditions and all five specialization gates. The final link check verifies the native SmartAI delegation, position lookup, currency debit and persisted marker setter in the executable.
- The exact production migration was applied manually and read back. Staged and canonical starts and the canonical link check used SHA-256 `06B42E29AC92604B03645D99D5479482374E49FCB956B0EB8FD910A7DBEEAB30`. Both servers exited cleanly and produced no new crashes, warnings, errors or assertions. The local archive `build-extractors/bin/Release/fixed/2026-11-21-mage-tower-xylem-entry/` includes 42 verified hashes and the cleanup record for 177,450,088 bytes of temporary outputs removed.

Actual client loading, return and retry, insufficient balance, login persistence, and all five specialization branches remain client checks. Xylem's combat and intermission are separate from this admission change.
