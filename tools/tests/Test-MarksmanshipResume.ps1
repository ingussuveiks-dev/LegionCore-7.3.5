param([switch]$Baseline,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and ($VerifyLiveData -or $VerifyReleaseBinary)){throw 'Missing-data counterfactual cannot certify live data or Release'}
$out="$repo/build-extractors/runtime-test/marksmanship-resume-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test/marksmanship-resume-data","$repo/build-extractors/runtime-test/marksmanship-entrance-native","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $cl=$env:CL;$link=$env:_LINK_
 try{$env:CL="$cl /Zi /Fd`"$out/catalog-compiler.pdb`"";$env:_LINK_="$link /DEBUG /PDB:`"$out/catalog.pdb`" /MAP:`"$out/catalog.map`"";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MarksmanshipEntranceCatalog.cpp -ProbeName MarksmanshipEntranceCatalog -OutputFile "$out/catalog.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native map route catalog failed'}}finally{$env:CL=$cl;$env:_LINK_=$link}
 $rows=@(Get-Content "$out/catalog.tsv")
 foreach($expected in @("LFG`t1162`t1489`t972`t12`t75795458","EFFECT`t201674`t0`t227`t0`t0`t1`t17","COMPLETE`t7")){if($rows -notcontains $expected){throw "Matching26972 route/source changed: $expected"}}
 if($Baseline){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -MarksmanshipEntranceBaseline *> "$out/native.log"}else{
  & "$PSScriptRoot/Test-MarksmanshipResumeData.ps1" -VerifyLiveData:$VerifyLiveData *> "$out/data.log";if(!$?){throw "Native entrance SQL regression failed: $out/data.log"}
  if($VerifyLiveData){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -MarksmanshipEntranceLive *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -MarksmanshipEntrance *> "$out/native.log"}
 }
 if(!$?){throw "Native Marksmanship entrance regression failed: $out/native.log"}
 $native=[IO.File]::ReadAllText("$out/native.log")
 if($Baseline){if($native -notmatch 'COUNTERFACTUAL native Marksmanship entrance22'){throw 'Unexpected missing-entrance baseline/count'};'COUNTERFACTUAL missing1162 data:14 native late records fail;ordinary/existing/shared940 and completed cleanup controls retained. TEMP-only,not ordinary success.'}else{
  if($native -notmatch 'PASS native Marksmanship entrance30' -or $native -match 'COUNTERFACTUAL native Marksmanship'){throw 'Unexpected corrected native entrance/count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_warrior_scenario_resume_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log";if($LASTEXITCODE){throw 'Actual linked native Recorder/LFG data branch failed'}}finally{$env:PYTHONPATH=$old}}
  'PASS Marksmanship resume entrance:native26972 catalog,TEMP16 refusals/idempotence/custom-name/full-table preservation,native30 recorder/prepare/live/fresh/constructor/floor/completed48210 cleanup and shared940. No character/item/quest grant. Earlier state/final event supplied;empty old receipts and full persisted-character/group/client traversal remain separate.'
 }
}
