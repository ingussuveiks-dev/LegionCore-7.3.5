param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Select baseline without repaired Release verification'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -HelyaBaseline:$Baseline
if(!$? -or $LASTEXITCODE){throw 'Helya introduction regression failed'}
if($VerifyReleaseBinary){
 $out="$repo/build-extractors/runtime-test/warswords-helya-channel"
 New-Item -ItemType Directory -Force $out|Out-Null
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Helya catalogue failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($expected in @("CAST`t195062`t1`t0`t0`t0`t0","DURATION`t195062`t165`t7000`t7000`t0","EFFECT`t195062`t0`t6`t4`t0`t0`t25`t0","DURATION`t205424`t21`t-1`t-1`t0","EFFECT`t205424`t0`t6`t4`t0`t0`t1`t0")){if($native -notcontains $expected){throw "Native Helya aura/cast data changed: $expected"}}
 & "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -VerifyReleaseBinary
 if(!$? -or $LASTEXITCODE){throw 'Native binding and canonical intro regression failed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_helya_channel_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Helya channel guard missing'}}finally{$env:PYTHONPATH=$previous}
 & "$PSScriptRoot/Test-WarswordsHelyaTargetNative.ps1"
 if(!$? -or $LASTEXITCODE){throw 'Actual native Helya recipient lifecycle failed'}
}
