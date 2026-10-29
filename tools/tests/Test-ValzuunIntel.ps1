param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/valzuun-intel"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 Copy-Item "$repo/src/server/scripts/Legion/ClassHalls/valzuun_intel.h" "$out/ValzuunIntel.inc"
 $source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 $a=$source.IndexOf('void Player::RewardPlayerAndGroupAtEvent(');$b=$source.IndexOf('bool Player::IsAtGroupRewardDistance(',$a)
 if($a -lt 0 -or $b -le $a){throw 'Missing native group event award'}
 [IO.File]::WriteAllText("$out/NativeGroupCredit.inc",$source.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ValzuunIntelTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Production Valzuun intel/native group regression failed'}
}
