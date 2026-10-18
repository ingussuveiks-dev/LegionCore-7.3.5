$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo 'build-extractors/runtime-test/vrekt-geometry'
New-Item -ItemType Directory -Force $output | Out-Null
$limits=@(Get-Content "$repo/src/server/game/Movement/PathGenerator.h" | Where-Object {$_ -match '^#define (MAX_PATH_LENGTH|MAX_POINT_PATH_LENGTH|SMOOTH_PATH_STEP_SIZE|SMOOTH_PATH_SLOP|VERTEX_SIZE)\s+'})
if($limits.Count -ne 5){throw 'Missing native path constants'}
[IO.File]::WriteAllLines("$output/NativePathLimits.inc",[string[]]$limits)
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Vrekt geometry query failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$points=@(Query @'
SELECT 'npc',guid,0,CAST(position_x AS DECIMAL(20,12)),CAST(position_y AS DECIMAL(20,12)),CAST(position_z AS DECIMAL(20,12)) FROM creature WHERE id IN(115846,116180,116600,116601,107830) AND map=1220 AND areaId=8362
UNION ALL SELECT 'wp',c.guid,w.point,CAST(w.position_x AS DECIMAL(20,12)),CAST(w.position_y AS DECIMAL(20,12)),CAST(w.position_z AS DECIMAL(20,12)) FROM creature c JOIN creature_addon a ON a.guid=c.guid JOIN waypoint_data w ON w.id=a.path_id WHERE c.id IN(115846,116180,116600,116601,107830) AND c.map=1220 AND c.areaId=8362;
'@)
if($points.Count -ne 100){throw 'Expected34 Vrekt anchors and66 patrol points'}
$points | Set-Content "$output/points.txt"
$movement=@(Query @'
SELECT 'templates',COUNT(*) FROM creature_template_movement WHERE CreatureId IN(115846,116180,116600,116601,107830,115885);
SELECT 'overrides',COUNT(*) FROM creature_movement_override o JOIN creature c ON c.guid=o.SpawnId WHERE c.id IN(115846,116180,116600,116601,107830);
SELECT 'patrols',COUNT(*) FROM creature c JOIN creature_addon a ON a.guid=c.guid WHERE c.id IN(115846,116180,116600,116601,107830) AND c.map=1220 AND c.areaId=8362 AND c.MovementType=2 AND a.path_id<>0;
'@)
if("templates`t0" -notin $movement -or "overrides`t0" -notin $movement -or "patrols`t10" -notin $movement){throw 'Movement inventory changed; re-audit filters/patrols'}
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
if($source -notmatch 'CreatureMovementData::CreatureMovementData\(\)\s*:\s*Ground\(CreatureGroundMovementType::Run\),\s*Flight\(CreatureFlightMovementType::None\),\s*Swim\(true\)'){throw 'Native movement defaults changed'}
function NativeBlock([string]$text,[string]$signature){
 $a=$text.IndexOf($signature);if($a -lt 0){throw "Missing native method $signature"};$b=$text.IndexOf('{',$a)+1;$depth=1
 while($depth){if($text[$b] -eq '{'){++$depth};if($text[$b] -eq '}'){--$depth};++$b};$text.Substring($a,$b-$a)
}
$pathSource=Get-Content "$repo/src/server/game/Movement/PathGenerator.cpp" -Raw
$methods=@('uint32 PathGenerator::FixupCorridor(','bool PathGenerator::GetSteerTarget(','dtStatus PathGenerator::FindSmoothPath(','bool PathGenerator::InRangeYZX(') | ForEach-Object {(NativeBlock $pathSource $_).Replace('PathGenerator::','NativePathSmoothing::')}
[IO.File]::WriteAllText("$output/VrektNativeSmoothing.inc",($methods -join "`n"))
$current=Get-Content "$repo/src/server/scripts/Legion/vrekt_scene.h" -Raw
$old=(& git -C $repo show '65611ab0:src/server/scripts/Legion/vrekt_scene.h') -join "`n"
if($LASTEXITCODE){throw 'Pinned old escape source unavailable'}
function Capture([string]$text,[string]$pattern){$m=[regex]::Match($text,$pattern);if(!$m.Success){throw "Missing flight/native parameter $pattern"};$m.Groups[1].Value}
function FlightOffset([string]$text,[string]$axis){
 $m=[regex]::Match($text,('home\.GetPosition'+$axis+'\(\)\s*([+-])\s*([0-9.]+)f'));if(!$m.Success){throw "Missing flight axis $axis"};$m.Groups[1].Value+$m.Groups[2].Value+'f'
}
$flight=@()
foreach($pair in @(@('Flight',$current),@('Old',$old))){
 $flight+="constexpr float $($pair[0])Rise="+(Capture $pair[1] 'me->GetPositionZ\(\)\s*\+\s*([0-9.]+)f')+'f;'
 foreach($axis in @('X','Y','Z')){$flight+="constexpr float $($pair[0])$axis="+(FlightOffset $pair[1] $axis)+';'}
}
$flight+='constexpr float FlightSpeed='+(Capture $current 'MoveSmoothFlyPath\(VrektScene::Escape, destination, ([0-9.]+)f\)')+'f;'
$flight+='constexpr int ReleaseDelay='+(Capture $current 'departure = ([1-9][0-9]*);')+';'
$flight+='constexpr int FlightTimeout='+(Capture $current 'timeout = ([1-9][0-9]*);')+';'
$flight+='constexpr float NativeAllowedPolyDist='+(Capture $pathSource 'float allowedPolyDist = ([0-9.]+)f;')+'f;'
$unitSource=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$baseFlight=Capture $unitSource '([0-9.]+)f,\s*// MOVE_FLIGHT\r?\n'
$rate=@(Query 'SELECT CAST(speed_fly AS DECIMAL(20,12)) FROM creature_template WHERE entry=115885;');if($rate.Count -ne 1){throw 'Missing drake flight speed'}
$flight+='constexpr float TakeoffSpeed='+$baseFlight+'f*'+$rate[0]+'f;'
[IO.File]::WriteAllLines("$output/VrektFlightSettings.inc",[string[]]$flight)
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision/Maps','src/common/Collision/Models','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='/I"'+$output+'"';$compilerArgs+='"'+$repo+'/tools/tests/VrektGeometry.cpp"';$compilerArgs+='/Fe"'+$output+'/VrektGeometry.exe"';$compilerArgs+='/Fo"'+$output+'/VrektGeometry.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build.cmd",$cmd);& "$output/build.cmd";if($LASTEXITCODE){throw 'Native Vrekt geometry build failed'}
$previousPath=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/VrektGeometry.exe" --self-test;if($LASTEXITCODE){throw 'Geometry status regression failed'}
 $rows=@(& "$output/VrektGeometry.exe" "$repo/build-extractors/bin/Release" "$output/points.txt")
 $result=$LASTEXITCODE;$rows | Set-Content "$output/native-lines.txt";$rows
 if($result){throw 'Vrekt geometry validation failed'}
}finally{$env:PATH=$previousPath}
$inputs=@("$output/points.txt","$output/VrektFlightSettings.inc","$repo/build-extractors/bin/Release/mmaps/1220.mmap","$repo/build-extractors/bin/Release/vmaps/1220.vmtree")
foreach($line in $rows){
 if($line -match '^TILE (\d+) (\d+)$'){
  $tx=[int]$Matches[1];$ty=[int]$Matches[2];$inputs+="$repo/build-extractors/bin/Release/maps/1220_${tx}_${ty}.map";$inputs+="$repo/build-extractors/bin/Release/vmaps/1220_${ty}_${tx}.vmtile"
  foreach($nx in ($tx-1)..($tx+1)){foreach($ny in ($ty-1)..($ty+1)){$path=Join-Path "$repo/build-extractors/bin/Release/mmaps" ('1220{0:D2}{1:D2}.mmtile' -f $nx,$ny);if(Test-Path $path){$inputs+=$path}}}
 }
 if($line -match '^MODEL (.+)$'){
  $vmRoot=[IO.Path]::GetFullPath("$repo/build-extractors/bin/Release/vmaps/");$path=[IO.Path]::GetFullPath((Join-Path $vmRoot $Matches[1]))
  if(!$path.StartsWith($vmRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Unexpected VMAP model path'};$inputs+=$path
 }
}
$hashes=@{};foreach($path in ($inputs | Select-Object -Unique)){$hashes[[IO.Path]::GetFileName($path)]=(Get-FileHash $path -Algorithm SHA256).Hash}
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"
'PASS: live anchors/patrols and native geometry hashes, including loaded VMAP models, recorded. This test changes no database coordinates or extracted data.'
