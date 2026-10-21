$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_21_00_warlock_mutual_friend_offer.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_spell_area LIKE spell_area;
INSERT INTO saved_spell_area SELECT * FROM spell_area;
CREATE TEMPORARY TABLE spell_area LIKE saved_spell_area;
INSERT INTO spell_area SELECT * FROM saved_spell_area;
UPDATE spell_area SET quest_end_status=74 WHERE spell=224338 AND area=7581 AND quest_start=40824 AND quest_end=44099;
$migration
$migration
SELECT 'offer',COUNT(*) FROM spell_area WHERE spell=224338 AND area=7581 AND quest_start=40824 AND quest_start_status=64 AND quest_end=44099 AND quest_end_status=1 AND classmask=256 AND autocast=1;
SELECT 'other-offers-changed',COUNT(*) FROM spell_area a JOIN saved_spell_area b USING(spell,area,quest_start,quest_end) WHERE a.spell<>224338 AND a.quest_end_status<>b.quest_end_status;
SELECT 'native-giver',COUNT(*) FROM creature_queststarter q JOIN creature_template c ON c.entry=q.id WHERE q.id=112693 AND q.quest=44099 AND c.AIName='SmartAI' AND (c.npcflag & 2)<>0;
SELECT 'accept-despawn',COUNT(*) FROM smart_scripts WHERE entryorguid=112693 AND source_type=0 AND event_type=19 AND event_param1=44099 AND action_type=41 AND action_param1=5000;
SELECT 'follow',COUNT(*) FROM smart_scripts WHERE entryorguid=112693 AND source_type=0 AND event_type=54 AND action_type=29 AND target_type=7;
SELECT 'personal',COUNT(*) FROM smart_scripts WHERE entryorguid=112693 AND source_type=0 AND action_type=207 AND target_type=7;
SELECT 'next-quest',COUNT(*) FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=42608 AND q.MinLevel=101 AND a.PrevQuestID=44099;
UPDATE spell_area SET quest_end_status=33 WHERE spell=224338 AND area=7581 AND quest_start=40824 AND quest_end=44099;
$migration
SELECT 'custom-preserved',COUNT(*) FROM spell_area WHERE spell=224338 AND area=7581 AND quest_start=40824 AND quest_end=44099 AND quest_end_status=33;
"@
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$prior=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$db[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Mutual Friend temporary SQL fixture failed'}
 $expected=@("offer`t1","other-offers-changed`t0","native-giver`t1","accept-despawn`t1","follow`t1","personal`t1","next-quest`t1","custom-preserved`t1")
 if(($rows -join '|') -ne ($expected -join '|')){throw "Unexpected Mutual Friend fixture: $($rows -join '|')"}
} finally {$env:MYSQL_PWD=$prior}
'PASS: repeated scoped migration, native giver/flags/follow/personal/accept cleanup, level-101 follow-up and custom/other-class preservation.'
& "$PSScriptRoot/../research/Read-LegionLegendaryCatalog.ps1" -SourceFile MutualFriendCatalog.cpp -ProbeName MutualFriendCatalog
