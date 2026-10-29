param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$tag=if($Baseline){'counterfactual'}else{'repaired'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$repo/.codex/stromkar-terminal-regeneration-$tag-2026-10-29" -RuntimeDirectories @("$repo/build-extractors/runtime-test/stromkar-zakajz") -Test {
 & "$PSScriptRoot/Test-XalatathFarthing.ps1" -Stromkar -AddressSanitizer -TerminalBaseline:$Baseline
 if(!$?){throw 'Stromkar terminal lifecycle failed'}
}
