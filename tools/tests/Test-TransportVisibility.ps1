param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/transport-visibility"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=[IO.File]::ReadAllText("$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp")
$start=$source.IndexOf('void VisibleNotifier::SendToSelf()')
if($start -lt 0){throw 'Missing native visibility cleanup'}
$end=$source.IndexOf('{',$start)+1;$depth=1
while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
[IO.File]::WriteAllText("$out/TransportSendToSelf.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TransportVisibilityTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native moving transport visibility regression failed'}
