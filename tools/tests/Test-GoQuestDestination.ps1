param([switch]$Baseline)
$ErrorActionPreference='Stop';$goQuestRepo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$goQuestOut="$goQuestRepo/build-extractors/runtime-test/go-quest-destination/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$goQuestOut/evidence" -RuntimeDirectories @($goQuestOut) -Test {
 $goQuestPlan=Get-Content "$goQuestRepo/tools/tests/GoQuestDestinationRepair.json" -Raw|ConvertFrom-Json
 $goQuestCurrent=[IO.File]::ReadAllText("$goQuestRepo/$($goQuestPlan.path)").Replace("`r",'')
 if(!$goQuestCurrent.Contains($goQuestPlan.new)){throw 'Declared complete production handler differs'}
 $goQuestSelected=$goQuestPlan.new
 if($Baseline){
  $goQuestPinned=(& git -C $goQuestRepo show "$($goQuestPlan.baseline):$($goQuestPlan.path)") -join "`n"
  if($LASTEXITCODE -or !$goQuestPinned.Contains($goQuestPlan.old)){throw 'Pinned counterfactual unavailable'}
  $goQuestSelected=$goQuestPlan.old
 }
 [IO.File]::WriteAllText("$goQuestOut/GoQuestCommand.inc",$goQuestSelected)
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$goQuestRepo/tools/tests/prepare_go_travel_20261114.py" $goQuestOut coordinates-only
 if($LASTEXITCODE){throw 'Native coordinate export failed'}
 $goQuestVs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $goQuestDefinition=if($Baseline){'/DGO_QUEST_BASELINE'}else{''}
 [IO.File]::WriteAllText("$goQuestOut/run.cmd","@echo off`ncall `"$goQuestVs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $goQuestDefinition /I`"$goQuestOut`" `"$goQuestRepo/tools/tests/GoQuestDestinationTest.cpp`" /Fe`"$goQuestOut/test.exe`" /Fo`"$goQuestOut/test.obj`" /Fd`"$goQuestOut/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$goQuestOut/test.exe`"`nexit /b %errorlevel%`n")
 & "$goQuestOut/run.cmd" *> "$goQuestOut/compiler-run.log"
 if($LASTEXITCODE){Get-Content "$goQuestOut/compiler-run.log" -Tail 12;throw 'Complete go-quest regression failed'}
 Get-Content "$goQuestOut/compiler-run.log"
}
