param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/smart-event-phase"
New-Item -ItemType Directory -Force $out|Out-Null
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
if($Baseline){$source=(& git -C $repo show '828374ec9792b2123d6ec9297cc162c0b95d6ed6:src/server/game/AI/SmartScripts/SmartScript.cpp') -join "`n";if($LASTEXITCODE){throw 'Cannot read pinned phase baseline'}}
$first=$source.IndexOf('bool SmartScript::IsInPhase(');if($first -lt 0){throw 'Native phase methods missing'}
[IO.File]::WriteAllText("$out/PhaseMethods.inc",$source.Substring($first))
$header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
$count=[regex]::Match($header,'SMART_EVENT_PHASE_COUNT\s*=\s*(\d+)');if(!$count.Success){throw 'Native phase count missing'}
[IO.File]::WriteAllText("$out/PhaseConstants.inc","constexpr uint32 SMART_EVENT_PHASE_COUNT=$($count.Groups[1].Value);`n")
$define=if($Baseline){'/DPHASE_BASELINE'}else{''}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $define /I`"$out`" `"$PSScriptRoot/SmartEventPhaseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native SmartAI phase regression failed'}
if($VerifyReleaseBinary){
 if($Baseline){throw 'Baseline cannot certify the repaired Release'}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_event_phase_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical phase range check failed'}}finally{$env:PYTHONPATH=$previousPython}
}
