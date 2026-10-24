param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-weapon-phase"
New-Item -ItemType Directory -Force $out|Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_56_warswords_weapon_phase.sql" -Raw
$tables=@('gameobject','gameobject_template','phase_definitions','conditions')
$reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$reset+="`nUPDATE gameobject SET PhaseId='7001 7003' WHERE guid=134150;"
$sql=@'
CREATE TEMPORARY TABLE saved_gameobject LIKE gameobject;
INSERT INTO saved_gameobject SELECT * FROM gameobject WHERE map=1511;
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry=248832;
CREATE TEMPORARY TABLE saved_phase_definitions LIKE phase_definitions;
INSERT INTO saved_phase_definitions SELECT * FROM phase_definitions WHERE zoneId=7541;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541;
'@
foreach($table in $tables){$sql+="`nCREATE TEMPORARY TABLE $table LIKE saved_$table;"}
$sql+="$reset`nSELECT 'before',current.* FROM gameobject current ORDER BY guid;SELECT 'old-phase',PhaseId FROM gameobject WHERE guid=134150;`n$migration`nSELECT 'first',current.* FROM gameobject current ORDER BY guid;SELECT 'new-phase',PhaseId FROM gameobject WHERE guid=134150;`n$migration`nSELECT 'second',current.* FROM gameobject current ORDER BY guid;`n"
foreach($table in @('gameobject_template','phase_definitions','conditions')){$sql+="SELECT '$table-before',current.* FROM saved_$table current ORDER BY 2,3,4;SELECT '$table-after',current.* FROM $table current ORDER BY 2,3,4;`n"}
$sql+="SELECT 'condition',SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceEntry IN(50,51,52) ORDER BY SourceEntry,ElseGroup;`n"
$variants=[ordered]@{
 'custom-phase'="UPDATE gameobject SET PhaseId='7003' WHERE guid=134150;"
 'custom-map'='UPDATE gameobject SET map=1220 WHERE guid=134150;'
 'custom-position'='UPDATE gameobject SET position_z=100 WHERE guid=134150;'
 'custom-mask'='UPDATE gameobject SET phaseMask=2 WHERE guid=134150;'
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom';"
 'custom-ai'="UPDATE gameobject_template SET AIName='SmartGameObjectAI';"
 'custom-template-flags'='UPDATE gameobject_template SET flags=16;'
 'custom-phase-stage'='UPDATE conditions SET ConditionValue2=5 WHERE SourceEntry=51;'
 'custom-phase-gate'='UPDATE conditions SET NegativeCondition=1 WHERE SourceEntry=51;'
 'custom-definition'="UPDATE phase_definitions SET phaseId='7003' WHERE entry=51;"
 'extra-gate'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1) VALUES(23,7541,51,0,0,9,0,40043);"
 'extra-spawn'='UPDATE gameobject SET id=248832 WHERE guid=134149;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',current.* FROM gameobject current ORDER BY guid;`n$migration`nSELECT '$name-after',current.* FROM gameobject current ORDER BY guid;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$sql){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Warswords TEMP phase fixture failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
function Rows([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Rows $label))}
foreach($pair in @(@('first','second'))+@($variants.Keys|ForEach-Object {,@("$_-before","$_-after")})+@('gameobject_template','phase_definitions','conditions'|ForEach-Object {,@("$_-before","$_-after")})){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Phase fixture changed unrelated/custom content: $pair"}}
if((Rows 'old-phase') -cne '7001 7003' -or (Rows 'new-phase') -cne '7001'){throw 'Expected only final-stage7001 on weapon'}
$expectedBefore=@(Rows 'before')
$expectedAfter=@(Rows 'first')
if($expectedBefore.Count -ne $expectedAfter.Count){throw 'Spawn count changed'}
for($index=0;$index -lt $expectedBefore.Count;++$index){$expected=$expectedBefore[$index];if($expected -like "134150`t*"){$expected=$expected.Replace("`t7001 7003`t","`t7001`t")};if($expected -cne $expectedAfter[$index]){throw 'Changed more than one exact weapon PhaseId'}}
foreach($entry in @(@('old-phase','old.tsv'),@('new-phase','new.tsv'),@('condition','conditions.tsv'))){[IO.File]::WriteAllLines("$out/$($entry[1])",[string[]](Rows $entry[0]))}
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Maps/PhaseMgr.cpp" -Raw
foreach($entry in @(@('Recalculate','void PhaseMgr::Recalculate()'),@('CheckDefinition','inline bool PhaseMgr::CheckDefinition('),@('AddDefinition','void PhaseData::AddPhaseDefinition('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$source=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
foreach($entry in @(@('SetPhases','void WorldObject::SetPhaseId('),@('SamePhaseIds','bool WorldObject::InSamePhaseId(std::set'),@('SameObjectPhaseIds','bool WorldObject::InSamePhaseId(WorldObject'),@('SamePhase','bool WorldObject::InSamePhase(WorldObject'))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
function Section([string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -le $first){throw 'Missing native phase condition'};$source.Substring($first,$last-$first)}
[IO.File]::WriteAllText("$out/PhaseConditions.inc",(Section '        case CONDITION_QUESTTAKEN:' '        case CONDITION_QUEST_COMPLETE:')+(Section '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:'))
[IO.File]::WriteAllText("$out/PhaseElseGroups.inc",(Block $source 'bool ConditionMgr::IsObjectMeetToConditionList('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/WarswordsWeaponPhaseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conditions.tsv`" `"$out/old.tsv`" `"$out/new.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Warswords phase regression failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_56_warswords_weapon_phase.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_56_warswords_weapon_phase.sql';SELECT 'phase',PhaseId FROM gameobject WHERE guid=134150 AND id=248832 AND map=1511;"
 if($live -notcontains "release`t$($hash.ToLower())`tRELEASED" -or $live -notcontains "phase`t7001"){throw 'Migration not deployed to canonical world data'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_weapon_phase_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native phase consumers failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: idempotent guarded weapon-only phase migration; shared environment, phase definitions, conditions and custom content retained. Native phase admission is separate from actual client visuals, cached-GUID Use, loot/equip and return.'
