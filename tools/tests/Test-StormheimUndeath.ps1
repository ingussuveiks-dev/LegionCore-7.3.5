param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimUndeathData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-undeath"
& "$PSScriptRoot/Test-StormheimNewBeginning.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/alliance-regression.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-new-beginning/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('uint64 race=1;','uint64 race=2;').Replace('team=ALLIANCE','team=HORDE')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$s=Get-Content "$PSScriptRoot/StormheimNewBeginningTest.cpp" -Raw
$s=$s.Replace('39122','38882').Replace('39092','39155').Replace('190073','205551').Replace('95269','94915').Replace('95804','94228').Replace('338653','340424').Replace('494','495').Replace('npc_stormheim_new_beginning_genn','npc_stormheim_undeath_sylvanas').Replace('case 2:p.team=HORDE','case 2:p.team=ALLIANCE').Replace('2851.9','2370.01').Replace('863.39','1849.31').Replace('77.8','29.17')
[IO.File]::WriteAllText("$out/UndeathTest.cpp",$s)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$out/UndeathTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Undeath native regression failed'}
if($VerifyReleaseBinary){
 $text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 foreach($marker in @('npc_stormheim_undeath_sylvanas','Stormheim: Undeath movie before personal camp transfer')){if(!$text.Contains($marker)){throw "Canonical Release lacks $marker"}}
 'PASS: canonical Release Undeath registration and shared final-link freshness.'
}
