# Dark Revelations return landing

The 7.3.5 client automatically boarded the player's Stellagosa after the Nightglaive kill and reached Illidari Stand. The user remained on the drake there and later reported falling through the scenery after logging back in.

The five-minute fallback called `Creature::DespawnOrUnsummon(300000)` at publication. That sets the despawn flag immediately, so the SmartAI force-despawn actions at native route 90982 point 7 or passenger removal could never replace the timer. The default native vehicle exit also targeted the flying creature's position. The final waypoint is 2.72 yards above the extracted terrain. The route ends within range of the existing return actor at Illidari Stand.

The ride AI maintains the five-minute fallback without preempting the native despawn actions. It resolves a phased ground position below the actual final waypoint, guides the verified seat-one owner out through the native exit spline and then leaves the existing despawn actions to remove the drake. The boarding callback retains a ground position at the start for the ordinary logout save. Actual endpoint arrival replaces it with the resolved landing position. The save hook covers the map-detached logout path and respects unrelated pending teleports and death recovery. Native path, conversation, boarding and arrival credit remain in their existing paths.

The generated Release scripts library recompiled the module. The separate non-incremental final link used the original Release game and scripts libraries. Its map shows the scoped exit position, ride update, companion cleanup, before-logout save and existing quest dialog implementation. Prepared worldserver SHA-256 `0FECAEC722DC24CC9825F8108C4EEEFE81BC711368C0FA11A65AE1F62A3AB495`. At inspection there were no canonical crash files and DBErrors.log was empty. No separate server, PE verifier or automated gameplay test was run under the user-led gameplay workflow. Release deployment and client landing/relog confirmation are pending.

An independent first-login failure and the earlier re-enqueued packet messages are tracked in the same day's [connection audit](instance-login-handshake-2026-11-25.md).
