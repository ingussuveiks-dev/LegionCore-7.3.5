$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/grey-shoals-geometry"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$limits=@(Get-Content "$repo/src/server/game/Movement/PathGenerator.h" | Where-Object {$_ -match '^#define MAX_(PATH_LENGTH|POINT_PATH_LENGTH)\s+\d+'})
if($limits.Count -ne 2){throw 'Missing native path buffer limits'}
[IO.File]::WriteAllLines("$output/NativePathLimits.inc",[string[]]$limits)
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='/I"'+$output+'"';$compilerArgs+='"'+$repo+'/tools/tests/GreyShoalsGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/GreyShoalsGeometry.exe"';$compilerArgs+='/Fo"'+$output+'/GreyShoalsGeometry.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Grey Shoals native geometry build failed'}
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Grey Shoals geometry SQL query failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows
}
$points=@(Query @'
SELECT 'npc',guid,position_x,position_y,position_z FROM creature WHERE id IN(115025,115026,115031,115751) AND map=1220 AND areaId=8297
UNION ALL SELECT 'rack',guid,position_x,position_y,position_z FROM gameobject WHERE guid BETWEEN 147040012 AND 147040015 AND id=266170 AND map=1220 AND areaId=8297;
'@)
if($points.Count -ne 24){throw 'Expected all20 actors and4 native racks'}
$points | Set-Content "$output/points.txt"
# Ground-only searches are insufficient for these shore actors: the templates
# inherit native Run/Swim(true). Refuse to silently reuse that assumption if DB
# movement overrides or the production defaults change.
$movement=@(Query @'
SELECT 'templates',COUNT(*) FROM creature_template_movement WHERE CreatureId IN(115025,115026,115031,115751);
SELECT 'overrides',COUNT(*) FROM creature_movement_override o JOIN creature c ON c.guid=o.SpawnId WHERE c.id IN(115025,115026,115031,115751);
'@)
if('templates'+"`t0" -notin $movement -or 'overrides'+"`t0" -notin $movement){throw 'Movement overrides changed; recompute actual walking/swimming filters'}
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
if($source -notmatch 'CreatureMovementData::CreatureMovementData\(\)\s*:\s*Ground\(CreatureGroundMovementType::Run\),\s*Flight\(CreatureFlightMovementType::None\),\s*Swim\(true\)'){throw 'Native default movement changed; re-audit capability filters'}
$previousPath=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/GreyShoalsGeometry.exe" --self-test
 if($LASTEXITCODE){throw 'Geometry query-status regression failed'}
 $rows=& "$output/GreyShoalsGeometry.exe" "$repo/build-extractors/bin/Release" "$output/points.txt"
 if($LASTEXITCODE){throw 'Grey Shoals native geometry failed'}
 $rows | Set-Content "$output/native-lines.txt";$rows
}finally{$env:PATH=$previousPath}
$inputs=@("$output/points.txt","$repo/build-extractors/bin/Release/mmaps/1220.mmap","$repo/build-extractors/bin/Release/vmaps/1220.vmtree")
foreach($tx in 30,31){
 $inputs+="$repo/build-extractors/bin/Release/maps/1220_${tx}_17.map"
 # VMAP filenames reverse tile axes; native loading above uses world tx,ty.
 $inputs+="$repo/build-extractors/bin/Release/vmaps/1220_17_${tx}.vmtile"
 foreach($nx in ($tx-1)..($tx+1)){foreach($ny in 16..18){$path="$repo/build-extractors/bin/Release/mmaps/1220$nx$ny.mmtile";if(Test-Path $path){$inputs+=$path}}}
}
$hashes=@{};foreach($path in ($inputs | Select-Object -Unique)){$hashes[[IO.Path]::GetFileName($path)]=(Get-FileHash $path -Algorithm SHA256).Hash}
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"
'PASS: read-only live position/movement inventory and canonical geometry input hashes recorded; no spawn coordinates or extracted data changed.'
