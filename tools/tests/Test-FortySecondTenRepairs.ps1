param([Parameter(Mandatory=$true)][string]$Case,[switch]$Baseline,[string]$PlanFile='FortySecondTenRepairs.json')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fortysecond-ten-repairs/$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $mode=if($Baseline){'baseline'}else{'fixed'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_fortysecond.py" $out $Case $mode
 if($LASTEXITCODE){throw 'Extraction failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags="/DCASE_$($Case.ToUpperInvariant()) /fsanitize=address /Zi";if($Baseline){$flags+=' /DCOUNTERFACTUAL'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD $flags /I`"$out`" `"$PSScriptRoot/LifetimeCallbackGuardTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif %errorlevel% neq 0 exit /b %errorlevel%`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/first-run.log`" 2>&1";$first=$LASTEXITCODE
 if($first -or !(Test-Path "$out/test.exe")){Get-Content "$out/first-run.log" -Tail 15;throw 'ASAN fixture compilation failed'}
 # The pinned baseline is intentionally expected to fail under ASAN after AI destruction.
 $callbacks=(Get-Content "$PSScriptRoot/$PlanFile" -Raw|ConvertFrom-Json|Where-Object key -eq $Case).callbacks
 [IO.File]::WriteAllText("$out/run-one.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" %1`nexit /b %errorlevel%`n")
 foreach($index in 0..($callbacks.Count-1)){
  & cmd.exe /d /c "`"$out/run-one.cmd`" $index > `"$out/run-$index.log`" 2>&1";$code=$LASTEXITCODE
  $raw=[string](Get-Content -LiteralPath "$out/run-$index.log" -Raw)
  if($Baseline){if(($code -eq 0) -or !($raw.Contains('AddressSanitizer: heap-use-after-free')) -or !($raw.Contains('ABORTING'))){throw "Expected pinned heap-use-after-free control missing:$Case/$index exit$code"};"PASS expected-broken ASAN heap-use-after-free counterfactual:$Case/$index"}
  elseif(($code -ne 0) -or !($raw.Contains('PASS selected actor queue'))){Get-Content "$out/run-$index.log" -Tail 15;throw "Repaired callback failed:$Case/$index exit$code"}
 }
 if(!(Test-Path "$out/test.exe")){Get-Content "$out/first-run.log" -Tail 15;throw 'ASAN fixture compilation failed'}

}
$global:LASTEXITCODE=0
