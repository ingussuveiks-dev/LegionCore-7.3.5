param([string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/.codex/fuzan-open-movement/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"}
$out=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
[void][IO.Directory]::CreateDirectory($out)
$config="$repo/build-extractors/bin/Release/worldserver.conf"
$configHash=(Get-FileHash -LiteralPath $config).Hash
$protected="$repo/build-extractors/runtime-test/legion-legendary-catalog/SlaghammerHandoffNative.exe"
$protectedHash=(Get-FileHash -LiteralPath $protected).Hash
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $prior=$env:LEGION_DIALOGUE_CONFIG
 try{
  $env:LEGION_DIALOGUE_CONFIG=$config
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FuZanOpenMovementCatalog.cpp -ProbeName FuZanOpenMovementCatalog -OutputFile "$out/catalog.tsv" *> "$out/compiler-run.log"
  if(!$? -or $LASTEXITCODE){throw 'Native movement catalog failed; inspect captured compiler-run.log'}
 }finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
 $lines=@(Get-Content -LiteralPath "$out/catalog.tsv" -Encoding UTF8)
 if($lines[-1] -cne "EOF`tFuZanOpenMovementCatalog"){throw 'Native EOF absent'}
 $paths=@{};$points=@{};$references=@();$criteria=@()
 foreach($line in $lines){
  $fields=$line.Split("`t")
  if($fields[0] -eq 'PATH'){
   $id=[uint32]$fields[1];if($paths.ContainsKey($id)){throw 'Repeated native path'}
   $paths[$id]=@{id=$id;point_count=[uint32]$fields[2];definition_present=([uint32]$fields[3] -eq 1);from=[uint32]$fields[4];to=[uint32]$fields[5]}
  }elseif($fields[0] -eq 'POINT'){
   $id=[uint32]$fields[1];if(!$points.ContainsKey($id)){$points[$id]=@()}
   if([uint32]$fields[3] -ne 1625){throw 'Selected native route leaves the Temple map; review before accepting'}
   $points[$id]+=[uint32]$fields[2]
  }elseif($fields[0] -eq 'SPELL_REFERENCE'){$references+=$line}
  elseif($fields[0] -eq 'CRITERION_REFERENCE'){$criteria+=$line}
 }
 $expected=@(5699,5700,5701,5719,5720,5721,5727,5728,5729)
 if($paths.Count -ne $expected.Count){throw 'Native candidate set changed; investigate instead of assuming an actor binding'}
 foreach($id in $expected){
  if(!$paths.ContainsKey([uint32]$id) -or $points[[uint32]$id].Count -ne $paths[[uint32]$id].point_count){throw 'Incomplete selected native route'}
  for($n=0;$n -lt $points[[uint32]$id].Count;++$n){if($points[[uint32]$id][$n] -ne $n){throw 'Native route indices are not continuous'}}
 }
 if(($points.Values|ForEach-Object Count|Measure-Object -Sum).Sum -ne 73){throw 'Selected native point count changed'}
 if($configHash -ne (Get-FileHash -LiteralPath $config).Hash -or $protectedHash -ne (Get-FileHash -LiteralPath $protected).Hash){throw 'Protected configuration/evidence changed'}
 $inputs=@{}
 foreach($relative in @('tools/research/FuZanOpenMovementCatalog.cpp','tools/tests/Test-FuZanOpenMovementCatalog.ps1','build-extractors/bin/Release/dbc/enUS/TaxiNodes.db2','build-extractors/bin/Release/dbc/enUS/TaxiPath.db2','build-extractors/bin/Release/dbc/enUS/TaxiPathNode.db2','build-extractors/bin/Release/dbc/enUS/Spell.db2','build-extractors/bin/Release/dbc/enUS/SpellEffect.db2','build-extractors/bin/Release/dbc/enUS/Criteria.db2')){$inputs[$relative]=(Get-FileHash -LiteralPath "$repo/$relative").Hash}
 @{status='READ_ONLY_CANDIDATES_NOT_ACTOR_BINDINGS';paths=@($paths.Values|Sort-Object id);points=73;spell_references=$references;event52731_criteria=$criteria;inputs=$inputs;catalog_sha256=(Get-FileHash -LiteralPath "$out/catalog.tsv").Hash;limits='No authored NPC-path association or return-flight implementation proven. No persistent DB/character/account writes;no server launched.'}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath "$out/result.json" -Encoding UTF8
 'PASS9 native Temple path candidates,73 ordered map1625 points,definition-presence metadata and spell/event references;no actor binding inferred'
}
$global:LASTEXITCODE=0
