param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-ambush-event"
New-Item -ItemType Directory -Force $out | Out-Null
$quests=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
$types=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.h" -Raw
$grids=Get-Content "$repo/src/server/game/Grids/GridDefines.h" -Raw
if($quests -notmatch 'QUEST_STATUS_INCOMPLETE\s*=\s*3' -or $quests -notmatch 'QUEST_STATUS_COMPLETE\s*=\s*1' -or $types -notmatch 'TYPEID_EVENTOBJECT\s*=\s*11' -or $grids -notmatch 'GRID_MAP_TYPE_MASK_EVENTOBJECT\s*=\s*0x80'){throw 'Update native fixture constants'}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Section $source 'namespace TalonclawAmbushEvent' 'namespace TitanstrikePrustaga'))
$source=Get-Content "$repo/src/server/game/Entities/EventObject/EventObject.cpp" -Raw
[IO.File]::WriteAllText("$out/EventUpdate.inc",(Section $source 'void EventObject::Update(' 'void EventObject::MoveInLineOfSight'))
[IO.File]::WriteAllText("$out/Activated.inc",$source.Substring($source.IndexOf('bool EventObject::ActivatedForPlayer(')))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/Legacy.inc",(Section $source 'class SmartEventObject : public EventObjectScript' 'void AddSC_SmartScripts()'))
$source=Get-Content "$repo/src/server/game/Entities/Object/ObjectVisitors.hpp" -Raw
$start=$source.IndexOf('template <typename Notifier>');$end=$source.IndexOf('template <typename Notifier>',$start+1)
[IO.File]::WriteAllText("$out/VisitAll.inc",$source.Substring($start,$end-$start))
$source=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.h" -Raw
$start=$source.IndexOf('struct WorldObjectWorker');$start=$source.IndexOf('        void Visit(EventObjectMapType &m)',$start);$end=$source.IndexOf('        template <typename NotInterested>',$start)
if($start -lt 0 -or $end -le $start){throw 'Native EventObject worker missing'}
[IO.File]::WriteAllText("$out/Worker.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TalonclawAmbushEventTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native ambush event lifecycle failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_33_talonclaw_ambush_event_lifecycle.sql" -Raw
$reset=@'
DELETE FROM eventobject_template; INSERT INTO eventobject_template SELECT * FROM saved_actor;
UPDATE eventobject_template SET ScriptName='SmartEventObject' WHERE entry=108 AND ScriptName='eventobject_talonclaw_ambush';
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM spell_scene; INSERT INTO spell_scene SELECT * FROM saved_scene;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE eventobject_template;
INSERT INTO saved_actor SELECT * FROM eventobject_template WHERE entry=108;
CREATE TEMPORARY TABLE eventobject_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=108 AND source_type=13;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=108;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;
INSERT INTO saved_scene SELECT * FROM spell_scene WHERE MiscValue=1145;
CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;
$reset
SELECT 'BEFORE',eventobject_template.* FROM eventobject_template;
SELECT 'SMARTBEFORE',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'CONDBEFORE',conditions.* FROM conditions;
$migration
SELECT 'FIRST',eventobject_template.* FROM eventobject_template;
$migration
SELECT 'SECOND',eventobject_template.* FROM eventobject_template;
SELECT 'SMARTAFTER',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'CONDAFTER',conditions.* FROM conditions;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE eventobject_template SET ScriptName='custom_event';"
 'radius'='UPDATE eventobject_template SET radius=25;'
 'template-spell'='UPDATE eventobject_template SET SpellID=197615;'
 'one-action'='UPDATE eventobject_template SET Flags=1;'
 'root-spell'='UPDATE smart_scripts SET action_param1=197355 WHERE id=2;'
 'root-target'='UPDATE smart_scripts SET target_type=1 WHERE id=2;'
 'link'='UPDATE smart_scripts SET link=0 WHERE id=0;'
 'data'='UPDATE smart_scripts SET action_param2=6 WHERE id=1;'
 'repeat'='UPDATE smart_scripts SET event_flags=0 WHERE id=2;'
 'phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=0;'
 'missing-gate'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3;'
 'wrong-source'='UPDATE conditions SET SourceId=10 WHERE SourceTypeOrReferenceId=22;'
 'wrong-step'='UPDATE conditions SET ConditionValue2=12 WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3;'
 'wrong-target'='UPDATE conditions SET ConditionTarget=1 WHERE SourceTypeOrReferenceId=22;'
 'scene-package'='UPDATE spell_scene SET SceneScriptPackageID=1513;'
 'scene-duration'='UPDATE spell_scene SET CustomDuration=1000;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',eventobject_template.* FROM eventobject_template;`n$migration`nSELECT '$name-after',eventobject_template.* FROM eventobject_template;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Lifecycle TEMP SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
$first=Snapshot 'FIRST';if($first.Count -ne 1 -or (Compare-Object $first (Snapshot 'SECOND'))){throw 'Lifecycle binding not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM eventobject_template;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'BEFORE'|ForEach-Object {$fields=$_.Split("`t");$fields[$columns.IndexOf('ScriptName')]='eventobject_talonclaw_ambush';$fields -join "`t"})
if(Compare-Object $first $expected){throw 'Lifecycle migration changed fields beyond script binding'}
foreach($prefix in @('SMART','COND')){if(Compare-Object (Snapshot "${prefix}BEFORE") (Snapshot "${prefix}AFTER")){throw "Native $prefix data modified"}}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom lifecycle overwritten: $name"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_33_talonclaw_ambush_event_lifecycle.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_33_talonclaw_ambush_event_lifecycle.sql'; SELECT 'binding',ScriptName,radius FROM eventobject_template WHERE entry=108;"
 foreach($line in @("release`t$hash`tRELEASED","binding`teventobject_talonclaw_ambush`t24")){if($live -notcontains $line){throw "Live lifecycle mismatch: $line"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_ambush_event_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed lifecycle missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: native cache/controller lifecycle, guarded binding-only migration and16 custom variants. Cast/spatial services are adapters; full scene playback/client traversal still pending.'
