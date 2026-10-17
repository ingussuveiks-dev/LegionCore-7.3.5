param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-undeath"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_11_stormheim_undeath.sql" -Raw
$sql=''
if(!$Live){$sql=@'
CREATE TEMPORARY TABLE saved_creature_template LIKE creature_template;
INSERT INTO saved_creature_template SELECT * FROM creature_template WHERE entry IN(94228,95804);
UPDATE saved_creature_template SET ScriptName='' WHERE entry=94228;
CREATE TEMPORARY TABLE creature_template LIKE saved_creature_template;
INSERT INTO creature_template SELECT * FROM saved_creature_template;
CREATE TEMPORARY TABLE saved_smart_scripts LIKE smart_scripts;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart_scripts;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_param1,action_type,action_param1,target_type,target_x,target_y,target_z,comment) VALUES
(94228,0,0,1,19,38882,85,205551,7,0,0,0,'cast'),
(94228,0,1,2,61,0,33,94915,7,0,0,0,'first'),
(94228,0,2,3,61,0,33,94431,7,0,0,0,'final'),
(94228,0,3,0,61,0,62,1220,7,2370.01,1849.31,29.17,'camp');
'@;$sql+=$migration+"`n"+$migration+"`n"}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39155,38882) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=38882 ORDER BY StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=38882;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=38882;
SELECT 'sylvanas',entry,npcflag,gossip_menu_id,AIName,ScriptName FROM creature_template WHERE entry=94228;
SELECT 'legacy',COUNT(*) FROM smart_scripts WHERE entryorguid=94228 AND source_type=0;
SELECT 'alliance',ScriptName FROM creature_template WHERE entry=95804;
SELECT 'position',guid,id,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id IN(94228,95755) ORDER BY guid;
SELECT 'spell-binding',COUNT(*) FROM spell_script_names WHERE spell_id=205551;
SELECT 'spell-linked',COUNT(*) FROM spell_linked_spell WHERE spell_trigger=205551 OR spell_effect=205551;
SELECT 'spell-scripts',COUNT(*) FROM spell_scripts WHERE id=205551;
'@
if(!$Live){$sql+=@"
UPDATE creature_template SET ScriptName='' WHERE entry=94228;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(94228,0,9,1,72,7,'custom');
$migration
SELECT 'custom',ScriptName='' FROM creature_template WHERE entry=94228;
SELECT 'custom-row',COUNT(*) FROM smart_scripts WHERE entryorguid=94228 AND id=9;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Undeath World query/replay failed'}}finally{$env:MYSQL_PWD=$old}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
foreach($r in @("quest`t38882`t35651592`t0`t234881970`t0`t0`t39155`t0`t0`t0`t751","objective`t38882`t278826`t0`t94431`t1`t0`t2","objective`t38882`t279420`t0`t94915`t1`t2`t0","starter`t94228`t38882","ender`t95755`t38882","sylvanas`t94228`t2`t0`tSmartAI`tnpc_stormheim_undeath_sylvanas","legacy`t0","alliance`tnpc_stormheim_new_beginning_genn","spell-binding`t0","spell-linked`t0","spell-scripts`t0")){if($rows -notcontains $r){throw "Undeath contract changed: $r"}}
if(!$Live){foreach($r in @("custom`t1","custom-row`t1")){if($rows -notcontains $r){throw 'Custom Sylvanas behavior overwritten'}}}
$sql="SELECT 'effect',COUNT(*) FROM spell_effect WHERE SpellID=205551; SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID=205551; SELECT 'movie',COUNT(*) FROM movie WHERE ID=495;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$h=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Undeath Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
$h=@($h | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$h | Set-Content "$out/hotfix.tsv"
foreach($kind in @('effect','misc','movie')){if($h -notcontains "$kind`t0"){throw "Unexpected native $kind override"}}
'PASS:38882 goals/reward751/relations, scoped Sylvanas replacement, repeat/custom protection, unchanged Alliance binding and native movie495 data.'
