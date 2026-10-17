$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimWeatherData.ps1"
# Reuse the existing production selector/personal-loot/quest extraction.
& "$PSScriptRoot/Test-StormheimOscillator.ps1" *> "$repo/.codex/weather-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-weather"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-oscillator/*.inc" | Copy-Item -Destination $out
$inventory=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$inventory.Replace('GetMaxStackSize()const{return 10;}','GetMaxStackSize()const{return 1;}'))
$source=Get-Content "$PSScriptRoot/StormheimOscillatorTest.cpp" -Raw
$prefix=$source.Substring(0,$source.IndexOf('int main(')).Replace('assert(lootId==240286);','assert(lootId==240677||lootId==240681||lootId==241460||lootId==241462);')
[IO.File]::WriteAllText("$out/WeatherFixture.inc",$prefix)
$loader=Get-Content "$repo/src/server/game/Globals/ScriptsData.cpp" -Raw
$a=$loader.IndexOf('        case SCRIPT_COMMAND_QUEST_EXPLORED:');$b=$loader.IndexOf('        case SCRIPT_COMMAND_KILL_CREDIT:',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native exploration script loader missing'}
[IO.File]::WriteAllText("$out/WeatherScriptLoader.inc",$loader.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimWeatherTest.cpp`" /Fe`"$out/Weather.exe`" /Fo`"$out/Weather.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Weather.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Weather native regression failed'}
