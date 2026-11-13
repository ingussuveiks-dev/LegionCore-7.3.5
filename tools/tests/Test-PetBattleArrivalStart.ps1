param([switch]$Baseline,[ValidateSet('all','early','late')][string]$Case='all')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $Case -eq 'all'){throw 'Select the specific expected-broken early/late counterfactual'}
$mode=if($Baseline){'baseline'}else{'fixed'};$out="$repo/.codex/pet-arrival-start-2026-11-13/$mode-$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/World/PetBattle/PetBattlePlayerScript.cpp'
 $source=if($Baseline){(& git -C $repo show "b3ee2154f:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
 if($Baseline -and $LASTEXITCODE){throw 'Pinned complete production arrival class unavailable'}
 $a=$source.IndexOf('class PlayerScriptPetBattle :');$b=$source.IndexOf('void AddSC_PetBattlePlayerScript()',$a)
 if($a -lt 0 -or $b -le $a){throw 'Actual production class boundaries missing'}
 [IO.File]::WriteAllText("$out/PetBattleArrivalScript.inc",$source.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/PetBattleArrivalStartTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/build.log";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 15;throw 'Arrival ASAN regression compilation failed'}
 $selected=if($Case -eq 'all'){''}else{$Case}
 [IO.File]::WriteAllText("$out/execute.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $selected`nexit /b %errorlevel%`n")
 & "$out/execute.cmd" *> "$out/native.log";$exit=$LASTEXITCODE
 if($Baseline){$message=if($Case -eq 'early'){'Arrival must not Begin before the original1000ms delay'}else{'First update after the deadline must consume and Begin the arrival'};if($exit -ne 1 -or !(Get-Content "$out/native.log" -Raw).Contains($message)){throw 'Unexpected counterfactual failure; inspect retained output'};"PASS expected-broken complete b3ee2154f arrival class:$Case;exit1;paired repaired regression required"}
 else{if($exit -or !(Get-Content "$out/native.log" -Raw).Contains('PASS exact production arrival class cases=')){throw 'Arrival ASAN regression failed; retained output requires review'};Get-Content "$out/native.log"}
}
$global:LASTEXITCODE=0
