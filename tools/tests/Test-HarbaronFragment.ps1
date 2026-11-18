param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/harbaron-fragment/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block($source,$marker){$a=$source.IndexOf($marker);if($a -lt 0){throw 'Native method missing'};$b=$source.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($source[$b] -eq '{'))-([int]($source[$b] -eq '}'));++$b};$source.Substring($a,$b-$a)}
$source=if($Baseline){(& git -C $repo show 'c92852906:src/server/scripts/Legion/MawofSouls/boss_harbaron.cpp')-join "`n"}else{Get-Content "$repo/src/server/scripts/Legion/MawofSouls/boss_harbaron.cpp" -Raw}
[IO.File]::WriteAllText("$out/HarbaronAuraCleanupActual.inc",$(if($Baseline){''}else{Block $source 'static void RemoveSoulFragmentAuras('}))
foreach($pair in @(@('Fragment','void SpellHitTarget('),@('Cleanup','void DespawnSouls('))){[IO.File]::WriteAllText("$out/Harbaron$($pair[0])Actual.inc",(Block $source $pair[1]))}
$fragment=$source.Substring($source.IndexOf('struct npc_harbaron_soul_fragment'))
[IO.File]::WriteAllText("$out/HarbaronDeathActual.inc",(Block $fragment 'void JustDied('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$extra=if($Baseline){'/DEXPECT_BASELINE '}else{''};if($AddressSanitizer){$extra+='/fsanitize=address /Zi /Fd"'+$out+'/test.pdb" '}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $extra/I`"$out`" `"$PSScriptRoot/HarbaronFragmentTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {& "$out/run.cmd";if($LASTEXITCODE){throw 'Harbaron Fragment regression failed'}}
