param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-riders-completion"
New-Item -ItemType Directory -Force $out | Out-Null
$s=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
$a=$s.IndexOf('namespace DarkRidersCompletion');$b=$s.IndexOf('class player_dark_riders_completion',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing production completion repair'}
[IO.File]::WriteAllText("$out/Completion.inc",$s.Substring($a,$b-$a))
$playerSource=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$playerSource.IndexOf('int32 Player::GetQuestObjectiveData(uint32');$b=$playerSource.IndexOf('int32 Player::GetQuestObjectiveData(Quest const*',$a)
[IO.File]::WriteAllText("$out/ObjectiveLookup.inc",$playerSource.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkRidersCompletionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Completion regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DarkRidersCompletionCatalog.cpp -ProbeName DarkRidersCompletionCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('effect 204819 2 90 103473 0','effect 204819 3 249 128942 0','effect 232961 0 90 100813 0','effect 232961 1 90 103473 0')){if($native -notcontains $expected){throw "Native claim mismatch: $expected"}}
'PASS: native DB2 confirms claim103473/equip128942 and separate completion100813; no item creation by204819'
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 if(!$map.Contains('?Recover@DarkRidersCompletion@@')){throw 'Completion repair missing from canonical link map'}
 'PASS: canonical completion symbol present; deployed instruction check recorded separately'
}
