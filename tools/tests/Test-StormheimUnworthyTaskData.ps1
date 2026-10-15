param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-unworthy-task"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_42_stormheim_unworthy_task.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('gameobject_template','quest_template','quest_template_addon','quest_start_scripts')){
  $filter=switch($table){gameobject_template{'entry IN(241778,241779)'}quest_template{'ID IN(39837,38324,38339)'}quest_template_addon{'ID IN(39837,38324,38339)'}quest_start_scripts{'id=39837'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+=@"
UPDATE gameobject_template SET type=0,Data0=0,Data1=1690,Data2=3000,Data3=0,Data4=21400,Data7=35984,Data14=0,Data20=0,Data26=0 WHERE entry=241778;
UPDATE quest_template SET StartScript=39837 WHERE ID=39837;
UPDATE quest_template_addon SET SpecialFlags=3 WHERE ID=39837;
DELETE FROM quest_start_scripts;
INSERT INTO quest_start_scripts VALUES(39837,0,7,39837,0,0,0,0,0,0),(39837,0,7,39837,0,0,0,0,0,0),(39837,0,7,39837,0,0,0,0,0,0),(39837,9,0,123,0,0,0,0,0,0);
$migration
$migration
SELECT 'retired',COUNT(*) FROM quest_start_scripts WHERE id=39837 AND command=7;
SELECT 'unrelated-script',COUNT(*) FROM quest_start_scripts WHERE id=39837 AND delay=9 AND command=0 AND datalong=123;
SELECT 'preserved-flag',SpecialFlags FROM quest_template_addon WHERE ID=39837;
"@
 # Whole-row comparison catches accidental changes to unrelated GO fields and quests.
 $expected=@('type=10','Data0=1691','Data1=39837','Data2=0','Data3=3000','Data4=0','Data7=0','Data14=21400','Data20=1','Data26=1') -join ','
 $sql+="UPDATE saved_gameobject_template SET $expected WHERE entry=241778; UPDATE saved_quest_template SET StartScript=0 WHERE ID=39837; UPDATE saved_quest_template_addon SET SpecialFlags=1 WHERE ID=39837;`n"
 foreach($table in @('gameobject_template','quest_template','quest_template_addon')){
  $key=if($table -eq 'gameobject_template'){'entry'}else{'ID'}
  $sql+="SELECT 'actual-$table',t.* FROM $table t ORDER BY $key; SELECT 'expected-$table',t.* FROM saved_$table t ORDER BY $key;`n"
 }
}
$sql+=@"
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39837;
SELECT 'cage',type,Data0,Data1,Data3,Data10,Data20,Data26 FROM gameobject_template WHERE entry=241778;
SELECT 'definition',StartScript,SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39837;
SELECT 'native-data',COUNT(*) FROM gameobject_template WHERE entry=241778 AND displayId=7469 AND flags=0 AND faction=0 AND AIName='' AND ScriptName='' AND Data2=0 AND Data4=0 AND Data5=0 AND Data6=0 AND Data7=0 AND Data8=0 AND Data9=0 AND Data11=0 AND Data12=0 AND Data13=0 AND Data14=21400 AND Data15=0 AND Data16=0 AND Data17=0 AND Data18=0 AND Data19=0 AND Data21=0 AND Data22=0 AND Data23=0 AND Data24=0 AND Data25=0 AND Data27=0 AND Data28=0 AND Data29=0 AND Data30=0 AND Data31=0 AND Data32=0;
SELECT 'spawn',COUNT(*) FROM gameobject WHERE guid=25164548 AND id=241778 AND map=1463 AND zoneId=7588 AND PhaseId='5578' AND state=1;
SELECT 'caged',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.guid=11546820 AND c.id=91531 AND c.map=1463 AND c.PhaseId='5578' AND t.npcflag=2 AND t.faction=35 AND t.ScriptName='';
SELECT 'freed',COUNT(*) FROM creature WHERE guid=11546819 AND id=91531 AND map=1463 AND PhaseId='5577';
SELECT 'later',COUNT(*) FROM creature WHERE guid=12742987 AND id=91531 AND map=1463 AND PhaseId='8306';
SELECT 'relations',COUNT(*) FROM creature_questender WHERE id=91531 AND quest=39837;
SELECT 'offers',COUNT(*) FROM creature_queststarter WHERE id=91531 AND quest IN(38324,38339);
SELECT 'next',q.RewardNextQuest,a.PrevQuestID,a.NextQuestID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39837;
SELECT 'parallel',COUNT(*) FROM quest_template_addon WHERE ID IN(38324,38339) AND PrevQuestID=39837 AND NextQuestID=38347 AND ExclusiveGroup=-38324;
SELECT 'phase',SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7588 AND SourceEntry IN(2,3) ORDER BY SourceEntry,ElseGroup;
SELECT 'phase-def',entry,phaseId FROM phase_definitions WHERE zoneId=7588 AND entry IN(2,3) ORDER BY entry;
SELECT 'no-spawn-script',COUNT(*) FROM gameobject_scripts WHERE id=25164548;
SELECT 'start-rows',COUNT(*) FROM quest_start_scripts WHERE id=39837 AND command=7 AND datalong=39837;
"@
if(!$Live){$sql+=@"
UPDATE gameobject_template SET ScriptName='custom_cage',type=0,Data0=0,Data1=1690,Data2=3000,Data7=35984 WHERE entry=241778;
UPDATE quest_template SET StartScript=12345 WHERE ID=39837;
UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=39837;
$migration
SELECT 'custom',ScriptName,type FROM gameobject_template WHERE entry=241778;
SELECT 'custom-start',StartScript FROM quest_template WHERE ID=39837;
SELECT 'custom-event-flag',SpecialFlags FROM quest_template_addon WHERE ID=39837;
-- A shared script must remain available to another quest.
UPDATE quest_template SET StartScript=39837 WHERE ID=38324;
INSERT INTO quest_start_scripts VALUES(39837,0,7,39837,0,0,0,0,0,0);
$migration
SELECT 'shared-script',COUNT(*) FROM quest_start_scripts WHERE id=39837 AND command=7;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Unworthy Task SQL check failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("objective`t280004`t2`t1`t241778`t1`t0","cage`t10`t1691`t39837`t3000`t0`t1`t1","definition`t0`t$(if($Live){0}else{1})","native-data`t1","spawn`t1","caged`t1","freed`t1","later`t1","relations`t1","offers`t2","next`t38339`t38818`t38324","parallel`t2","phase`t2`t0`t9`t39837`t0","phase`t3`t0`t28`t39837`t0","phase`t3`t1`t8`t39837`t0","phase-def`t2`t5578","phase-def`t3`t5577","no-spawn-script`t0")){if($rows -notcontains $line){throw "Missing: $line"}}
if(!$Live){
 foreach($table in @('gameobject_template','quest_template','quest_template_addon')){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)})
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)})
  if(!$actual.Count -or (Compare-Object $actual $expected)){throw "Unexpected $table changes"}
 }
 foreach($line in @("retired`t0","unrelated-script`t1","preserved-flag`t1","custom`tcustom_cage`t0","custom-start`t12345","custom-event-flag`t2","shared-script`t1")){if($rows -notcontains $line){throw "Missing: $line"}}
}
if($rows -notcontains "start-rows`t0"){throw 'Obsolete acceptance completion remains'}
$h=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$hotfixSql='SELECT (SELECT COUNT(*) FROM `lock` WHERE ID=1691)+(SELECT COUNT(*) FROM gameobject_display_info WHERE ID=7469)+(SELECT COUNT(*) FROM creature_display_info WHERE ID=63385)+(SELECT COUNT(*) FROM creature_model_data WHERE ID=2594)+(SELECT COUNT(*) FROM spell_effect WHERE SpellID=3365);'
try{$env:MYSQL_PWD=$h[3];$overrides=@($hotfixSql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($h[0]) --port=$($h[1]) --user=$($h[2]) --database=$($h[4]) --batch --raw --skip-column-names);if($LASTEXITCODE -or $overrides.Count -ne 1 -or $overrides[0] -ne '0'){throw 'Native cage catalog has unchecked Hotfix overrides'}}finally{$env:MYSQL_PWD=$old}
$rows+="hotfix-overrides`t0"
$rows | Set-Content "$out/data$(if($Live){'-live'}).tsv"
($rows | Where-Object {$_ -like "cage`t*"}) -replace '^cage\t','' | Set-Content "$out/cage.tsv"
'PASS: native cage fields/objective/phase/Colborn/parallel handoff; repeated SQL, whole-row boundaries, independent flags and custom/shared script preservation. Client visuals pending.'
