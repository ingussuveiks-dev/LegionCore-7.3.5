param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/scenario-resume"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw
$a=$source.IndexOf('namespace');$b=$source.IndexOf('bool Player::ExitCompletedBrokenShoreLogin()', $a)
[IO.File]::WriteAllText("$output/Resume.inc",$source.Substring($a,$b-$a))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
if($player.IndexOf('PrepareScenarioLogin(mapId,instanceId)') -gt $player.IndexOf('else if (transLowGUID)')){throw 'Recovery after transport fallback'}
if($player.IndexOf('PrepareScenarioLogin(mapId,instanceId)') -gt $player.IndexOf('    Map* map = m_scenarioLoginInstance')){throw 'Recovery after map creation'}
if(!$player.Contains('m_scenarioResume.Read(fields[f_scenarioResume].GetString())') -or !$player.Contains('stmt->setString(index++, m_scenarioResume.Write())')){throw 'Checkpoint DB persistence missing'}
$access=Get-Content "$repo/src/server/game/Maps/MapManager.cpp" -Raw
$a=$access.IndexOf('        uint32 introQuest =')
$b=$access.IndexOf('    InstanceTemplate const* instance',$a)
$block=$access.Substring($a,$b-$a)
[IO.File]::WriteAllText("$output/Access.inc",$block.Substring(0,$block.LastIndexOf('    }')))
if(!$access.Contains('activeScenario->GetMap() == player->GetMap()') -or !$access.Contains('player->CanResumeScenarioLogin(activeScenario->GetScenarioId())')){throw 'Post-construction access must use actual live scenario'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" /I`"$repo/src/server/game/Scenarios`" `"$PSScriptRoot/ScenarioResumeTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Scenario resume production handler tests failed'}
if($VerifyReleaseBinary){
 $ascii=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 foreach($marker in @('Scenario resume: player %u rejoins live scenario','Scenario resume: player %u restarts scenario','Login map rejected: player','killPoints, scenarioResume FROM characters','killPoints=?,scenarioResume=? WHERE guid=?')){
  if(!$ascii.Contains($marker)){throw "Canonical executable lacks $marker"}
 }
 'PASS: canonical Release contains global scenario recovery and login diagnostics'
}
