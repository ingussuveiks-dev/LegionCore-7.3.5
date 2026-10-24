param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-pickup-cast"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_17_titanstrike_pickup_cast_admission.sql" -Raw
$scope='SourceTypeOrReferenceId=17 AND SourceEntry=212010'
$reset=@"
DELETE FROM conditions WHERE $scope;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1;
INSERT INTO conditions SELECT * FROM saved_conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1;
UPDATE gameobject_template current JOIN saved_go old USING(entry) SET current.ScriptName=old.ScriptName;
UPDATE smart_scripts current JOIN saved_smart old USING(entryorguid,source_type,id) SET current.action_param1=old.action_param1,current.action_param2=old.action_param2;
DELETE FROM spell_script_names;
INSERT INTO spell_script_names SELECT * FROM saved_bindings;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=249812;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=249812 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
INSERT INTO saved_bindings SELECT * FROM spell_script_names WHERE spell_id=212010;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
INSERT INTO spell_script_names SELECT * FROM saved_bindings;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(212010,212011));
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$reset
SELECT 'baseline-count',COUNT(*) FROM conditions WHERE $scope;
$migration
$migration
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'copied',COUNT(*) FROM conditions current JOIN saved_conditions old ON current.ElseGroup=old.ElseGroup AND current.ConditionTypeOrReference=old.ConditionTypeOrReference AND current.ConditionTarget=old.ConditionTarget AND current.ConditionValue1=old.ConditionValue1 AND current.ConditionValue2=old.ConditionValue2 AND current.ConditionValue3=old.ConditionValue3 AND current.NegativeCondition=old.NegativeCondition WHERE current.SourceTypeOrReferenceId=17 AND current.SourceEntry=212010 AND old.SourceTypeOrReferenceId=22;
SELECT 'root',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1;
SELECT 'alive',COUNT(*) FROM conditions WHERE $scope AND ConditionTypeOrReference=36 AND ConditionTarget=0 AND ConditionValue1=0;
SELECT 'other',COUNT(*) FROM conditions current JOIN saved_conditions old ON current.SourceTypeOrReferenceId=old.SourceTypeOrReferenceId AND current.SourceEntry=old.SourceEntry AND current.ConditionTypeOrReference=old.ConditionTypeOrReference WHERE current.SourceTypeOrReferenceId=17 AND current.SourceEntry=212011;
SELECT 'saved-other',COUNT(*) FROM saved_conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=212011;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom_pickup';"
 'custom-spell'='UPDATE smart_scripts SET action_param1=212011 WHERE id=0;'
 'custom-flags'='UPDATE smart_scripts SET action_param2=2 WHERE id=0;'
 'custom-binding'="INSERT INTO spell_script_names VALUES(212010,'custom_scene');"
 'custom-stage'='UPDATE conditions SET ConditionValue2=3 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=44;'
 'custom-root-notice'='UPDATE conditions SET ErrorTextId=123 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=44;'
 'missing-root'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND ElseGroup=0 AND ConditionTypeOrReference=2;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
$sql+=@"
$reset
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(17,0,212010,0,0,4,0,1,'Custom cast condition');
$migration
SELECT 'custom-condition',COUNT(*) FROM conditions WHERE $scope;
SELECT 'custom-condition-kept',COUNT(*) FROM conditions WHERE $scope AND Comment='Custom cast condition' AND ConditionValue1=1;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Titanstrike cast TEMP fixture failed'}}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
foreach($expected in @("baseline-count`t0","count`t28","copied`t24","root`t24","alive`t4","custom-condition`t1","custom-condition-kept`t1")+@($variants.Keys|ForEach-Object {"$_`t0"})){if($rows -notcontains $expected){throw "Titanstrike cast mismatch: $expected"}}
if((($rows|Where-Object {$_ -like "other`t*"}) -replace '^other','') -ne (($rows|Where-Object {$_ -like "saved-other`t*"}) -replace '^saved-other','')){throw 'Unrelated cast conditions changed'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)}))
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -TitanstrikeRowsPath "$out/rows.tsv" -TitanstrikeCast
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
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TitanstrikePickupCastTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native delayed pickup cast consumer failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_pickup_cast_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed pickup cast consumer failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: repeated guarded source17 migration rechecks native Titanstrike final-stage ownership/quest/life at spell execution; predicate cases and delayed consumer tested separately, no early equip/scene effect sink on rejected context'
