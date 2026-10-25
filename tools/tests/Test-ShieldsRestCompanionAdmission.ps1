param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/shields-rest-companion-admission"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "dd021ddb:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned source missing'}}
[IO.File]::WriteAllText("$out/Step.inc",(Block $source 'void onScenarionNextStep('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/ShieldsRestCompanionAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Shared companion admission host failed'}
if($VerifyReleaseBinary){
 if($source.Contains('scenarioid') -or $source.Contains('OnPlyerEnter')){throw 'Stale unregistered admission cache remains'}
 $native=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
 if(([regex]::Matches($native,'script->setScenarioStep\(currentStep\);\s*script->onScenarionNextStep\(currentStep\);')).Count -ne 2){throw 'Native current-step dispatch changed'}
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TruthguardPickupCatalog.cpp -ProbeName TruthguardPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if($LASTEXITCODE){throw 'Native shared steps/companion effects failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $rows=Get-Content "$out/native.tsv"
 foreach($identity in @(909,1068,1082)){if(@($rows|Where-Object {$_ -like "SHARED_STEP`t$identity`t1`t*"}).Count -ne 1){throw "Missing native shared step1: $identity"}}
 foreach($pair in @(@(210036,209945),@(210043,209953))){if($rows -notcontains "EFFECT`t$($pair[0])`t0`t6`t428`t1`t$($pair[1])`t1`t0"){throw "Native summon-controller changed: $pair"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SELECT ScenarioID,MapID,Class FROM scenario_data WHERE MapID=1495 ORDER BY ScenarioID;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Shared scenario mapping read failed'}}finally{$env:MYSQL_PWD=$previous}
 if(Compare-Object @("909`t1495`t1","1068`t1495`t3","1082`t1495`t2") $live){throw 'Shared mapping changed; review new routes'}
 [IO.File]::WriteAllLines("$out/live.tsv",[string[]]$live)
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_shields_rest_companion_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed companion admission failed'}}finally{$env:PYTHONPATH=$previous}
}
