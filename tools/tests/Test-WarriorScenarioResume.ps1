param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Expected-broken source control cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/warrior-scenario-resume-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('warrior-scenario-resume-native','scenario-resume','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-ScenarioResume.ps1" -AddressSanitizer -WarriorBaseline:$Baseline *> "$out/source.log";if(!$?){throw "Actual/pinned source regression failed: $out/source.log"}
 if($Baseline){
  'COUNTERFACTUAL pinned2396b47f7 source64:44 expected-broken shared Warrior entrance/source cases,20 retained controls. Original native pre-build4 observations are separate evidence;current game.lib is not a historical baseline.'
 }else{
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -WarriorLogin *> "$out/native.log";if(!$?){throw "Native shared-map Warrior login failed: $out/native.log"}
  $native=[IO.File]::ReadAllText("$out/native.log")
  if($native -notmatch 'PASS native shared Warrior login cases=56' -or $native -match 'COUNTERFACTUAL shared Warrior entrance='){throw "Unexpected native variant/count: $out/native.log"}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_warrior_scenario_resume_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw "Selected linked Release failed: $out/release.log"}}finally{$env:PYTHONPATH=$old}}
  'PASS Fury944/1511 and Arms1037/1539 legacy entrance:actual source64 ASAN/native56,live/stale/full-map destruction/recycled-ID/native constructor,foreign shared LFG and class controls,24 actual native invalid-source refusals,preserved existing/ordinary-first entry. No movement/progression/character data grant;full client/all3 Warrior remains separate.'
 }
}
