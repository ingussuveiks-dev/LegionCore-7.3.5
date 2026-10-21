param([switch]$VerifyReleaseBinary, [string]$ReleaseDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/scenario-lfg-lifecycle"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$marker) {
 $a=$source.IndexOf($marker,[StringComparison]::Ordinal)
 if($a -lt 0){throw "Missing production block $marker"}
 $b=$source.IndexOf('{',$a);$end=$b+1;$depth=1
 while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){$depth++};if($source[$end] -eq '}'){$depth--};$end++}
 if($depth){throw "Unbalanced production block $marker"}
 $source.Substring($a,$end-$a)
}
function Save([string]$name,[string]$value){[IO.File]::WriteAllText("$out/$name.inc",$value)}
$mgr=Get-Content "$repo/src/server/game/DungeonFinding/LFGMgr.cpp" -Raw
$state=Get-Content "$repo/src/server/game/DungeonFinding/LFG.h" -Raw
Save 'States' ((Block $state 'enum LfgState')+';')
Save 'UpdateTypes' ((Block $state 'enum LfgUpdateType')+';')
Save 'ScenarioTypes' ((Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.h" -Raw) 'enum ScenarioType')+';')
foreach($pair in @(
 @('Setup','void LFGMgr::SetupGroupMember('),@('RemoveGroup','void LFGMgr::RemoveGroupData('),
 @('RemoveQueue','void LFGMgr::RemoveFromQueue('),@('RemoveGroupQueue','void LFGMgr::RemoveFromGroupQueue('),
 @('Leave','void LFGMgr::LeaveLfg('),@('Packet','void LFGMgr::SendUpdateStatus('),
 @('Cleanup','void LFGMgr::CleanupFinishedSoloScenario('))) {Save $pair[0] (Block $mgr $pair[1])}
Save 'PlayerSetState' (Block (Get-Content "$repo/src/server/game/DungeonFinding/LFGPlayerData.cpp" -Raw) 'void LfgPlayerData::SetState(')
Save 'GroupSetState' (Block (Get-Content "$repo/src/server/game/DungeonFinding/LFGGroupData.cpp" -Raw) 'void LfgGroupData::SetState(')
$intro=Get-Content "$repo/src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp" -Raw
Save 'LegacyCleanup' (Block $intro 'void CleanupCompletedScenario(')
# Execute the real completion bookkeeping up to reward processing. Reward delivery
# is deliberately outside this fixture; no quest-completion/client claim is made.
$finish=Block $mgr 'void LFGMgr::FinishDungeon('
$a=$finish.IndexOf('    SetState(gguid, LFG_STATE_FINISHED_DUNGEON, queueId);')
$b=$finish.IndexOf('        StartAllOtherQueue(guid, queueId);',$a)
if($a -lt 0 -or $b -lt 0){throw 'Completion state block missing'}
Save 'FinishState' ($finish.Substring($a,$b-$a)+"    }`n")
if(!$finish.Contains('_SaveToDB(gguid, group->GetDbStoreId())')){throw 'Completion state must be persisted'}
$scenario=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
$dispatch=Block $scenario 'switch (_scenarioEntry->Type)'
Save 'RewardDispatch' $dispatch
if($dispatch.IndexOf('case SCENARIO_TYPE_LEGION_INVASION:') -gt $dispatch.IndexOf('sLFGMgr->FinishDungeon(')){throw 'Broken Shore completion bypasses LFG finish'}
if(!$intro.Contains('void OnMapChanged(Player* player) override') -or [regex]::Matches($intro,'LegionIntroSequence::CleanupCompletedScenario\(player\);').Count -ne 3){throw 'Legacy cleanup needs login, map change and quest reward hooks'}
$scripts=Get-Content "$repo/src/server/game/DungeonFinding/LFGScripts.cpp" -Raw
if([regex]::Matches($scripts,'sLFGMgr->CleanupFinishedSoloScenario\(player\);').Count -ne 2){throw 'Generic cleanup needs login and map change hooks'}
$boost=Get-Content "$repo/src/server/scripts/Scenario/BoostExperience/boost_experience.cpp" -Raw
if($boost -match 'JoinLfg\(' -or !$boost.Contains('sScenarioMgr->AddScenario(instance, scenarioId)')){throw 'Re-audit boost: expected a direct scenario without LFG'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/ScenarioLfgLifecycleTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Scenario LFG lifecycle regression failed'}
& "$PSScriptRoot/../research/Read-LegionLegendaryCatalog.ps1" -SourceFile ScenarioLfgCatalog.cpp -ProbeName ScenarioLfgCatalog -OutputFile "$out/native-catalog.tsv"
$catalog=Get-Content "$out/native-catalog.tsv"
foreach($row in @("LFG`t908`t1460`t786`t4`t12`t1", "LFG`t1183`t1522`t962`t0`t12`t1", "LFG`t1184`t1522`t998`t0`t12`t1", "LFG`t1250`t1498`t1097`t0`t12`t1", "LFG`t1185`t1533`t988`t0`t12`t1")) {
 if(!($catalog | Where-Object {$_.StartsWith($row+"`t")})){throw "Native runtime scenario routing changed: $row"}
}
'PASS: native runtime DB2 confirms Broken Shore type 4 and later Warlock scenario routing.'
if($VerifyReleaseBinary){
 if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
 $exe=Join-Path $ReleaseDirectory 'worldserver.exe'
 $ascii=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$ascii.Contains('Retiring completed solo scenario %u for %s after exit')){throw 'Release lacks scenario cleanup implementation'}
 foreach($relative in @('DungeonFinding/LFGMgr.cpp','DungeonFinding/LFGScripts.cpp','Scenarios/Scenario.cpp')){
  if((Get-Item "$repo/src/server/game/$relative").LastWriteTimeUtc -gt (Get-Item "$repo/build-extractors/src/server/game/Release/game.lib").LastWriteTimeUtc){throw "Final-link game library predates $relative"}
 }
 if((Get-Item "$repo/src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp").LastWriteTimeUtc -gt (Get-Item "$repo/build-extractors/src/server/scripts/Release/scripts.lib").LastWriteTimeUtc){throw 'Final-link scripts library predates introduction cleanup'}
 'PASS: Release contains cleanup implementation; original final-link libraries are fresh.'
 $python=Get-Command python -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_scenario_lfg_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Linked Release scenario lifecycle verification failed'}
}
