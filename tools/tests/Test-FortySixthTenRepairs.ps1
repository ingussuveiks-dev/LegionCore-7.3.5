param([Parameter(Mandatory=$true)][string]$Case,[switch]$Baseline,[switch]$NullCounterfactual)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$caseInfo=Get-Content "$PSScriptRoot/FortySixthTenRepairs.json" -Raw|ConvertFrom-Json|Where-Object key -eq $Case
if(!$caseInfo){throw 'Unknown death regression case'}
if($NullCounterfactual -and (!$Baseline -or $Case -notin @('SilverHandDeathCredit','FlamewakerSentinelDeathCredit'))){throw 'Extra null counterfactual is only for the pinned Silver Hand/Sentinel baselines'}
$out="$repo/build-extractors/runtime-test/fortysixth-ten-repairs/$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $mode=if($Baseline){'baseline'}else{'fixed'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_fortysixth.py" $out $Case $mode
 if($LASTEXITCODE){throw 'Full death method extraction failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags='/fsanitize=address /Zi';if($Baseline){$flags+=' /DCOUNTERFACTUAL'}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD $flags /I`"$out`" `"$PSScriptRoot/FortySixthTenRepairsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/build.cmd`" > `"$out/build.log`" 2>&1"
 if($LASTEXITCODE -or !(Test-Path "$out/test.exe")){Get-Content "$out/build.log" -Tail 18;throw 'Full death fixture compile failed'}
 $extra=if($NullCounterfactual){' null'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $($caseInfo.index)$extra`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/run.log`" 2>&1";$code=$LASTEXITCODE;$raw=Get-Content "$out/run.log" -Raw
 if($Baseline -and ($caseInfo.kind -eq 'victim_instance' -or $NullCounterfactual)){
  if(!$code -or $raw -notmatch 'AddressSanitizer' -or $raw -notmatch '(?i)access-violation|DEADLYSIGNAL|SEGV'){Get-Content "$out/run.log" -Tail 12;throw 'Expected old null-killer ASAN failure missing'}
  'PASS expected-broken original death hook:null killer dereference fails under ASAN'
 }elseif($Baseline){
  if($code -or !$raw.Contains('PASS expected-broken full-method counterfactual')){Get-Content "$out/run.log" -Tail 12;throw 'Pinned guardian progression-loss counterfactual missing'}
  'PASS expected-broken original death method misses the eligible guardian owner endpoint;actual progression is a separate native/client scope'
 }elseif($code -or !$raw.Contains('PASS full extracted death method')){Get-Content "$out/run.log" -Tail 12;throw 'Repaired full death regression failed'}
}
$global:LASTEXITCODE=0
