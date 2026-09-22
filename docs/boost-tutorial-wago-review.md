# Wago scene and boost reference investigation

Date: 2026-09-22. Build: **7.3.5.26972**.

Read-only investigation. No database import, server changes, client changes,
or character relocation. Downloaded scene scripts were treated only as data,
never executed.

## Reproduction and provenance

Run `tools/research/Read-WagoBoostScenes.ps1`. It downloads/caches eleven CSV
tables from `https://wago.tools/db2/<Table>/csv?build=7.3.5.26972` and writes
`boost-tutorial-wago-evidence.json`. Each source has a row count and SHA256.
Cache: `%TEMP%/legion-wago-735-scenes`. Delete or use a different cache folder
when a fresh remote snapshot is required; the script normally reuses its cache.

The HTML page's embedded `currentVersion` was independently checked and was
`7.3.5.26972`. The web search tool could not render Wago, but direct HTTPS
requests returned valid CSV and the build-specific page metadata.

## Results

### Tutorial steps: independent agreement

Wago's scenario1084 is Combat Training, Flags2, Type5. All14 Warlock steps
match the local extracted DB2 evidence by ID, title, description, ScenarioID,
Supersedes, RewardQuestID, OrderIndex, Flags, CriteriaTreeID and RelatedStep.
This comparison does not independently recheck every descendant criteria tree.

### Transport paths: coordinates exist, but not NPC placement

| TaxiPathID | Continent/map | Node count |
| --- | --- | --- |
| 5528 | 1554, Alliance tutorial | 14 |
| 5529 | 1557, Horde tutorial | 14 |
| 5950 | 1554 | 14 |
| 5951 | 1557 | 14 |

Full selected nodes are in the JSON. These are transport route nodes, not
trainer spawn coordinates or transport-local boarding positions. Alternative
paths5950/5951 must not be assumed active without server evidence.

### Exit vehicle: historical client support confirmed

Vehicle4933 exists in this7.3.5 export, with SeatID_0=16967 and other seat IDs0.
Seat16967 exists: AttachmentID21, EnterSpeed7, ExitPreDelay10, ExitSpeed7.
The modern TDB's VehicleID4933 is therefore not inherently a modern-only ID.
This does **not** independently prove that tutorial NPC107543/112565 used it
in7.3.5, or validate the local runtime NPC/vehicle binding and boarding behavior.

### Spell chain agrees with local evidence

The six effects of219912 are FORCE_CAST140, triggering227058,215387,215598,
215607,230503,231511.227058 has dummy effect3, not a teleport effect with a
destination in this row.219615 and223763 also have dummy effects.

Eleven overlapping effect records were compared with
`boost-tutorial-spell-evidence.json`: no differences in Effect, EffectIndex,
EffectAura and EffectTriggerSpell. This is not a full-field comparison.

The ordinary faction introduction spells differ:216356 references scene1335
through EffectMiscValue_0;225147 references1439. The existing source describes
scene1335/package1661 in its packet example. The boost exit must not simply
be replaced with this ordinary introduction scene on the basis of its name.

### Scene graph investigation

Downloaded5969 SceneScriptText rows,5969 SceneScript rows,1378 packages,
6919 package members and20 SceneScriptGlobalText rows. Traversed child
packages and NextSceneScriptID chains for12 Broken Shore/introduction packages:
1523,1531,1661,1666,1677,1678,1691,1711,1712,1715,1716,1723.
No unresolved package/script/text references were found in those traversals.

The two pre-launch departure packages are:

-1661, Stormwind: main script14229 and sound script14659, plus shared framework.
-1715, Orgrimmar: main script14478 and sound script14660, plus shared framework.

The main scripts place visual ships/fliers at the faction departure areas,
move them along cinematic paths, fade and end the scene. They do not provide
Zakgra/Hughes tutorial placement. Stormwind uses actors108984/108989/108990;
Orgrimmar uses113221/113291/113290, not the tutorial exit NPC IDs.

No `enterBrockenShores` text occurred in the complete SceneScriptText export.
The server-side name therefore cannot be treated as a recovered client event
string. Shared framework trigger helper functions do not prove tutorial logic.

The exact-ID/name search for105317,100448,112565,107543,219615,219912,227058,
230503,231511, Zakgra, Hughes, class trial and combat training found no relevant
tutorial script. The broad word `boost` produced five false positives:
13624,13625,13626,13627,13637, all Shaman artifact XP visual effects. The global
text search had no hits. This is a scoped semantic search, not proof that no
unnamed/indirect tutorial-related visual script exists anywhere.

## What remains missing

Wago provides valuable client reference data and strengthens the existing
DB2 evidence. It has not supplied the server-side implementation or a verified
full NPC layout: trainer/dummy/sparring spawns, phases, surrender rules,
wave scheduling, stage event dispatch, reconnect recovery and exit handoff.
Do not enable automatic tutorial relocation based on these findings alone.

Next useful evidence would be tutorial packet captures, a compatible scripted
implementation with its SQL, or explicit authorization for a custom reconstruction
whose invented timings/placement are clearly distinguished from authentic data.
