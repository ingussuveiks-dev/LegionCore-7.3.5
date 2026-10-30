param([string]$OutputDirectory="$PSScriptRoot/../../.codex/eagle-flight-native-paths-2026-10-30")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $old=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile EagleFlightPathCatalog.cpp -ProbeName EagleFlightPathCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
 $rows=@(Get-Content "$out/native.tsv");if(!$rows.Count -or $rows[-1] -notmatch '^COMPLETE\t(\d+)\t(\d+)\t(\d+)\t(\d+)$'){throw 'Missing native ordered-path receipt'}
 $pathCount=[int]$Matches[2];$pointCount=[int]$Matches[3];$candidateCount=[int]$Matches[4]
 if($pathCount -le 0 -or $pointCount -le 0 -or @($rows|Where-Object {$_ -like "PATH`t*"}).Count -ne $pathCount -or @($rows|Where-Object {$_ -like "CANDIDATE`t*"}).Count -ne $candidateCount){throw 'Incomplete native path inventory'}
 $selected=@($rows|Where-Object {$_ -like "POINT`t5762`t*"});if($selected.Count -ne 19){throw 'Reinspect candidate5762 native node count'}
 for($i=0;$i -lt 19;++$i){if([int]$selected[$i].Split("`t")[2] -ne $i -or [int]$selected[$i].Split("`t")[3] -ne 1220){throw 'Candidate node ordering/map differs'}}
 foreach($route in @{5762=19;5763=10;5955=10;5765=12}.GetEnumerator()){
  $points=@($rows|Where-Object {$_ -like "ROUTE_POINT`t$($route.Key)`t*"})
  if($points.Count -ne $route.Value -or $rows -notcontains "ROUTE_DEFINITION`t$($route.Key)`t0`t0"){throw 'Reinspect native route definition/count'}
  for($i=0;$i -lt $points.Count;++$i){$f=$points[$i].Split("`t");if($f.Count -ne 12 -or [int]$f[2] -ne $i -or [int]$f[3] -ne 1220){throw 'Native route point schema/order/map differs'}}
 }
 foreach($expected in @("PATH_DEFINITION`t5762`t0`t0","ZERO_NODE`tABSENT","SPELL_REFERENCE`t201652`tRelentless Storm`t179`t5762`t0","REFERENCE_COUNT`t1")){if($rows -notcontains $expected){throw "Reinspect native candidate definition/reference: $expected"}}
 $rows|Where-Object {$_ -match '^(COMPLETE|CANDIDATE|MOUNT_NODE|PATH_DEFINITION|SPELL_REFERENCE|REFERENCE_COUNT|ZERO_NODE)\t'}
 "PASS read-only native all-path geometry inventory; candidate count=$candidateCount is discovery only, not an implemented or verified39427 flight"
}
