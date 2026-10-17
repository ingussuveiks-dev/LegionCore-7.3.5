$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimJournalsCatalog.ps1"
& "$PSScriptRoot/Test-StormheimJournalAccess.ps1"
$out="$repo/build-extractors/runtime-test/stormheim-journals"
$previousPath=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $all=@();$failures=@()
 # Halvir's unchanged roster has a collision-floor walking approach beyond the
 # navmesh boundary. JournalAccess above requires the full corridor to its join
 # and every physical walk segment; the old partial direct route is not a pass.
 foreach($target in @('3393.29 1508.63 60.9625','3357.12 1648.86 7.55221','3438.79 1560.27 7.01219','3521.58 1615.26 11.3535')){
  @('3296.72 1377.97 97.9798',$target) | Set-Content "$out/ground.txt"
  $rows=@(& "$out/StormheimStemTideGeometry.exe" $runtime "$out/ground.txt" 65535 1220 walk-swim);$code=$LASTEXITCODE
  $all+="target $target exit=$code";$all+=$rows;$rows
  if($code){$failures+=$target}
 }
 $all | Set-Content "$out/geometry.txt"
 @('2672.65 1352.05 43.4566','2690.31 1636.36 12.9985') | Set-Content "$out/ground.txt"
 $rows=@(& "$out/StormheimStemTideGeometry.exe" $runtime "$out/ground.txt" 65535);$code=$LASTEXITCODE
 $rows | Set-Content "$out/priest-geometry.txt";$rows
 if($code){$failures+='Priestess293517'}
 # Keep this diagnostic separate from catalog/progression tests. A failed
 # static approach is not a green interaction check or a proven bad spawn.
 if($failures.Count){throw "UNVERIFIED static approaches: $($failures -join '; '). See geometry.txt; do not claim full quest/client verification."}
}finally{$env:PATH=$previousPath}
