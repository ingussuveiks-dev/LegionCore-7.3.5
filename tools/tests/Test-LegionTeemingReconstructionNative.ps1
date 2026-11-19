param([Parameter(Mandatory=$true)][string]$ModelDataDirectory,[Parameter(Mandatory=$true)][string]$MigrationFile)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legion-teeming-reconstruction/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
$config+="`nLogger.spells=2,Server`n"
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$fixture="$out/fixture";New-Item -ItemType Directory -Force $fixture|Out-Null
& "C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe" "$repo/tools/research/generate_legion_teeming_spawns.py" --output "$fixture/generated.sql" --fixture-directory $fixture
if($LASTEXITCODE -or (Get-FileHash "$fixture/generated.sql").Hash -ne (Get-FileHash -LiteralPath $MigrationFile).Hash){throw "Migration differs from reviewed catalog"}
$env:LEGION_TEEMING_SHADOW_SQL="$fixture/shadow.sql";$env:LEGION_TEEMING_NATIVE_ROWS="$fixture/native-rows.tsv";$env:LEGION_TEEMING_MIGRATION_SQL=(Resolve-Path -LiteralPath $MigrationFile).Path;$env:LEGION_MAW_MODEL_DATA=(Resolve-Path -LiteralPath $ModelDataDirectory).Path.Replace("\","/")
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$repo/src/server/game/Entities/Creature/Creature.cpp","$repo/src/server/game/Scenarios/ScenarioMgr.cpp","$repo/src/server/game/Scenarios/Scenario.cpp","$repo/src/server/game/Challenge/Challenge.cpp","$repo/src/server/game/Entities/Creature/TemporarySummon.cpp","$repo/src/server/game/Entities/Unit/Unit.cpp","$repo/src/server/game/Entities/Object/Object.cpp","$repo/tools/tests/LegionChallengeDeathAffixesNative.cpp","$repo/src/server/scripts/World/challenge_scripts.cpp","$repo/tools/tests/LegionTeemingReconstructionNative.cpp","$repo/tools/tests/LegionChallengeLifecycleNative.cpp","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$repo/build-extractors/src/server/game/Release/game.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash)
$sealed+=@(Get-FileHash -LiteralPath @($MigrationFile,"$repo/docs/research/legion-teeming-reconstruction-2026-11-19.json","$repo/tools/research/generate_legion_teeming_spawns.py","$repo/tools/tests/LegionTeemingCriteriaNative.cpp","$repo/src/server/scripts/Legion/MawofSouls/MawSkjalCrew.h","$repo/src/server/scripts/Legion/MawofSouls/instance_maw_of_souls.cpp","$repo/src/server/scripts/Legion/MawofSouls/boss_helya.cpp","$ModelDataDirectory/mmaps/go31852.mmap","$ModelDataDirectory/mmaps/go31853.mmap")|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""

 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionTeemingReconstructionNative.cpp -ProbeName LegionTeemingReconstructionNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
   if(!$? -or $LASTEXITCODE){throw "Native Teeming verification failed: $out/compiler.log"}
   Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(META|PASS|DATA|ENV|INSPECT|COUNTERFACTUAL)'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link
 foreach($name in @('LEGION_TEEMING_SHADOW_SQL','LEGION_TEEMING_NATIVE_ROWS','LEGION_TEEMING_MIGRATION_SQL','LEGION_MAW_MODEL_DATA')){Remove-Item "Env:$name" -ErrorAction SilentlyContinue}
 if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force}
 foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native Teeming input changed while running'}}
}
