param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-new-beginning"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_10_stormheim_new_beginning.sql" -Raw
$sql=''
if(!$Live){
 $sql=@'
CREATE TEMPORARY TABLE saved_creature_template LIKE creature_template;
INSERT INTO saved_creature_template SELECT * FROM creature_template WHERE entry IN(95804,94228);
UPDATE saved_creature_template SET ScriptName='' WHERE entry=95804;
CREATE TEMPORARY TABLE creature_template LIKE saved_creature_template;
INSERT INTO creature_template SELECT * FROM saved_creature_template;
CREATE TEMPORARY TABLE saved_smart_scripts LIKE smart_scripts;
INSERT INTO saved_smart_scripts SELECT * FROM smart_scripts WHERE entryorguid=94228 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart_scripts;
INSERT INTO smart_scripts SELECT * FROM saved_smart_scripts;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_param1,action_type,action_param1,target_type,target_x,target_y,target_z,comment) VALUES
(95804,0,0,1,19,39122,85,190073,7,0,0,0,'cast'),
(95804,0,1,2,61,0,33,95269,7,0,0,0,'first credit'),
(95804,0,2,3,61,0,33,94431,7,0,0,0,'final credit'),
(95804,0,3,0,61,0,62,1220,7,2851.9,863.39,77.8,'camp'),
(95804,0,0,0,62,18843,62,1220,7,2476.6,935.151,596.14,'stray Vetir');
'@
 $sql+=$migration+"`n"+$migration+"`n"
}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39092,39122) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=39122 ORDER BY StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=39122;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=39122;
SELECT 'genn',entry,npcflag,gossip_menu_id,AIName,ScriptName FROM creature_template WHERE entry=95804;
SELECT 'legacy',COUNT(*) FROM smart_scripts WHERE entryorguid=95804 AND source_type=0;
SELECT 'horde',COUNT(*) FROM smart_scripts WHERE entryorguid=94228 AND source_type=0;
SELECT 'other-menu',MenuID,OptionID,OptionBroadcastTextID FROM gossip_menu_option WHERE MenuID=18843;
SELECT 'other-condition',ConditionTypeOrReference,ConditionValue1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18843;
SELECT 'position',guid,id,map,position_x,position_y,position_z FROM creature WHERE id IN(95804,95932) ORDER BY id;
SELECT 'spellarea',spell,area,quest_start,quest_start_status,quest_end_status FROM spell_area WHERE quest_start=39122;
SELECT 'spell-binding',COUNT(*) FROM spell_script_names WHERE spell_id IN(190073,229076);
SELECT 'spell-linked',COUNT(*) FROM spell_linked_spell WHERE spell_trigger=190073 OR spell_effect=190073;
'@
if(!$Live){$sql+=@"
SELECT 'horde-preserved',(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=94228 AND source_type=0)=(SELECT COUNT(*) FROM saved_smart_scripts);
UPDATE creature_template SET ScriptName='' WHERE entry=95804;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(95804,0,9,1,72,7,'custom');
$migration
SELECT 'custom',ScriptName='' FROM creature_template WHERE entry=95804;
SELECT 'custom-row',COUNT(*) FROM smart_scripts WHERE entryorguid=95804 AND id=9;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'New Beginning World query/replay failed'}}finally{$env:MYSQL_PWD=$old}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
foreach($r in @("quest`t39122`t35651592`t0`t824181837`t0`t0`t39092`t0`t0`t0`t754","objective`t39122`t278952`t0`t95269`t1`t1`t0","objective`t39122`t278954`t0`t94431`t1`t2`t2","starter`t95804`t39122","ender`t95932`t39122","genn`t95804`t2`t0`tSmartAI`tnpc_stormheim_new_beginning_genn","legacy`t0","other-menu`t18843`t0`t99941","other-condition`t9`t40005","spellarea`t100044`t7736`t39122`t8`t74","spell-binding`t0","spell-linked`t0")){if($rows -notcontains $r){throw "New Beginning contract changed: $r"}}
if(!$Live){foreach($r in @("custom`t1","custom-row`t1","horde-preserved`t1")){if($rows -notcontains $r){throw 'Custom Genn or unrelated Horde behavior overwritten'}}}
$sql="SELECT 'effect',COUNT(*) FROM spell_effect WHERE SpellID IN(190073,229076); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(190073,229076); SELECT 'movie',COUNT(*) FROM movie WHERE ID=494;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$h=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'New Beginning Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
$h=@($h | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$h | Set-Content "$out/hotfix.tsv"
foreach($kind in @('effect','misc','movie')){if($h -notcontains "$kind`t0"){throw "Unexpected native $kind override"}}
'PASS:39122 goals/reward754/relations, scoped Genn replacement, repeat/custom protection and unchanged Horde/Vetir paths; no native movie/effect overrides.'
