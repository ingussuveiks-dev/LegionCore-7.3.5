param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/halls-aegis"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_15_halls_of_valor_aegis.sql" -Raw
$tables=@{quest_objectives='QuestID IN(40072,43349)';gameobject_template='entry IN(251991,251992)';gameobject='id IN(251991,251992)';gameobject_queststarter='id IN(251991,251992)';gameobject_questender='id IN(251991,251992)';conditions='SourceTypeOrReferenceId=23 AND SourceGroup=7672 AND SourceEntry=7'}
$sql=''
if(!$Live){
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="CREATE TEMPORARY TABLE saved_$t LIKE $t; INSERT INTO saved_$t SELECT * FROM $t WHERE $($tables[$t]); CREATE TEMPORARY TABLE $t LIKE saved_$t; INSERT INTO $t SELECT * FROM saved_$t;`n"}
 $sql+="UPDATE gameobject_template SET ScriptName='' WHERE entry=251991; UPDATE quest_objectives SET Flags=2 WHERE ID=285235;`n$migration`n$migration`nUPDATE saved_gameobject_template SET ScriptName='go_halls_of_valor_quest_aegis' WHERE entry=251991; UPDATE saved_quest_objectives SET Flags=Flags|1 WHERE ID=285235;`n"
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="SELECT 'actual-$t',t.* FROM $t t; SELECT 'expected-$t',t.* FROM saved_$t t;`n"}
}
$sql+=@"
SELECT 'quest',q.ID,q.Flags,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(40072,43349) ORDER BY q.ID;
SELECT 'objective',ID,QuestID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=40072 ORDER BY ID;
SELECT 'go',entry,type,displayId,flags,size,Data0,Data1,Data2,AIName,ScriptName FROM gameobject_template WHERE entry=251991;
SELECT 'spawn',guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z FROM gameobject WHERE id=251991;
SELECT 'go-starter',id,quest FROM gameobject_queststarter WHERE id=251991;
SELECT 'go-ender',id,quest FROM gameobject_questender WHERE id=251991;
SELECT 'havi-starter',id,quest FROM creature_queststarter WHERE quest=40072;
SELECT 'havi-ender',id,quest FROM creature_questender WHERE quest=40072;
SELECT 'phase',zoneId,entry,phaseId,flags FROM phase_definitions WHERE zoneId=7672 AND entry=7;
SELECT 'phase-condition',ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7672 AND SourceEntry=7;
SELECT 'dungeon',map,script FROM instance_template WHERE map=1477;
SELECT 'boss',c.guid,c.id,c.map,c.position_x,c.position_y,c.position_z,c.modelid,t.ScriptName,w.Name1,w.Displayid1 FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE c.id IN(95675,95676) AND c.map=1477 ORDER BY c.id;
SELECT 'menu',MenuID,OptionID,OptionNpc,OptionBroadcastTextID FROM gossip_menu_option WHERE MenuID=20352;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(95675,95676) ORDER BY Entry;
"@
if(!$Live){$sql+="UPDATE gameobject_template SET ScriptName='custom_aegis' WHERE entry=251991; UPDATE quest_objectives SET Flags=18 WHERE ID=285235;`n$migration`nSELECT 'custom-script',ScriptName FROM gameobject_template WHERE entry=251991; SELECT 'custom-bit',Flags FROM quest_objectives WHERE ID=285235;"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Aegis World query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($e in @("quest`t40072`t33554440`t18446744073709551615`t0`t0`t40005`t0`t0","quest`t43349`t33554440`t18446744073709551615`t0`t0`t40072`t0`t0","objective`t280268`t40072`t0`t110425`t1`t0`t0","objective`t285235`t40072`t0`t95676`t1`t1`t3","go`t251991`t2`t35704`t0`t1`t0`t20352`t0`t`tgo_halls_of_valor_quest_aegis","spawn`t25178659`t251991`t1477`t7672`t7672`t8388870`t1`t3554`t2375.19`t559.552`t754.515","go-starter`t251991`t43349","go-ender`t251991`t40072","havi-starter`t97988`t40072","havi-ender`t97988`t40072","phase`t7672`t7`t3554`t16","phase-condition`t14`t0`t40072`t1","dungeon`t1477`tinstance_halls_of_valor")){if($rows -notcontains $e){throw "Missing Aegis data $e"}}
if(!$Live){
 foreach($t in $tables.Keys){$a="actual-$t`t";$e="expected-$t`t";$actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object);$expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object);if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $t"}}
 if($rows -notcontains "custom-script`tcustom_aegis" -or $rows -notcontains "custom-bit`t19"){throw 'Custom Aegis setting overwritten'}
}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql="SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(65873,67230,76630); SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID IN(8518,8695,8323); SELECT 'go-display',COUNT(*) FROM gameobject_display_info WHERE ID=35704; SELECT 'phase',COUNT(*) FROM phase WHERE ID=3554; SELECT 'map',COUNT(*) FROM map WHERE ID=1477; SELECT 'difficulty',COUNT(*) FROM map_difficulty WHERE MapID=1477; SELECT 'quest-line',COUNT(*) FROM quest_line_x_quest WHERE QuestID IN(40072,43349);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Aegis Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($name in @('display','model','go-display','phase','map','difficulty','quest-line')){if($rows -notcontains "$name`t0"){throw "Unexpected Aegis native-reference overlay $name"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: Aegis repeated migration, full-row preservation, custom flags/script, real questgiver pedestal/phase, required Odyn target and43349 follow-up.'
