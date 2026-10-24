param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-farthing"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Farthing.inc",(Section $source 'namespace XalatathFarthing' 'namespace XalatathSoulAttack'))
if(!$source.Contains('RegisterCreatureAI(npc_xalatath_farthing);')){throw 'Missing creature registration'}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SmartDamage.inc",(Section $source 'void SmartAI::DamageTaken(' 'void SmartAI::HealReceived('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$source.IndexOf('uint32 Unit::DealDamage(');$scaled=$source.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);',$first)
if($source.IndexOf('victim->GetAI()->DamageTaken(this, damage, damagetype);',$first) -ge $scaled){throw 'Native pre-scaling damage order changed'}
$hook=$source.IndexOf('    if (Creature* creature = victim->ToCreature())',$scaled)
if($first -lt 0 -or $scaled -lt $first -or $hook -lt $scaled){throw 'Native scaled hook missing'}
$end=$source.IndexOf('{',$hook)+1;$depth=1
while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
if($depth -or $source.IndexOf('    if (health <= damage)',$end) -lt $end){throw 'Native death branch missing after scaling hook'}
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",$source.Substring($scaled,$end-$scaled))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
if((Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw) -notmatch 'SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT\s*=\s*0x020'){throw 'Native HP-event flag changed'}
[IO.File]::WriteAllText("$out/NativeDie.inc",(Section $source '        case SMART_ACTION_DIE:' '        case SMART_ACTION_SET_IN_COMBAT_WITH_ZONE:'))
[IO.File]::WriteAllText("$out/HealthEvent.inc",(Section $source '        case SMART_EVENT_HEALTH_PCT:' '        case SMART_EVENT_TARGET_HEALTH_PCT:'))
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_41_xalatath_farthing_finale.sql" -Raw
$normalization=Get-Content "$PSScriptRoot/XalatathFarthingFinaleBaseline.sql" -Raw
$reset=@'
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
UPDATE creature_template SET ScriptName='' WHERE entry=101148;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=101148;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=101148;
$normalization
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
$reset
SELECT 'smart-before',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;
$migration
SELECT 'first',creature_template.* FROM creature_template;
$migration
SELECT 'second',creature_template.* FROM creature_template;
SELECT 'smart-after',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;
SELECT 'binding',AIName,ScriptName FROM creature_template;
UPDATE creature_template SET ScriptName='';
SELECT 'actor-restored',creature_template.* FROM creature_template;
SELECT 'actor-before',saved_actor.* FROM saved_actor;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom';"
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-invincibility'='UPDATE smart_scripts SET action_param1=2 WHERE source_type=0 AND id=24;'
 'custom-health'='UPDATE smart_scripts SET event_param2=2 WHERE source_type=0 AND id=25;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE source_type=0 AND id=26;'
 'custom-finale'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=28;'
 'custom-hit'='UPDATE smart_scripts SET event_param1=1 WHERE source_type=0 AND id=29;'
 'custom-delay'='UPDATE smart_scripts SET event_param1=0 WHERE source_type=9 AND id=0;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=1 WHERE source_type=9 AND id=0;'
 'custom-summon'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=9 AND id=1;'
 'custom-death'='UPDATE smart_scripts SET action_type=1 WHERE source_type=9 AND id=2;'
 'missing-action'='DELETE FROM smart_scripts WHERE source_type=0 AND id=28;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',creature_template.* FROM creature_template;`n$migration`nSELECT '$name-after',creature_template.* FROM creature_template;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Farthing TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
if($rows -notcontains "binding`tSmartAI`tnpc_xalatath_farthing"){throw 'Missing scoped binding'}
if(Compare-Object (Snapshot 'first') (Snapshot 'second')){throw 'Binding not idempotent'}
if(Compare-Object (Snapshot 'smart-before') (Snapshot 'smart-after')){throw 'Native33 SmartAI actions changed'}
$before=@((Snapshot 'actor-before') -replace 'npc_xalatath_farthing','')
if(Compare-Object $before (Snapshot 'actor-restored')){throw 'Other actor metadata changed'}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom actor changed: $name"}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$baselineFlag=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $baselineFlag /I`"$out`" `"$PSScriptRoot/XalatathFarthingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Farthing damage regression failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_41_xalatath_farthing_finale.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_41_xalatath_farthing_finale.sql'; SELECT 'binding',AIName,ScriptName FROM creature_template WHERE entry=101148;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "binding`tSmartAI`tnpc_xalatath_farthing"){throw 'Live Farthing binding/hash mismatch'}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_farthing_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Farthing policy failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: guarded Farthing binding preserves native SmartAI finale and metadata; final scaled damage policy is scoped to101148/1539/991/7-8 and retains native DIE. This does not certify cast201899, successful GO summon, native nearest-player credit or client visuals.'
