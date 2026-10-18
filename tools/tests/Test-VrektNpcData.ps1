param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Vrekt NPC SQL check failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_13_vrekt_rotation.sql" -Raw
$rows=@(Query @"
CREATE TEMPORARY TABLE saved_ct LIKE creature_template;
INSERT INTO saved_ct SELECT * FROM creature_template WHERE entry IN(115846,116180,116600,116601,115025);
CREATE TEMPORARY TABLE creature_template LIKE saved_ct;
INSERT INTO creature_template SELECT * FROM saved_ct;
UPDATE creature_template SET ScriptName='' WHERE entry IN(115846,116180,116600,116601);
UPDATE saved_ct SET ScriptName='npc_vrekt' WHERE entry IN(115846,116180,116600,116601);
SELECT 'expected',c.* FROM saved_ct c ORDER BY entry;
$migration
SELECT 'first',c.* FROM creature_template c ORDER BY entry;
$migration
SELECT 'repeat',c.* FROM creature_template c ORDER BY entry;
UPDATE creature_template SET ScriptName='external_sentinel' WHERE entry=115846;
UPDATE creature_template SET AIName='EventAI',ScriptName='' WHERE entry=116180;
$migration
SELECT 'foreign-script',COUNT(*) FROM creature_template WHERE entry=115846 AND ScriptName='external_sentinel';
SELECT 'foreign-ai',COUNT(*) FROM creature_template WHERE entry=116180 AND ScriptName='' AND AIName='EventAI';
"@)
function Tagged($tag){@($rows | Where-Object {$_ -like "$tag`t*"} | ForEach-Object {($_ -split "`t",2)[1]})}
if(((Tagged 'expected') -join '|') -ne ((Tagged 'first') -join '|') -or ((Tagged 'first') -join '|') -ne ((Tagged 'repeat') -join '|') -or "foreign-script`t1" -notin $rows -or "foreign-ai`t1" -notin $rows){throw 'NPC migration changed other template fields/neighbour or external binding'}
if($Live){
 $liveRows=@(Query "SELECT entry,IF(AIName='','-',AIName),ScriptName FROM creature_template WHERE entry IN(115846,116180,116600,116601) ORDER BY entry;")
 $expected=@("115846`t-`tnpc_vrekt_scene","116180`t-`tnpc_vrekt","116600`t-`tnpc_vrekt","116601`t-`tnpc_vrekt")
 if(($liveRows -join '|') -ne ($expected -join '|')){throw 'Live actor binding differs from retained Aggressor selection'}
}
$spawns=@(Query 'SELECT id,COUNT(*) FROM creature WHERE id IN(115846,116180,116600,116601) AND map=1220 AND zoneId=7541 AND areaId=8362 GROUP BY id ORDER BY id;')
if(($spawns -join '|') -ne "115846`t1|116180`t6|116600`t10|116601`t16"){throw 'Vrekt actor inventory changed; re-audit scope'}
'PASS: repeated scoped script binding changes only ScriptName; AIName/faction/flags/loot/spells/neighbour/external scripts preserved,33 live Vrekt DB spawns retain their original identities.'
