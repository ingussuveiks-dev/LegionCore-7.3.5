param([int]$Batch=1,[switch]$Baseline,[string]$Case='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ten-ten-20261111/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $mode=if($Baseline){'baseline'}else{'fixed'}
 $prepareArgs=@("$PSScriptRoot/prepare_ten_ten_20261111.py",$out,$mode,$Batch);if($Case){$prepareArgs+=$Case}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe @prepareArgs
 if($LASTEXITCODE){throw 'Selected production extraction failed'}
 $keys=@(Get-Content "$out/cases.json" -Raw|ConvertFrom-Json);$indices=if($Case){$index=[Array]::IndexOf($keys,$Case);if($index -lt 0){throw 'Unknown case'};@($index)}else{@(0..9)}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $source=if($Batch -eq 1){'TenTen20261111FirstTest.cpp'}elseif($Batch -eq 2){'TenTen20261111SecondTest.cpp'}else{throw 'Batch fixture is not implemented'}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/$source`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/build.cmd`" > `"$out/build.log`" 2>&1";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 20;throw 'Full-method fixture compilation failed'}
 foreach($index in $indices){
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $index`nexit /b %errorlevel%`n")
  & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/run-$index.log`" 2>&1";$code=$LASTEXITCODE;$raw=Get-Content "$out/run-$index.log" -Raw
  if($Baseline){if(!$code){throw "Expected pinned failure missing:$($keys[$index])"};"PASS expected-broken pinned counterfactual:$($keys[$index]);paired repaired run required"}
  elseif($code -or !$raw.Contains('PASS complete production')){Get-Content "$out/run-$index.log" -Tail 15;throw "Selected repaired regression failed:$($keys[$index])"}else{$raw.Trim()}
 }
}
$global:LASTEXITCODE=0
