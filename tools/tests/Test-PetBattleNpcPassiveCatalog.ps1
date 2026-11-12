param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/pet-npc-passive-catalog-2026-11-12/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory("$out/logs")
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";& "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile 'PetBattleNpcPassiveCatalog.cpp' -ProbeName PetBattleNpcPassiveCatalogNative -OutputFile "$out/native.tsv" *> "$out/build.log"
 if(!$? -or $LASTEXITCODE){Get-Content "$out/build.log" -Tail 16;throw 'Native lifecycle probe failed; outputs retained'}
 if(!([IO.File]::ReadAllText("$out/native.tsv")).Contains('WORLD_READ_EOF')){throw 'Native readonly world capture did not reach EOF'};Get-Content "$out/native.tsv" -Tail 18
}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$global:LASTEXITCODE=0
