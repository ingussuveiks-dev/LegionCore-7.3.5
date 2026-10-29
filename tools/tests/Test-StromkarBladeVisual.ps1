param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/stromkar-blade-visual-regression-2026-10-29"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarBladeVisualCatalog.cpp -ProbeName StromkarBladeVisualCatalog -OutputFile "$out/native.tsv"
 if(!$?){throw 'Native blade serialization regression failed'}
 Get-Content "$out/native.tsv"
 & "$PSScriptRoot/Test-StromkarBladeSchema.ps1" -VerifyLiveData:$VerifyReleaseBinary
 if(!$?){throw 'Native blade schema regression failed'}
 if($VerifyReleaseBinary){
  $old=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$old";& 'C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/verify_stromkar_blade_visual_release.py" "$repo/build-extractors/bin/Release" "$out/native.tsv";if($LASTEXITCODE){throw 'Canonical blade client hotfix path failed'}}finally{$env:PYTHONPATH=$old}
 }
}
