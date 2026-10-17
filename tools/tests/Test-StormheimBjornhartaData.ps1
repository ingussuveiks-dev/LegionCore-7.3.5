$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-bjornharta"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38808,38811) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=38808;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(38808,38811) ORDER BY quest;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=38808;
SELECT 'loot',Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Reference,Currency FROM creature_loot_template WHERE Item=124503;
SELECT 'bear',t.entry,t.lootid,t.flags_extra,t.faction,w.Displayid1,w.KillCredit1,w.KillCredit2,w.FlagQuest,w.VignetteID FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=93095;
SELECT 'spawns',COUNT(*),SUM(map=1220),SUM(modelid<>0),SUM(PhaseId<>''),SUM((spawnMask&1)=0) FROM creature WHERE id=93095;
SELECT 'props',c.unit_flags,COALESCE(a.auras,'<none>'),COUNT(*) FROM creature c LEFT JOIN creature_addon a ON a.guid=c.guid WHERE c.id=93095 GROUP BY c.unit_flags,a.auras;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry=93095;
SELECT 'giver',c.guid,c.id,c.map,c.position_x,c.position_y,c.position_z,c.PhaseId,t.npcflag,t.ScriptName FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=93231;
SELECT 'position',guid,id,map,position_x,position_y,position_z,modelid,PhaseId FROM creature WHERE id=93095 ORDER BY guid;
SELECT 'conditions',COUNT(*) FROM conditions WHERE(SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38808,38811)) OR(SourceTypeOrReferenceId=1 AND SourceEntry=124503);
SELECT 'item-overrides',COUNT(*) FROM item_template WHERE entry=124503;
SELECT 'item-flags',COUNT(*) FROM item_template_addon WHERE Id=124503;
SELECT 'start-scripts',COUNT(*) FROM quest_start_scripts WHERE id=38808 OR(command=7 AND datalong=38808);
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Bjornharta World query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/world.tsv"
foreach($row in @("quest`t38808`t33554440`t0`t18446744073709551615`t0`t38811`t39796`t38811`t0`t0`t650","quest`t38811`t33554440`t0`t18446744073709551615`t0`t38817`t38810`t38817`t0`t0`t0","objective`t38808`t278456`t1`t124503`t4`t0`t0","starter`t93231`t38808","starter`t93231`t38811","ender`t93231`t38808","loot`t93095`t124503`t31.193`t1`t0`t0`t1`t1`t0`t0","bear`t93095`t93095`t0`t14`t33920`t0`t0`t0`t0","spawns`t42`t42`t0`t0`t0","scaling`t93095`t98`t110","giver`t292106`t93231`t1220`t3073.09`t1056.88`t127.021`t`t3`t","props`t0`t145953`t8","props`t512`t29266`t4")){if($rows -notcontains $row){throw "Bjornharta contract changed: $row"}}
foreach($kind in @('conditions','item-overrides','item-flags','start-scripts')){if($rows -notcontains "$kind`t0"){throw "Unexpected Bjornharta $kind"}}
$sql="SELECT 'item',COUNT(*) FROM item_sparse WHERE ID=124503; SELECT 'effects',COUNT(*) FROM item_effect WHERE ItemID=124503; SELECT 'rewards',COUNT(*) FROM quest_package_item WHERE PackageID=650; SELECT 'models',COUNT(*) FROM creature_display_info WHERE ID IN(33920,65138); SELECT 'model-data',COUNT(*) FROM creature_model_data WHERE ID IN(8575,8447);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Bjornharta Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('item','effects','rewards','models','model-data')){if($rows -notcontains "$kind`t0"){throw "Unexpected Bjornharta native $kind override"}}
'PASS: Bjornharta collect4, native bear loot selector/31.193% quest loot,42 spawns and prop states,98..110 scaling, Vydhar relations and38811 links; no data repair required.'
