param([switch]$Baseline,[switch]$Native,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
$out="$repo/build-extractors/runtime-test/warswords-stage-four/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp'
 $source=if($Baseline){(& git -C $repo show "6a5b1f80b:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw};if(!$source){throw 'Missing instance source'}
 foreach($pair in @(@('Step','void onScenarionNextStep('),@('Initialize','void Initialize('),@('Participant','namespace WarswordsArrival'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $source $pair[1]))}
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$definition=if($Baseline){'/DBASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $definition /I`"$out`" `"$PSScriptRoot/WarswordsStageFourTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 20;throw 'Stage4 source/native queue regression failed'}
 Get-Content "$out/test.log"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
 if($Native){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe WarswordsStageFourNative -AdditionalBaseline:$Baseline *> "$out/native.log";if(!$?){Get-Content "$out/native.log" -Tail 12;throw 'Stage4 native regression failed'};Get-Content "$out/native.log"|Where-Object {$_ -match '^(PASS|CASE|COUNTERFACTUAL)'}}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_warswords_stage_four_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Stage4 actual Release verification failed'}}finally{$env:PYTHONPATH=$prior}}
}
