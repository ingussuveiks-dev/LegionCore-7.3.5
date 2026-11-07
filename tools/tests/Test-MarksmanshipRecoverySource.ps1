$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-recovery-source/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $p=Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw;[IO.File]::WriteAllText("$out/Earned.inc",(Block $p 'bool Player::HasEarnedMarksmanshipReturn() const'))
 $s=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw;$a=$s.IndexOf('namespace MarksmanshipRecovery');$b=$s.IndexOf('namespace MarksmanshipReturn',$a);[IO.File]::WriteAllText("$out/Recovery.inc",$s.Substring($a,$b-$a));[IO.File]::WriteAllText("$out/Return.inc",(Block $s 'namespace MarksmanshipReturn'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" /I`"$out`" `"$PSScriptRoot/MarksmanshipRecoveryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){Get-Content "$out/source.log" -Tail 8;throw 'Recovery source ASAN failed'};Get-Content "$out/source.log" -Tail 2
}
