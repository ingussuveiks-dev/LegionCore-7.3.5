param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-WarswordsPickupAdmission.ps1" -DepartureBaseline
if(!$? -or $LASTEXITCODE){throw 'Pinned stale departure baseline failed'}
& "$PSScriptRoot/Test-WarswordsPickupAdmission.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$? -or $LASTEXITCODE){throw 'Current return admission failed'}
if($VerifyReleaseBinary){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $out="$repo/build-extractors/runtime-test/warswords-return-admission"
 New-Item -ItemType Directory -Force $out|Out-Null
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native return completion catalogue failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $kinds=@(Get-Content "$out/native.tsv"|Where-Object {$_ -like "STEP_KIND`t*"})
 if($kinds.Count -ne 7 -or @($kinds|Where-Object {$_ -notmatch '\t0$'}).Count -or $kinds -notcontains "STEP_KIND`t6`t1914`t0"){throw 'Native944 nonbonus completion threshold changed'}
 if((Get-Content "$out/native.tsv") -notcontains "STEP_REWARD`t6`t1914`t0"){throw 'Native944 final step has an automatic quest reward requiring review'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_return_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical return admission missing'}}finally{$env:PYTHONPATH=$previous}
}
