param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/prustaga-damage"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing production section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Protection.inc",(Section $source 'namespace TitanstrikePrustaga' 'struct npc_titanstrike_final_prustaga'))
[IO.File]::WriteAllText("$out/Wrapper.inc",(Section $source 'struct npc_titanstrike_final_prustaga' 'namespace TitanstrikeAftermath'))
foreach($identity in @('npc_titanstrike_prustaga','npc_titanstrike_final_prustaga')){
 if(@(& rg -l -F "RegisterCreatureAI($identity)" "$repo/src/server/scripts" -g '*.cpp').Count -ne 1 -or @(& rg -l -F "struct $identity :" "$repo/src/server/scripts" -g '*.cpp').Count -ne 1){throw "Duplicate Prustaga definition/registration: $identity"}
}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SmartDamage.inc",(Section $source 'void SmartAI::DamageTaken(' 'void SmartAI::HealReceived('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$start=$source.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);')
$hook=$source.IndexOf('    if (Creature* creature = victim->ToCreature())',$start)
$end=$source.IndexOf('{',$hook)+1;$depth=1
while($depth){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
if($start -lt 0 -or $hook -lt $start -or $source.IndexOf('    if (health <= damage)',$end) -lt $end){throw 'Native post-scaling bridge/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",$source.Substring($start,$end-$start))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/HealthEvent.inc",(Section $source '        case SMART_EVENT_HEALTH_PCT:' '        case SMART_EVENT_TARGET_HEALTH_PCT:'))
[IO.File]::WriteAllText("$out/DieAction.inc",(Section $source '        case SMART_ACTION_DIE:' '        case SMART_ACTION_SET_IN_COMBAT_WITH_ZONE:'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/PrustagaDamageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Prustaga damage regression failed'}
$migration=(Get-Content "$repo/sql/updates/world/2026_10_24_19_titanstrike_prustaga_damage.sql" -Raw)+"`n"+(Get-Content "$repo/sql/updates/world/2026_10_24_20_titanstrike_final_prustaga_binding.sql" -Raw)
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=106730;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
UPDATE creature_template SET ScriptName='';
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=106730 AND source_type IN(0,9);
INSERT INTO saved_smart(entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,action_param2,target_type,target_param1,comment)
SELECT 106730,9,6,0,100,1,205,92,50997,21,100,'Pinned pre23 handoff'
WHERE NOT EXISTS(SELECT 1 FROM saved_smart WHERE source_type=9 AND id=6);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
$migration
$migration
SELECT 'binding',AIName,ScriptName FROM creature_template;
SELECT 'native-actions',COUNT(*) FROM smart_scripts current JOIN saved_smart old USING(entryorguid,source_type,id) WHERE current.link=old.link AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2 AND current.event_type=old.event_type AND current.event_flags=old.event_flags;
UPDATE creature_template SET ScriptName='custom_prustaga';
$migration
SELECT 'custom',ScriptName FROM creature_template;
UPDATE creature_template SET ScriptName='';
UPDATE smart_scripts SET action_param1=20 WHERE source_type=0 AND id=8;
$migration
SELECT 'custom-floor',ScriptName='' FROM creature_template;
UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=8;
UPDATE smart_scripts SET action_type=1 WHERE source_type=9 AND id=7;
$migration
SELECT 'custom-death',ScriptName='' FROM creature_template;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Prustaga TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("binding`tSmartAI`tnpc_titanstrike_final_prustaga","native-actions`t18","custom`tcustom_prustaga","custom-floor`t1","custom-death`t1")){if($rows -notcontains $expected){throw "Prustaga migration mismatch: $expected"}}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeIntroCatalog.cpp -ProbeName TitanstrikeIntroCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @("STEP`t1099`t2`t2480`t48019`t0","CRITERIA`t1099`t48020`t48019`t1`t30091`t92`t50921","CRITERIA`t1099`t48094`t48019`t1`t30125`t92`t50997","STEP`t1099`t3`t2481`t48021`t0")){
 if($native -notcontains $expected){throw "Native Prustaga stage mismatch: $expected"}
}
if($VerifyReleaseBinary){
 $query=@'
SELECT 'live-binding',AIName,ScriptName FROM creature_template WHERE entry=106730;
SELECT 'rune-binding',AIName,ScriptName FROM creature_template WHERE entry=104949;
SELECT 'live-count',source_type,COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type IN(0,9) GROUP BY source_type;
SELECT 'live-scaling',LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry=106730;
SELECT 'released',hash,state FROM updates WHERE name='2026_10_24_20_titanstrike_final_prustaga_binding.sql';
SELECT 'released-initial',hash,state FROM updates WHERE name='2026_10_24_19_titanstrike_prustaga_damage.sql';
SELECT 'released-handoff',hash,state FROM updates WHERE name='2026_10_24_23_titanstrike_aftermath_handoff.sql';
'@
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Prustaga live readback failed'}}finally{$env:MYSQL_PWD=$previous}
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_20_titanstrike_final_prustaga_binding.sql" -Algorithm SHA1).Hash
 $initialHash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_19_titanstrike_prustaga_damage.sql" -Algorithm SHA1).Hash
 $handoffHash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_23_titanstrike_aftermath_handoff.sql" -Algorithm SHA1).Hash
 $listCount=if($live -contains "released-handoff`t$handoffHash`tRELEASED"){7}else{8}
 foreach($expected in @("live-binding`tSmartAI`tnpc_titanstrike_final_prustaga","rune-binding`tSmartAI`tnpc_titanstrike_prustaga","live-count`t0`t10","live-count`t9`t$listCount","live-scaling`t98`t110","released`t$hash`tRELEASED","released-initial`t$initialHash`tRELEASED")){if($live -notcontains $expected){throw "Live Prustaga mismatch: $expected"}}
 $live | Set-Content -Encoding utf8 "$out/live.tsv"
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_prustaga_damage_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Prustaga protection missing'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: scoped native Prustaga protection, pinned pre23 binding migration and exact live handoff-version count; health event, surviving timed visuals and scripted death retained. Full client combat remains pending.'
