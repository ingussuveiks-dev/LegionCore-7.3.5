param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimSalvationData.ps1"
& "$PSScriptRoot/Test-StormheimBreakSpine.ps1" *> "$PSScriptRoot/../../.codex/salvation-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-salvation"
$upstream="$repo/build-extractors/runtime-test/stormheim-break-spine"
Get-ChildItem "$upstream/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$upstream/SpineFixture.inc" -Raw
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {uint32 teleportAttempts=0,teleports=0;bool teleportSuccess=true;Position destination;bool TeleportTo(uint32,float x,float y,float z,float o){++teleportAttempts;destination={x,y,z,o};if(!teleportSuccess)return false;++teleports;static_cast<Position&>(*this)=destination;teleport=true;return true;}')
[IO.File]::WriteAllText("$out/SalvationFixture.inc",$f)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_salvation.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','');[IO.File]::WriteAllText("$out/Salvation.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_salvation()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimSalvationTest.cpp`" /Fe`"$out/Salvation.exe`" /Fo`"$out/Salvation.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Salvation.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Salvation production/native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$bytes.Contains('npc_stormheim_salvation_vethir')){throw 'Canonical Release lacks Salvation registration'}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1;if(!$relative){throw "Missing actual $name link input"};$lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative));if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name input"}}
 'PASS: canonical Release Salvation registration and actual game/scripts final-link inputs.'
}
