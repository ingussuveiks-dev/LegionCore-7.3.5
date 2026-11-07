param([switch]$Baseline,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and ($VerifyLiveData -or $VerifyReleaseBinary)){throw 'Expected-broken control does not certify live data or Release'}
$out="$repo/build-extractors/runtime-test/earthwarder-resume-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('magnar-login-native','earthwarder-resume-data','scenario-resume','scepter-login','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -LoginBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Native missing-entrance control failed: $out/counterfactual.log"}
  'COUNTERFACTUAL native Human/Orc genuine10 deaths/46427/4 plus absent1028 prevents recording legacy entrance;observed missing-row state modeled in a TEMP shadow,not a successful ordinary run.'
 }else{
  & "$PSScriptRoot/Test-EarthwarderResumeData.ps1" -VerifyLiveData:$VerifyLiveData *> "$out/data.log";if(!$?){throw "Entrance SQL regression failed: $out/data.log"}
  & "$PSScriptRoot/Test-ScenarioResume.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/source.log";if(!$?){throw "Actual generic login regression failed: $out/source.log"}
  if($VerifyLiveData){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -LoginLiveData *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -LoginRecovery *> "$out/native.log"}
  if(!$?){throw "Native Protection route/login regression failed: $out/native.log"}
  if($VerifyReleaseBinary){& "$PSScriptRoot/Test-ScepterLogin.ps1" -VerifyReleaseBinary *> "$out/scepter.log";if(!$?){throw "Canonical Scepter regression failed: $out/scepter.log"}}
  'PASS native Protection resume8: actual owned10/46427/4 records187803 entrance/floor66.3065,live instance keeps chamber,boot/instance mismatch returns to entrance,actual full Map destruction/recycledID/LFG construction starts909-0 without old combat/credit/items. Full-schema idempotence/8 source/custom refusals preserve other routes. No production C++ or character data change;full client traversal remains separate.'
 }
}
