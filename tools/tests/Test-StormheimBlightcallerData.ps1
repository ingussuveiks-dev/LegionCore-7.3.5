$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-blightcaller"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@"
SELECT 'quest',q.ID,q.QuestType,q.MinLevel,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=38361;
SELECT 'objective',ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=38361;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=38361;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=38361;
SELECT 'actor',t.entry,w.KillCredit1,w.KillCredit2,t.faction,t.npcflag,t.unit_flags,t.unit_flags2,t.AIName,t.ScriptName,w.Displayid1 FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(91158,91414,91571,91581) ORDER BY t.entry;
SELECT 'spawns',id,COUNT(*),SUM(map=1220),SUM(modelid<>0),SUM(unit_flags<>0) FROM creature WHERE id IN(91158,91414,91571,91581) GROUP BY id ORDER BY id;
SELECT 'position',guid,id,position_x,position_y,position_z,PhaseId FROM creature WHERE id IN(91158,91414,91571,91581) ORDER BY id,guid;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(91158,91414,91571,91581) ORDER BY Entry;
SELECT 'model-info',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID IN(66644,62752,62718,62743) ORDER BY DisplayID;
SELECT 'spell',entry,spell,difficultyMask,Difficulties FROM creature_template_spell WHERE entry IN(91414,91571,91581) ORDER BY entry,spell;
SELECT 'addon',entry,auras FROM creature_template_addon WHERE entry IN(91414,91571,91581) ORDER BY entry;
SELECT 'start-script-count',COUNT(*) FROM quest_start_scripts WHERE id=38361;
SELECT 'quest-condition-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry=38361;
SELECT 'phase',zoneId,entry,phaseId FROM phase_definitions WHERE zoneId=7541 AND phaseId=6375;
SELECT 'phase-condition',ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=90 ORDER BY ElseGroup;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Blightcaller World query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($line in @("quest`t38361`t2`t98`t34603016`t65536`t234881970`t0`t0`t38332`t0`t0","objective`t277874`t0`t91581`t15`t0`t0","starter`t91158`t38361","ender`t91158`t38361","spawns`t91158`t4`t4`t0`t0","spawns`t91414`t7`t7`t0`t0","spawns`t91571`t1`t1`t0`t0","spawns`t91581`t24`t24`t0`t0","start-script-count`t0","quest-condition-count`t0","phase`t7541`t90`t6375","phase-condition`t0`t28`t38360`t0","phase-condition`t1`t8`t38360`t0")){if($rows -notcontains $line){throw "Missing Blightcaller contract: $line"}}
foreach($id in @(91158,91414,91571,91581)){if($rows -notcontains "scaling`t$id`t98`t110"){throw "Invalid active scaling$id"}}
if(@($rows | Where-Object {$_ -like "objective`t*"}).Count -ne 1){throw 'Unexpected Blightcaller objective'}
if(@($rows | Where-Object {$_ -like "position`t*"}).Count -ne 36){throw 'Missing source/giver positions'}
if(@($rows | Where-Object {$_ -like "spell`t*"}).Count -ne 10){throw 'Actor spells changed'}
if(@($rows | Where-Object {$_ -like "model-info`t*"}).Count -ne 4){throw 'Missing actor model metadata'}
$rows | Set-Content "$out/world.tsv"
$sql="SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(66644,62752,62718,62743); SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID IN(10608,49,53,55); SELECT 'faction',COUNT(*) FROM faction_template WHERE ID IN(84,118,1,2);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Blightcaller Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
if($rows -notcontains "display`t0" -or $rows -notcontains "model`t0" -or $rows -notcontains "faction`t0"){throw 'Unexpected native-reference override'}
$rows | Set-Content "$out/hotfix.tsv"
'PASS:38361 live objective15, native credit links,24 paratroopers/1 Thane/7 protected riders, four Nathanos copies, prerequisites/scaling/spells and phase conditions.'
