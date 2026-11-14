param([string]$OutputDirectory="$PSScriptRoot/../../.codex/fuzan-return-route")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
$protected="$repo/build-extractors/runtime-test/legion-legendary-catalog/SlaghammerHandoffNative.exe"
$protectedHash=(Get-FileHash -LiteralPath $protected).Hash
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FuZanReturnFlight.cpp -ProbeName FuZanReturnFlight -OutputFile "$out/geometry.tsv" *> "$out/build.log"
 if(!$? -or $LASTEXITCODE){throw 'Native reconstructed route geometry failed; inspect captured output'}
 $lines=@(Get-Content "$out/geometry.tsv")
 if($lines[-1] -cne "EOF`tFuZanReturnFlight`t1" -or @($lines|Where-Object {$_ -match '^SMOOTH_SEGMENT\t1625\t\d+\t\d+\t[^\t]+\t1$'}).Count -ne 3){throw 'Departure geometry segment set changed'}
 if($protectedHash -ne (Get-FileHash -LiteralPath $protected).Hash){throw 'Protected unresolved LINK2 executable changed'}
 $inputs=@{}
 foreach($relative in @('tools/research/FuZanReturnFlight.cpp','src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_return_route.h')){$inputs[$relative]=(Get-FileHash "$repo/$relative").Hash}
 @{inputs=$inputs;geometry_sha256=(Get-FileHash "$out/geometry.tsv").Hash;status='FUNCTIONAL_RECONSTRUCTION_NOT_RETAIL_SPLINE';protected_link2_sha256=$protectedHash}|ConvertTo-Json -Depth 4|Set-Content "$out/result.json"
 Get-Content "$out/geometry.tsv"
}
