param([switch]$Baseline,[string]$Case='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/second-ten-ten-20261112/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $mode=if($Baseline){'baseline'}else{'fixed'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_second_ten_ten_20261112.py" $out $mode
 if($LASTEXITCODE){throw 'Extraction failed'}
 $keys=@(Get-Content "$out/cases.json" -Raw|ConvertFrom-Json);$indices=if($Case){$index=[Array]::IndexOf($keys,$Case);if($index -lt 0){throw 'Unknown case'};@($index)}else{@(0..9)}
 $failures=@('Ordinary forfeit applies health penalty','Actual wild GUID is released exactly once','Repeated finish cannot replay XP or callbacks','A temporary swap lock cannot cancel earned experience','All crossed native thresholds trigger their level side effects','MoveType zero must reach its leave switch arm','Equal-weight ordering must be irreflexive','A fresh proposal requires fresh answers from both sides','Replacing a pending request must release its earlier allocation','Admission must measure the actual two participants')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SecondTenTen20261112FirstTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/build.cmd`" > `"$out/build.log`" 2>&1";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 18;throw 'Fixture compilation failed'}
 foreach($index in $indices){
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $index`nexit /b %errorlevel%`n")
  & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/run-$index.log`" 2>&1";$code=$LASTEXITCODE;$raw=Get-Content "$out/run-$index.log" -Raw
  if($Baseline){if($code -ne 1 -or $raw.Trim() -cne $failures[$index]){throw "Expected pinned assertion missing:$($keys[$index])"};"PASS expected-broken pinned counterfactual:$($keys[$index]);paired repaired run required"}
  elseif($code -or !$raw.Contains('PASS complete production')){Get-Content "$out/run-$index.log" -Tail 18;throw "Regression failed:$($keys[$index])"}else{"$($keys[$index]):$($raw.Trim())"}
 }
}
$global:LASTEXITCODE=0
