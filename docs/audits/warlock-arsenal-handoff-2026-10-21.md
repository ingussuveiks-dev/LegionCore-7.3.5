# Missing final artifact selection breadcrumb, 2026-10-21

The user asked whether hiding the map marker repaired a skipped stage for new
players. The earlier fixes repaired gossip/choice/map predicates only. They
did not restore quest 44089's missing creature_queststarter relation. The quest
was neither active nor rewarded in the reported character history, even though
40623, 42125 and 43254 were rewarded and all three weapons acquired. Return
quests 40712/41156 also were not rewarded; this fix does not grant them or claim
every return handoff has been client-verified.

Native QuestLine 274 orders 40684 (0), 43984 (1), 44089 (2). The world had Calydus
101097 as 44089's ender but no starter, and no accept events to open the artifact
choice for either later breadcrumb. Because the generic artifact gossip was
independent, acquisition could proceed without recording the final selection
breadcrumb. Thus the user's history gap was real, while it did not mean an
artifact acquisition combat stage still needed repeating.

Migration 2026_10_21_05 restores Calydus as 44089's starter, adds the preceding
breadcrumb prerequisites to the two formerly-zero PrevQuestID fields, and
opens native choice spell 199411 on accepting 43984/44089. The accepting Player
is both caster and target. Source-19 unfinished-route/class conditions also
retire 43984 after all acquisitions; 44089 retains the prior map repair.
The native spell reader confirms 199411 effect 205 launches choice 245.

The breadcrumbs guide selection; this change does not make their rewards a
universal requirement for every artifact gossip/retry. A player can still use
the independent gossip path. Do not claim that every new character is forced
to reward every optional selection breadcrumb or that a full artifact replay
was tested. No existing character is auto-awarded/reset/replayed to fill history.

Test-WarlockProgression.ps1 repeats migration 05 in temporary tables and checks
the actual starter and accept rows. Its native prerequisite/race/class/level
and condition fixtures cover before introduction, second selection, before
the final breadcrumb prerequisite, two acquired weapons, another class, low
level and all three acquired. Existing campaign/zone/110 admission checks pass.
The 64-case artifact choice/map tests pass; the native reader validates the
three map records, preceding line entries and choice spell.

Live SQL was backed up and applied transactionally. Starter 101097/44089,
prerequisites 40684 -> 43984 -> 44089, two accept rows and the additional fifteen
conditions were verified. The shared canonical startup check loaded these
rows without DBErrors and shut down the owned server; deployment evidence is
in [the troop audit](warlock-troop-training-2026-10-21.md).

Actual new-character acceptance, delayed self-cast/choice display, reward and
subsequent acquisition are client-pending. SQL admission/startup and native
prerequisite tests do not verify that complete interaction in the client.
