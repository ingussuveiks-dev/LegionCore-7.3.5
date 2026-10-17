$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-journals"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,a.ProvidedItemCount,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39405,39984) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(39405,39984) ORDER BY QuestID,StorageIndex;
SELECT 'starter',id,quest FROM gameobject_queststarter WHERE quest IN(39405,39984) ORDER BY id,quest;
SELECT 'go-ender',id,quest FROM gameobject_questender WHERE quest IN(39405,39984);
SELECT 'npc-ender',id,quest FROM creature_questender WHERE quest IN(39405,39984);
SELECT 'alternate-starter',id,quest FROM creature_queststarter WHERE quest IN(39405,39984);
SELECT 'giver',entry,type,displayId,flags,Data10,ScriptName FROM gameobject_template WHERE entry IN(251571,244708) ORDER BY entry;
SELECT 'book',entry,type,flags,Data0,Data10,Data19,Data22,ScriptName FROM gameobject_template WHERE entry IN(244717,244718,244719,244720) ORDER BY entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,PhaseId FROM gameobject WHERE id IN(251571,244708,244717,244718,244719,244720) ORDER BY guid;
SELECT 'loot',Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Reference,Currency FROM creature_loot_template WHERE Item=126946;
SELECT 'source',t.entry,t.lootid,t.flags_extra,w.FlagQuest,w.VignetteID,w.Displayid1 FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=94856;
SELECT 'priestesses',COUNT(*),SUM(modelid<>0),SUM(PhaseId<>''),SUM((spawnMask&1)=0) FROM creature WHERE id=94856;
SELECT 'priest-anchor',guid,id,map,position_x,position_y,position_z FROM creature WHERE guid=293517;
SELECT 'text',g.MenuID,g.TextID,n.Probability0,n.BroadcastTextID0 FROM gossip_menu g JOIN npc_text n ON n.ID=g.TextID WHERE g.MenuID IN(18822,18825,18826,18827) ORDER BY g.MenuID;
SELECT 'havi',guid,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id=97973;
SELECT 'havi-condition',ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=97973;
SELECT 'havi-los',event_type,event_param1,event_param2,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=97973 AND source_type=0;
SELECT 'havi-credit',action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=9797300 AND source_type=9 AND id=0;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(94856,97973) ORDER BY Entry;
SELECT 'conditions',COUNT(*) FROM conditions WHERE(SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(39405,39984)) OR(SourceTypeOrReferenceId=1 AND SourceEntry=126946);
SELECT 'item-overrides',COUNT(*) FROM item_template WHERE entry IN(126946,128324);
SELECT 'item-flags',COUNT(*) FROM item_template_addon WHERE Id IN(126946,128324);
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Journal data query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
$rows | Set-Content "$out/world.tsv"
$expected=@("quest`t39405`t34144256`t0`t18446744073709551615`t126946`t1`t0`t0`t0`t0","quest`t39984`t33554440`t0`t18446744073709551615`t0`t0`t0`t0`t0`t0","starter`t251571`t39405","starter`t244708`t39984","npc-ender`t97973`t39984","objective`t39405`t279357`t1`t126946`t8`t0`t0","loot`t94856`t126946`t92.3688`t1`t0`t0`t1`t1`t0`t0","source`t94856`t94856`t0`t0`t0`t68940","priestesses`t16`t0`t0`t0","havi-credit`t33`t97990`t7","havi-condition`t41`t0`t39984`t97990`t1","havi-los`t10`t1`t10`t80`t9797300`t1")
foreach($row in $expected){if($rows -notcontains $row){throw "Journal contract changed: $row"}}
if(@($rows | Where-Object {$_ -like "go-ender`t*" -or $_ -like "alternate-starter`t*"}).Count){throw 'Unexpected journal ender or NPC starter'}
foreach($row in @("giver`t244708`t2`t29023`t0`t0`t","giver`t251571`t2`t15781`t0`t0`t","book`t244717`t10`t0`t0`t0`t18822`t36152`t","book`t244718`t10`t0`t0`t0`t18825`t36152`t","book`t244719`t10`t0`t0`t0`t18826`t36152`t","book`t244720`t10`t0`t0`t0`t18827`t36152`t")){if($rows -notcontains $row){throw "Journal interaction template changed: $row"}}
foreach($row in @("spawn`t115386`t251571`t1220`t2672.65`t1352.05`t43.4566`t","spawn`t115402`t251571`t1220`t2693.04`t1635.92`t14.6927`t","spawn`t115008`t244708`t1220`t3296.72`t1377.97`t97.9798`t","spawn`t115001`t244717`t1220`t3393.29`t1508.63`t60.9625`t","spawn`t114995`t244718`t1220`t3357.12`t1648.86`t7.55221`t","spawn`t114999`t244719`t1220`t3432.79`t1564.77`t9.06416`t","spawn`t114975`t244720`t1220`t3515.9`t1526.83`t8.7028`t","havi`t369621`t1220`t3521.58`t1615.26`t11.3535`t","priest-anchor`t293517`t94856`t1220`t2690.31`t1636.36`t12.9985")){if($rows -notcontains $row){throw "Journal geometry anchor changed: $row"}}
foreach($pair in @(@(18822,27418,99828),@(18825,27422,99839),@(18826,27423,99840),@(18827,27424,99841))){if($rows -notcontains "text`t$($pair[0])`t$($pair[1])`t1`t$($pair[2])"){throw 'Book gossip text changed'}}
foreach($id in @(94856,97973)){if($rows -notcontains "scaling`t$id`t98`t110"){throw 'Invalid actor scaling'}}
foreach($kind in @('conditions','item-overrides','item-flags')){if($rows -notcontains "$kind`t0"){throw "Unexpected $kind"}}
$sql='SELECT ''broadcast'',ID,LENGTH(Text),LENGTH(Text1),SoundEntriesID1,SoundEntriesID2 FROM broadcast_text WHERE ID IN(99828,99839,99840,99841,99874) ORDER BY ID; SELECT ''item'',COUNT(*) FROM item_sparse WHERE ID IN(126946,128324); SELECT ''effects'',COUNT(*) FROM item_effect WHERE ItemID IN(126946,128324); SELECT ''objects'',COUNT(*) FROM gameobject_display_info WHERE ID IN(15781,29023,17805,14702,15405,25948); SELECT ''actors'',COUNT(*) FROM creature_display_info WHERE ID IN(68940,76630); SELECT ''models'',COUNT(*) FROM creature_model_data WHERE ID IN(8891,8323); SELECT ''condition'',COUNT(*) FROM player_condition WHERE ID=36152;'
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Journal Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('item','effects','objects','actors','models','condition')){if($rows -notcontains "$kind`t0"){throw "Unexpected native $kind override"}}
foreach($id in @(99828,99839,99840,99841,99874)){$row=@($rows | Where-Object {$_ -like "broadcast`t$id`t*"});if($row.Count -ne 1){throw "Missing actual Hotfix broadcast $id"};$fields=$row[0].Split("`t");if(([int]$fields[2]+[int]$fields[3]) -le 0){throw "Empty Hotfix broadcast $id"}}
'PASS: existing original journal starter relations, mandatory8 verses/source item,16 priestesses/loot, four book menus/objectives and Havi final credit/ender. No migration required.'
