param([string]$BaselineRevision,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-report-use"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
if($BaselineRevision){$source=(& git -C $repo show "${BaselineRevision}:src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read pickup baseline'}}
$acquisition=Section $source 'namespace ArtifactAcquisition' 'namespace DarkRidersCompletion'
$acquisition=$acquisition.Substring($acquisition.IndexOf('{')+1)
[IO.File]::WriteAllText("$out/Acquisition.inc",$acquisition.Substring(0,$acquisition.LastIndexOf('}')))
$class=if($source.Contains('class go_marksmanship_pickup')){'go_marksmanship_pickup'}else{'go_artifact_acquisition'}
[IO.File]::WriteAllText("$out/Binding.inc",(Section $source "class $class :" 'namespace TalonclawPresentation'))
[IO.File]::WriteAllText("$out/BindingChoice.inc","using PickupBinding = $class;")
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SmartLifecycle.inc",(Section $source 'SmartGameObjectAI::SmartGameObjectAI(' '// Called when a player selects a gossip item'))
[IO.File]::WriteAllText("$out/StateChanged.inc",(Section $source 'void SmartGameObjectAI::OnStateChanged(' 'void SmartGameObjectAI::EventInform('))
$source=Get-Content "$repo/src/server/game/AI/CreatureAISelector.cpp" -Raw
$first=$source.IndexOf('    GameObjectAI* SelectGameObjectAI(')
if($first -lt 0){throw 'Missing native gameobject AI selector'}
[IO.File]::WriteAllText("$out/SelectAI.inc",$source.Substring($first,$source.LastIndexOf('}')-$first))
$source=Get-Content "$repo/src/server/game/Handlers/SpellHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/ReportUse.inc",(Section $source 'void WorldSession::HandleGameobjectReportUse(' 'void WorldSession::HandleCastSpellOpcode('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipReportUseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Marksmanship report-use regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_20_marksmanship_report_use.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=248419;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=248419 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE gameobject_template SET ScriptName='go_artifact_acquisition' WHERE entry=248419;
$migration
$migration
SELECT 'binding',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='go_marksmanship_pickup';
SELECT 'object',COUNT(*) FROM gameobject_template current JOIN saved_go old USING(entry) WHERE current.Data0=old.Data0 AND current.Data1=old.Data1 AND current.flags=old.flags AND current.AIName=old.AIName;
SELECT 'actions',COUNT(*) FROM smart_scripts current JOIN saved_smart old USING(entryorguid,source_type,id) WHERE current.link=old.link AND current.event_type=old.event_type AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2 AND current.target_type=old.target_type AND current.event_flags=old.event_flags;
UPDATE gameobject_template SET ScriptName='custom_pickup' WHERE entry=248419;
$migration
SELECT 'custom-script',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='custom_pickup';
UPDATE gameobject_template SET ScriptName='go_artifact_acquisition' WHERE entry=248419;
UPDATE smart_scripts SET action_param1=123 WHERE entryorguid=248419 AND source_type=1 AND id=2;
$migration
SELECT 'custom-action',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='go_artifact_acquisition';
UPDATE smart_scripts SET action_param1=200279 WHERE entryorguid=248419 AND source_type=1 AND id=2;
UPDATE gameobject_template SET AIName='custom_ai' WHERE entry=248419;
$migration
SELECT 'custom-ai',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='go_artifact_acquisition' AND AIName='custom_ai';
UPDATE gameobject_template SET AIName='SmartGameObjectAI',Data1=123 WHERE entry=248419;
$migration
SELECT 'custom-loot',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='go_artifact_acquisition' AND Data1=123;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Report-use temporary migration fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("binding`t1","object`t1","actions`t4","custom-script`t1","custom-action`t1","custom-ai`t1","custom-loot`t1")){if($rows -notcontains $expected){throw "Report-use binding mismatch: $expected"}}
'PASS: idempotent report-use binding preserves native metadata/actions and custom scripts/actions/AI/loot'
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_report_use_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Deployed Marksmanship report-use wrapper failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
