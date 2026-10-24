param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-bow-summon-retry"
$reveal="$repo/build-extractors/runtime-test/marksmanship-bow-reveal"
$search="$repo/build-extractors/runtime-test/marksmanship-search-path"
$common="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipBowReveal.ps1" *> "$out/reveal-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_13_marksmanship_bow_summon_retry.sql" -Raw
$normalization=(Get-Content "$PSScriptRoot/MarksmanshipFinalStageBaseline.sql" -Raw)+"`n"+(Get-Content "$PSScriptRoot/MarksmanshipSummonRetryBaseline.sql" -Raw)
$scope='SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup=63'
$reset="DELETE FROM conditions WHERE $scope; DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart; DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor; DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object; DELETE FROM gameobject; INSERT INTO gameobject SELECT * FROM saved_spawn;"
$sql=@"
SELECT 'shape',entryorguid,source_type,id,event_flags,link,action_param1,action_param2,action_param3,target_x,target_y,target_z,target_o FROM smart_scripts WHERE action_type=50 AND target_type=8 ORDER BY entryorguid,source_type,id;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100397;
$normalization
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart; INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE expected_smart LIKE saved_smart; INSERT INTO expected_smart SELECT * FROM saved_smart;
UPDATE expected_smart SET action_type=22,action_param1=2,action_param2=0,target_type=1,target_x=0,target_y=0,target_z=0,target_o=0,comment='Marksmanship: retain pending bow creation after discovery speech' WHERE source_type=0 AND id=50;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template; INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100397;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor; INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template; INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=248419;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object; INSERT INTO gameobject_template SELECT * FROM saved_object;
CREATE TEMPORARY TABLE saved_spawn LIKE gameobject; INSERT INTO saved_spawn SELECT * FROM gameobject WHERE id=248419;
CREATE TEMPORARY TABLE gameobject LIKE saved_spawn; INSERT INTO gameobject SELECT * FROM saved_spawn;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND NOT($scope);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
SELECT 'before-smart',expected_smart.* FROM expected_smart ORDER BY source_type,id;
SELECT 'before-gates',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'before-object',gameobject_template.* FROM gameobject_template;
$migration
SELECT 'first-count',COUNT(*) FROM conditions WHERE $scope;
$migration
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'new-row',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=62;
SELECT 'after-smart',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=0 AND id=62) ORDER BY source_type,id;
SELECT 'after-gates',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'after-object',gameobject_template.* FROM gameobject_template;
SELECT 'row',entryorguid,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE source_type=0 AND id IN(45,50,62) ORDER BY id;
SELECT 'gate',SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(46,51,63) ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_actor';"
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-go-script'="UPDATE gameobject_template SET ScriptName='custom_bow';"
 'custom-model'='UPDATE gameobject_template SET displayId=1;'
 'custom-loot'='UPDATE gameobject_template SET Data1=1;'
 'custom-flags'='UPDATE gameobject_template SET flags=16;'
 'custom-position'='UPDATE smart_scripts SET target_z=-50 WHERE source_type=0 AND id=50;'
 'custom-duration'='UPDATE smart_scripts SET action_param2=300 WHERE source_type=0 AND id=50;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE source_type=0 AND id=50;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE source_type=0 AND id=50;'
 'custom-speech'='UPDATE smart_scripts SET action_param1=20 WHERE source_type=0 AND id=45;'
 'custom-timing'='UPDATE smart_scripts SET action_param2=6000 WHERE source_type=0 AND id=45;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=47847 WHERE source_type=0 AND id=49;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=2 WHERE source_type=0 AND id=0;'
 'custom-phase-action'='UPDATE smart_scripts SET action_type=22 WHERE source_type=0 AND id=0;'
 'custom-search'='UPDATE smart_scripts SET event_phase_mask=2 WHERE source_type=0 AND id=61;'
 'custom-extra-source'='UPDATE smart_scripts SET link=50 WHERE source_type=0 AND id=44;'
 'occupied-slot'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100397,0,62,60,1,1,'Custom retry');"
 'custom-arm-gate'="UPDATE conditions SET ConditionValue2=7 WHERE SourceGroup=51 AND ConditionTypeOrReference=44;"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
$sql+="$reset`nINSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,63,100397,0,1,4,1,1489,'Custom bow retry');`n$migration`nSELECT 'custom-gate',COUNT(*) FROM conditions WHERE $scope;`nSELECT 'custom-preserved',COUNT(*) FROM conditions WHERE Comment='Custom bow retry';`n"
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Bow retry TEMP fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("first-count`t4","count`t4","new-row`t1","custom-gate`t1","custom-preserved`t1")+@($variants.Keys|ForEach-Object {"$_`t0"})){if($rows -notcontains $expected){throw "Bow retry mismatch: $expected"}}
foreach($kind in @('smart','gates','object')){$before=@($rows|Where-Object {$_ -like "before-$kind`t*"}|ForEach-Object {$_.Substring(8+$kind.Length)});$after=@($rows|Where-Object {$_ -like "after-$kind`t*"}|ForEach-Object {$_.Substring(7+$kind.Length)});if(Compare-Object $before $after){throw "Unexpected bow retry $kind change"}}
foreach($kind in @('row','gate','shape')){[IO.File]::WriteAllLines("$out/$kind.tsv",[string[]]@($rows|Where-Object {$_ -like "$kind`t*"}|ForEach-Object {$_.Substring($kind.Length+1)}))}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=if($Baseline){(& git -C $repo show '3e67822dc6c8a77262ab7a119fa7c969a225f587:src/server/game/AI/SmartScripts/SmartScript.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw}
if(!$source){throw 'Missing pinned or current summon source'}
[IO.File]::WriteAllText("$out/SummonGo.inc",(Section $source '        case SMART_ACTION_SUMMON_GO:' '        case SMART_ACTION_KILL_UNIT:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/ValidateSummon.inc",(Section $source '        case SMART_ACTION_SUMMON_GO:' '        case SMART_ACTION_ADD_ITEM:'))
$define=if($Baseline){'/DSUMMON_RETRY_BASELINE'}else{''}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $define /I`"$out`" /I`"$reveal`" /I`"$search`" /I`"$common`" `"$PSScriptRoot/MarksmanshipBowSummonRetryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/row.tsv`" `"$out/gate.tsv`" `"$out/shape.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native bow summon retry regression failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_fixed_summon_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical fixed summon PE check failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: native50 becomes a phase2 arm, unused62 copies its original summon, source63 permits only living972/6-7; repeat migration preserves original dialogue/credit/pickup data and custom definitions'
