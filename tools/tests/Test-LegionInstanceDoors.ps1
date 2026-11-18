param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legion-instance-doors/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $extra=if($Baseline){@('--baseline','ee492cf20')}else{@()}
 & $python -X utf8 "$PSScriptRoot/prepare_legion_instance_doors.py" $out @extra
 if($LASTEXITCODE){throw 'Actual door array extraction failed'}
 $compile="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/test.pdb`" /I`"$out`" `"$PSScriptRoot/LegionInstanceDoorsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/build.cmd",$compile);& "$out/build.cmd" *> "$out/compiler.log";if($LASTEXITCODE){throw "ASAN door compile failed:$out/compiler.log"}
 $broken=@(1458,1477,1492,1493,1501,1516,1571,1651,1677);$failures=0
 foreach($map in @(1456,1458,1466,1477,1492,1493,1501,1516,1571,1651,1677,1753)){
  [IO.File]::WriteAllText("$out/run-$map.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" $map > `"$out/$map.stdout.log`" 2> `"$out/$map.stderr.log`"`nexit /b %errorlevel%`n")
  & "$out/run-$map.cmd";$status=$LASTEXITCODE
  $stderr=Get-Content "$out/$map.stderr.log" -Raw
  if($Baseline -and $map -in $broken){if(!$status -or $stderr -notmatch 'AddressSanitizer: global-buffer-overflow'){throw "Expected original out-of-bounds read not reproduced:$map"};++$failures;"COUNTERFACTUAL map=$map expected ASAN global-buffer-overflow;not an ordinary successful run"}
  else{if($status -or $stderr){throw "Unexpected door test failure:$map exit=$status"};Get-Content "$out/$map.stdout.log"}
 }
 if($Baseline -and $failures -ne 9){throw 'Missing original nine-table failures'}
}
