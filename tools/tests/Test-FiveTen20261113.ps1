param([switch]$Baseline)
$ErrorActionPreference='Stop';$env:PYTHONUTF8='1'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/.codex/five-ten-20261113/$mode-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_five_ten_20261113.py" $out $mode
 if($LASTEXITCODE){throw 'Production source extraction failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/FiveTen20261113Test.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/build.cmd`" > `"$out/build.log`" 2>&1"
 if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 18;throw 'Source fixture build failed'}
 foreach($i in 0..9){
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $i`nexit /b %errorlevel%`n")
  & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/run-$i.log`" 2>&1";$code=$LASTEXITCODE;$raw=Get-Content "$out/run-$i.log" -Raw
  if($Baseline){
   if(!$code -or ($i -notin 6,8 -and $raw -notmatch 'AddressSanitizer') -or ($i -eq 8 -and !$raw.Contains('Gate activation must queue')) -or ($i -eq 6 -and !$raw.Contains('Failed area trigger command must report failure'))){Get-Content "$out/run-$i.log" -Tail 8;throw "Unexpected counterfactual:$i"}
   "PASS expected-broken pinned source case=$i;exit=$code;paired repaired run required"
  }else{if($code -or !$raw.Contains('PASS complete source')){Get-Content "$out/run-$i.log" -Tail 12;throw "Repaired case failed:$i"};$raw.Trim()}
 }
}
$global:LASTEXITCODE=0
