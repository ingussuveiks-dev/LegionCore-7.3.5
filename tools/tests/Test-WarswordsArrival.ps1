param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Choose baseline without current Release verification'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-arrival"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "9fe2be7b:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned arrival baseline'}}
foreach($name in @('Initialize','OnPlayerEnter','Update')){[IO.File]::WriteAllText("$out/$name.inc",(Block $source "void $name("))}
[IO.File]::WriteAllText("$out/Policy.inc",$(if($Baseline){''}else{Block $source 'namespace WarswordsArrival'}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/WarswordsArrivalTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Warswords arrival callback regression failed'}
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
