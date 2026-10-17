param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-shields"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_08_stormheim_shielded_secrets.sql" -Raw
$sql=''
if(!$Live){
 foreach($t in @('gameobject_template','conditions','smart_scripts')){
  $sql+="CREATE TEMPORARY TABLE saved_$t LIKE $t;`n"
  if($t -eq 'gameobject_template'){$sql+="INSERT INTO saved_$t SELECT * FROM $t WHERE entry IN(242673,251557,251558,251559,251560,251561,242994); UPDATE saved_$t SET ScriptName='' WHERE entry<>242994;`n"}
  $sql+="CREATE TEMPORARY TABLE $t LIKE saved_$t;`n"
 }
 $sql+=@'
INSERT INTO gameobject_template SELECT * FROM saved_gameobject_template;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,target_type,comment) VALUES(242673,1,0,64,33,109346,7,'fixture'),(251557,1,0,64,33,109346,7,'fixture'),(251558,1,0,64,33,109346,7,'fixture'),(251559,1,0,64,33,109346,7,'fixture'),(251560,1,0,64,33,109346,7,'fixture'),(251561,1,0,64,33,109346,7,'fixture');
INSERT INTO saved_smart_scripts SELECT * FROM smart_scripts;
'@
 $sql+=$migration+"`n"+$migration+"`n"
}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39062,39063,39092) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=39063 ORDER BY StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(39062,39063,39092) ORDER BY quest;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(39062,39063) ORDER BY quest;
SELECT 'go',entry,type,displayId,flags,size,Data0,Data1,Data2,Data3,Data7,Data10,Data19,ScriptName FROM gameobject_template WHERE entry IN(242673,251557,251558,251559,251560,251561) ORDER BY entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,PhaseId FROM gameobject WHERE id IN(242673,251557,251558,251559,251560,251561) ORDER BY id;
SELECT 'giver',guid,id,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id IN(93779,95921) ORDER BY id;
SELECT 'scope',COUNT(*),SUM(map=1220 AND zoneId=7541 AND PhaseId='') FROM gameobject WHERE id IN(242673,251557,251558,251559,251560,251561);
SELECT 'story',g.MenuID,g.TextID,n.BroadcastTextID0 FROM gossip_menu g JOIN npc_text n ON n.ID=g.TextID WHERE g.MenuID=18581 ORDER BY g.TextID;
SELECT 'condition',SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ElseGroup FROM conditions WHERE SourceTypeOrReferenceId=14 AND SourceGroup=18581 ORDER BY SourceEntry;
SELECT 'legacy-smart',COUNT(*) FROM smart_scripts WHERE source_type=1 AND entryorguid IN(242673,251557,251558,251559,251560,251561) AND action_type=33 AND action_param1=109346;
'@
if(!$Live){$sql+=@"
SELECT 'unrelated',COUNT(*) FROM gameobject_template WHERE entry=242994 AND ScriptName='';
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_gameobject_template;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart_scripts;
DELETE FROM conditions;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(242673,1,7,64,72,'custom');
$migration
SELECT 'custom-preserved',COUNT(*) FROM gameobject_template WHERE entry<>242994 AND ScriptName='';
SELECT 'custom-condition-count',COUNT(*) FROM conditions;
SELECT 'custom-smart-count',COUNT(*) FROM smart_scripts;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Shields World query/replay failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
foreach($line in @("quest`t39062`t33554440`t0`t824181837`t0`t39063`t39472`t39063`t0`t0`t0","quest`t39063`t37748744`t8192`t824181837`t0`t39092`t39062`t39092`t0`t0`t0","objective`t39063`t278876`t0`t109346`t6`t0`t0","starter`t94318`t39062","starter`t93779`t39063","starter`t95921`t39092","ender`t93779`t39062","ender`t95921`t39063","legacy-smart`t0")){if($rows -notcontains $line){throw "Shields contract changed: $line"}}
$entries=@(242673,251557,251558,251559,251560,251561)
for($i=0;$i -lt 6;$i++){
 $entry=$entries[$i];$text=26930+$i;$broadcast=96736+$i;$obj=284925+$i;$storage=$i+1;$display=if($i){35192}else{27378}
 foreach($line in @("story`t18581`t$text`t$broadcast","condition`t$text`t31`t1`t5`t$entry`t0`t0`t0","objective`t39063`t$obj`t2`t$entry`t1`t$storage`t0","go`t$entry`t10`t$display`t262144`t1.5`t0`t0`t0`t500`t0`t0`t18581`tgo_stormheim_engraved_shield")){if($rows -notcontains $line){throw "Missing distinct shield contract: $line"}}
}
if(!$Live){foreach($line in @("unrelated`t1","custom-preserved`t6","custom-condition-count`t0","custom-smart-count`t7")){if($rows -notcontains $line){throw 'Shield migration changed unrelated/custom data'}}}
foreach($line in @("scope`t6`t6","spawn`t115401`t242673`t1220`t2712.52`t1641.37`t18.7454`t","spawn`t115389`t251557`t1220`t2784.25`t1476.16`t24.8708`t","spawn`t115385`t251558`t1220`t2675.02`t1356.35`t47.5821`t","spawn`t115377`t251559`t1220`t2658.06`t1427.62`t32.433`t","spawn`t115394`t251560`t1220`t2853.91`t1523.14`t13.0343`t","spawn`t115380`t251561`t1220`t2728.16`t1465.83`t26.0828`t")){if($rows -notcontains $line){throw "Shield source anchor changed: $line"}}
$sql="SELECT 'display',COUNT(*) FROM gameobject_display_info WHERE ID IN(27378,35192); SELECT 'actor',COUNT(*) FROM creature_display_info WHERE ID=34520; SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID=50; SELECT 'text',ID,LENGTH(Text)+LENGTH(Text1) FROM broadcast_text WHERE ID BETWEEN 96736 AND 96741 ORDER BY ID;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$h=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Shield native override/text query failed'}}finally{$env:MYSQL_PWD=$previous}
$h=@($h | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$h | Set-Content "$out/hotfix.tsv"
foreach($kind in @('display','actor','model')){if($h -notcontains "$kind`t0"){throw "Unexpected native shield $kind override"}}
for($id=96736;$id -le 96741;$id++){$r=@($h | Where-Object {$_ -like "text`t$id`t*"});if($r.Count -ne 1 -or [int]$r[0].Split("`t")[2] -le 0){throw "Missing authored shield text$id"}}
'PASS:39062/39063 relations, six distinct shield objectives/stories, personal script bindings, repeat migration and custom SmartAI preservation.'
