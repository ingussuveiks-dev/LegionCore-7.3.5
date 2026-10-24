param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-ambush-damage"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing production section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Protection.inc",(Section $source 'namespace TalonclawAmbush' 'struct npc_talonclaw_ambush_dakarr'))
[IO.File]::WriteAllText("$out/Wrapper.inc",(Section $source 'struct npc_talonclaw_ambush_dakarr' 'namespace TitanstrikePrustaga'))
foreach($needle in @('RegisterCreatureAI(npc_talonclaw_ambush_dakarr)','struct npc_talonclaw_ambush_dakarr :')){
 if(@(& rg -l -F $needle "$repo/src/server/scripts" -g '*.cpp').Count -ne 1){throw "Ambush definition/registration count: $needle"}
}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SmartDamage.inc",(Section $source 'void SmartAI::DamageTaken(' 'void SmartAI::HealReceived('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$start=$source.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);')
$hook=$source.IndexOf('    if (Creature* creature = victim->ToCreature())',$start)
$end=$source.IndexOf('{',$hook)+1;$depth=1
while($depth){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
if($start -lt 0 -or $hook -lt $start -or $source.IndexOf('    if (health <= damage)',$end) -lt $end){throw 'Native post-scaling/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",$source.Substring($start,$end-$start))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/HealthEvent.inc",((Section $source '        case SMART_EVENT_HEALTH_PCT:' '        case SMART_EVENT_TARGET_HEALTH_PCT:') -creplace '\be\b','nativeEvent'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TalonclawAmbushDamageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Talonclaw ambush damage regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_30_talonclaw_ambush_damage.sql" -Raw
$reset=@'
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
UPDATE creature_template SET ScriptName='' WHERE entry=100376;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(100376,99480,100235);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100376 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100376 AND SourceId=0;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'SMARTBEFORE',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'CONDBEFORE',conditions.* FROM conditions ORDER BY SourceGroup;
$migration
$migration
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=100376 AND AIName='SmartAI' AND ScriptName='npc_talonclaw_ambush_dakarr';
SELECT 'SMARTAFTER',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'CONDAFTER',conditions.* FROM conditions ORDER BY SourceGroup;
SELECT 'other',COUNT(*) FROM creature_template current JOIN saved_actor old USING(entry) WHERE current.entry<>100376 AND current.ScriptName=old.ScriptName AND current.AIName=old.AIName;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET AIName='' WHERE entry=100376;"
 'custom-script'="UPDATE creature_template SET ScriptName='custom_dakarr' WHERE entry=100376;"
 'first-threshold'='UPDATE smart_scripts SET event_param2=74 WHERE id=2;'
 'second-threshold'='UPDATE smart_scripts SET event_param2=59 WHERE id=6;'
 'floor'='UPDATE smart_scripts SET action_param2=69 WHERE id=9;'
 'first-credit'='UPDATE smart_scripts SET action_param2=47688 WHERE id=4;'
 'second-credit'='UPDATE smart_scripts SET action_param2=47688 WHERE id=7;'
 'first-link'='UPDATE smart_scripts SET link=0 WHERE id=2;'
 'second-link'='UPDATE smart_scripts SET link=0 WHERE id=6;'
 'wuho'='UPDATE smart_scripts SET action_param2=3 WHERE id=2;'
 'apata'='UPDATE smart_scripts SET action_param1=5 WHERE id=6;'
 'despawn'='UPDATE smart_scripts SET action_param1=5000 WHERE id=5;'
 'repeat'='UPDATE smart_scripts SET event_flags=0 WHERE id=6;'
 'phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=6;'
 'chance'='UPDATE smart_scripts SET event_chance=50 WHERE id=2;'
 'difficulty'="UPDATE smart_scripts SET Difficulties='1' WHERE id=9;"
 'missing-row'='DELETE FROM smart_scripts WHERE id=9;'
 'condition-stage'='UPDATE conditions SET ConditionValue2=8 WHERE SourceGroup=3;'
 'condition-target'='UPDATE conditions SET ConditionTarget=0 WHERE SourceGroup=7;'
 'missing-condition'='DELETE FROM conditions WHERE SourceGroup=10;'
}
foreach($name in $variants.Keys){
 $sql+="`n$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM creature_template WHERE entry=100376 AND ScriptName='npc_talonclaw_ambush_dakarr';`n"
}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ambush TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("binding`t1","other`t2")){if($rows -notcontains $expected){throw "Ambush SQL mismatch: $expected"}}
foreach($name in $variants.Keys){if($rows -notcontains "$name`t0"){throw "Custom ambush variant modified: $name"}}
foreach($prefix in @('SMART','COND')){
 $before=@($rows|Where-Object {$_ -like "$($prefix)BEFORE`t*"}|ForEach-Object {$_.Substring($prefix.Length+7)})
 $after=@($rows|Where-Object {$_ -like "$($prefix)AFTER`t*"}|ForEach-Object {$_.Substring($prefix.Length+6)})
 if(!$before.Count -or (Compare-Object $before $after)){throw "Native $prefix rows changed"}
}
$rows | Set-Content -Encoding utf8 "$out/sql.tsv"
& "$PSScriptRoot/Test-SurvivalArtifactCatalog.ps1"
$native=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
foreach($expected in @("CRITERION`t9`t46249`t29250`t92`t47640`t0","CRITERION`t10`t46288`t29250`t92`t47640`t0")){
 if($native -notcontains $expected){throw "Native repeated ambush stage changed: $expected"}
}
if($VerifyReleaseBinary){
 $query=@'
SELECT 'live-binding',AIName,ScriptName FROM creature_template WHERE entry=100376;
SELECT 'live-scaling',LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry=100376;
SELECT 'released',hash,state FROM updates WHERE name='2026_10_24_30_talonclaw_ambush_damage.sql';
'@
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Live ambush readback failed'}}finally{$env:MYSQL_PWD=$previous}
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_30_talonclaw_ambush_damage.sql" -Algorithm SHA1).Hash
 foreach($expected in @("live-binding`tSmartAI`tnpc_talonclaw_ambush_dakarr","live-scaling`t98`t110","released`t$hash`tRELEASED")){
  if($live -notcontains $expected){throw "Live ambush mismatch: $expected"}
 }
 $live | Set-Content -Encoding utf8 "$out/live.tsv"
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_ambush_damage_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical ambush protection missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: guarded idempotent ambush binding,20 custom variants and unchanged ten native SmartAI/three condition rows. Native repeated47640 stages remain separate; full client traversal pending.'
