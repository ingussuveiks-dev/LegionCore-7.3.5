# Highmountain quest repairs: deployed Release evidence

Canonical runtime: `build-extractors/bin/Release`.

Final worldserver SHA256: `8632882ccdcb5afe19dd1ba29f6e8a7b21aedcaec14c23e7cebf554be35d8c9d`.

Focused implementation commits: journal `00bda9f1`, quest details `c68d075a`, mover handoffs `00909b4c`, Thunder Totem admission `8883cbb7`, Addie hunting `821df731`. Runtime configurations, DLLs, certificates and extracted data were preserved. Targeted world-table and previous executable backups are local and excluded from Git; no personal quest progress was reset.

Release libraries were built at their original generated paths. Only the final worldserver link used a staging OutDir with BuildProjectReferences=false; the checked executable/map were copied to the canonical runtime. The revision banner can retain the earlier pre-commit `502402d3+` metadata; that banner/startup alone is not used to establish implementation identity.

Automated checks passed:

- Test-AdventureMapGossip.ps1: actual journal preview/start handlers and existing class-hall board/service admission.
- Test-QuestDetailsDelivery.ps1: single serialization/send, without an appended duplicate body.
- Test-WarlockRegression.ps1: issued mover handoffs, timeout/unrelated GUID rejection and existing Warlock cases.
- Test-PetLoadSelection.ps1 and Test-AuraScriptReentry.ps1.
- Test-ThunderTotemAdmission.ps1: native SmartEvent/quest/objective admission and twice-replayed temporary migration.
- Test-AddieHunting.ps1: native event-quest completion gate, persistent training, owner/pet kill dispatch, skill quantities, retry/cleanup and twice-replayed temporary migration.
- Test-ScepterLogin.ps1 -VerifyReleaseBinary.
- verify_highmountain_quest_release.py against the final canonical executable: actual ButtonActionType offset, journal acceptance, one QuestDetails::Write call, native objective validation/progression/actor calls and bounded mover expiry. Its shared PE verifier also checks the ten existing scenario completion/persistence call paths.

The final owned hidden server (PID 27064) reached ready, admitted the migrated database scripts/conditions without ERROR/WARN or DBErrors, then exited gracefully with code 0. No agent-owned server remains. Existing user servers were already stopped before deployment and were not stopped by the agent.

Client verification is still required for both Adventure Guide buttons, quest-window closure, Addie follow/bar/voices/full turn-in, Bolas targets/projectiles, native rescue and vehicle/camera/pet behavior. Krau's separate Kill Command lesson remains unresolved pending the requested client action-slot/known-spell/cooldown/focus screenshot. The intermittent first instance socket initialization failure is also not claimed fixed by these changes.
