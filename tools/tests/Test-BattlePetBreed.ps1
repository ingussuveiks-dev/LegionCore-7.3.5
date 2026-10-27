param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/battle-pet-breed"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog",$repo) -Test {
 $oldConfig=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile BattlePetBreedCatalog.cpp -ProbeName BattlePetBreedCatalog -OutputFile "$out/catalog.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$oldConfig}
 [IO.File]::WriteAllText("$out/catalog.tsv",[IO.File]::ReadAllText("$out/catalog.tsv"),[Text.UTF8Encoding]::new($false))
 $source=Get-Content "$repo/src/server/game/Globals/BattlePetData.cpp" -Raw
 [IO.File]::WriteAllText("$out/Selection.inc",(Block $source 'uint16 BattlePetDataStoreMgr::GetRandomBreedID('))
 [IO.File]::WriteAllText("$out/Weight.inc",(Block $source 'uint8 BattlePetDataStoreMgr::GetWeightForBreed('))
 $old=(& git -C $repo show 'cda9d021e:src/server/game/Globals/BattlePetData.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned baseline absent'}
 [IO.File]::WriteAllText("$out/Baseline.inc",(Block $old 'uint16 BattlePetDataStoreMgr::GetRandomBreedID('))
 [IO.File]::WriteAllText("$out/Stats.inc",(Block (Get-Content "$repo/src/server/game/PetBattle/PetBattle.cpp" -Raw) 'void BattlePet::UpdateStats('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/BattlePetBreedTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe catalog.tsv`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native breed regression failed'}
}
