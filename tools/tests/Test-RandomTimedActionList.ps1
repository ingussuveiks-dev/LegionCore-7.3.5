param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/build-extractors/runtime-test/random-timed-action-list-$mode"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $source=if($Baseline){(& git -C $repo show da42f12f5:src/server/game/AI/SmartScripts/SmartScript.cpp) -join "`n"}else{Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw}
 if($Baseline -and $LASTEXITCODE){throw 'Pinned pre-fix source unavailable'}
 $begin=$source.IndexOf('        case SMART_ACTION_CALL_RANDOM_TIMED_ACTIONLIST:')
 $end=$source.IndexOf('        case SMART_ACTION_CALL_RANDOM_RANGE_TIMED_ACTIONLIST:',$begin)
 if($begin -lt 0 -or $end -le $begin){throw 'Missing production action87 section'}
 [IO.File]::WriteAllText("$out/RandomTimedActionList.inc",$source.Substring($begin,$end-$begin))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/RandomTimedActionListTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n")
 & "$out/build.cmd" *> "$out/build.log"
 if($LASTEXITCODE){Get-Content "$out/build.log";throw 'Native action87 probe did not compile'}
 $start=New-Object Diagnostics.ProcessStartInfo
 $argument=if($Baseline){' baseline'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`"$argument`nexit /b %errorlevel%`n")
 $start.FileName=$env:ComSpec; $start.WorkingDirectory=$out
 $start.UseShellExecute=$false; $start.CreateNoWindow=$true
 $start.RedirectStandardOutput=$true; $start.RedirectStandardError=$true
 $start.Arguments='/d /c "run.cmd"'
 $process=[Diagnostics.Process]::Start($start)
 $stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
 if(!$process.WaitForExit(30000)){throw 'Native probe exceeded deadline'}
 [IO.File]::WriteAllText("$out/stdout.log",$stdout.Result)
 [IO.File]::WriteAllText("$out/stderr.log",$stderr.Result)
 [IO.File]::WriteAllText("$out/exit-code.txt",[string]$process.ExitCode)
 if($Baseline){
  if(!$process.ExitCode -or $stderr.Result -notmatch 'AddressSanitizer: stack-buffer-overflow' -or $stderr.Result -notmatch 'ProcessAction'){throw 'Expected old inclusive-upper-bound ASan failure was not reproduced'}
  'EXPECTED FAIL counterfactual: pinned da42f12f5 action87 reads temp[6]; ASan stack-buffer-overflow captured. This is not a passing ordinary run.'
 }else{
  if($process.ExitCode -or $stderr.Result.Length){Get-Content "$out/stderr.log";throw 'Repaired native action87 probe failed or wrote stderr'}
  $stdout.Result
 }
}
