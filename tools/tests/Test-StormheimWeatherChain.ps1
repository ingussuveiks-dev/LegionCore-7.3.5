param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimWeatherChainData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimSupplies.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$repo/.codex/weather-chain-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-weather"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-helheim-supplies/*.inc" | Copy-Item -Destination $out
$source=Get-Content "$PSScriptRoot/StormheimHelheimSuppliesTest.cpp" -Raw
[IO.File]::WriteAllText("$out/WeatherChainFixture.inc",$source.Substring(0,$source.IndexOf('int main(')))
$source=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$source.IndexOf('        if (qinfo->ExclusiveGroup)');$b=$source.IndexOf('        if (qinfo->LimitTime)',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native exclusive group loader missing'}
[IO.File]::WriteAllText("$out/WeatherGroupLoader.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimWeatherChainTest.cpp`" /Fe`"$out/WeatherChain.exe`" /Fo`"$out/WeatherChain.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/WeatherChain.exe`" `"$out/chain.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run-chain.cmd",$cmd);& "$out/run-chain.cmd"
if($LASTEXITCODE){throw 'Weather native prerequisite regression failed'}
