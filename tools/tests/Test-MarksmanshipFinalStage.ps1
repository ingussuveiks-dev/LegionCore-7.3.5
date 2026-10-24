param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-final-stage"
$search="$repo/build-extractors/runtime-test/marksmanship-search-path"
$common="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipBowSummonRetry.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/summon-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_14_marksmanship_final_stage_cue.sql" -Raw
$normalization=Get-Content "$PSScriptRoot/MarksmanshipFinalStageBaseline.sql" -Raw
$scope='SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(50,52,64)'
$reset="DELETE FROM conditions WHERE $scope; DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart; DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;"
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts; INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100397;
$normalization
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart; INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE expected_smart LIKE saved_smart; INSERT INTO expected_smart SELECT * FROM saved_smart;
UPDATE expected_smart SET link=0 WHERE source_type=0 AND id=48;
UPDATE expected_smart SET link=0,event_type=52,event_param1=22,event_param2=100397,action_type=22,action_param1=3,action_param2=0,target_type=1,target_param1=0,comment='Marksmanship: arm final-stage permission only after the native Take dialogue timer' WHERE source_type=0 AND id=49;
UPDATE expected_smart SET event_phase_mask=6 WHERE source_type=0 AND id=62;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template; INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100397;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor; INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND NOT($scope);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
SELECT 'before-smart',expected_smart.* FROM expected_smart ORDER BY source_type,id;
SELECT 'before-gates',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first-count',COUNT(*) FROM conditions WHERE $scope;
$migration
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'new-row',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=63;
SELECT 'bow-mask',event_phase_mask FROM smart_scripts WHERE source_type=0 AND id=62;
SELECT 'after-smart',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=0 AND id=63) ORDER BY source_type,id;
SELECT 'after-gates',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'row',entryorguid,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE source_type=0 AND id IN(48,49,51,63) ORDER BY id;
SELECT 'gate',SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY SourceGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_actor';"
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-take-link'='UPDATE smart_scripts SET link=51 WHERE source_type=0 AND id=48;'
 'custom-speech'='UPDATE smart_scripts SET action_param1=23 WHERE source_type=0 AND id=48;'
 'custom-timing'='UPDATE smart_scripts SET action_param2=10000 WHERE source_type=0 AND id=48;'
 'custom-take-source'='UPDATE smart_scripts SET event_param1=20 WHERE source_type=0 AND id=48;'
 'custom-criteria'='UPDATE smart_scripts SET action_param2=47847 WHERE source_type=0 AND id=49;'
 'custom-range'='UPDATE smart_scripts SET target_param1=100 WHERE source_type=0 AND id=49;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE source_type=0 AND id=49;'
 'custom-credit'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=51;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE source_type=0 AND id=49;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=4 WHERE source_type=0 AND id=0;'
 'custom-phase-action'='UPDATE smart_scripts SET action_type=22 WHERE source_type=0 AND id=0;'
 'custom-search-phase'='UPDATE smart_scripts SET action_param1=3 WHERE source_type=0 AND id=43;'
 'custom-arm-phase'='UPDATE smart_scripts SET action_param1=3 WHERE source_type=0 AND id=50;'
 'custom-search-mask'='UPDATE smart_scripts SET event_phase_mask=4 WHERE source_type=0 AND id=61;'
 'custom-bow-mask'='UPDATE smart_scripts SET event_phase_mask=4 WHERE source_type=0 AND id=62;'
 'custom-bow-position'='UPDATE smart_scripts SET target_z=-50 WHERE source_type=0 AND id=62;'
 'custom-bow-retry'='UPDATE smart_scripts SET action_param3=0 WHERE source_type=0 AND id=62;'
 'custom-extra-source'='UPDATE smart_scripts SET link=49 WHERE source_type=0 AND id=47;'
 'occupied-slot'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100397,0,63,60,1,1,'Custom final stage');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
foreach($group in @(50,52,64)){$sql+="$reset`nINSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,$group,100397,0,1,4,1,1489,'Custom final-stage cue');`n$migration`nSELECT 'custom-gate-$group',COUNT(*) FROM conditions WHERE $scope;`nSELECT 'custom-preserved-$group',COUNT(*) FROM conditions WHERE Comment='Custom final-stage cue';`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Final-stage TEMP fixture failed'}}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
foreach($expected in @("first-count`t5","count`t5","new-row`t1","bow-mask`t6")+@($variants.Keys|ForEach-Object {"$_`t0"})+@(50,52,64|ForEach-Object {"custom-gate-$_`t1";"custom-preserved-$_`t1"})){if($rows -notcontains $expected){throw "Final-stage mismatch: $expected"}}
foreach($kind in @('smart','gates')){$before=@($rows|Where-Object {$_ -like "before-$kind`t*"}|ForEach-Object {$_.Substring(8+$kind.Length)});$after=@($rows|Where-Object {$_ -like "after-$kind`t*"}|ForEach-Object {$_.Substring(7+$kind.Length)});if(Compare-Object $before $after){throw "Unexpected final-stage $kind change"}}
foreach($kind in @('row','gate')){[IO.File]::WriteAllLines("$out/$kind.tsv",[string[]]@($rows|Where-Object {$_ -like "$kind`t*"}|ForEach-Object {$_.Substring($kind.Length+1)}))}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/CreditPlayer.inc",(Section $source 'Player* GetScriptCreditPlayer(' 'Unit* GetScriptCreditCaster('))
[IO.File]::WriteAllText("$out/ClosestPlayer.inc",(Section $source '        case SMART_TARGET_CLOSEST_PLAYER:' '        case SMART_TARGET_OWNER_OR_SUMMONER:'))
[IO.File]::WriteAllText("$out/CriteriaAction.inc",(Section $source '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$search`" /I`"$common`" `"$PSScriptRoot/MarksmanshipFinalStageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/row.tsv`" `"$out/gate.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native final-stage regression failed'}
& "$PSScriptRoot/Test-ScriptPetCredit.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/native-credit-prerequisite.log"
'PASS: only48 link,49 expiry/phase arm and62 phase mask change; new63 retains original criteria/recipient/link; five native stage/alive gates and custom-data preservation pass'
