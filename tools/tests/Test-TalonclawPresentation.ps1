param([switch]$BaselineScene,[string]$BaselineRevision,[switch]$VerifyReleaseBinary,[switch]$Marksmanship)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$route=if($Marksmanship){'marksmanship'}else{'talonclaw'}
$out="$repo/build-extractors/runtime-test/$route-presentation"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -lt $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
if($BaselineRevision){$source=(& git -C $repo show "${BaselineRevision}:src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read presentation baseline'}}
$begin=if($Marksmanship){'namespace MarksmanshipPresentation'}else{'namespace TalonclawPresentation'}
$end=if(!$Marksmanship -and $source.Contains('namespace MarksmanshipPresentation')){'namespace MarksmanshipPresentation'}else{'namespace TalonclawReturn'}
[IO.File]::WriteAllText("$out/Presentation.inc",(Section $source $begin $end))
[IO.File]::WriteAllText("$out/Active.inc",(Section $source 'bool Active(Player*' 'bool Use(Player*'))
if($BaselineScene){$source=(& git -C $repo show 4449e4cc:src/server/game/Entities/Player/Player.cpp) -join "`n"}else{$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw}
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Section $source 'void Player::SendSpellScene(' 'void Player::HandleArenaDeserter()'))
$source=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Handlers.inc",$source.Substring($source.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
$source=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
[IO.File]::WriteAllText("$out/Owner.inc",(Section $source 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$first=$source.IndexOf('tempLastInvoker->AddDelayedEvent(200,');$last=$source.IndexOf('});',$first)
if($first -lt 0 -or $last -lt $first){throw 'Native deferred invoker cast missing'}
[IO.File]::WriteAllText("$out/DeferredInvoker.inc",$source.Substring($first,$last+3-$first))
[IO.File]::WriteAllText("$out/PickupCriteria.inc",(Section $source '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:').Replace('e.action.','event.action.').Replace('GetTargets(e,','GetTargets(event,'))
$source=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/ScenarioCriteriaSend.inc",'template<> '+(Section $source 'void AchievementMgr<Scenario>::SendCriteriaUpdate(' 'template<>' ))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$define=if($Marksmanship){'/DMARKSMANSHIP_PRESENTATION_TEST'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $define /I`"$out`" `"$PSScriptRoot/TalonclawPresentationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw "Native $route presentation regression failed"}
$migrationName=if($Marksmanship){'2026_10_23_18_marksmanship_presentation.sql'}else{'2026_10_23_11_talonclaw_presentation.sql'}
$migration=Get-Content "$repo/sql/updates/world/$migrationName" -Raw
$spell=if($Marksmanship){200279}else{205589}
$object=if($Marksmanship){248419}else{248834}
$criterion=if($Marksmanship){48210}else{47689}
$expectedActions=if($Marksmanship){3}else{2}
$replay=if($Marksmanship){" OR (id=2 AND link=1 AND event_type=64 AND event_param1=3 AND action_type=85 AND action_param1=200279 AND target_type=7)"}else{''}
$sql=@"
CREATE TEMPORARY TABLE spell_script_names (spell_id INT,ScriptName VARCHAR(128));
INSERT INTO spell_script_names VALUES($spell,'custom_scene'),(123,'other_spell');
$migration
$migration
SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=$spell AND ScriptName='spell_${route}_presentation';
SELECT 'preserved',COUNT(*) FROM spell_script_names WHERE ScriptName IN('custom_scene','other_spell');
SELECT 'native-order',COUNT(*) FROM smart_scripts WHERE entryorguid=$object AND source_type=1 AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND action_type=85 AND action_param1=$spell AND target_type=7) OR (id=1 AND event_type=61 AND action_type=205 AND action_param1=92 AND action_param2=$criterion AND target_type=7)$replay);
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Presentation temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
if($rows -notcontains "binding`t1" -or $rows -notcontains "preserved`t2"){throw 'Presentation binding idempotence/preservation failed'}
if($rows -notcontains "native-order`t$expectedActions"){throw 'Native pickup scene/criterion ordering changed'}
'PASS: additive repeated scene binding preserves unrelated handlers'
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  $verify=if($Marksmanship){'verify_marksmanship_presentation_release.py'}else{'verify_talonclaw_deferred_scene_release.py'}
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$verify" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Deployed deferred-scene admission check failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
