param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-soul-attack"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/SoulAttack.inc",(Section $source 'namespace XalatathSoulAttack' 'namespace XalatathPresentation'))
[IO.File]::WriteAllText("$out/FarthingAI.inc",(Section $source 'struct npc_xalatath_farthing : SmartAI' 'namespace XalatathSoulAttack'))
if(!$source.Contains('RegisterCreatureAI(npc_xalatath_soul_attack);')){throw 'Missing soul actor registration'}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeIsSummonedBy.inc",(Section $source 'void SmartAI::IsSummonedBy(' 'void SmartAI::DamageDealt('))
[IO.File]::WriteAllText("$out/NativeSpellHit.inc",(Section $source 'void SmartAI::SpellHit(' 'void SmartAI::DamageTaken('))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeInvoker.inc",(Section $source '        case SMART_TARGET_NONE:' '        case SMART_TARGET_ACTION_INVOKER_VEHICLE:'))
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$last=$source.IndexOf('        // Needs to be called after dealing damage/healing')
$first=$source.LastIndexOf('    if (spellHitTarget)',$last)
if($first -lt 0 -or $last -le $first){throw 'Native hit callback boundary missing'}
[IO.File]::WriteAllText("$out/NativeHitDispatch.inc",($source.Substring($first,$last-$first)+"`n    }`n"))
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_43_xalatath_soul_attack_retry.sql" -Raw
$normalization=Get-Content "$PSScriptRoot/XalatathActorRetryBaseline.sql" -Raw
$reset=@'
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
UPDATE creature_template SET ScriptName='' WHERE entry=102247;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET target_type=19,target_param1=101148,target_param2=30 WHERE entryorguid=102247 AND source_type=0 AND id=0;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM spell_script_names; INSERT INTO spell_script_names SELECT * FROM saved_bindings;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(101148,102247);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(101148,102247);
$normalization
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry IN(101148,102247)) OR (SourceTypeOrReferenceId=17 AND SourceEntry=201899);
DELETE FROM saved_conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101148 AND SourceId=0 AND SourceGroup IN(33,34);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
INSERT INTO saved_bindings SELECT * FROM spell_script_names WHERE spell_id=201899;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
$reset
SELECT 'actor-before',creature_template.* FROM creature_template ORDER BY entry;
SELECT 'smart-before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
$migration
SELECT 'first',creature_template.* FROM creature_template ORDER BY entry;
SELECT 'graph-first',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
$migration
SELECT 'second',creature_template.* FROM creature_template ORDER BY entry;
SELECT 'graph-second',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'binding',AIName,ScriptName FROM creature_template WHERE entry=102247;
SELECT 'target',target_type,target_param1,target_param2 FROM smart_scripts WHERE entryorguid=102247 AND source_type=0 AND id=0;
UPDATE creature_template SET ScriptName='' WHERE entry=102247;
UPDATE smart_scripts SET target_type=19,target_param1=101148,target_param2=30 WHERE entryorguid=102247 AND source_type=0 AND id=0;
SELECT 'actor-restored',creature_template.* FROM creature_template ORDER BY entry;
SELECT 'smart-restored',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET AIName='' WHERE entry=102247;"
 'custom-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=102247;"
 'custom-owner'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101148;"
 'custom-target'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=102247;'
 'custom-radius'='UPDATE smart_scripts SET target_param2=1 WHERE entryorguid=102247;'
 'custom-cast'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=102247;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE entryorguid=102247;'
 'custom-once'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=102247;'
 'custom-event'='UPDATE smart_scripts SET event_type=0 WHERE entryorguid=102247;'
 'custom-owner-hit'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=101148 AND source_type=0 AND id=29;'
 'extra-event'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(102247,0,1,54,1,'Custom');"
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,30,101148,0,22,1,1539,'Custom');"
 'custom-spell-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(17,201899,22,1539,'Custom');"
 'custom-spell-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(201899,'custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',creature_template.* FROM creature_template ORDER BY entry;`nSELECT '$name-graph-before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;`n$migration`nSELECT '$name-after',creature_template.* FROM creature_template ORDER BY entry;`nSELECT '$name-graph-after',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Soul attack TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}) -join "`n"}
foreach($pair in @(@('first','second'),@('graph-first','graph-second'),@('actor-before','actor-restored'),@('smart-before','smart-restored'))){if((Snapshot $pair[0]) -ne (Snapshot $pair[1])){throw "Snapshot mismatch $pair"}}
if($rows -notcontains "binding`tSmartAI`tnpc_xalatath_soul_attack" -or $rows -notcontains "target`t7`t0`t0"){throw 'Scoped soul binding/invoker target missing'}
foreach($name in $variants.Keys){if((Snapshot "$name-before") -ne (Snapshot "$name-after") -or (Snapshot "$name-graph-before") -ne (Snapshot "$name-graph-after")){throw "Custom content overwritten: $name"}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$baselineFlag=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $baselineFlag /I`"$out`" `"$PSScriptRoot/XalatathSoulAttackTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native soul attack regression failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_43_xalatath_soul_attack_retry.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_43_xalatath_soul_attack_retry.sql'; SELECT 'binding',AIName,ScriptName FROM creature_template WHERE entry=102247; SELECT 'target',target_type,target_param1,target_param2 FROM smart_scripts WHERE entryorguid=102247 AND source_type=0 AND id=0;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "binding`tSmartAI`tnpc_xalatath_soul_attack" -or $live -notcontains "target`t7`t0`t0"){throw 'Live soul binding/target/hash mismatch'}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_soul_attack_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical soul attack policy failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: guarded sole native201899 cast targets the exact summoner; selected SmartAI retry retains actual SpellHit acknowledgement. Full sequence, actor creation, criterion/loot/equip and visuals remain separate.'
