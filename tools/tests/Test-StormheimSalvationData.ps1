param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-salvation"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_14_stormheim_salvation.sql" -Raw
$tables=@{quest_template_addon='ID IN(40001,40002,40003,40004,40005,40072)';quest_objectives='QuestID IN(40003,40004,40005,40072)';creature_template='entry IN(92307,97979,97986,97988,98190)';smart_scripts='entryorguid IN(97986,98190)';conditions='SourceTypeOrReferenceId=15 AND SourceGroup IN(18843,18852)'}
$sql=''
if(!$Live){
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="CREATE TEMPORARY TABLE saved_$t LIKE $t; INSERT INTO saved_$t SELECT * FROM $t WHERE $($tables[$t]); CREATE TEMPORARY TABLE $t LIKE saved_$t; INSERT INTO $t SELECT * FROM saved_$t;`n"}
 $sql+="UPDATE quest_template_addon SET PrevQuestID=0,ExclusiveGroup=0 WHERE ID=40004; UPDATE quest_template_addon SET ExclusiveGroup=0 WHERE ID=40003; UPDATE quest_template_addon SET PrevQuestID=0 WHERE ID=40005; UPDATE quest_objectives SET Flags=0 WHERE ID=280182; UPDATE creature_template SET ScriptName='' WHERE entry IN(97986,98190);`n$migration`n$migration`n"
 $sql+="UPDATE saved_quest_template_addon SET PrevQuestID=40002,ExclusiveGroup=-40003 WHERE ID=40004; UPDATE saved_quest_template_addon SET ExclusiveGroup=-40003 WHERE ID=40003; UPDATE saved_quest_template_addon SET PrevQuestID=40003 WHERE ID=40005; UPDATE saved_quest_objectives SET Flags=Flags|1 WHERE ID=280182; UPDATE saved_creature_template SET ScriptName='npc_stormheim_salvation_vethir' WHERE entry IN(97986,98190);`n"
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="SELECT 'actual-$t',t.* FROM $t t; SELECT 'expected-$t',t.* FROM saved_$t t;`n"}
}
$sql+=@"
SELECT 'quest',q.ID,q.Flags,q.AllowableRaces,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(40002,40003,40004,40005,40072) ORDER BY q.ID;
SELECT 'objective',ID,QuestID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=40005 ORDER BY ID;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(40005,40072) ORDER BY quest;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(40005,40072) ORDER BY quest;
SELECT 'actor',c.guid,c.id,c.position_x,c.position_y,c.position_z,c.map,c.zoneId,c.phaseMask,c.PhaseId,c.modelid,t.faction,t.npcflag,t.unit_flags,t.AIName,t.ScriptName,t.VehicleId,w.Displayid1 FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE c.guid IN(291417,338656,341218,338669) ORDER BY c.id;
SELECT 'gossip',entryorguid,id,event_type,event_param1,event_param2,action_type,action_param1,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE source_type=0 AND entryorguid IN(97986,98190) ORDER BY entryorguid,id;
SELECT 'menu',MenuID,OptionID,OptionNpc,OptionBroadcastTextID FROM gossip_menu_option WHERE MenuID IN(18843,18852) ORDER BY MenuID;
SELECT 'condition',SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup IN(18843,18852) ORDER BY SourceGroup;
SELECT 'spell',entry,spell,difficultyMask,Difficulties FROM creature_template_spell WHERE entry=92307;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry=92307;
SELECT 'model',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID IN(65873,68213,76630) ORDER BY DisplayID;
SELECT 'bindings',COUNT(*) FROM creature_template WHERE entry IN(97986,98190) AND AIName='SmartAI' AND ScriptName='npc_stormheim_salvation_vethir';
SELECT 'group-members',GROUP_CONCAT(ID ORDER BY ID) FROM quest_template_addon WHERE ExclusiveGroup=-40003;
"@
if(!$Live){$sql+="UPDATE creature_template SET ScriptName='custom_vethir' WHERE entry=97986; UPDATE quest_template_addon SET PrevQuestID=777 WHERE ID=40004; UPDATE quest_template_addon SET ExclusiveGroup=777 WHERE ID=40003; UPDATE quest_objectives SET Flags=16 WHERE ID=280182;`n$migration`nSELECT 'custom-script',ScriptName FROM creature_template WHERE entry=97986; SELECT 'custom-prev',PrevQuestID FROM quest_template_addon WHERE ID=40004; SELECT 'custom-group',ExclusiveGroup FROM quest_template_addon WHERE ID=40003; SELECT 'custom-bit',Flags FROM quest_objectives WHERE ID=280182;"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Salvation World query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($e in @("quest`t40003`t38928392`t18446744073709551615`t0`t40002`t0`t-40003`t0","quest`t40004`t38797320`t18446744073709551615`t0`t40002`t0`t-40003`t0","quest`t40005`t35651592`t18446744073709551615`t40072`t40003`t40072`t0`t0","quest`t40072`t33554440`t18446744073709551615`t0`t40005`t0`t0`t0","objective`t280182`t40005`t0`t92307`t1`t3`t1","objective`t282130`t40005`t0`t103766`t1`t0`t28","starter`t97986`t40005","ender`t97988`t40005","starter`t97988`t40072","ender`t97988`t40072","menu`t18843`t0`t0`t99941","menu`t18852`t0`t0`t99988","condition`t18843`t0`t0`t9`t0`t40005","condition`t18852`t0`t0`t28`t0`t40005","scaling`t92307`t98`t110")){if($rows -notcontains $e){throw "Missing Salvation data $e"}}
if($rows -notcontains "bindings`t2"){throw 'Missing Salvation actor bindings'}
if($rows -notcontains "group-members`t40003,40004"){throw 'Unexpected each-from-all group membership'}
if(!$Live){
 foreach($t in $tables.Keys){$a="actual-$t`t";$e="expected-$t`t";$actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object);$expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object);if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $t"}}
 foreach($e in @("custom-script`tcustom_vethir","custom-prev`t777","custom-group`t777","custom-bit`t17")){if($rows -notcontains $e){throw 'Custom setting overwritten'}}
}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql="SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(65873,68213,76630); SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID IN(8518,8673,8323); SELECT 'faction',COUNT(*) FROM faction_template WHERE ID IN(190,2780); SELECT 'quest-line',COUNT(*) FROM quest_line_x_quest WHERE QuestID IN(40005,40072); SELECT 'spell',COUNT(*) FROM spell_effect WHERE SpellID IN(182921,209076,214298); SELECT 'vehicle',COUNT(*) FROM vehicle WHERE ID=4367; SELECT 'seat',COUNT(*) FROM vehicle_seat WHERE ID=15925;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Salvation Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($name in @('display','model','faction','quest-line','spell','vehicle','seat')){if($rows -notcontains "$name`t0"){throw "Unexpected native-reference overlay $name"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: Salvation repeat migration/full-row preservation/custom settings, each-from-all prerequisites, optional credit, actors and both unchanged teleport/menu routes.'
