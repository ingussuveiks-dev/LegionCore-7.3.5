$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimJournalsCatalog.ps1"
$out="$repo/build-extractors/runtime-test/stormheim-journals"
$previousPath=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $all=@();$failures=@()
 foreach($target in @('3393.29 1508.63 60.9625','3357.12 1648.86 7.55221','3432.79 1564.77 9.06416','3515.9 1526.83 8.7028','3521.58 1615.26 11.3535')){
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
