param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-combat-dialogue"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_25_titanstrike_combat_dialogue.sql" -Raw
$reset='DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart; DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor; DELETE FROM spell_script_names;'
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=10671400 AND source_type=9;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=106714;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
$reset
SELECT 'BEFORE',smart_scripts.* FROM smart_scripts ORDER BY id;
$migration
SELECT 'FIRST',spell_id,ScriptName FROM spell_script_names ORDER BY spell_id;
$migration
SELECT 'SECOND',spell_id,ScriptName FROM spell_script_names ORDER BY spell_id;
SELECT 'AFTER',smart_scripts.* FROM smart_scripts ORDER BY id;
$reset
INSERT INTO spell_script_names VALUES(211826,'custom_combat_dialogue');
$migration
SELECT 'custom-binding',COUNT(*) FROM spell_script_names WHERE spell_id=211826 AND ScriptName='custom_combat_dialogue';
SELECT 'other-bindings',COUNT(*) FROM spell_script_names WHERE ScriptName='spell_titanstrike_combat_dialogue';
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-script'="UPDATE creature_template SET ScriptName='custom_thorim';"
 'custom-timer'='UPDATE smart_scripts SET event_param1=1000 WHERE id=6;'
 'custom-spell'='UPDATE smart_scripts SET action_param1=1 WHERE id=7;'
 'custom-flags'='UPDATE smart_scripts SET event_flags=0 WHERE id=8;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE id=6;'
 'custom-cast'='UPDATE smart_scripts SET action_param2=2 WHERE id=6;'
 'custom-chance'='UPDATE smart_scripts SET event_chance=50 WHERE id=6;'
 'custom-link'='UPDATE smart_scripts SET link=1 WHERE id=6;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=6;'
 'missing-action'='DELETE FROM smart_scripts WHERE id=8;'
 'extra-action'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(10671400,9,20,0,37,1,'Custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM spell_script_names;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($query | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Combat dialogue SQL failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
$first=@($rows | Where-Object {$_ -like "FIRST`t*"} | ForEach-Object {$_.Substring(6)})
$second=@($rows | Where-Object {$_ -like "SECOND`t*"} | ForEach-Object {$_.Substring(7)})
$expected=@(211824,211826,211827 | ForEach-Object {"$_`tspell_titanstrike_combat_dialogue"})
if(Compare-Object $first $second){throw 'Combat binding not idempotent'}
if(Compare-Object $first $expected){throw 'Missing native combat bindings'}
$before=@($rows | Where-Object {$_ -like "BEFORE`t*"} | ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "AFTER`t*"} | ForEach-Object {$_.Substring(6)})
if($before.Count -ne 9 -or (Compare-Object $before $after)){throw 'Native timed list changed'}
foreach($expected in @("custom-binding`t1","other-bindings`t2")){if($rows -notcontains $expected){throw 'Custom spell binding overwritten'}}
foreach($name in $variants.Keys){if($rows -notcontains "$name`t0"){throw "Unsupported list bound: $name"}}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing production section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Combat.inc",(Section $source 'namespace TitanstrikeCombatDialogue' 'class spell_titanstrike_combat_dialogue'))
$source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
[IO.File]::WriteAllText("$out/BaseUpdate.inc",(Section $source 'void Conversation::Update(uint32 p_time)' 'void Conversation::Remove()'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TitanstrikeCombatDialogueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native combat-dialogue regression failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeFinaleCatalog.cpp -ProbeName TitanstrikeFinaleCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("EFFECT`t211824`t0`t219`t0`t2152`t0`t18`t0","EFFECT`t211826`t0`t219`t0`t2153`t0`t18`t0","EFFECT`t211827`t0`t219`t0`t2154`t0`t18`t0","COMBAT_LINE`t4565`t110370`t5000`t4566","COMBAT_LINE`t4566`t110371`t1000`t0","COMBAT_LINE`t4567`t110372`t1000`t0","COMBAT_LINE`t4568`t110374`t1000`t0")){if($native -notcontains $expected){throw "Native combat mapping changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_25_titanstrike_combat_dialogue.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_25_titanstrike_combat_dialogue.sql'; SELECT 'binding',spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(211824,211826,211827);"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Combat migration not deployed'}
 foreach($identity in 211824,211826,211827){if($live -notcontains "binding`t$identity`tspell_titanstrike_combat_dialogue"){throw 'Missing live combat binding'}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_combat_dialogue_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical combat dialogue missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: native combat identities, unchanged nine-row list, idempotent bindings and$($variants.Count) unsupported-list variants; custom spell retained while other native bindings applied; no client playback claim"
