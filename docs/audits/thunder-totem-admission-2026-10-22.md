# Thunder Totem interactions — 2026-10-22

Maiko (109) heard Mayla's drogbar assault introduction without receiving a task. Native hotfix BroadcastText 107065 and EventObject 309 / conversation 1580 identify this as world quest 41705, Assault on Thunder Totem. It is an outdoor world quest, not an automatic Scenario instance. The existing quest-taken condition was stored under scene source 10, while the dispatcher asks for EventObject source 13. Move that condition to its real source. Do not force a level-110 world quest onto a level-109 character or bypass its active rotation/unlock conditions.

The separate Legion assault introduction (EventObject 368 / conversation 4577, quest 45572 Holding Our Ground) had the same wrong source; retain its objective gate and require the active quest too.

Thunder Totem Natives 117453/117520 use spellclick 233844 and credit 117454, an optional three-rescue objective of 45572. Add matching spellclick admission and actual spell-hit event conditions; neither interaction nor its credit/despawn action list should execute without the quest or after three rescues.

Bolas Bastion 42630 requires 42 Coven attackers (credit 108052). The original launcher 107988 casts controller 214537, whose native summon 214538 creates personal vehicle 108081. Admit both original click and personal vehicle boarding only during the quest. Existing attacker spawns 108052/108060 use phase 5467; phase definition Highmountain 7503/351 belongs to 42630. Preserve their native phase, missile spells 214357/214476 -> 214473/214479 and target-entry filters.

Automated: Test-ThunderTotemAdmission.ps1 runs the migration twice on temporary table copies, then executes the native SmartEvent lookup and quest/objective condition branches. It covers no quest, active/completed/rewarded states, both launcher entries, both rescue entries, all rescue-count boundaries and both conversation sources. Live character state is untouched.

Pending client checks: click flag refresh when accepting/finishing/abandoning; Bolas targets, both projectile hits and full 42-credit turn-in; personal rescue visuals/despawn; conversation when the corresponding world quest is actually active. Correct source admission is not proof of a complete world quest or either assault finale.

Deployment: targeted rows backed up locally outside Git; migration applied to MariaDB with no worldserver process running. Canonical startup accepted the migrated scripts/conditions with empty DBErrors.log and no ERROR/WARN in the captured startup. The owned hidden validation server was gracefully stopped.
