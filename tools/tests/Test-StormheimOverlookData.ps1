param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-overlook"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Overlook([string]$database,[string]$sql,[string]$name){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw "Overlook $database query failed"}}finally{$env:MYSQL_PWD=$old}
 $rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/$name.tsv";return $rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_15_stormheim_overlook.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature_addon','creature','creature_template_addon','waypoint_data')){
  $filter=switch($table){'creature_addon' {'guid IN(288637,288730)'} 'creature' {'guid IN(288637,288730,288742,340420,341178)'} 'creature_template_addon' {'entry IN(91460,91468,91470)'} 'waypoint_data' {'id IN(288637,2887300)'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 # Reproduce the original missing binding even after live deployment.
 $sql+="DELETE FROM creature_addon WHERE guid=288637;`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38362,38308,38317) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(38308,38317) ORDER BY QuestID,StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(38308,38317) ORDER BY quest,id;
SELECT 'go-starter',id,quest FROM gameobject_queststarter WHERE quest IN(38308,38317) ORDER BY quest,id;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(38308,38317);
SELECT 'go-enders',COUNT(*) FROM gameobject_questender WHERE quest IN(38308,38317);
SELECT 'actor',t.entry,w.KillCredit1,w.KillCredit2,t.faction,t.npcflag,t.unit_flags,t.unit_flags2,t.AIName,t.ScriptName,w.Displayid1,w.Displayid2,t.lootid FROM creature_template t JOIN creature_template_wdb w USING(entry) WHERE t.entry IN(91460,91468,91470,97250,113911) ORDER BY t.entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,modelid,phaseMask,PhaseId,MovementType,dynamicflags FROM creature WHERE id IN(91460,91468,91470,97250,113911) ORDER BY id,guid;
SELECT 'report',guid,id,map,position_x,position_y,position_z,phaseMask,PhaseId FROM gameobject WHERE id=244453;
SELECT 'report-template',entry,type,flags,Data1,ScriptName FROM gameobject_template WHERE entry=244453;
SELECT 'click',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells WHERE npc_entry IN(91470,113911);
SELECT 'smart',entryorguid,source_type,id,link,Difficulties,event_type,event_chance,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(91460,91468,91470,97250,113911,-288637,-288730,-288742,-340420,-341178)) ORDER BY entryorguid,id;
SELECT 'addon',entry,path_id,mount,bytes1,bytes2,emote,auras FROM creature_template_addon WHERE entry IN(91460,91468,91470) ORDER BY entry;
SELECT 'patrol-addon',guid,path_id,mount,bytes1,bytes2,emote,auras FROM creature_addon WHERE guid IN(288637,288730) ORDER BY guid;
SELECT 'patrol',id,point,position_x,position_y,position_z,move_type,action FROM waypoint_data WHERE id IN(288637,2887300) ORDER BY id,point;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(91460,91468,91470) ORDER BY Entry;
SELECT 'spell',entry,spell,difficultyMask,Difficulties FROM creature_template_spell WHERE entry IN(91460,91468,91470) ORDER BY entry,spell;
SELECT 'extra-conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38308,38317)) OR (SourceTypeOrReferenceId=18 AND SourceGroup=91470) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(91460,91468,91470,-288637,-288730,-288742));
SELECT 'spell-bindings',COUNT(*) FROM spell_script_names WHERE spell_id IN(181532,145363,207420,182517,154795);
SELECT 'intel-loot',COUNT(*) FROM creature_loot_template WHERE Item=122581;
'@
$rows=Read-Overlook World $sql 'world'
foreach($r in @("quest`t38308`t37814280`t0`t234881970`t0`t38317`t38362`t38317`t0`t0`t0","quest`t38317`t38273024`t0`t234881970`t0`t0`t38308`t0`t0`t0`t0","objective`t38308`t277692`t1`t122581`t1`t3`t0","objective`t38317`t277694`t0`t91468`t1`t0`t0","objective`t38317`t277695`t0`t91460`t1`t1`t0","go-starter`t244453`t38308","ender`t97250`t38317","go-enders`t0","report-template`t244453`t2`t0`t19510`t","click`t91470`t181532`t1`t0`t1","smart`t91470`t0`t0`t0`t`t73`t100`t0`t0`t0`t56`t122581`t1`t7","addon`t91460`t0`t0`t0`t0`t0`t182517 154795","addon`t91468`t0`t0`t0`t0`t0`t207420","addon`t91470`t0`t0`t262144`t0`t0`t145363","patrol-addon`t288637`t288637`t0`t0`t0`t0`t182517 154795","patrol-addon`t288730`t2887300`t0`t0`t0`t0`tNULL","extra-conditions`t0","spell-bindings`t0","intel-loot`t0")){if($rows -notcontains $r){throw "Overlook contract changed: $r"}}
foreach($entry in @(91460,91468,91470)){if($rows -notcontains "scaling`t$entry`t98`t110"){throw "Inactive scaling $entry"}}
if(@($rows | Where-Object {$_ -like "objective`t*"}).Count -ne 3 -or @($rows | Where-Object {$_ -like "smart`t*"}).Count -ne 1){throw 'Unexpected objectives/SmartAI'}
foreach($r in @("actor`t91460`t0`t0`t84`t0`t32768`t2048`t`t`t62465`t0`t91460","actor`t91468`t0`t0`t84`t0`t32768`t2048`t`t`t29084`t0`t91468","actor`t91470`t0`t0`t84`t0`t537166592`t18433`tSmartAI`t`t57965`t0`t0")){if($rows -notcontains $r){throw 'Leader/click actor flags, credit, faction or model changed'}}
if(@($rows | Where-Object {$_ -like "starter`t*"}).Count -or @($rows | Where-Object {$_ -like "click`t*"}).Count -ne 1){throw 'Unexpected quest/click source'}
if(@($rows | Where-Object {$_ -like "patrol`t288637`t*"}).Count -ne 5 -or @($rows | Where-Object {$_ -like "patrol`t2887300`t*"}).Count -ne 16){throw 'Native patrol changed'}
foreach($r in ($rows | Where-Object {$_ -like "spawn`t*"})){$p=$r.Split("`t");if($p[3] -ne '1220' -or $p[7] -ne '0' -or $p[8] -ne '1' -or $p[9]){throw 'Unexpected spawn map/model/phase'}}
if(@($rows | Where-Object {$_ -like "spawn`t*"}).Count -ne 5){throw 'Unexpected actor spawn set'}
if(!$Live){
 $guard=@"
UPDATE creature_addon SET path_id=777,auras='custom' WHERE guid=288637;
$migration
SELECT 'custom-path',COUNT(*) FROM creature_addon WHERE guid=288637 AND path_id=777 AND auras='custom';
UPDATE creature_addon SET path_id=0 WHERE guid=288637;
$migration
SELECT 'existing-zero-path',COUNT(*) FROM creature_addon WHERE guid=288637 AND path_id=288637 AND auras='custom';
DELETE FROM creature_addon WHERE guid=288637;
DELETE FROM waypoint_data WHERE id=288637;
$migration
SELECT 'missing-path',COUNT(*) FROM creature_addon WHERE guid=288637;
SELECT 'unrelated',COUNT(*) FROM creature_addon a JOIN saved_creature_addon b USING(guid) WHERE a.guid=288730 AND a.path_id=b.path_id AND a.auras <=> b.auras;
"@
 # Guard cases need the same temporary tables/connection as the migration.
 $all=Read-Overlook World ($sql+$guard) 'migration'
 foreach($r in @("custom-path`t1","existing-zero-path`t1","missing-path`t0","unrelated`t1")){if($all -notcontains $r){throw "Migration guard failed: $r"}}
}
$h=Read-Overlook Hotfix "SELECT 'item',COUNT(*) FROM item_sparse WHERE ID=122581; SELECT 'spell-effect',COUNT(*) FROM spell_effect WHERE SpellID IN(181532,145363,207420,182517,154795); SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(62465,29084,57965,62917,62918);" 'hotfix'
foreach($r in @("item`t0","spell-effect`t0","display`t0")){if($h -notcontains $r){throw 'Unexpected native catalog overlay'}}
if($Live){'PASS: live Overlook quest/actor/click/auras/scaling data and restored patrol binding.'}else{'PASS: Overlook native quest flags/goals/report/click/add-item data, both leaders, spawn/scaling/auras and existing patrols; repeated migration preserves disguise/custom paths/unrelated actor and refuses a missing route.'}
