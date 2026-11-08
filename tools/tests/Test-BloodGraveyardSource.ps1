param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/blood-graveyard-source/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $source=if($Baseline){(& git -C $repo show '9d0d0d1bb:src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp') -join "`n"}else{Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp" -Raw}
 $a=$source.IndexOf('        WorldLocation* GetClosestGraveYard(');$b=$source.IndexOf('        void OnUnitDeath(',$a);if($a -lt 0 -or $b -le $a){throw 'Actual/pinned cemetery callback missing'}
 [IO.File]::WriteAllText("$out/TuureGraveyard.inc",$source.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flag=if($Baseline){'/DBLOOD_GRAVEYARD_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" $flag /I`"$out`" `"$PSScriptRoot/BloodGraveyardTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){Get-Content "$out/source.log" -Tail 4;throw 'Blood cemetery source ASAN failed'};Get-Content "$out/source.log" -Tail 2
}
