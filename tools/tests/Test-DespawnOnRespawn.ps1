param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/despawn-on-respawn/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe';$mode=if($Baseline){'baseline'}else{'fixed'}
 & $python -X utf8 "$PSScriptRoot/prepare_despawn_on_respawn.py" $out $mode;if($LASTEXITCODE){throw 'Helper extraction differs'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$flags=if($Baseline){'/DCOUNTERFACTUAL'}else{''}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /MD /fsanitize=address /Zi $flags /I`"$out`" `"$PSScriptRoot/DespawnOnRespawnTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`" /link /INCREMENTAL:NO`r`nexit /b %errorlevel%`r`n")
 & "$out/build.cmd" *> "$out/build.log";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 10;throw 'Helper ASAN build failed'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/stdout.log`" 2> `"$out/stderr.log`"";$exit=$LASTEXITCODE;$raw=[IO.File]::ReadAllText("$out/stdout.log")+[IO.File]::ReadAllText("$out/stderr.log")
 @{exit=$exit;expected_counterfactual=[bool]$Baseline;stdout=(Get-FileHash "$out/stdout.log").Hash;stderr=(Get-FileHash "$out/stderr.log").Hash}|ConvertTo-Json|Set-Content "$out/process-result.json"
 if($Baseline){if(!$exit -or $raw -notmatch 'AddressSanitizer: heap-use-after-free' -or $raw -notmatch 'ABORTING'){throw 'Expected old helper ASAN counterfactual missing'};'PASS expected-broken full-helper ASAN use-after-free counterfactual'}elseif($exit -or (Get-Item "$out/stderr.log").Length -or $raw -notmatch '^PASS complete DespawnOnRespawn helper:'){throw 'Repaired helper regression failed'}else{$raw.Trim()}
}
