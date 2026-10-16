param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-heavy-burden"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_05_stormheim_heavy_burden.sql" -Raw
$sql=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.RewardSpell,q.RewardNextQuest,q.StartScript,q.CompleteScript,a.NextQuestID,a.SourceSpellID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40078;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=40078;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.id=s.id AND e.quest=s.quest WHERE s.id=92539 AND s.quest=40078;
SELECT 'next',COUNT(*) FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID JOIN creature_queststarter s ON s.quest=q.ID WHERE q.ID=40001 AND a.PrevQuestID=40078 AND a.NextQuestID=40002 AND q.RewardNextQuest=40002 AND s.id=92539;
SELECT 'poi',COUNT(*) FROM quest_poi_points WHERE QuestID=40078 AND ((Idx1=1 AND X=3220 AND Y=1555) OR (Idx1=2 AND X=3194 AND Y=1520));
SELECT 'spear',COUNT(*) FROM gameobject WHERE guid=115015 AND id=244872 AND map=1220 AND PhaseId='' AND ABS(position_x-3194.48)<0.001 AND ABS(position_y-1519.95)<0.001 AND ABS(position_z-183.274)<0.001;
SELECT 'havi',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=92539 AND c.guid=291280 AND c.map=1220 AND c.PhaseId='' AND t.npcflag=3 AND t.AIName='SmartAI' AND t.ScriptName='';
SELECT 'no-spell-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(193563,193576,193577);
SELECT 'no-area',COUNT(*) FROM spell_area WHERE quest_start=40078 OR quest_end=40078 OR spell=193563;
"@
$tables=@{
 quest_template='ID IN(39855,40078,40001)';quest_template_addon='ID IN(39855,40078,40001)';quest_objectives='QuestID IN(40078,40001)';
 creature_queststarter='quest IN(40078,40001)';creature_questender='quest IN(40078,40001)';
 gameobject_template='entry IN(244867,244872,244859,253250)';gameobject='id IN(244867,244872,244859,253250) OR guid=147040078';
 creature_template='entry=92539';creature='id=92539';smart_scripts='(source_type=0 AND entryorguid=92539) OR (source_type=9 AND entryorguid IN(9253900,9253901))';
 spell_script_names='spell_id IN(193563,193576,193577,227837)';conditions='(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(193563,193576,193577)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry=40078)'
}
if(!$Live){
 foreach($table in ($tables.Keys | Sort-Object)){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+=@"
UPDATE quest_template_addon SET PrevQuestID=0,RequiredSkillID=185,RequiredSkillPoints=1 WHERE ID=40078;
UPDATE gameobject_template SET ScriptName='' WHERE entry IN(244867,244872);
DELETE FROM gameobject WHERE id=244867 AND map=1220 AND ABS(position_x-3220)<1 AND ABS(position_y-1555)<1;
DELETE FROM spell_script_names WHERE spell_id=193576 AND ScriptName='spell_stormheim_heavy_burden';
$migration
$migration
UPDATE saved_quest_template_addon SET PrevQuestID=39855,RequiredSkillID=0,RequiredSkillPoints=0 WHERE ID=40078;
UPDATE saved_gameobject_template SET ScriptName='go_stormheim_heavy_burden' WHERE entry IN(244867,244872);
INSERT INTO saved_spell_script_names(spell_id,ScriptName) SELECT 193576,'spell_stormheim_heavy_burden' WHERE NOT EXISTS(SELECT 1 FROM saved_spell_script_names WHERE spell_id=193576 AND ScriptName='spell_stormheim_heavy_burden');
"@
 foreach($table in ($tables.Keys | Sort-Object)){
  $exclude=if($table -eq 'gameobject'){' WHERE NOT(id=244867 AND map=1220 AND ABS(position_x-3220)<1 AND ABS(position_y-1555)<1)'}else{''}
  $sql+="SELECT 'actual-$table',t.* FROM $table t$exclude; SELECT 'expected-$table',t.* FROM saved_$table t$exclude;`n"
 }
}
$sql+=@"
SELECT 'prerequisite',COUNT(*) FROM quest_template_addon WHERE ID=40078 AND PrevQuestID=39855 AND RequiredSkillID=0 AND RequiredSkillPoints=0;
SELECT 'bindings',COUNT(*) FROM gameobject_template WHERE entry IN(244867,244872) AND type=10 AND Data1=40078 AND ScriptName='go_stormheim_heavy_burden' AND AIName='' AND flags=262148 AND Data3=3000 AND ((entry=244867 AND displayId=29179 AND size=1.5 AND Data0=1691 AND Data10=193563 AND Data22=36277) OR (entry=244872 AND displayId=29174 AND size=0.75 AND Data10=193577 AND Data22=36278));
SELECT 'barrel',COUNT(*) FROM gameobject WHERE guid=147040078 AND id=244867 AND map=1220 AND zoneId=7541 AND areaId=7803 AND spawnMask=1 AND phaseMask=1 AND PhaseId='' AND position_x=3220 AND position_y=1555 AND ABS(position_z-180.773)<0.001 AND orientation=0 AND rotation0=0 AND rotation1=0 AND rotation2=0 AND rotation3=1 AND spawntimesecs=180 AND animprogress=255 AND AiID=0 AND state=1 AND isActive=0 AND personal_size=0;
SELECT 'single-barrel',COUNT(*) FROM gameobject WHERE id=244867 AND map=1220 AND ABS(position_x-3220)<1 AND ABS(position_y-1555)<1;
SELECT 'spell-binding',COUNT(*) FROM spell_script_names WHERE spell_id=193576 AND ScriptName='spell_stormheim_heavy_burden';
"@
if(!$Live){$sql+=@"
UPDATE quest_template_addon SET PrevQuestID=123,RequiredSkillID=777,RequiredSkillPoints=50 WHERE ID=40078;
UPDATE gameobject_template SET ScriptName='custom_fish' WHERE entry=244867;
UPDATE gameobject_template SET ScriptName='custom_spear' WHERE entry=244872;
$migration
SELECT 'custom',COUNT(*) FROM quest_template_addon q JOIN gameobject_template b ON b.entry=244867 JOIN gameobject_template s ON s.entry=244872 WHERE q.ID=40078 AND q.PrevQuestID=123 AND q.RequiredSkillID=777 AND q.RequiredSkillPoints=50 AND b.ScriptName='custom_fish' AND s.ScriptName='custom_spear';
UPDATE gameobject_template SET ScriptName='go_stormheim_heavy_burden' WHERE entry=244867;
UPDATE gameobject SET guid=147040079 WHERE guid=147040078;
$migration
SELECT 'existing-barrel',COUNT(*) FROM gameobject WHERE id=244867 AND guid=147040079;
SELECT 'no-duplicate',COUNT(*) FROM gameobject WHERE id=244867 AND map=1220 AND ABS(position_x-3220)<1 AND ABS(position_y-1555)<1;
UPDATE gameobject SET guid=147040078,id=244859,position_x=1000 WHERE guid=147040079;
$migration
SELECT 'collision-preserved',COUNT(*) FROM gameobject WHERE guid=147040078 AND id=244859 AND position_x=1000;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Heavy Burden World query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("quest`t40078`t37748744`t8192`t227837`t40001`t0`t0`t40001`t0`t0","objective`t280283`t0`t1`t98239`t1`t0","relations`t1","next`t1","poi`t2","spear`t1","havi`t1","no-spell-condition`t0","no-area`t0","prerequisite`t1","bindings`t2","barrel`t1","single-barrel`t1","spell-binding`t1")){if($rows -notcontains $line){throw "Missing Heavy Burden data $line"}}
if(!$Live){
 foreach($table in $tables.Keys){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object)
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object)
  if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $table"}
 }
 foreach($line in @("custom`t1","existing-barrel`t1","no-duplicate`t1","collision-preserved`t1")){if($rows -notcontains $line){throw "Failed custom/existing/collision boundary $line"}}
}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql="SELECT 'effects',COUNT(*) FROM spell_effect WHERE SpellID IN(193563,193576,193577,227837); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(193563,193576,193577,227837); SELECT 'interrupts',COUNT(*) FROM spell_interrupts WHERE SpellID IN(193563,193576,193577); SELECT 'visuals',COUNT(*) FROM spell_x_spell_visual WHERE SpellID=193563; SELECT 'barrel-model',COUNT(*) FROM gameobjects WHERE ID=244867; SELECT 'anim-kit175',COUNT(*) FROM anim_kit WHERE ID=175; SELECT 'aura-restrictions',COUNT(*) FROM spell_aura_restrictions WHERE SpellID IN(193563,193576,193577,227837); SELECT 'target-restrictions',COUNT(*) FROM spell_target_restrictions WHERE SpellID IN(193576,193577); SELECT 'displays',COUNT(*) FROM gameobject_display_info WHERE ID IN(29179,29174); SELECT 'quest-line',COUNT(*) FROM quest_line_x_quest WHERE QuestID IN(39855,40078); SELECT 'poi-conditions',COUNT(*) FROM player_condition WHERE ID IN(45684,45685);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$hotfix=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Heavy Burden Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("effects`t0","misc`t0","interrupts`t0","visuals`t0","barrel-model`t0","anim-kit175`t0","aura-restrictions`t0","target-restrictions`t0","displays`t0","quest-line`t0","poi-conditions`t0")){if($hotfix -notcontains $line){throw "Unexpected Heavy Burden Hotfix $line"}}
$hotfix | Set-Content "$out/hotfix.tsv"
'PASS: real MySQL repeated40078 migration/full rows, story prerequisite/no cooking gate, native POIs/templates/spear/Havi/reward-next and unrelated content, one restored barrel, custom bindings and duplicate/GUID collision preservation.'
