param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'};$out="$repo/build-extractors/runtime-test/random-smart-action-pools-$mode"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $source=if($Baseline){(& git -C $repo show 97be50dc3:src/server/game/AI/SmartScripts/SmartScript.cpp) -join "`n"}else{Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw}
 if($Baseline -and $LASTEXITCODE){throw 'Pinned pre-fix source unavailable'}
 foreach($section in @(@('RandomEmote','SMART_ACTION_RANDOM_EMOTE','SMART_ACTION_THREAT_ALL_PCT'),@('RandomPhase','SMART_ACTION_RANDOM_PHASE','SMART_ACTION_RANDOM_PHASE_RANGE'))){
  $begin=$source.IndexOf("        case $($section[1]):");$end=$source.IndexOf("        case $($section[2]):",$begin)
  if($begin -lt 0 -or $end -le $begin){throw "Missing actual pool section $($section[0])"}
  [IO.File]::WriteAllText("$out/$($section[0]).inc",$source.Substring($begin,$end-$begin))
 }
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 if($header -notmatch 'SMART_ACTION_RANDOM_EMOTE\s*=\s*10\s*,' -or $header -notmatch 'SMART_ACTION_RANDOM_PHASE\s*=\s*30\s*,'){throw 'Native action enums changed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/RandomSmartActionPoolsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n")
 & "$out/build.cmd" *> "$out/build.log";if($LASTEXITCODE){Get-Content "$out/build.log";throw 'Pool probe did not compile'}
 foreach($probe in $(if($Baseline){@('emote','phase')}else{@('fixed')})){
  $argument=if($Baseline){" $probe"}else{''}
  [IO.File]::WriteAllText("$out/run-$probe.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`"$argument`nexit /b %errorlevel%`n")
  $info=[Diagnostics.ProcessStartInfo]::new();$info.FileName=$env:ComSpec;$info.Arguments="/d /c run-$probe.cmd";$info.WorkingDirectory=$out;$info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
  $process=[Diagnostics.Process]::Start($info);$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
  if(!$process.WaitForExit(30000)){$process.Kill($true);throw 'Owned pool probe exceeded deadline'}
  [IO.File]::WriteAllText("$out/$probe.stdout",$stdout.Result);[IO.File]::WriteAllText("$out/$probe.stderr",$stderr.Result);[IO.File]::WriteAllText("$out/$probe.exit",[string]$process.ExitCode)
  if($Baseline){if(!$process.ExitCode -or $stderr.Result -notmatch 'AddressSanitizer: stack-buffer-overflow' -or $stderr.Result -notmatch 'ProcessAction'){throw "Expected old $probe overflow not reproduced"};"EXPECTED FAIL counterfactual: pinned97be50dc3 $probe temp[6] read, actual ASan stack-buffer-overflow captured; not an ordinary passing run"}
  else{if($process.ExitCode -or $stderr.Result.Length){Get-Content "$out/$probe.stderr";throw 'Repaired pool probe failed or wrote stderr'};$stdout.Result}
  $process.Dispose()
 }
}
