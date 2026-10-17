$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-hunting"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39789,39793) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags,Flags2 FROM quest_objectives WHERE QuestID IN(39789,39793) ORDER BY QuestID;
SELECT 'followup',ID,PrevQuestID,NextQuestID FROM quest_template_addon WHERE ID IN(39787,39786,39792) ORDER BY ID;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(39789,39793) ORDER BY quest;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(39789,39793) ORDER BY quest;
SELECT 'actor',t.entry,w.KillCredit1,w.KillCredit2,t.faction,t.npcflag,t.unit_flags,t.unit_flags2,w.Displayid1,t.lootid,w.FlagQuest FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(97091,107753,97828,97258,107675) ORDER BY t.entry;
SELECT 'spawns',id,COUNT(*),SUM(map=1220),SUM(modelid<>0) FROM creature WHERE id IN(97091,107753,97828,97258,107675) GROUP BY id ORDER BY id;
SELECT 'position',guid,id,position_x,position_y,position_z,PhaseId FROM creature WHERE id IN(97091,107753,97828,97258,107675) ORDER BY id,guid;
SELECT 'loot',Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Reference,Currency FROM creature_loot_template WHERE Item=138021;
SELECT 'pose',c.id,COALESCE(a.auras,'<none>'),COUNT(*) FROM creature c LEFT JOIN creature_addon a ON a.guid=c.guid WHERE c.id IN(97091,107753,97828) GROUP BY c.id,a.auras ORDER BY c.id,a.auras;
SELECT 'goat-state',c.unit_flags,COALESCE(a.auras,'<none>'),COUNT(*) FROM creature c LEFT JOIN creature_addon a ON a.guid=c.guid WHERE c.id=97828 GROUP BY c.unit_flags,a.auras ORDER BY c.unit_flags;
SELECT 'spawn-admission',id,SUM((spawnMask & 1)=0),SUM(PhaseId<>'') FROM creature WHERE id IN(97091,107753,97828) GROUP BY id ORDER BY id;
SELECT 'template-pose',entry,auras FROM creature_template_addon WHERE entry IN(97091,107753,97828) ORDER BY entry;
SELECT 'spell',entry,spell,difficultyMask,Difficulties FROM creature_template_spell WHERE entry IN(97091,107753,97828) ORDER BY entry,spell;
SELECT 'model-metadata',COUNT(*) FROM creature_model_info WHERE DisplayID IN(70178,70587,68749,65137,70227) AND BoundingRadius>0 AND CombatReach>0;
SELECT 'loot-selector',t.entry,t.flags_extra,w.Classification,w.VignetteID FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(97091,107753,97828) ORDER BY t.entry;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(97091,107753,97828,97258,107675) ORDER BY Entry;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(39789,39793)) OR (SourceTypeOrReferenceId=1 AND SourceEntry=138021);
SELECT 'start-scripts',COUNT(*) FROM quest_start_scripts WHERE id IN(39789,39793) OR (command=7 AND datalong IN(39789,39793));
SELECT 'item-overrides',COUNT(*) FROM item_template WHERE entry=138021;
SELECT 'item-flags',COUNT(*) FROM item_template_addon WHERE Id=138021;
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hunting World query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content "$out/world.tsv"
foreach($line in @("quest`t39789`t37748744`t2`t18446744073709551615`t0`t39793`t0`t39793`t0`t0`t17907","quest`t39793`t37748744`t0`t18446744073709551615`t0`t0`t39789`t0`t0`t0`t17909","objective`t39789`t279952`t0`t97091`t10`t0`t0`t0","objective`t39793`t279960`t1`t138021`t18`t0`t0`t0","loot`t97828`t138021`t64.694`t1`t0`t0`t1`t3`t0`t0","spawns`t97091`t43`t43`t0","spawns`t97828`t61`t61`t0","spawns`t107753`t3`t3`t0","spawns`t97258`t1`t1`t0","spawns`t107675`t1`t1`t0","starter`t97258`t39789","starter`t107675`t39793","ender`t107675`t39789","ender`t107675`t39793","pose`t97828`t29266`t15","pose`t97091`t145953`t13","model-metadata`t5","goat-state`t0`t<none>`t46","goat-state`t512`t29266`t15","spawn-admission`t97091`t0`t0","spawn-admission`t97828`t0`t0","spawn-admission`t107753`t0`t0","loot-selector`t97091`t0`t0`t0","loot-selector`t97828`t0`t0`t0","loot-selector`t107753`t0`t0`t0")){if($rows -notcontains $line){throw "Hunting contract changed: $line"}}
foreach($kind in @('conditions','start-scripts','item-overrides','item-flags')){if($rows -notcontains "$kind`t0"){throw "Unexpected hunting override: $kind"}}
foreach($id in @(97091,107753,97828,97258,107675)){if($rows -notcontains "scaling`t$id`t98`t110"){throw "Invalid hunting scaling: $id"}}
if(@($rows | Where-Object {$_ -like "spell`t*"}).Count -ne 9){throw 'Hunting spell count changed'}
$sql='SELECT ''item'',COUNT(*) FROM item_sparse WHERE ID=138021; SELECT ''base-item'',COUNT(*) FROM item WHERE ID=138021; SELECT ''effects'',COUNT(*) FROM item_effect WHERE ItemID=138021; SELECT ''display'',COUNT(*) FROM creature_display_info WHERE ID IN(70178,70587,68749,65137,70227); SELECT ''model'',COUNT(*) FROM creature_model_data WHERE ID IN(9036,4261,60,831); SELECT ''pose-effects'',COUNT(*) FROM spell_effect WHERE SpellID IN(145953,29266,213961,123169); SELECT ''rewards'',COUNT(*) FROM quest_package_item WHERE PackageID IN(17907,17909); SELECT ''faction'',COUNT(*) FROM faction_template WHERE ID IN(2030,7,2785,190,1,2);'
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hunting native overlay query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('item','base-item','effects','display','model','pose-effects','rewards','faction')){if($rows -notcontains "$kind`t0"){throw "Unexpected hunting native overlay: $kind"}}
'PASS: hunting kill10/item18/prerequisite/actual giver relations, native credit/loot,109 source/questgiver spawns, sleep/dead props, scaling and overlays.'
