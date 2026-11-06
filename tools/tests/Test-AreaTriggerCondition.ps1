param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/areatrigger-condition/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $source=if($Baseline){(& git -C $repo show 'a6a93ec3c:src/server/game/Conditions/ConditionMgr.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw}
 $a=$source.IndexOf('case CONDITION_SOURCE_TYPE_AREATRIGGER_ACTION:',$source.IndexOf('bool ConditionMgr::isSourceTypeValid('));$b=$source.IndexOf('case CONDITION_SOURCE_TYPE_LOOT_ITEM:',$a)
 if($a -lt 0 -or $b -le $a){throw 'Actual complete area validator case missing'}
 [IO.File]::WriteAllText("$out/Validation.inc",$source.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/AreaTriggerConditionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 12;throw 'Actual area condition validator regression failed'}
 Get-Content "$out/test.log"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
}
