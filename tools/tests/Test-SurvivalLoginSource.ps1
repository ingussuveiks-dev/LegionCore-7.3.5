param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/survival-login-source/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $source=if($Baseline){(& git -C $repo show '0af584162:src/server/game/Entities/Player/PlayerScenario.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw}
 $a=$source.IndexOf('namespace');$b=$source.IndexOf('bool Player::ExitCompletedBrokenShoreLogin()', $a);[IO.File]::WriteAllText("$out/Resume.inc",$source.Substring($a,$b-$a))
 $loader=if($Baseline){(& git -C $repo show '0af584162:src/server/game/Entities/Player/Player.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw}
 $a=$loader.IndexOf('    // Restore before the stale-transport fallback');$b=$loader.IndexOf('    if(m_atLoginFlags & AT_LOGIN_UNLOCK)',$a);if($a -lt 0 -or $b -le $a){throw 'Actual early loading block missing'};[IO.File]::WriteAllText("$out/WarriorEarly.inc",$loader.Substring($a,$b-$a))
 $a=$loader.IndexOf('    if (!scenarioQuestStatusLoaded)');$b=$loader.IndexOf('    _LoadQuestStatusObjectives(',$a);[IO.File]::WriteAllText("$out/NormalQuestLoad.inc",$loader.Substring($a,$b-$a))
 $a=$loader.IndexOf('    // A new entrance-only Hunter recovery');if($Baseline){$binding='/* original loader has no Hunter binding guard */'}else{$b=$loader.IndexOf('    if (!map)',$a);if($a -lt 0 -or $b -le $a){throw 'Actual Hunter selected-map guard missing'};$binding=$loader.Substring($a,$b-$a)};[IO.File]::WriteAllText("$out/SurvivalBinding.inc",$binding)
 $access=Get-Content "$repo/src/server/game/Maps/MapManager.cpp" -Raw;$a=$access.IndexOf('        uint32 introQuest =');$b=$access.IndexOf('    InstanceTemplate const* instance',$a);$block=$access.Substring($a,$b-$a);[IO.File]::WriteAllText("$out/Access.inc",$block.Substring(0,$block.LastIndexOf('    }')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flag=if($Baseline){'/DSURVIVAL_LOGIN_SOURCE_BASELINE'}else{''}
 $cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" $flag /I`"$out`" /I`"$repo/src/server/game/Scenarios`" `"$PSScriptRoot/SurvivalLoginRecoveryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
 [IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){throw "Hunter source ASAN failed: $out/source.log"};Get-Content "$out/source.log" -Tail 2
}
