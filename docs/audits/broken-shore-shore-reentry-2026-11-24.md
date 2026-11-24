# Broken Shore shore checkpoint on re-entry

After a server restart, Yim logged in with Broken Shore scenario 786 and no saved transport. The login fallback reconstructed the instance at its stored entrance. The checkpoint still contained LFG 908's deck-local coordinates even though the character had previously logged out on the island. The ship leg consequently started again and disembarked the character after the existing 60-second fallback.

The Alliance and Horde landings now record their existing faction shore coordinates as the entrance after the landing criterion advances map 1460. A new instance uses that shore entrance, suppresses the recurring ship transfer and intro, and dispatches only the native arrival criterion once the player is present. A matching live instance retains the character's position and stage through the ordinary rejoin. Actors and later scenario stages remain transient when an instance is recreated.

Legacy checkpoints qualify only for the matching faction scenario and prior instance and boot, a saved position without a transport, and the exact old LFG deck checkpoint. The saved world position must have nearby static ground and no liquid. Ambiguous positions retain the ordinary ship arrival. The first boarding and normal completion exit retain their existing routes. No character rows were edited by hand.

This active client workflow uses Release compilation, final-link review, installed file hashes and the user's follow-up. Separate regressions and an agent-owned server start were intentionally omitted. Retest a live relog and a restart after reaching shore, with first-trip boarding and Horde behavior as separate paths. The duplicate map-admission log did not cause the fallback entrance selection and remains open.
