param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/native-loot-damage/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
$source="$repo/src/server/game/Entities/Creature/Creature.h";$hash=(Get-FileHash $source).Hash
$priorCompiler=$env:CL
try{
 if($Baseline){$old=(& git -C $repo show '5cc72f230:src/server/game/Entities/Creature/Creature.h') -join "`n";if($LASTEXITCODE){throw 'Pinned actual Creature header missing'};[IO.File]::WriteAllText("$out/PinnedLootCreature.h",$old);$env:CL="$priorCompiler /DLOOT_COUNTER_BASELINE /I`"$out`""}
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile NativeLootDamageEligibility.cpp -ProbeName NativeLootDamageEligibility -OutputFile "$out/result.tsv" *> "$out/build.log"
  if(!$? -or $LASTEXITCODE){Get-Content "$out/build.log" -Tail 16;throw 'Native Creature eligibility test failed'}
  Get-Content "$out/result.tsv"
 }
}finally{$env:CL=$priorCompiler}
if((Get-FileHash $source).Hash -ne $hash){throw 'Native header changed during test'}
