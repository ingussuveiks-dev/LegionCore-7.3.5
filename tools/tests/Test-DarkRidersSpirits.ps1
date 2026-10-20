param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-riders-spirits"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_riders_artifact.cpp" -Raw
$a=$source.IndexOf('namespace DarkRidersSpirits');$b=$source.IndexOf('namespace DarkRidersRevil',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing production contact handler'}
[IO.File]::WriteAllText("$out/DarkRidersSpirits.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkRidersSpiritsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Spirit contact regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DarkRidersSpiritsCatalog.cpp -ProbeName DarkRidersSpiritsCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('effect 198721 0 6 4 0 0 0 1','effect 198654 0 6 33 75 0 0 1','effect 198654 1 6 312 0 0 0 1','effect 198654 2 6 23 0 1000 199568 1','effect 199568 0 2 0 25000 0 0 1','duration 198654 -1 -1')){
 if($native -notcontains $expected){throw "Native spirit data mismatch: $expected"}
}
'PASS: native7.3.5 DB2 confirms visual-only NPC aura,75% slow, animation, one-second damage trigger,25k base hit and CASTER targets'
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 if($map -notmatch '\?\?R<lambda_1>@[^\s]*Update@Instance@instance_dark_riders_artifact@@'){throw 'Canonical map lacks contact update lambda'}
 foreach($symbol in @('?OnPlayerLeave@Instance@instance_dark_riders_artifact@@')){
  if(!$map.Contains($symbol)){throw "Canonical link map lacks contact caller/cleanup: $symbol"}
 }
 'PASS: canonical map contains contact caller/cleanup (contact is inlined); deployed instruction verification recorded separately'
}
