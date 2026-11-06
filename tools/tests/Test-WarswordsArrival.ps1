param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$LoadingBaseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -or $LoadingBaseline){if($VerifyReleaseBinary){throw 'A pinned counterfactual cannot certify current Release'};& "$PSScriptRoot/Test-WarswordsArrivalLegacy.ps1" -Baseline:$Baseline -LoadingBaseline:$LoadingBaseline;return}
& "$PSScriptRoot/Test-WarswordsArrivalRecovery.ps1"
if(!$? -or $LASTEXITCODE){throw 'Fury arrival recovery failed'}
$out="$repo/build-extractors/runtime-test/warswords-arrival"
New-Item -ItemType Directory -Force $out|Out-Null
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native arrival catalogue failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($spell in @(205534,189695,205744)){if(!@($native|Where-Object {$_ -like "SPELL`t$spell`t*"}).Count){throw "Missing native arrival spell $spell"}}
 if($native -notcontains "STEP_KIND`t0`t1888`t0"){throw 'Native arrival scenario step changed'}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$bindings=@("SELECT map,script FROM instance_template WHERE map=1511;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Native arrival binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/binding.tsv",[string[]]$bindings)
 if($bindings.Count -ne 1 -or $bindings[0] -cne "1511`tinstance_warswords"){throw "Shared1511 instance binding changed: rows=$($bindings.Count), values=$([string]::Join('|',[string[]]$bindings))"}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_arrival_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical arrival admission missing'}}finally{$env:PYTHONPATH=$previous}
}
