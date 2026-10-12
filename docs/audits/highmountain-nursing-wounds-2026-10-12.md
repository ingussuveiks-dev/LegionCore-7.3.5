# Nursing the Wounds (39318): Skyhorn opening and Totem of Winds

## Confirmed corrections

This is the Skyhorn branch, not Rivermane. Runtime 7.3.5 QuestLineXQuest
records 1075/1077/1076 put 38913, 39318 and 38910 at positions 0/1/2 in
line 93. TDB735 independently supplies the same RewardNextQuest links.
The live addon instead required Bloodtotem's Step into the Dark (40229)
before 39318, and sent 38913 toward A Walk With the Spirits (40515).
The migration restores 38913 -> 39318 -> 38910. The existing independent
38909 prerequisite for 40515 and the 40229 -> 39456 return remain intact.

The Totem of Winds (189182) spell_area row used status mask 66:
COMPLETE and REWARDED, excluding INCOMPLETE. The common production
SpellArea predicate therefore rejected the aura during the quest and
accepted it after reward. This predicate participates in CheckLocation
and quest/area aura maintenance. The corrected mask 10 accepts INCOMPLETE
and COMPLETE, rejecting NONE, FAILED and REWARDED. Autocast remains off:
the player must take Julan's totem. Gossip menu 18894 option 0 now requires
39318 incomplete, also allowing a retry if the active player's aura is lost.

This is a data correction; no replacement spell or generic core workaround
is necessary. Native SpellEffect 276695 gives aura 293 with override data
603, whose first spell is Suppressing Wind 189168. Native 189168 has the
dummy effect used by the existing fire SmartAI. Fire appearance remains
the native Skyhorn Fire aura 188911. The adjacent JSON records archive and
DB2 hashes, quest objectives, links, override data and spell effects.

## Execution audit

- Lasan 93841, unphased spawn 282571 in Skyhorn, ends 38913, starts and
  ends 39318, and offers the next quest 38910.
- Julan 95403, unphased spawn 282501, has the existing three linked gossip
  actions: credit 95403, player self-cast 189182, close gossip. The native
  objective is 280335, storage 1, amount 1.
- 27 unphased Fire Stalker 95261 spawns exist in Skyhorn. Their existing
  SmartAI accepts only 189168, passes the caster as the timed-list invoker,
  gives that invoker credit 95261, hides the fire and restores it after
  ten seconds. The shared ten-second cooldown and existing visuals are
  preserved. Objective 279202 requires eight credits in storage 0.
- Extracted production credit/completion handlers require both Julan and
  eight fires, cap duplicates and reject absent/rewarded quests. Completing
  only either objective does not finish the quest. Rewarding 39318 opens
  38910; merely having it active does not satisfy the positive prerequisite.
- The actual SetQuestUpdate handler retains an acquired aura while
  incomplete/complete and removes it on abandonment, failure or reward
  in Skyhorn. The common predicate rejects other areas; returning with an
  incomplete quest permits manual reacquisition. It does not auto-grant
  the totem after abandonment/reacceptance or after login.

The native objective order puts finding Julan before extinguishing fires.
The interaction supplies both the find credit and the tool required to
extinguish them. The completion regression additionally checks that fire
credits alone cannot bypass finding him.

The [2016 quest playthrough](https://www.youtube.com/watch?v=yR1OpCdlrKA)
is a contemporary external reference. Modern retail level/reward values
were not imported into the 7.3.5 data.

## Validation and limits

- `Test-HighmountainNursingWoundsNative.py`: compares the saved evidence
  with the TDB735 archive and actual runtime DB2 files.
- `Test-HighmountainNursingWoundsData.ps1`: applies the migration twice to
  temporary copies, checks links, masks, gossip, SmartAI, actors and
  objective data, and verifies unrelated rows are preserved. `-Live` is
  read-only.
- `Test-HighmountainNursingWounds.ps1`: compiles the production common
  SpellArea predicate and complete SetQuestUpdate handler using real
  QuestStatus enum values. Reproduces the old mask defect and tests the
  corrected state transitions. Spell-specific branches and world services
  are outside this fixture.
- `Test-HighmountainBloodtotem.ps1`: expanded production prerequisite,
  kill-credit and completion tests cover the independent Skyhorn opening
  and both objectives, alongside the existing Bloodtotem regressions.

These are server/data checks, not a client playthrough. The extra action
button, aiming, fire disappearance/reappearance and full visual sequence
still require the 7.3.5 client. Rocs vs Eagles is checked here only as the
next quest link/objective data; its rescue interaction is a separate audit.
Boost, ship scenario, pet/mover and warlock production paths are untouched.
