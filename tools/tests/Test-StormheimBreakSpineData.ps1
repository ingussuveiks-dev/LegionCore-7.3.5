param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-break-spine"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_13_stormheim_break_spine.sql" -Raw
$sql=''
if(!$Live){
 $sql=@"
CREATE TEMPORARY TABLE saved_objectives LIKE quest_objectives;
INSERT INTO saved_objectives SELECT * FROM quest_objectives WHERE QuestID IN(40003,40004,40005);
CREATE TEMPORARY TABLE quest_objectives LIKE saved_objectives;
INSERT INTO quest_objectives SELECT * FROM saved_objectives;
UPDATE quest_objectives SET Flags=0 WHERE QuestID=40004;
$migration
$migration
SELECT 'actual',q.* FROM quest_objectives q ORDER BY ID;
SELECT 'expected',q.ID,q.QuestID,q.Type,q.StorageIndex,q.ObjectID,q.Amount,IF(q.QuestID=40004,q.Flags|1,q.Flags),q.Flags2,q.TaskStep,q.Description,q.VerifiedBuild,q.Bugged FROM saved_objectives q ORDER BY ID;
UPDATE quest_objectives SET Flags=16 WHERE ID=280178;
$migration
SELECT 'custom-bit',Flags FROM quest_objectives WHERE ID=280178;
UPDATE quest_objectives SET ObjectID=999,Flags=0 WHERE ID=280178;
$migration
SELECT 'custom-target',Flags FROM quest_objectives WHERE ID=280178;
"@
}
$sql+=@"
SELECT 'quest',q.ID,q.QuestType,q.MinLevel,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40004;
SELECT 'objective',ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=40004 ORDER BY ID;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=40004;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=40004;
SELECT 'spawn',c.guid,c.id,c.position_x,c.position_y,c.position_z,c.map,c.zoneId,c.areaId,c.spawnMask,c.phaseMask,c.PhaseId,c.modelid,c.unit_flags,c.npcflag,t.faction,t.unit_flags,t.AIName,t.ScriptName,w.KillCredit1,w.KillCredit2,w.Displayid1,w.Displayid2,w.Displayid3,w.Displayid4 FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE c.id IN(97822,97859,97942,97944) ORDER BY c.id;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(97822,97859,97942,97944) ORDER BY Entry;
SELECT 'spell',entry,spell,difficultyMask,Difficulties FROM creature_template_spell WHERE entry IN(97822,97859,97942,97944) ORDER BY entry,spell;
SELECT 'model',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID IN(63482,64046,64645,65514) ORDER BY DisplayID;
SELECT 'scripts',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(97822,97859,97942,97944,-291435,-291507,-291594,-146706021);
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry=40004;
SELECT 'disabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry=40004;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Break the Spine World query failed'}}finally{$env:MYSQL_PWD=$previous}
if(!$Live){
 $actual=@($rows | Where-Object {$_ -like "actual`t*"} | ForEach-Object {$_.Substring(7)})
 $expected=@($rows | Where-Object {$_ -like "expected`t*"} | ForEach-Object {$_.Substring(9)})
 if(!$actual.Count -or (Compare-Object $actual $expected)){throw 'Migration changed unrelated objective fields/rows'}
 if($rows -notcontains "custom-bit`t17" -or $rows -notcontains "custom-target`t0"){throw 'Migration lost custom flag/target protection'}
 # Restore the actual four objectives for the regression input, after the mutation-only cases.
 $rows=@($rows | Where-Object {$_ -notlike "objective`t*"})
 foreach($r in $actual){$f=$r.Split("`t");if($f[1] -eq '40004'){$rows+="objective`t$($f[0])`t$($f[2])`t$($f[4])`t$($f[5])`t$($f[3])`t$($f[6])"}}
}
foreach($expected in @("starter`t97979`t40004","ender`t97986`t40004","scripts`t0","conditions`t0","disabled`t0","objective`t280178`t0`t97822`t1`t3`t1","objective`t280179`t0`t97859`t1`t4`t1","objective`t280180`t0`t97944`t1`t5`t1","objective`t280181`t0`t97942`t1`t6`t1")){if($rows -notcontains $expected){throw "Missing World evidence $expected"}}
$acceptedQuestRows=@("quest`t40004`t2`t98`t38797320`t8192`t18446744073709551615`t0`t0`t40002`t0`t-40003`t0")
if(!($rows | Where-Object {$_ -in $acceptedQuestRows})){throw 'Unexpected Break the Spine chain metadata'}
if(@($rows | Where-Object {$_ -like "spawn`t*"}).Count -ne 4 -or @($rows | Where-Object {$_ -like "spell`t*"}).Count -ne 10){throw 'Missing lieutenant/spell rows'}
foreach($entry in @(97822,97859,97942,97944)){if($rows -notcontains "scaling`t$entry`t98`t110"){throw 'Inactive scaling range'}}
$rows | Set-Content "$out/$(if($Live){'world-live'}else{'world'}).tsv"
$sql="SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(63482,64046,64645,65514); SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID IN(68,8162,8246,8420); SELECT 'faction',COUNT(*) FROM faction_template WHERE ID=14; SELECT 'quest-line',COUNT(*) FROM quest_line_x_quest WHERE QuestID IN(40002,40003,40004,40005);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Break the Spine Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($name in @('display','model','faction','quest-line')){if($rows -notcontains "$name`t0"){throw "Unexpected native-reference Hotfix overlay $name"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: four native tracking bits, repeat migration, full-row preservation, custom flags/targets and live lieutenant/quest data.'
