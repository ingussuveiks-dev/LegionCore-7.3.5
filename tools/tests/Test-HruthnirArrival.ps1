param([switch]$Baseline,[switch]$OfferBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $OfferBaseline) -and $VerifyReleaseBinary){throw 'Arrival baseline cannot certify installed code'}
if($Baseline -and $OfferBaseline){throw 'Select only one arrival baseline'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-arrival"
New-Item -ItemType Directory -Force $out|Out-Null
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/HruthnirArrivalGeometry.cpp -ProbeName HruthnirArrivalGeometry -OutputFile "$out/geometry.txt"
if($LASTEXITCODE){throw 'Actual Position geometry failed'}
$geometry=Get-Content "$out/geometry.txt"
if($geometry.Count -ne 1 -or $geometry -notmatch '^PASS native Position:58 actual3D'){throw 'Native Position evidence missing'}
$geometry
if($Baseline){& "$PSScriptRoot/Test-HruthnirGossip.ps1" -ArrivalBaseline}
elseif($OfferBaseline){& "$PSScriptRoot/Test-HruthnirGossip.ps1" -OfferArrivalBaseline}
else{& "$PSScriptRoot/Test-HruthnirGossip.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary}
if(!$? -or $LASTEXITCODE){throw 'Extracted Hruthnir arrival/queue failed'}
