param([string]$Release='build-extractors/bin/Release',[string]$OutputDirectory='',[string]$RootsJson='',[switch]$Counterfactual,[string]$BaselineRef='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/.codex/artifact-release-certificates/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"}
[void][IO.Directory]::CreateDirectory($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$OutputDirectory/evidence" -RuntimeDirectories @($Release) -Test {
 $arguments=@("$PSScriptRoot/verify_artifact_release_certificates.py",$Release,'--out',$OutputDirectory)
 if($RootsJson){$arguments+=@('--roots-json',$RootsJson)}
 if($Counterfactual){$arguments+='--counterfactual'}
 if($BaselineRef){$arguments+=@('--baseline-ref',$BaselineRef)}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe @arguments
 if($LASTEXITCODE){throw 'Artifact certificate collection contains unresolved failures; individual exit/output retained'}
}
$global:LASTEXITCODE=0
