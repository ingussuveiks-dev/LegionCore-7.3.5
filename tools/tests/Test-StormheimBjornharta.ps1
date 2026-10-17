$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimBjornhartaData.ps1"
& "$PSScriptRoot/Test-StormheimBuiltScale.ps1" *> "$repo/.codex/bjornharta-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-bjornharta"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-built-scale/*.inc" | Copy-Item -Destination $out
$source=Get-Content "$PSScriptRoot/StormheimBuiltScaleTest.cpp" -Raw
[IO.File]::WriteAllText("$out/BjornhartaLootFixture.inc",$source.Substring(0,$source.IndexOf('int main(')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimBjornhartaTest.cpp`" /Fe`"$out/BjornhartaLoot.exe`" /Fo`"$out/BjornhartaLoot.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/BjornhartaLoot.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run-loot.cmd",$cmd);& "$out/run-loot.cmd"
if($LASTEXITCODE){throw 'Bjornharta native item regression failed'}
