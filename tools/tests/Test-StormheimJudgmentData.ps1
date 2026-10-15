param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-judgment"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_37_stormheim_judgment.sql" -Raw
function Query([string]$kind,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${kind}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';')
 $old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Judgment SQL check failed'};return $rows}finally{$env:MYSQL_PWD=$old}
}
$sql=@"
SELECT 'objective',ID,ObjectID,StorageIndex,Type,Amount,Flags FROM quest_objectives WHERE QuestID=38811 ORDER BY StorageIndex;
SELECT 'bowls',COUNT(*) FROM gameobject_template WHERE entry IN(241864,241868,241869) AND type=10 AND displayId=14638 AND flags=262148 AND Data0=1691 AND Data1=38811 AND Data3=6000 AND Data10=0;
SELECT 'spawns',guid,id,map,zoneId,phaseMask,position_x,position_y,position_z,spawntimesecs FROM gameobject WHERE id IN(241864,241868,241869) ORDER BY id;
SELECT 'rulers',t.entry,t.faction,t.unit_flags,t.npcflag,w.Displayid1,s.LevelScalingMin,s.LevelScalingMax FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry JOIN creature_template_scaling s ON s.Entry=t.entry WHERE t.entry IN(92889,92918,92920) ORDER BY t.entry;
SELECT 'models',COUNT(*) FROM creature_model_info WHERE DisplayID IN(63347,63369,63370,328) AND BoundingRadius>0 AND CombatReach>0;
SELECT 'texts',COUNT(*) FROM creature_text WHERE (CreatureID=92889 AND GroupID IN(0,1) AND BroadcastTextID IN(95185,95188)) OR (CreatureID=92918 AND GroupID IN(0,1) AND BroadcastTextID IN(95192,95193)) OR (CreatureID=92920 AND GroupID=0 AND BroadcastTextID=95196);
SELECT 'legacy',COUNT(*) FROM smart_scripts WHERE (source_type=1 AND entryorguid IN(241864,241868,241869)) OR (source_type=9 AND entryorguid IN(24186400,24186800,24186900)) OR (source_type=0 AND entryorguid IN(92889,92918,92920));
SELECT 'summon',entryorguid,event_param1,action_param1,target_x,target_y,target_z,target_o FROM smart_scripts WHERE source_type=9 AND entryorguid IN(24186400,24186800,24186900) AND action_type=12 ORDER BY entryorguid;
SELECT 'incense-source',COUNT(*) FROM smart_scripts WHERE source_type=9 AND entryorguid=3584500 AND action_type=11 AND action_param1=184831;
SELECT 'helper',ScriptName FROM creature_template WHERE entry=35845;
SELECT 'later-bowl',COUNT(*) FROM gameobject_template WHERE entry=241877 AND type=10 AND Data1=38815 AND ScriptName='';
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=93231 AND quest=38811;
SELECT 'ender-next',COUNT(*) FROM creature_questender e JOIN creature_queststarter s ON s.id=e.id WHERE e.id=93446 AND e.quest=38811 AND s.quest=38817;
SELECT 'handoff',q.ID,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38811,38817) ORDER BY q.ID;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry IN(241864,241868,241869,92889,92918,92920)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry=38811);
"@
if($Live){$sql+=@"
SELECT 'installed-opening',COUNT(*) FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_judgment_opening';
SELECT 'installed-go',COUNT(*) FROM gameobject_template WHERE entry IN(241864,241868,241869) AND AIName='' AND ScriptName='go_stormheim_judgment';
SELECT 'installed-king',COUNT(*) FROM creature_template WHERE entry IN(92889,92918,92920) AND AIName='' AND ScriptName='npc_stormheim_judgment_king';
"@}else{$sql+=@"
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
INSERT INTO saved_bindings SELECT * FROM spell_script_names WHERE spell_id=3365;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
INSERT INTO spell_script_names SELECT * FROM saved_bindings;
DELETE FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_judgment_opening';
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry IN(241864,241868,241869,241877);
CREATE TEMPORARY TABLE saved_king LIKE creature_template;
INSERT INTO saved_king SELECT * FROM creature_template WHERE entry IN(92889,92918,92920,35845);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE creature_template LIKE saved_king;
INSERT INTO creature_template SELECT * FROM saved_king;
UPDATE gameobject_template SET AIName='SmartGameObjectAI',ScriptName='' WHERE entry IN(241864,241868,241869);
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(92889,92918,92920);
$migration
$migration
UPDATE saved_go SET AIName='',ScriptName='go_stormheim_judgment' WHERE entry IN(241864,241868,241869);
UPDATE saved_king SET AIName='',ScriptName='npc_stormheim_judgment_king' WHERE entry IN(92889,92918,92920);
SELECT 'opening',COUNT(*) FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_judgment_opening';
SELECT 'go-actual',g.* FROM gameobject_template g ORDER BY entry;
SELECT 'go-expected',g.* FROM saved_go g ORDER BY entry;
SELECT 'king-actual',g.* FROM creature_template g ORDER BY entry;
SELECT 'king-expected',g.* FROM saved_king g ORDER BY entry;
UPDATE gameobject_template SET AIName='SmartGameObjectAI',ScriptName='' WHERE entry IN(241864,241868,241869);
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(92889,92918,92920);
DELETE FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_judgment_opening';
UPDATE gameobject_template SET ScriptName='custom_bowl' WHERE entry=241868;
$migration
SELECT 'custom-opening',COUNT(*) FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_judgment_opening';
SELECT 'custom-go',COUNT(*) FROM gameobject_template WHERE entry IN(241864,241868,241869) AND AIName='SmartGameObjectAI' AND ScriptName IN('','custom_bowl');
SELECT 'no-partial-kings',COUNT(*) FROM creature_template WHERE entry IN(92889,92918,92920) AND AIName='SmartAI' AND ScriptName='';
UPDATE gameobject_template SET ScriptName='' WHERE entry=241868;
UPDATE creature_template SET ScriptName='custom_ruler' WHERE entry=92918;
$migration
SELECT 'no-partial-go',COUNT(*) FROM gameobject_template WHERE entry IN(241864,241868,241869) AND AIName='SmartGameObjectAI' AND ScriptName='';
SELECT 'custom-king',COUNT(*) FROM creature_template WHERE entry IN(92889,92918,92920) AND AIName='SmartAI' AND ScriptName IN('','custom_ruler');
"@}
$rows=@(Query 'World' $sql)
foreach($expected in @(
 "objective`t278464`t92889`t0`t0`t1`t0","objective`t278465`t92918`t1`t0`t1`t0","objective`t278466`t92920`t2`t0`t1`t0",
 "bowls`t3","rulers`t92889`t14`t32832`t0`t63347`t98`t110","rulers`t92918`t14`t32832`t0`t63369`t98`t110","rulers`t92920`t14`t32832`t0`t63370`t98`t110",
 "texts`t5","models`t4","legacy`t12","incense-source`t2",
 "summon`t24186400`t7500`t92920`t3417.11`t1162.67`t139`t3.86",
 "summon`t24186800`t7500`t92918`t3377.39`t1279.22`t128.48`t4.5",
 "summon`t24186900`t7500`t92889`t3513.62`t1102.43`t157.74`t1.51","helper`tnpc_stormheim_might_actor","later-bowl`t1","starter`t1","ender-next`t1","conditions`t0",
 "handoff`t38811`t38817`t18446744073709551615`t38810`t38817`t0","handoff`t38817`t38815`t18446744073709551615`t39791`t38815`t0",
 "spawns`t115119`t241864`t1220`t7541`t1`t3406.63`t1153.15`t136.178`t180",
 "spawns`t115115`t241868`t1220`t7541`t1`t3375.76`t1272.61`t126.355`t180",
 "spawns`t115152`t241869`t1220`t7541`t1`t3513.71`t1113.86`t153.034`t180"
)){if($rows -notcontains $expected){throw "Judgment data mismatch: $expected"}}
if($Live){foreach($value in @("installed-go`t3","installed-king`t3","installed-opening`t1")){if($rows -notcontains $value){throw "Missing installed binding: $value"}}}else{
 foreach($kind in @('go','king')){
  $actual=@($rows | Where-Object {$_ -like "$kind-actual`t*"} | ForEach-Object {$_.Substring($kind.Length+8)})
  $expected=@($rows | Where-Object {$_ -like "$kind-expected`t*"} | ForEach-Object {$_.Substring($kind.Length+10)})
  if($actual.Count -ne 4 -or (Compare-Object $actual $expected)){throw "Unexpected full-row $kind change/replay mismatch"}
 }
 foreach($value in @("custom-go`t3","no-partial-kings`t3","no-partial-go`t3","custom-king`t3","opening`t1","custom-opening`t0")){if($rows -notcontains $value){throw "Custom binding isolation failed: $value"}}
}
$hotfix=@(Query 'Hotfix' "SELECT ID,Text,SoundEntriesID1,EmotesID FROM broadcast_text WHERE ID IN(95185,95188,95192,95193,95196) ORDER BY ID;")
if($hotfix.Count -ne 5){throw 'Missing effective dialogue Hotfix overlays'}
foreach($expected in @("95185`tTo walk the high halls is the greatest honor of the valorous.`t0`t1","95188`tA pity you will never set foot beyond its gates!`t0`t274","95192`tWhat's this? An outsider seeks my judgment?`t51660`t1","95193`tInsolent worm!  You would disgrace the Halls of Valor with your presence!`t51661`t25","95196`tCome then, show me your fury!`t0`t15")){if($hotfix -notcontains $expected){throw 'Effective judgment dialogue mismatch'}}
$suffix=if($Live){'live'}else{'fixture'}
[IO.File]::WriteAllLines("$output/data-$suffix.tsv",[string[]]$rows)
[IO.File]::WriteAllLines("$output/broadcast-hotfix.tsv",[string[]]$hotfix)
"PASS: $suffix exact three bowls/rulers/objectives, six bindings/replay/custom isolation, full-row preservation, unchanged shared helper/later bowl/legacy SAI, scaling/relations and five effective Hotfix dialogue rows."
