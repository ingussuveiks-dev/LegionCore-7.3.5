$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/new-blood-full"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/game/Quests/QuestDef.cpp" -Raw
$a=$source.IndexOf('void Quest::LoadQuestObjective(');$b=$source.IndexOf('void Quest::LoadQuestObjectiveVisualEffect(',$a)
[IO.File]::WriteAllText("$output/QuestObjectiveLoad.inc",$source.Substring($a,$b-$a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('    // The New Blood: the native smash spells');$b=$effects.IndexOf('    // Init dest coordinates',$a)
[IO.File]::WriteAllText("$output/JumpDestination.inc",$effects.Substring($a,$b-$a))
$scenario=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
$reward=[regex]::Match($scenario,'Quest const\* quest = scenarioId == 962[^;]+;').Value
if(!$reward){throw 'Missing manual hand-in rule'}
[IO.File]::WriteAllText("$output/RewardSelection.inc",$reward)
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_06_new_blood_full_sequence.sql" -Raw
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_13_07_new_blood_enslave_button.sql" -Raw)
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_13_08_new_blood_five_jailers.sql" -Raw)
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_13_09_new_blood_captors_and_allies.sql" -Raw)
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_13_10_new_blood_prisoner_lifecycle.sql" -Raw)
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_13_11_new_blood_archive_handoff.sql" -Raw)
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_14_02_new_blood_tome_transition.sql" -Raw)
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_14_03_new_blood_escape_hazards.sql" -Raw)
$fixture=(@('conditions','smart_scripts','gameobject_template','spell_target_position','spell_linked_spell','creature_template','spell_script_names','creature_movement_override') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+="`n$migration`n$migration`n"
$fixture+=@"
SELECT JSON_OBJECT('kind','script','entry',entryorguid,'source',source_type,'id',id,'link',link,'event',event_type,'phase',event_phase_mask,'flags',event_flags,'ep1',event_param1,'ep2',event_param2,'action',action_type,'a1',action_param1,'a2',action_param2,'a3',action_param3,'target',target_type,'t1',target_param1,'t2',target_param2,'x',target_x,'y',target_y,'z',target_z) FROM smart_scripts WHERE entryorguid IN(98771,98775,98783,98793,98836,99178,99516,99627,98831,98834,101460,245233,245234,245924,9883101,9917801,9917802);
SELECT JSON_OBJECT('kind','condition','source',SourceTypeOrReferenceId,'group',SourceGroup,'entry',SourceEntry,'sourceId',SourceId,'branch',ElseGroup,'type',ConditionTypeOrReference,'target',ConditionTarget,'v1',ConditionValue1,'v2',ConditionValue2,'v3',ConditionValue3,'negative',NegativeCondition) FROM conditions WHERE (SourceTypeOrReferenceId=23 AND SourceGroup=8392 AND SourceEntry=200) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(101460,98771,99178,98831,99627,245234)) OR (SourceTypeOrReferenceId=4 AND SourceGroup=245924) OR (SourceTypeOrReferenceId=18 AND SourceGroup IN(98775,98783,98793)) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(199336,200032)) OR (SourceTypeOrReferenceId=13 AND SourceEntry=200038);
SELECT JSON_OBJECT('kind','instantCaptureLinks','count',COUNT(*)) FROM spell_linked_spell WHERE spell_trigger=199362 AND spell_effect=199359;
SELECT JSON_OBJECT('kind','exit','map',target_map,'z',target_position_z) FROM spell_target_position WHERE id=196340;
SELECT JSON_OBJECT('kind','book','loot',Data1) FROM gameobject_template WHERE entry=245924;
SELECT JSON_OBJECT('kind','infernalDestination','map',target_map,'x',target_position_x,'y',target_position_y,'z',target_position_z) FROM spell_target_position WHERE id=199215;
SELECT JSON_OBJECT('kind','meteorBinding','count',COUNT(*)) FROM spell_script_names WHERE spell_id=199031 AND ScriptName='spell_new_blood_escape_meteor';
SELECT JSON_OBJECT('kind','creature','entry',entry,'faction',faction,'flags',unit_flags) FROM creature_template WHERE entry IN(98771,98775,98783,98793,98836,101460);
SELECT JSON_OBJECT('kind','captureScript','count',COUNT(*)) FROM spell_script_names WHERE spell_id=200033 AND ScriptName='spell_gen_new_blood_imprisoned';
SELECT JSON_OBJECT('kind','prisonerAI','count',COUNT(*)) FROM creature_template WHERE entry IN(98775,98783,98793) AND AIName='SmartAI' AND ScriptName='npc_new_blood_prisoner';
SELECT JSON_OBJECT('kind','prisonerGravity','count',COUNT(*)) FROM creature c JOIN creature_movement_override m ON m.SpawnId=c.guid WHERE c.map=1522 AND c.id IN(98775,98783,98793) AND m.Ground=1 AND m.Flight=1;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=$fixture | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names;if($LASTEXITCODE){throw 'SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
[IO.File]::WriteAllLines("$output/data.jsonl",[string[]]$rows)
$py='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $py -X utf8 "$PSScriptRoot/new_blood_sequence_test.py" "$output/data.jsonl"
if($LASTEXITCODE){throw 'Sequence data/native DB2 regression failed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/NewBloodObjectiveOrderTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Objective loader regression failed'}
$generic=Get-Content "$repo/src/server/scripts/Spells/spell_generic.cpp" -Raw
$a=$generic.IndexOf('class spell_gen_new_blood_imprisoned');$a=$generic.IndexOf('    void FilterCaster(', $a);$b=$generic.IndexOf('    void Register()', $a)
[IO.File]::WriteAllText("$output/ImprisonedTargets.inc",$generic.Substring($a,$b-$a))
$cmd=$cmd.Replace('NewBloodObjectiveOrderTest.cpp','NewBloodImprisonedTargetsTest.cpp')
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Imprisonment target regression failed'}
'PASS: full sequence data checks; in-game movement, pacing and rendering still need a client run.'
