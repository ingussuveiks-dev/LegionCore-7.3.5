# Trīs kvestu sākuma avotu pārbaude

Ar servera DB2 lasītāju pārbaudīts 26972 `QuestV2`, `QuestLineXQuest`, `QuestV2CliTask`, `ItemSparse`, `SpellEffect`, `AdventureJournal` un `AdventureMapPOI`. Visiem trim ID pārbaudīti arī pasaules aktieru, objektu, pasākumu, starta skriptu un nākamā kvesta ieraksti. Atrasta pa vienai nodošanas saitei katram ID. [Vietējais kataloga un datubāzes arhīvs](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-closeout/manifest.json)

| Kvests | Pārbaudes rezultāts |
| --- | --- |
| `14097` Get That Merchant Out of Here! | TrinityCore Cataclysm TDB `434.34` tabulā `disables` ir tiešs `Deprecated quest` ieraksts. Legion glabā vēsturisko Gwen Armstead `34936` nodošanas saiti un vietējais ieraksts satur `UNAVAILABLE`. Parastā Gilneas ķēde izmanto `14094` un `14098` kā `14099` priekšnoteikumus. |
| `37474` Basics First | `QuestV2` ir indekss `18373`. `24330` un `26365` izdrukas un vietējais teksts apstiprina nosaukumu un norādi uz Audrey Burnhep. Piedāvātājs, izsaukums un pieejamības prasības joprojām nav apstiprināti. Audrey patstāvīgā petu apmācības ķēde sākas ar `31591` un turpinās ar `31592` un `31821`. [Legion izdruka](https://github.com/TrinityCore/TrinityCore/blob/7f2b7dc9c2165d2608742473a931f55b1c1a753a/sql/old/7/world/01_2018_02_19/2017_07_02_03_world.sql) |
| `41016` BC TL | Vietējais nominālais līmenis ir 100, un mērķis ir 15 `BC Crystal` `133955`. `24330` izdrukā ir seši mērķa kartes punkti Westfall. 26972 `QuestV2` nav šā ieraksta. T.Q. Bret `102735` ir nodošanas saite bez darba novietojuma. Punkti un mērķis apliecina kataloga ierakstu, kura parastais piedāvājums joprojām nav apstiprināts. [BC TL kataloga ieraksts](https://www.wowhead.com/quest=41016/bc-tl) |

TrinityCore WoD `19034` arhīvā jau ir `37474` teksts, nulles minimālais līmenis un norāde doties pie Audrey. Arī šajā avotā piedāvātāja identitāte nav apstiprināta. `37474` sākums paliek atvērts līdz versijai atbilstoša piedāvājuma vai izsaukuma ieraksta atrašanai. [WoD izdruka](https://github.com/TrinityCore/TrinityCore/blob/5be0cf7120ad41c79c51d03e904f95b115581f0e/sql/old/6.x/world/00_2014_10_19/2014_12_29_00_world.sql#L571)
