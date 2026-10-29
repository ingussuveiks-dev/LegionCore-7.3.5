param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$tag=if($Baseline){'counterfactual'}else{'repaired'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$repo/.codex/stromkar-exit-lifetime-$tag-2026-10-29" -RuntimeDirectories @("$repo/build-extractors/runtime-test/stromkar-return") -Test {
 & "$PSScriptRoot/Test-StromkarReturn.ps1" -AddressSanitizer -LifetimeBaseline:$Baseline
 if(!$?){throw 'Exit lifetime regression failed'}
}
