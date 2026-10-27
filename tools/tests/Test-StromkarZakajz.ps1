param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
& "$PSScriptRoot/Test-XalatathFarthing.ps1" -Stromkar -Baseline:$Baseline -VerifyReleaseBinary:$VerifyReleaseBinary -AddressSanitizer:$AddressSanitizer
if(!$?){throw 'Stromkar Zakajz regression failed'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-zakajz"
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$?){throw 'Native Zakajz catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("OVERRIDE`t684`t0`t206455","EFFECT`t206455`t0`t42`t0`t35`t206456","EFFECT`t206456`t0`t61`t0`t49109","SPELL`t201904`tDark Drain","SPELL`t207454`tCosmetic Dissolve","CRITERION`t8`t29551`t92`t49108","CRITERION`t9`t29552`t92`t49109")){if(!$native.Contains($expected)){throw "Native finale data changed: $expected"}}
foreach($expected in @("SPELL`t130966`tPermanent Feign Death","EFFECT`t130966`t0`t6`t4", "EFFECT`t130966`t1`t6`t12", "EFFECT`t130966`t2`t6`t120", "POSE_VISUAL`t130966`t15933", "POSE_EVENT`t15933`t7`t8`t6872", "POSE_KIT_EFFECT`t4824`t6`t1007", "POSE_ANIMATION`t1007`t65535`t196`t0", "ZAKAJZ_MODEL`t67454`t3057`t250582", "ZAKAJZ_MODEL`t68556`t8861`t250582")){if(!$native.Contains($expected)){throw "Native defeat pose chain changed: $expected"}}
if($VerifyReleaseBinary){$priorPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$priorPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_defeat_pose_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Zakajz pose failed'}}finally{$env:PYTHONPATH=$priorPython}}
'PASS: native684 -> Execute206455/jump206456/49109 traced separately from preserved legacy Dark Drain201904 list; no completed client execution or departure claimed'
