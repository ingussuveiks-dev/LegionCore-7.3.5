# A Round for the Guards 29351

Daryl Riknussun `5159/82192` offers the Ironforge daily when cooking pool `5688` selects it. Cooking 1, the six-guard objective and the reusable Ironforge Rations `69981` are unchanged. The world data contain 98 original Ironforge Guards `5595` in city zone `1537`. The same creature template has additional placements outside the city. The full 57-second walkthrough was reviewed at five-second intervals, covering acceptance, feeding in the city and the return to Daryl. [Archived gameplay](https://www.youtube.com/watch?v=9CWoiLhmTTQ)

Rations `99479` require an active accepted attempt, carried rations and a real, living Ironforge guard. The original delayed trigger applies native Well Fed `99478` for two minutes. The caster earns one personal objective when that exact aura has reached the selected guard. The original immediate credit effect previously advanced the quest without any confirmed feeding. A fed guard remains unavailable to other players until the aura expires. The native localized guard acknowledgements `52137–52142` are selected uniformly after a confirmed feeding. The original choice weights were unavailable.

## Server verification

- The pinned `945208bf8` game library reproduced the defect. The rations applied the aura to quest giver Daryl and advanced one objective.
- The repaired native test selected the daily through the pool, denied Daryl and a real guard outside the city, fed six actual different city guards, and verified personal credit and repeat denial for both players. It checked the two-minute aura metadata and observed the normal expiry path with a shortened fixture timer. A canceled attempt and an inactive leftover item could no longer feed a guard. The actual reward packet removed the reusable rations.
- The retained Ironforge Bait regression passed. The test used real coordinates and selected nearby approaches with native line of sight and facing. SQL and existing shared guard AI were unchanged.
- The actual final link consumed `game.lib` SHA-256 `5837AD69E80D0CF3217559CAB74F93C82EE24AD2AE377BC9CCCF3DEE10FDFDFF` and `scripts.lib` SHA-256 `FAA1829A165D9A6C90272CFF2C8F06D2A8B32EDCBF06AA6473208DDFA04B3303`. The final map and PE match `worldserver.exe` SHA-256 `D10FD00D33A2BB5ED65EC5411C766800F8F7990F111B237531E26835AD4255D6`.
- The staged and installed canonical `worldserver.exe` both reached readiness and shut down cleanly using the isolated world data. No check produced a new crash, unexplained server warning or DB error. The 143 original diagnostic files were archived and hash-verified under `build-extractors/bin/Release/fixed/ironforge-rations-2026-11-22/`. Cleanup reclaimed 2,899,792,442 bytes. The previous executable and map remain in the local `rollback` directory.

Live 7.3.5 client checks remain open for the daily offer, feeding animation and dialogue presentation, real walking approaches to the selected guards and the reward UI.
