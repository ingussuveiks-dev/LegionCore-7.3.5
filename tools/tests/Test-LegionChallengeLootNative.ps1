param([string]$MigrationFile)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationFile){$MigrationFile=Join-Path $repo 'sql/updates/world/2026_11_19_05_challenge_chest_loot_links.sql'}
$MigrationFile=(Resolve-Path -LiteralPath $MigrationFile).Path
$out="$repo/build-extractors/runtime-test/legion-challenge-loot-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));$config+="`nLogger.spells=2,Server`nLogger.loot=2,Server`n";[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$migrationPrevious=$env:LEGION_CHEST_MIGRATION;$env:LEGION_CHEST_MIGRATION=$MigrationFile
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$repo/src/server/game/Challenge/Challenge.cpp","$repo/src/server/game/Scenarios/Scenario.cpp","$repo/tools/tests/LegionChallengeLootNative.cpp","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$repo/build-extractors/src/server/game/Release/game.lib")
$inputs+=@($MigrationFile,"$repo/tools/tests/LegionChallengeCompletionNative.cpp","$repo/tools/tests/LegionChallengeLifecycleNative.cpp","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/Loot/LootMgr.cpp")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash);$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""

 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionChallengeLootNative.cpp -ProbeName LegionChallengeLootNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
  if(!$? -or $LASTEXITCODE){throw "Native challenge verification failed: $out/compiler.log"}
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|LOOT|CHEST)'}
 }}finally{Pop-Location}
}finally{if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force};$env:LEGION_CHEST_MIGRATION=$migrationPrevious;$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link;foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native challenge probe input changed while running'}}}
