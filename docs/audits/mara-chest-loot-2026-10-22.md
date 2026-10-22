# Mara Swiftmane reward chest, 2026-10-22

Client report: after rescuing Mara from Dargok Thunderuin and "Sure-Shot" Arnie, Battered Chest opens an empty Items window.

The local SmartAI for Mara 100238 summons chest 245479 at 4737.47, 4609.62, 669.581 after the hunter-death transition. Its chest template has Data1=0. Existing gameobject_loot_template 245479 contains currency 1220 (70), item 131781 and item 138839 with existing probabilities. Native GameObjectTemplate::GetLootId selects Data1; Player::SendLoot only invokes FillLoot when this ID is nonzero. The local TDB 7.3.5 snapshot also has the missing zero link; it supplies no alternative reward table.

Migration 2026_10_22_03 links only this chest to its existing loot. It does not invent rewards, make probabilistic items guaranteed, change personal/one-time rules, or reset character state. Both player and pet hunter kills keep their existing encounter transition.

Automated evidence: Test-MaraChestLoot.ps1 reproduces the zero link in temporary tables, applies the migration twice, checks all three existing reward rows, Mara's actual summon row, preserved settings and an unrelated chest. Its C++ test executes the production GetLootId selector. No client loot replay has been performed. Restart is required to refresh the cached gameobject template; an already generated empty loot object may need a fresh encounter summon.

Deployment: saved the previous template, loot and encounter rows in an ignored private backup, then applied the migration to the configured MariaDB world database. No executable change is necessary. No user servers were running. A hidden validation worldserver from the canonical Release directory reached ready with no ERROR or database-loader errors and was gracefully shut down (owned PID 18928, exit 0). No agent-owned server remains. Actual chest contents and subsequent looting still need a client check.
