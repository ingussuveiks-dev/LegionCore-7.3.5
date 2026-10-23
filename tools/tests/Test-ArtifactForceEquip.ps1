param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-force-equip"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ArtifactEquipCatalog.cpp -ProbeName ArtifactEquipCatalog -OutputFile "$out/native.tsv"
if($Baseline){$source=(& git -C $repo show c235689c:src/server/game/Spells/SpellEffects.cpp) -join "`n"}else{$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw}
$start=$source.IndexOf('void Spell::EffectForceEquipItem(')
$end=$source.IndexOf('void Spell::EffectGiveArtifactPower(',$start)
if($start -lt 0 -or $end -le $start){throw 'Missing production force-equip handler'}
[IO.File]::WriteAllText("$out/ForceEquip.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$command="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ArtifactForceEquipTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/native.tsv`"`r`nexit /b %errorlevel%`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$command)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Production artifact force-equip regression failed'}
