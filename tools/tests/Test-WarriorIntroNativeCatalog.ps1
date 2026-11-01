param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/warrior-intro-native-catalog"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $prior=$env:LEGION_DIALOGUE_CONFIG
 try{
  $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MalgalorCatalog.cpp -ProbeName MalgalorCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw 'Native Warrior intro catalog execution failed'}
 }finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
 $native=Get-Content "$out/native.tsv" -Raw
 foreach($expected in @(
  "STEP`t933`t0`t45768","STEP`t933`t1`t45770","STEP`t933`t2`t46911","STEP`t933`t3`t45772",
  "CRITERIA`t45769`t92`t46877","CRITERIA`t45771`t92`t46878","CRITERIA`t46175`t92`t47474",
  "CRITERIA`t46912`t92`t49203","CRITERIA`t46929`t92`t49262",
  "EFFECT`t205287`t0`t6`t4`t0`t0`t0`t22`t7",
  "EFFECT`t205295`t0`t28`t0`t103688`t3133`t0`t18`t0",
  "EFFECT`t205296`t0`t28`t0`t103687`t3133`t0`t18`t0",
  "DURATION`t205287`t8000`t8000",
  "TREE`t46911`t0`t4`t0`t0","TREE`t46912`t46911`t0`t1`t29573","TREE`t46929`t46911`t0`t1`t29585"
 )){if(!$native.Contains($expected)){throw "Matching native intro catalog changed: $expected"}}
 if(@($native -split "`n"|Where-Object {$_ -match '^TREE\t\d+\t46911\t'}).Count -ne 2){throw 'Native Val kyr tree no longer has exactly its two acknowledged leaves'}
 'PASS matching26972 DB2 plus read-only hotfix-loaded933 steps/criteria46877-46878-47474-49203-49262, native205287 aura8s and205295/205296 summons103688/103687 properties3133. No Players/character saves; no actual visual/cast/credit traversal proof.'
}
