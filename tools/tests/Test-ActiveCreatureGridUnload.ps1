param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/active-creature-grid-unload"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $path='src/server/game/Maps/Map.cpp';$current=Get-Content "$repo/$path" -Raw -Encoding UTF8
 $pinned=(& git -C $repo show "0e238cb1:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned map baseline unavailable'}
 $markers=@('void Map::AddToActive(Creature*','void Map::RemoveFromActive(Creature*','void Map::UnloadAll(')
 $restored=$current
 foreach($marker in $markers){$restored=$restored.Replace((Block $current $marker),(Block $pinned $marker))}
 if(($restored -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unreviewed Map change outside active lifecycle'}
 $selected=if($Baseline){$pinned}else{$current}
 [IO.File]::WriteAllText("$out/MapActive.inc",(($markers|ForEach-Object {Block $selected $_}) -join "`n"))
 [IO.File]::WriteAllText("$out/SetActive.inc",(Block (Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw -Encoding UTF8) 'void WorldObject::setActive('))
 [IO.File]::WriteAllText("$out/RespawnPosition.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw -Encoding UTF8) 'void Creature::GetRespawnPosition('))
 $header=Get-Content "$repo/src/server/game/Maps/Map.h" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/ActiveHelpers.inc",'template<class T> '+(Block $header 'void AddToActiveHelper(')+'template<class T> '+(Block $header 'void RemoveFromActiveHelper('))
 $grid=Get-Content "$repo/src/server/game/Grids/GridDefines.h" -Raw -Encoding UTF8
 $constants=($grid -split "`n"|Where-Object {$_ -match '^#define (MAX_NUMBER_OF_GRIDS|SIZE_OF_GRIDS|CENTER_GRID_ID|CENTER_GRID_OFFSET)\s'}) -join "`n"
 [IO.File]::WriteAllText("$out/GridCoordinates.inc",$constants+"`nnamespace Trinity {"+(Block $grid 'template<class RET_TYPE, int CENTER_VAL>')+(Block $grid 'inline GridCoord ComputeGridCoord(')+'}')
 $grid=Get-Content "$repo/src/server/game/Grids/GridInfo.cpp" -Raw -Encoding UTF8
 $ngrid=Get-Content "$repo/src/server/game/Grids/NGrid.cpp" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/GridLocks.inc",(($(@('bool GridInfo::getUnloadLock(','void GridInfo::incUnloadActiveLock(','void GridInfo::decUnloadActiveLock(')|ForEach-Object {Block $grid $_})+ $(@('void NGrid::incUnloadActiveLock(','void NGrid::decUnloadActiveLock(')|ForEach-Object {Block $ngrid $_})) -join "`n"))
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Encoding UTF8|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'SPAWN',guid,id,map,position_x,position_y FROM creature WHERE id=106570;SELECT 'PATH',entry,pointid,position_x,position_y FROM waypoints WHERE entry=106570 AND pointid=8;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Read-only Grif spawn/path check failed'}
 }finally{$env:MYSQL_PWD=$previous}
 if(Compare-Object @("SPAWN`t11078574`t106570`t1579`t2340.01`t2572.9","PATH`t106570`t8`t2763.35`t2552.36") $rows){throw 'Native Grif path fixture needs review against changed live data'}
 [IO.File]::WriteAllText("$out/GrifPositions.inc",'constexpr float spawnX=2340.01f,spawnY=2572.9f,destinationX=2763.35f,destinationY=2552.36f;')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/ActiveCreatureGridUnloadTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native active-grid unload regression failed'}
 if($VerifyReleaseBinary){
  $previous=$env:PYTHONPATH;$previousErrors=$ErrorActionPreference
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";$ErrorActionPreference='Continue';& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_active_creature_grid_unload_release.py" $ReleaseDirectory *> "$out/pe-check.log";$code=$LASTEXITCODE;$ErrorActionPreference=$previousErrors;Get-Content "$out/pe-check.log" -Encoding UTF8;if($code){throw 'Linked map cleanup implementation missing'}}finally{$env:PYTHONPATH=$previous;$ErrorActionPreference=$previousErrors}
 }
}
