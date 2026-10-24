param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
& "$PSScriptRoot/Test-XalatathFarthing.ps1" -Stromkar -Baseline:$Baseline -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$?){throw 'Stromkar Zakajz regression failed'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-zakajz"
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$?){throw 'Native Zakajz catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("OVERRIDE`t684`t0`t206455","EFFECT`t206455`t0`t42`t0`t35`t206456","EFFECT`t206456`t0`t61`t0`t49109","SPELL`t201904`tDark Drain","SPELL`t207454`tCosmetic Dissolve","CRITERION`t8`t29551`t92`t49108","CRITERION`t9`t29552`t92`t49109")){if(!$native.Contains($expected)){throw "Native finale data changed: $expected"}}
'PASS: native684 -> Execute206455/jump206456/49109 traced separately from preserved legacy Dark Drain201904 list; no completed client execution or departure claimed'
