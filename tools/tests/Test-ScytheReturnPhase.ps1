param([switch]$AddressSanitizer,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/scythe-return-phase"
New-Item -ItemType Directory -Force $out|Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_26_08_azsuna_scythe_allari_return.sql" -Raw
$scope='SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry=3'
$sql=(@('conditions','phase_definitions','creature','quest_objectives')|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$reset=@"
DELETE FROM conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE $scope AND ElseGroup=1;
DELETE FROM phase_definitions; INSERT INTO phase_definitions SELECT * FROM saved_phase_definitions;
DELETE FROM creature; INSERT INTO creature SELECT * FROM saved_creature;
DELETE FROM quest_objectives; INSERT INTO quest_objectives SELECT * FROM saved_quest_objectives;
"@
$sql+="`n$reset`nSELECT 'BASELINE'; SELECT SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry BETWEEN 3 AND 7 ORDER BY SourceEntry,ElseGroup,ConditionTypeOrReference;"
$sql+="`n$migration`nSELECT 'FIRST'; SELECT * FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;"
$sql+="`n$migration`nSELECT 'SECOND'; SELECT * FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference; SELECT 'END';"
$sql+=@"
SELECT 'row',SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry BETWEEN 3 AND 7 ORDER BY SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'unchanged-other-conditions',COUNT(*) FROM conditions WHERE NOT ($scope);
SELECT 'saved-other-conditions',COUNT(*) FROM saved_conditions WHERE NOT ($scope);
SELECT 'actors',COUNT(*) FROM creature;
SELECT 'saved-actors',COUNT(*) FROM saved_creature;
SELECT 'allari',COUNT(*) FROM creature WHERE guid=271101 AND id=89398 AND PhaseId='4265';
"@
$variants=[ordered]@{
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionValue1) VALUES(23,7334,3,0,2,4,1);"
 'changed-base'="UPDATE conditions SET NegativeCondition=0 WHERE $scope;"
 'custom-script'="UPDATE conditions SET ScriptName='custom_return' WHERE $scope;"
 'changed-phase'="UPDATE phase_definitions SET phaseId='9999' WHERE zoneId=7334 AND entry=3;"
 'changed-actor'="UPDATE creature SET PhaseId='9999' WHERE guid=271101;"
 'missing-victory'='DELETE FROM quest_objectives WHERE ID=277159;'
 'changed-return'='UPDATE quest_objectives SET ObjectID=1 WHERE ID=286117;'
}
foreach($name in $variants.Keys){$sql+="`n$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope AND ElseGroup=1;"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Scythe phase SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}
}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/diagnostics.tsv",[string[]]$rows)
$first=[Array]::IndexOf($rows,'FIRST');$second=[Array]::IndexOf($rows,'SECOND');$end=[Array]::IndexOf($rows,'END')
if($second-$first -ne 4 -or (Compare-Object $rows[($first+1)..($second-1)] $rows[($second+1)..($end-1)])){throw 'Return phase migration not repeat-safe'}
foreach($name in $variants.Keys){if($rows -notcontains "$name`t0"){throw "Custom scope not refused: $name"}}
foreach($pair in @(@('unchanged-other-conditions','saved-other-conditions'),@('actors','saved-actors'))){
 $a=($rows|Where-Object {$_ -like "$($pair[0])`t*"}).Split("`t")[1];$b=($rows|Where-Object {$_ -like "$($pair[1])`t*"}).Split("`t")[1];if($a -ne $b){throw 'Unrelated row count changed'}
}
$baseline=[Array]::IndexOf($rows,'BASELINE')
[IO.File]::WriteAllLines("$out/baseline.tsv",[string[]]$rows[($baseline+1)..($first-1)])
[IO.File]::WriteAllLines("$out/fixed.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)}))
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing $start"};$source.Substring($first,$last-$first)}
$header=(Get-Content "$repo/src/server/game/Maps/PhaseMgr.h" -Raw) -replace '(?m)^#include.*$','' -replace 'class TC_GAME_API PhaseMgr','class PhaseMgr'
[IO.File]::WriteAllText("$out/PhaseHeader.inc",$header)
$phase=Get-Content "$repo/src/server/game/Maps/PhaseMgr.cpp" -Raw
$methods=@('PhaseMgr::PhaseMgr(','void PhaseMgr::Update(','void PhaseMgr::NotifyConditionChanged(','void PhaseMgr::Recalculate(','inline bool PhaseMgr::CheckDefinition(','bool PhaseMgr::NeedsPhaseUpdateWithData(','uint32 PhaseData::GetCurrentPhasemask(','inline uint32 PhaseData::GetPhaseMaskForSpawn(','void PhaseData::SendPhaseMaskToPlayer(','void PhaseData::AddPhaseDefinition(','void PhaseData::AddAuraInfo(','uint32 PhaseData::RemoveAuraInfo(','void PhaseUpdateData::AddQuestUpdate(','bool PhaseUpdateData::IsConditionRelated(')
[IO.File]::WriteAllText("$out/PhaseMethods.inc",(($methods|ForEach-Object {Block $phase $_}) -join "`n"))
$cond=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/Conditions.inc",(Section $cond '        case CONDITION_QUESTTAKEN:' '        case CONDITION_QUEST_COMPLETE:')+(Section $cond '        case CONDITION_QUEST_OBJECTIVE_DONE:' '        case CONDITION_INSTANCE_INFO:'))
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Block $cond 'bool ConditionMgr::IsObjectMeetToConditionList('))
[IO.File]::WriteAllText("$out/PhaseLookup.inc",(Block $cond 'ConditionList ConditionMgr::GetConditionsForPhaseDefinition('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$sanitize=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $sanitize /I`"$out`" `"$PSScriptRoot/ScytheReturnPhaseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/baseline.tsv`" baseline`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/fixed.tsv`" fixed`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native return phase regression failed'}
if($VerifyLiveData){
 $live=Query "SELECT ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;"
 $expected=@("0`t9`t0`t37660`t0`t0`t1","1`t9`t0`t37660`t0`t0`t0","1`t41`t0`t37660`t89673`t1`t0")
 if(($live -join '|') -ne ($expected -join '|')){throw 'Live Allari phase does not match tested repair'}
 'PASS exact live phase readback; existing native EXE handles both condition types.'
}
'PASS temporary migration twice, custom/missing guard refusal, native original deadlock and repaired phase lifecycle. No persistent data or character changes by this test.'
