param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/sharasdal-legacy-source/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $source=if($Baseline){(& git -C $repo show '677430a07:src/server/game/Entities/Player/PlayerScenario.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw}
 $a=$source.IndexOf('namespace');$b=$source.IndexOf('bool Player::ExitCompletedBrokenShoreLogin()',$a);[IO.File]::WriteAllText("$out/Resume.inc",$source.Substring($a,$b-$a))
 $loader=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw;$a=$loader.IndexOf('    // Restore before the stale-transport fallback');$b=$loader.IndexOf('    if(m_atLoginFlags & AT_LOGIN_UNLOCK)',$a);[IO.File]::WriteAllText("$out/WarriorEarly.inc",$loader.Substring($a,$b-$a));$a=$loader.IndexOf('    if (!scenarioQuestStatusLoaded)');$b=$loader.IndexOf('    _LoadQuestStatusObjectives(',$a);[IO.File]::WriteAllText("$out/NormalQuestLoad.inc",$loader.Substring($a,$b-$a))
 $access=Get-Content "$repo/src/server/game/Maps/MapManager.cpp" -Raw;$a=$access.IndexOf('        uint32 introQuest =');$b=$access.IndexOf('    InstanceTemplate const* instance',$a);$part=$access.Substring($a,$b-$a);[IO.File]::WriteAllText("$out/Access.inc",$part.Substring(0,$part.LastIndexOf('    }')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flag=if($Baseline){'/DSHARASDAL_LEGACY_SOURCE_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" $flag /I`"$out`" /I`"$repo/src/server/game/Scenarios`" `"$PSScriptRoot/SharasdalLegacyLoginTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){Get-Content "$out/source.log" -Tail 6;throw 'Sharasdal legacy source ASAN failed'};Get-Content "$out/source.log" -Tail 2
}
