# Warlock mission follow-up and research books, 2026-10-21

The user showed an empty class-hall mission board after the first tutorial
mission, with Information at Any Cost ready for turn-in. A later read-only
snapshot showed Recruiting the Troops 41797 active. No follower, mission, quest,
currency or item state was edited for this inquiry.

## Campaign follow-up

The existing world mission_quest_link and native catalog still contain:

- 42603 Information at Any Cost -> mission 827, two minutes.
- After its turn-in: 41797 Recruiting the Troops, through Gakin and Dyala.
- 42602 Troops in the Field -> mission 928, one hour.
- After the research tutorial: 42097 Searching the Archives -> mission 839,
  one hour.

Thus an empty board after mission 827 is not the end of the campaign. The next
campaign mission is attached to a later quest, rather than being offered simply
because the player has opened the board again. Existing native progression
checks and their client limitations are in warlock-progression-availability-
2026-10-21.md; actual troop/research/mission interactions remain client checks.

Subsequent client evidence exposed duplicate Dyala options and an inverted
tutorial/normal shipment selector. Those are repaired and covered by native
lifecycle checks in [the troop audit](warlock-troop-training-2026-10-21.md).

The core also has a separate ordinary mission generator. It filters by follower
level/item level/count, player conditions, mission category and reward data.
Campaign and unsupported quest mission categories are excluded from this random
pool. A DAY-based periodic timer is present, along with other generation call
sites. The saved generator time in the read-only snapshot corresponded to
2026-10-22 13:42:23 Riga time (database FROM_UNIXTIME displayed UTC). This audit
does not certify every random mission's availability, refill timing or reward
path from SQL presence or the tutorial screenshot. No generator change was made.

## Obsolete books

Mile Raitheborne 111740 currently sells:

- 147853 Artifact Research Compendium: Volumes I-VI.
- 147854 Artifact Research Compendium: Volumes I-VII.

Both vendor rows use ExtendedCost 6075; the screenshot shows 1000 Order Resources
each. The native sparse ItemSparse reader confirms the obsolete-description
text, and the complete ItemEffect table contains zero effects for either item.
These particular books cannot grant Artifact Power or Artifact Knowledge through
an item-use spell. They remain in the vendor list, but purchasing them does not
provide the old research benefit. No vendor rows were changed by this inquiry.

Blizzard's official 7.3 notes state that Artifact Knowledge advances automatically
weekly for everyone, without research work orders:
https://worldofwarcraft.blizzard.com/en-us/news/20947822 .
This supports the historical system explanation; the zero-effect native records
are the evidence for the two local items. The server's global knowledge world
state 20023 was 46. That is not asserted to be the level-106 character's stored
personal currency: the local code treats below-110 and max-level synchronization
separately.

Reproduce the native item audit with:
`tools/research/Read-LegionLegendaryCatalog.ps1 -SourceFile WarlockMissionBooksCatalog.cpp -ProbeName WarlockMissionBooksCatalog`.
The identical probe was compiled/executed during investigation and read the
canonical 7.3.5 runtime DB2 files; no fixed-offset sparse reader was used.
