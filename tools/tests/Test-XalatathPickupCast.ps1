param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-pickup-cast"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_40_xalatath_pickup_cast_admission.sql" -Raw
$scope='SourceTypeOrReferenceId=17 AND SourceEntry=201780'
$reset=@"
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE $scope;
DELETE FROM spell_script_names; INSERT INTO spell_script_names SELECT * FROM saved_bindings;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template;
INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247690;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=247690 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
INSERT INTO saved_bindings SELECT * FROM spell_script_names WHERE spell_id=201780;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=247690 OR (SourceTypeOrReferenceId=4 AND SourceGroup=247690) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(201780,201917,204919));
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'object-before',gameobject_template.* FROM gameobject_template;
SELECT 'smart-before',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'other-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'second',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'object-after',gameobject_template.* FROM gameobject_template;
SELECT 'smart-after',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'other-after',conditions.* FROM conditions WHERE NOT ($scope) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'count',COUNT(*) FROM conditions WHERE $scope AND SourceGroup=0 AND SourceId=0;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom';"
 'custom-ai'="UPDATE gameobject_template SET AIName='';"
 'custom-lock'='UPDATE gameobject_template SET Data0=1;'
 'custom-loot'='UPDATE gameobject_template SET Data1=0;'
 'custom-open-text'='UPDATE gameobject_template SET Data14=1;'
 'custom-spell'='UPDATE smart_scripts SET action_param1=201781 WHERE id=0;'
 'custom-flags'='UPDATE smart_scripts SET action_param2=2 WHERE id=2;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=1;'
 'custom-target'='UPDATE smart_scripts SET target_type=21 WHERE id=2;'
 'missing-action'='DELETE FROM smart_scripts WHERE id=3;'
 'custom-binding'="INSERT INTO spell_script_names VALUES(201780,'custom_scene');"
 'custom-stage'='UPDATE conditions SET ConditionValue2=7 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=44;'
 'custom-bank'='UPDATE conditions SET ConditionValue3=1 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=2;'
 'custom-error'='UPDATE conditions SET ErrorTextId=123 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=44;'
 'missing-root'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND ElseGroup=0 AND ConditionTypeOrReference=2;'
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(17,0,201780,0,0,4,0,1,'Custom cast condition');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Xalatath cast TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
if($rows -notcontains "count`t28"){throw 'Expected28 native cast conditions'}
if(Compare-Object (Snapshot 'first') (Snapshot 'second')){throw 'Cast conditions not idempotent'}
foreach($kind in @('object','smart','other')){if(Compare-Object (Snapshot "$kind-before") (Snapshot "$kind-after")){throw "Changed native $kind"}}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom variant changed: $name"}}
$nativeRows=Snapshot 'row'
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$nativeRows)
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -XalatathRowsPath "$out/rows.tsv" -XalatathRoot *> "$out/predicates-native.log"
if($LASTEXITCODE){throw 'Native Xalatath cast predicate matrix failed'}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
[IO.File]::WriteAllText("$out/CastConditions.inc",(Section $source '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/NotGroupedConditions.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForNotGroupedEntry(' 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent('))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$first=$source.IndexOf('tempLastInvoker->AddDelayedEvent(200,');$last=$source.IndexOf('});',$first)
if($first -lt 0 -or $last -lt $first){throw 'Native deferred invoker payload missing'}
[IO.File]::WriteAllText("$out/DeferredInvoker.inc",$source.Substring($first,$last+3-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /DPICKUP_SPELL_ID=201780 /I`"$out`" `"$PSScriptRoot/TitanstrikePickupCastTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native delayed Xalatath cast consumer failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_40_xalatath_pickup_cast_admission.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_40_xalatath_pickup_cast_admission.sql'; SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope AND SourceGroup=0 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference;"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Live cast migration hash mismatch'}
 $liveRows=@($live|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)})
 if(Compare-Object $nativeRows $liveRows){throw 'Live cast rules differ from tested TEMP rules'}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_pickup_cast_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native delayed cast consumer failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: guarded Xalatath source17 cast rules recheck nonbank blade/Priest40710/scenario991/8-9/life at execution; real SQL predicate matrix and native200ms consumer checked separately. Immediate48493 credit and later204919/201917 casts remain separate.'
