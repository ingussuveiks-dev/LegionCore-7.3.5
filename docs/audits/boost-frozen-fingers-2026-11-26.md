# Frozen Fingers on the boost ship

The player observed two Fingers of Frost charges, followed by their disappearance with the lesson still at 0/2. Scenario 1132, criteria tree 48836, tracks casts of the hidden acknowledgement 215880. The local watcher records have no trigger supplying that acknowledgement.

Ice Lance records consumption of aura 44544 on cast. A successful primary hit on training dummy 107104 issues 215880 once for that lance, while the mage and dummy share the ship during this lesson. A pet can supply the charge. Generating it, casting an ordinary lance or hitting a secondary target cannot advance this step. The lesson bar maps the hidden criterion to learned Frozen Orb while retaining Ice Lance and the player's saved layout.

The Release build linked the original game and scripts libraries. The canonical link map identifies Player.obj and spell_mage.obj for the changed methods. No separate automated regression or server startup was run during the user's active client traversal. The new bar and both counter increments await client confirmation.

## Water Elemental commands

The previous runtime logged manual Waterbolt (31707) and Water Jet (135029) commands and Water Jet periodic ticks. The normal spell path resolves SpellXSpellVisual before sending its packets. The manual pet path omitted that selection. It now uses the same selector after successful pet-cast validation with the resolved target, preserving any visual already supplied. The deployed link map identifies Spell.obj for preparePetCast. Waterbolt flight and Water Jet channel rendering await client confirmation.

## Frozen Orb on a transport

Spell 84714 creates trigger 8661. The local client data associates the spell with SpellXSpellVisual 39087 and SpellVisual 38672. The orb was attached as a passenger with its initial offset, while its spline used world positions and its movement never updated that offset. Transport relocation reused the initial offset. Same-transport distance queries also use passenger offsets.

The orb's initial path, on-hit slowdown and published spline now use ship-local points on a transport. Each movement update records the new passenger offset and transforms it to the current world position for map relocation. The native visual, damage and first-hit charge rules are unchanged. Seeing the orb move across the deck and hit the dummy still requires the user's retest.

The final Release link used game/Release/game.lib and scripts/Release/scripts.lib. The installed map resolves AreaTrigger::UpdateSplinePosition to AreaTrigger.obj. Canonical SHA-256 values after offline promotion were `9340D30ED6F647725C1DCAE41F25E469C8678601B90E34FE5FAE777C96A61228` (EXE) and `4DA18753B7E7BB557DF0228EBB198FDC0FEA684AAE86E060B28EDB9F80AF5E15` (map). The preceding runtime logged an orderly shutdown at 12:07, with no warning/error/fatal lines found during the 11:55–12:07 mage observation. The earlier Sheilun diagnostics remain in the original log. Before retest the crash directory held zero files, Server.log was 41614314 bytes and DBErrors.log was empty. No agent-owned server was started.

Removed hash-identical staged worldserver.exe/map and deployment replacement backups under the ignored .codex/frozen-fingers-20261126 directory (540804647 bytes). The previous canonical EXE/map are retained in its rollback subdirectory for this client retest.
