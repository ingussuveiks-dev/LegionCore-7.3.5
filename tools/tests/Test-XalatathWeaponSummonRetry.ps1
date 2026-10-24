param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-weapon-summon-retry"
$common="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
$reveal="$repo/build-extractors/runtime-test/marksmanship-bow-reveal"
$search="$repo/build-extractors/runtime-test/marksmanship-search-path"
$retry="$repo/build-extractors/runtime-test/marksmanship-bow-summon-retry"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipBowSummonRetry.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/native-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_42_xalatath_weapon_summon_retry.sql" -Raw
$normalization=Get-Content "$PSScriptRoot/XalatathFarthingFinaleBaseline.sql" -Raw
$reset=@'
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=101148;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template;
INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247690;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=101148;
$normalization
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101148 AND SourceGroup NOT IN(31,32);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'unchanged-before',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=9 AND id IN(1,2)) ORDER BY source_type,id;
SELECT 'actor-before',creature_template.* FROM creature_template;
SELECT 'object-before',gameobject_template.* FROM gameobject_template;
$migration
SELECT 'first',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;
SELECT 'first-gates',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'second',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;
SELECT 'second-gates',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'unchanged-after',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=9 AND id=1) AND NOT(source_type=0 AND id IN(30,31)) ORDER BY source_type,id;
SELECT 'actor-after',creature_template.* FROM creature_template;
SELECT 'object-after',gameobject_template.* FROM gameobject_template;
SELECT 'count',COUNT(*) FROM smart_scripts;
SELECT 'gate-count',COUNT(*) FROM conditions WHERE SourceGroup=31 AND SourceId=0;
SELECT 'old-death',COUNT(*) FROM smart_scripts WHERE source_type=9 AND id=2;
SELECT 'row',entryorguid,source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE source_type=9 AND id=1 OR source_type=0 AND id IN(30,31) ORDER BY source_type DESC,id;
SELECT 'gate',SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceGroup=31 AND SourceId=0 ORDER BY ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-actor'="UPDATE creature_template SET ScriptName='custom';"
 'custom-object'="UPDATE gameobject_template SET ScriptName='custom';"
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE source_type=0 AND id=24;'
 'custom-phase-action'='UPDATE smart_scripts SET action_type=22 WHERE source_type=0 AND id=24;'
 'custom-timer-type'='UPDATE smart_scripts SET action_param2=0 WHERE source_type=0 AND id=29;'
 'custom-delay'='UPDATE smart_scripts SET event_param1=0 WHERE source_type=9 AND id=0;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=1 WHERE source_type=9 AND id=0;'
 'custom-position'='UPDATE smart_scripts SET target_z=1 WHERE source_type=9 AND id=1;'
 'custom-duration'='UPDATE smart_scripts SET action_param2=1 WHERE source_type=9 AND id=1;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE source_type=9 AND id=1;'
 'custom-death'='UPDATE smart_scripts SET action_type=1 WHERE source_type=9 AND id=2;'
 'occupied-id'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(101148,0,30,60,1,1,'Custom action');"
 'custom-gate'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1) VALUES(22,31,101148,0,4,1539);"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;`nSELECT '$name-gate-before',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;`nSELECT '$name-gate-after',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Weapon retry TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Snapshot $label))}
foreach($expected in @("count`t34","gate-count`t3","old-death`t0")){if($rows -notcontains $expected){throw "Retry mismatch: $expected"}}
foreach($kind in @('unchanged','actor','object')){if(Compare-Object (Snapshot "$kind-before") (Snapshot "$kind-after")){throw "Changed $kind"}}
foreach($pair in @(@('first','second'),@('first-gates','second-gates'))){if(Compare-Object (Snapshot $pair[0]) (Snapshot $pair[1])){throw 'Retry not idempotent'}}
foreach($name in $variants.Keys){foreach($suffix in @('','-gate')){if((Signature "$name$suffix-before") -cne (Signature "$name$suffix-after")){throw "Custom variant changed: $name"}}}
foreach($kind in @('row','gate')){[IO.File]::WriteAllLines("$out/$kind.tsv",[string[]](Snapshot $kind))}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$hostSource=Get-Content "$PSScriptRoot/MarksmanshipBowSummonRetryTest.cpp" -Raw
[IO.File]::WriteAllText("$out/RetryHost.inc",$hostSource.Substring(0,$hostSource.IndexOf('int main(')))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeDie.inc",(Section $source '        case SMART_ACTION_DIE:' '        case SMART_ACTION_SET_IN_COMBAT_WITH_ZONE:'))
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/MapCondition.inc",(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$PSScriptRoot`" /I`"$retry`" /I`"$reveal`" /I`"$search`" /I`"$common`" `"$PSScriptRoot/XalatathWeaponSummonRetryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/row.tsv`" `"$out/gate.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native weapon retry lifecycle failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_42_xalatath_weapon_summon_retry.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_42_xalatath_weapon_summon_retry.sql'; SELECT 'row',entryorguid,source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE entryorguid=101148 AND (source_type=9 AND id=1 OR source_type=0 AND id IN(30,31)) ORDER BY source_type DESC,id; SELECT 'gate',SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101148 AND SourceGroup=31 AND SourceId=0 ORDER BY ConditionTypeOrReference; SELECT 'old-death',COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND id=2;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "old-death`t0"){throw 'Live retry hash/old death mismatch'}
 foreach($kind in @('row','gate')){$current=@($live|Where-Object {$_ -like "$kind`t*"}|ForEach-Object {$_.Substring($kind.Length+1)});if(Compare-Object (Snapshot $kind) $current){throw 'Live retry differs from tested graph'}}
}
'PASS: actual native creation pointer controls Farthing death; failed creation remains phase1 pending with500ms retries, scoped to living1539/991/8. Native20s credit and102247/201899 entry, client visuals/loot remain separate.'
