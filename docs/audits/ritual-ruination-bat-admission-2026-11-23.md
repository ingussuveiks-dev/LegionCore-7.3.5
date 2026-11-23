# Ritual Ruination Fel Bat interaction after turn-in

The user saw the riding cursor on the Fel Bat at Krasus' Landing after completing Ritual Ruination. The map 1220 actor is spawn `376568`, entry `110479`, in phase `6961`. The phase is deliberately present while the earlier quest `43153` is active, complete or rewarded. Its other actor is Calydus. The bat's SpellClick row advertises `219917` through the NPC flag. There was no source-18 filter, so the client could show the ride even when the spell script rejected the player. Readback showed that the reported character had already been rewarded for `43254`, with no saved taxi path.

Migration `2026_11_23_06` adds source-18 conditions for that spell and entry. The native object update consults these conditions per viewer, and the server checks them again when handling the click. They admit a Warlock with an incomplete or unrewarded complete `43254` before the real final return credit `110607`. The complete branch preserves recovery for an old complete entry with missing return credit. The final objective is sequenced behind the other five. The native cast still checks live position, phase, vehicle state and quest stage. The phase, Calydus, NPC flags at the template level, the travel spell and the ordinary flight master all remain intact.

The user-owned worldserver was running during diagnosis. The migration is queued for the next startup. No client retest or separate test was run. Verify the riding cursor on a fresh eligible attempt and its absence after return or reward.

After shutdown, the six spell-click filters were read back in MariaDB and committed with updater receipt `623B44035CE2F71165DF1DBCCB3371D357B8F87E`. The next worldserver process will load them.

After the user started the updated canonical server on 23 November, the user confirmed the Fel Bat no longer offered boarding for the character who had already redeemed quest `43254`. The observed run had no new DBErrors or crashes. An eligible character's initial boarding and active-quest recovery were not retested.
