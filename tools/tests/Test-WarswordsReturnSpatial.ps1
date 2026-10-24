param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-return-spatial"
New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native return map catalogue failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@("SELECT 'weapon',guid,map,zoneId,areaId,position_x,position_y,position_z FROM gameobject WHERE id=248832;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Native return anchor read failed'}}finally{$env:MYSQL_PWD=$previous}
if($rows.Count -ne 1 -or $rows[0] -cne "weapon`t134150`t1511`t7541`t7830`t3515.24`t1800.51`t0.538696"){throw 'Native return anchor changed'}
[IO.File]::WriteAllLines("$out/world.tsv",[string[]]$rows)
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/DataStores/DB2Stores.cpp" -Raw
foreach($entry in @(@('Lookup','WorldMapAreaEntry const* DB2Manager::GetWorldMapArea('),@('ToWorld','void DB2Manager::Zone2MapCoordinates('),@('ToZone','void DB2Manager::Map2ZoneCoordinates('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/WarswordsReturnSpatialTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/native.tsv`" `"$out/world.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native return map conversion failed'}
& "$PSScriptRoot/Test-OdynGazeVisual.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$? -or $LASTEXITCODE){throw 'Existing shared Warrior beam native visual regression failed'}
