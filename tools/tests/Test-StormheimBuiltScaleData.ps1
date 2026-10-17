$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-built-scale"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.QuestPackageID,q.StartItem,q.RewardNextQuest,q.StartScript,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38337,38616) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(38337,38616) ORDER BY QuestID;
SELECT 'loot',Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM creature_loot_template WHERE Entry IN(91202,91486,91799) AND Item IN(122609,122610,124037) ORDER BY Entry,Item;
SELECT 'actor',t.entry,t.lootid,t.faction,t.unit_flags,t.unit_flags2,t.AIName,t.ScriptName,w.Displayid1,w.FlagQuest FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(91202,91486,91799) ORDER BY t.entry;
SELECT 'spawns',id,COUNT(*),SUM(modelid<>0) FROM creature WHERE id IN(91202,91486,91799) GROUP BY id ORDER BY id;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(38337,38616) ORDER BY quest,id;
SELECT 'recipient',guid,id,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id IN(91222,91481,91519,91553,91771,92561,92566,92567) ORDER BY id,guid;
SELECT 'npc-starters',COUNT(*) FROM creature_queststarter WHERE quest IN(38337,38616);
SELECT 'go-starters',COUNT(*) FROM gameobject_queststarter WHERE quest IN(38337,38616);
SELECT 'scripts',COUNT(*) FROM quest_start_scripts WHERE id IN(38337,38616) OR (command=7 AND datalong IN(38337,38616));
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(1,4,17,30) AND SourceEntry IN(122609,122610,124037)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38337,38616));
SELECT 'custom-items',COUNT(*) FROM item_template WHERE entry IN(122609,122610,124037);
SELECT 'custom-item-flags',COUNT(*) FROM item_template_addon WHERE Id IN(122609,122610,124037);
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Built to Scale World query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content "$out/world.tsv"
foreach($line in @("quest`t38337`t34078720`t0`t824181837`t651`t0`t0`t0`t0`t0`t0`t0","quest`t38616`t34078720`t0`t234881970`t651`t0`t0`t0`t0`t0`t0`t0","objective`t38337`t277714`t1`t122609`t50`t0`t0","objective`t38616`t278226`t1`t122609`t50`t0`t0","spawns`t91202`t27`t0","spawns`t91486`t3`t0","spawns`t91799`t6`t0","loot`t91202`t122609`t35.9458`t1`t0`t0`t5`t15","loot`t91486`t122609`t26.613`t1`t0`t0`t5`t15","loot`t91799`t122609`t8.0226`t1`t0`t0`t5`t15","ender`t91481`t38337","ender`t92566`t38616")){if($rows -notcontains $line){throw "Built to Scale source contract changed: $line"}}
foreach($kind in @('npc-starters','go-starters','scripts','conditions','custom-items','custom-item-flags')){if($rows -notcontains "$kind`t0"){throw "Unexpected Built to Scale override: $kind"}}
$sql='SELECT ''items'',COUNT(*) FROM item_sparse WHERE ID IN(122609,122610,124037); SELECT ''effects'',COUNT(*) FROM item_effect WHERE ItemID IN(122609,122610,124037); SELECT ''models'',COUNT(*) FROM creature_display_info WHERE ID IN(70608,70589,70590); SELECT ''rewards'',COUNT(*) FROM quest_package_item WHERE PackageID=651; SELECT ''base-items'',COUNT(*) FROM item WHERE ID IN(122609,122610,124037); SELECT ''model-data'',COUNT(*) FROM creature_model_data WHERE ID IN(8755,8673);'
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Built to Scale native-reference overlay query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('items','effects','models','rewards','base-items','model-data')){if($rows -notcontains "$kind`t0"){throw "Unexpected Built to Scale native overlay: $kind"}}
'PASS: native item-started faction pair, mandatory50 scales, live drake loot/spawns/enders and absent custom/Hotfix overrides.'
