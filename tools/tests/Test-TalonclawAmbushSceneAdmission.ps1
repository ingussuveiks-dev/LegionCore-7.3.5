param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-ambush-scene-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_32_talonclaw_ambush_scene_admission.sql" -Raw
$reset=@'
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
UPDATE conditions SET SourceId=10 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=108 AND SourceId=13 AND SourceGroup IN(1,3) AND ConditionTypeOrReference=44 AND ConditionValue1=973;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM eventobject_template; INSERT INTO eventobject_template SELECT * FROM saved_actor;
DELETE FROM spell_scene; INSERT INTO spell_scene SELECT * FROM saved_scene;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=108;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=108 AND source_type=13;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_actor LIKE eventobject_template;
INSERT INTO saved_actor SELECT * FROM eventobject_template WHERE entry=108;
CREATE TEMPORARY TABLE eventobject_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;
INSERT INTO saved_scene SELECT * FROM spell_scene WHERE MiscValue=1145;
CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;
$reset
SELECT 'BEFORE',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceId;
SELECT 'SMARTBEFORE',smart_scripts.* FROM smart_scripts ORDER BY id;
$migration
SELECT 'FIRST',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceId;
$migration
SELECT 'SECOND',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceId;
SELECT 'SMARTAFTER',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'row',SourceGroup,SourceId,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=108 ORDER BY SourceGroup;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE eventobject_template SET ScriptName='custom_event';"
 'custom-radius'='UPDATE eventobject_template SET radius=25;'
 'custom-one-action'='UPDATE eventobject_template SET Flags=1;'
 'custom-spell'='UPDATE smart_scripts SET action_param1=197355 WHERE id=2;'
 'custom-invoker'='UPDATE smart_scripts SET target_type=1 WHERE id=2;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=0;'
 'custom-data'='UPDATE smart_scripts SET action_param2=6 WHERE id=1;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=2;'
 'custom-condition'='UPDATE conditions SET ConditionValue2=12 WHERE SourceGroup=3;'
 'custom-target'='UPDATE conditions SET ConditionTarget=1 WHERE SourceGroup=1;'
 'missing-condition'='DELETE FROM conditions WHERE SourceGroup=1;'
 'existing-source13'='UPDATE conditions SET SourceId=13 WHERE SourceGroup=1;'
 'custom-package'='UPDATE spell_scene SET SceneScriptPackageID=1513;'
 'custom-duration'='UPDATE spell_scene SET CustomDuration=1000;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceId;`n$migration`nSELECT '$name-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceId;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ambush scene TEMP SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
$first=Snapshot 'FIRST'
if(!$first.Count -or (Compare-Object $first (Snapshot 'SECOND'))){throw 'Scene conditions not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM conditions;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'BEFORE'|ForEach-Object {$fields=$_.Split("`t");if($fields[$columns.IndexOf('SourceTypeOrReferenceId')] -eq '22'){$fields[$columns.IndexOf('SourceId')]='13'};$fields -join "`t"})
if(Compare-Object $first $expected){throw 'Scene migration changed fields beyond SourceId'}
if(Compare-Object (Snapshot 'SMARTBEFORE') (Snapshot 'SMARTAFTER')){throw 'Scene migration changed native actions'}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom scene overwritten: $name"}}
$data=@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$fields=$_.Substring(4).Split("`t");if($Baseline){$fields[1]='10'};$fields -join "`t"})
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$data)
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/ConditionLookup.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded()'))
[IO.File]::WriteAllText("$out/ScenarioCondition.inc",(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/Dispatcher.inc",(Section $source 'class SmartEventObject : public EventObjectScript' 'void AddSC_SmartScripts()'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
if($source -notmatch 'SMART_SCRIPT_TYPE_EVENTOBJECT\s*=\s*13' -or $source -notmatch 'SMART_EVENT_EVENTOBJECT_ONTRIGGER\s*=\s*89'){throw 'Native EO source/event changed'}
$event=Get-Content "$repo/src/server/game/Entities/EventObject/EventObject.cpp" -Raw
if(!$event.Contains('if (ActivatedForPlayer(player))') -or !$event.Contains('affectedPlayers.insert(player->GetGUID());')){throw 'Re-audit native EO activation cache'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawAmbushSceneAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conditions.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native ambush scene admission failed'}
& "$PSScriptRoot/Test-SurvivalArtifactCatalog.ps1"
$catalog=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv" -Raw
foreach($needle in @("SCENE_TEXT`t13703`t7.0 Artifacts - Hunter - Surv - Boss Cave - Init - JMC",'textApataDeath1 = 102116','textApataDeath2 = 103432','local idApata = 101053','SetCameraFollowPath(368, 1, pathCam1Source, pathCam1Target, 15)','scene:ApplyScreenEffect(827)','SendTrigger("Despawn Fog")')){
 if(!$catalog.Contains($needle)){throw "Native death scene missing: $needle"}
}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_32_talonclaw_ambush_scene_admission.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_32_talonclaw_ambush_scene_admission.sql'; SELECT 'gate',SourceGroup,SourceId,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=108 ORDER BY SourceGroup; SELECT 'scene',SceneScriptPackageID,PlaybackFlags FROM spell_scene WHERE MiscValue=1145;"
 foreach($line in @("release`t$hash`tRELEASED","gate`t1`t13`t973`t10","gate`t3`t13`t973`t11","scene`t1517`t25")){if($live -notcontains $line){throw "Live scene mismatch: $line"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_ambush_scene_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical EO admission consumer missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: only two EO108 condition SourceIds corrected;14 custom variants/native actions retained, correct scene package/text/camera inspected. In-radius stage-change/retry cache and complete client scene remain unresolved separately.'
