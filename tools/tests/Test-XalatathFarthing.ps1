param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$Stromkar,[switch]$AddressSanitizer,[switch]$PoseBaseline,[switch]$TerminalBaseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$route=if($Stromkar){'stromkar-zakajz'}else{'xalatath-farthing'}
$entry=if($Stromkar){104276}else{101148}
$binding=if($Stromkar){'npc_stromkar_zakajz'}else{'npc_xalatath_farthing'}
$migrationName=if($Stromkar){'2026_10_24_48_stromkar_zakajz_scaled_finale.sql'}else{'2026_10_24_41_xalatath_farthing_finale.sql'}
$out="$repo/build-extractors/runtime-test/$route"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
if($PoseBaseline){if(!$Stromkar -or $VerifyReleaseBinary -or $Baseline){throw 'Pose counterfactual requires only Stromkar'};$source=(git -C $repo show '5a6165442:src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp') -join "`n"}
if($TerminalBaseline){if(!$Stromkar -or $VerifyReleaseBinary -or $Baseline -or $PoseBaseline){throw 'Terminal counterfactual requires only Stromkar'};$source=(git -C $repo show '51b83ffc2:src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp') -join "`n"}
$begin=if($Stromkar){'namespace StromkarZakajz'}else{'namespace XalatathFarthing'}
$end=if($Stromkar){'namespace XalatathFarthing'}else{'namespace XalatathSoulAttack'}
[IO.File]::WriteAllText("$out/Farthing.inc",(Section $source $begin $end))
if(!$source.Contains("RegisterCreatureAI($binding);")){throw 'Missing creature registration'}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SmartDamage.inc",(Section $source 'void SmartAI::DamageTaken(' 'void SmartAI::HealReceived('))
if($Stromkar){
 $conditionsSource=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/StromkarConditionTail.inc",(Section $conditionsSource '    if (NegativeCondition)' 'uint32 Condition::GetSearcherTypeMaskForCondition()'))
}
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
if($Stromkar){$creatureSource=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw;[IO.File]::WriteAllText("$out/NativeRegeneration.inc",(Section $creatureSource 'void Creature::RegenerateHealth()' 'uint32 Creature::GetRegenerateInterval()'))}
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
$migration=Get-Content "$repo/sql/updates/world/$migrationName" -Raw
$normalization=if($Stromkar){''}else{(Get-Content "$PSScriptRoot/XalatathActorRetryBaseline.sql" -Raw)+"`n"+(Get-Content "$PSScriptRoot/XalatathFarthingFinaleBaseline.sql" -Raw)}
if($Stromkar){
 $bodyPath="$repo/sql/updates/world/2026_10_29_06_stromkar_terminal_body.sql"
 if(Test-Path $bodyPath){
  $bodyText=Get-Content $bodyPath -Raw
  $bodyHash=(Get-FileHash $bodyPath -Algorithm SHA1).Hash
  $fingerprint=($bodyText -split "`n"|Where-Object {$_ -like 'SET @stromkar_body_hash*'}).Replace('FROM smart_scripts','FROM saved_smart')
  $normalization+="`nSET SESSION group_concat_max_len=1048576;`n$fingerprint`n"
  $normalization+=@"
SET @released_body := @stromkar_body_hash='9f8568398a6bafd39b014b8a9cd8185b68e64fa68cea76cfad7a5a5163cc3ac1'
AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_29_06_stromkar_terminal_body.sql' AND hash='$bodyHash' AND state='RELEASED');
UPDATE saved_smart SET action_param2=0 WHERE entryorguid=104276 AND source_type=9 AND id=0 AND @released_body;
INSERT INTO saved_smart(entryorguid,source_type,id,event_chance,event_flags,action_type,target_type,comment)
SELECT 104276,9,3,100,1,41,1,'TS - Despawn' WHERE @released_body;
"@
 }
 $followupPath="$repo/sql/updates/world/2026_10_24_51_stromkar_thoradin_presence.sql"
 if(Test-Path $followupPath){
  $followupHash=(Get-FileHash $followupPath -Algorithm SHA1).Hash
  $normalization+=@"
DELETE FROM saved_smart WHERE entryorguid=104276 AND source_type=0 AND id=9 AND link=0 AND event_type=60 AND event_phase_mask=0 AND event_chance=100 AND event_flags=0
AND event_param1=0 AND event_param2=0 AND event_param3=500 AND event_param4=500 AND action_type=12 AND action_param1=104307 AND action_param2=8 AND action_param3=0
AND action_param4=0 AND action_param5=0 AND action_param6=0 AND target_type=8 AND target_param1=0 AND target_param2=0 AND target_param3=0
AND ABS(target_x-1886.91)<0.001 AND ABS(target_y-2194.33)<0.001 AND ABS(target_z-37.1166)<0.001 AND target_o=0
AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_24_51_stromkar_thoradin_presence.sql' AND hash='$followupHash' AND state='RELEASED');
"@
 }
}
$reset=@"
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
UPDATE creature_template SET ScriptName='' WHERE entry=$entry;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=$entry;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=$entry;
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
if($Stromkar){$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom';"
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-invincibility'='UPDATE smart_scripts SET action_param1=2 WHERE source_type=0 AND id=7;'
 'custom-health'='UPDATE smart_scripts SET event_param2=2 WHERE source_type=0 AND id=3;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE source_type=0 AND id=4;'
 'custom-weapon'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=5;'
 'custom-placement'='UPDATE smart_scripts SET target_x=0 WHERE source_type=0 AND id=5;'
 'custom-summon'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=6;'
 'custom-hit'='UPDATE smart_scripts SET event_param1=1 WHERE source_type=0 AND id=8;'
 'custom-delay'='UPDATE smart_scripts SET event_param1=0 WHERE source_type=9 AND id=2;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=1 WHERE source_type=9 AND id=2;'
 'missing-action'='DELETE FROM smart_scripts WHERE source_type=0 AND id=6;'
}}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',creature_template.* FROM creature_template;`n$migration`nSELECT '$name-after',creature_template.* FROM creature_template;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Farthing TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
if($rows -notcontains "binding`tSmartAI`t$binding"){throw 'Missing scoped binding'}
if(Compare-Object (Snapshot 'first') (Snapshot 'second')){throw 'Binding not idempotent'}
if(Compare-Object (Snapshot 'smart-before') (Snapshot 'smart-after')){throw 'Native33 SmartAI actions changed'}
$before=@((Snapshot 'actor-before') -replace $binding,'')
if(Compare-Object $before (Snapshot 'actor-restored')){throw 'Other actor metadata changed'}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom actor changed: $name"}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$baselineFlag=if($Baseline){'/DBASELINE'}else{''}
if($Stromkar){$baselineFlag+=' /DSTROMKAR_ZAKAJZ_TEST'}
if($AddressSanitizer){$baselineFlag+=' /fsanitize=address /Zi'}
if($PoseBaseline){$baselineFlag+=' /DPOSE_BASELINE'}
if($TerminalBaseline){$baselineFlag+=' /DTERMINAL_BASELINE'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $baselineFlag /I`"$out`" `"$PSScriptRoot/XalatathFarthingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Farthing damage regression failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/$migrationName" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='$migrationName'; SELECT 'binding',AIName,ScriptName FROM creature_template WHERE entry=$entry;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "binding`tSmartAI`t$binding"){throw 'Live finale binding/hash mismatch'}
 $previousPython=$env:PYTHONPATH
 $verifier=if($Stromkar){'verify_stromkar_zakajz_release.py'}else{'verify_xalatath_farthing_release.py'}
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$verifier" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical finale policy failed'}}finally{$env:PYTHONPATH=$previousPython}
}
"PASS: guarded $route binding preserves native SmartAI finale and metadata; post-scaled lethal policy and native bypass tested. Cast/summon/nearest-player credit and client finale remain separate."
