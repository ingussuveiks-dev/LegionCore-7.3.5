param([switch]$Baseline,[switch]$Mechanics,[string]$Case='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/five-ten-20261111/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $extra=if($Baseline){'baseline'}else{'fixed'}
 $prepareArgs=@("$PSScriptRoot/prepare_five_ten_20261111.py",$out,$extra);if($Case){$prepareArgs+=$Case}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe @prepareArgs
 if($LASTEXITCODE){throw 'Production method extraction failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $selected=$null;if($Case){$selected=Get-Content "$out/cases.json" -Raw|ConvertFrom-Json|Where-Object key -eq $Case;if(!$selected){throw 'Unknown repair case'};$Mechanics=[bool]$selected.mechanics}
 $source=if($Mechanics){'FiveTen20261111MechanicsTest.cpp'}else{'FiveTen20261111MethodsTest.cpp'}
 $includes="/I`"$out`" /I`"$repo/src/common`" /I`"$repo/src/server/game/Spells`" /I`"C:/local/boost_1_85_0`""
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD /DNOMINMAX /fsanitize=address /Zi $includes `"$PSScriptRoot/$source`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/build.cmd`" > `"$out/build.log`" 2>&1"
 if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 28;throw 'Production method fixture compile failed'}
 $count=if($Mechanics){21}else{29}
 $indices=if($selected){@([int]$selected.index)}else{@(0..($count-1))}
 foreach($caseIndex in $indices){
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $caseIndex`nexit /b %errorlevel%`n")
  & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/run-$caseIndex.log`" 2>&1";$code=$LASTEXITCODE;$raw=Get-Content "$out/run-$caseIndex.log" -Raw
  if($Baseline){if(!$code){throw "Pinned counterfactual unexpectedly passes: $caseIndex"};"PASS expected-broken pinned method $caseIndex; repaired pair required"}
  elseif($code -or (!$raw.Contains('PASS complete production method') -and !$raw.Contains('PASS extracted mechanics case'))){Get-Content "$out/run-$caseIndex.log" -Tail 16;throw "Repaired production scope failed: $caseIndex"}
  else{$raw.Trim()}
 }
}
$global:LASTEXITCODE=0
