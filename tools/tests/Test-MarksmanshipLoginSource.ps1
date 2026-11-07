param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-login-source/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $source=if($Baseline){(& git -C $repo show '387b8a385:src/server/game/Entities/Player/PlayerScenario.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw}
 if($Baseline){
  $pinned=(& git -C $repo show '387b8a385:src/server/game/Scenarios/AcquisitionScenarioRoute.h') -join "`n"
  if($LASTEXITCODE){throw 'Pinned acquisition route missing'}
  $pinned=$pinned.Replace('ACQUISITION_SCENARIO_ROUTE_H','PINNED_ACQUISITION_SCENARIO_ROUTE_H').Replace('namespace AcquisitionScenarioRoute','namespace PinnedAcquisitionScenarioRoute')
  [IO.File]::WriteAllText("$out/PinnedAcquisitionRoute.inc",$pinned)
  $source=$source.Replace('AcquisitionScenarioRoute::','PinnedAcquisitionScenarioRoute::')
 }
 $a=$source.IndexOf('namespace');$b=$source.IndexOf('bool Player::ExitCompletedBrokenShoreLogin()', $a);[IO.File]::WriteAllText("$out/Resume.inc",$source.Substring($a,$b-$a))
 $loader=if($Baseline){(& git -C $repo show '387b8a385:src/server/game/Entities/Player/Player.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw}
 if($Baseline){$loader=$loader.Replace('AcquisitionScenarioRoute::','PinnedAcquisitionScenarioRoute::')}
 $a=$loader.IndexOf('    // Restore before the stale-transport fallback');$b=$loader.IndexOf('    if(m_atLoginFlags & AT_LOGIN_UNLOCK)',$a);if($a -lt 0 -or $b -le $a){throw 'Actual early loading block missing'};[IO.File]::WriteAllText("$out/WarriorEarly.inc",$loader.Substring($a,$b-$a))
 $a=$loader.IndexOf('    if (!scenarioQuestStatusLoaded)');$b=$loader.IndexOf('    _LoadQuestStatusObjectives(',$a);[IO.File]::WriteAllText("$out/NormalQuestLoad.inc",$loader.Substring($a,$b-$a))
 $a=$loader.IndexOf('    // A new entrance-only Hunter recovery');if($Baseline){$binding='/* pinned loader has no Marksmanship binding guard */'}else{$b=$loader.IndexOf('    if (!map)',$a);if($a -lt 0 -or $b -le $a){throw 'Actual Marksmanship selected-map guard missing'};$binding=$loader.Substring($a,$b-$a)};[IO.File]::WriteAllText("$out/MarksmanshipBinding.inc",$binding)
 $access=Get-Content "$repo/src/server/game/Maps/MapManager.cpp" -Raw;$a=$access.IndexOf('        uint32 introQuest =');$b=$access.IndexOf('    InstanceTemplate const* instance',$a);$block=$access.Substring($a,$b-$a);[IO.File]::WriteAllText("$out/Access.inc",$block.Substring(0,$block.LastIndexOf('    }')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flag=if($Baseline){'/DMARKSMAN_LOGIN_SOURCE_BASELINE /FI"'+$out+'/PinnedAcquisitionRoute.inc"'}else{''}
 $cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" $flag /I`"$out`" /I`"$repo/src/server/game/Scenarios`" `"$PSScriptRoot/MarksmanshipLoginRecoveryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
 [IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){throw "Marksmanship source ASAN failed: $out/source.log"};Get-Content "$out/source.log" -Tail 2
}
