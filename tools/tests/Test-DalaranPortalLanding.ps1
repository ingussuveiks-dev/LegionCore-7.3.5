param()
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/dalaran-portal-landing"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$args+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$args+='"'+$repo+'/tools/tests/DalaranPortalLandingTest.cpp"'
$args+='/Fe"'+$output+'/DalaranPortalLandingTest.exe"'
$args+='/Fo"'+$output+'/DalaranPortalLandingTest.obj"'
$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-item.cmd",$cmd)
& "$output/build-item.cmd"
if($LASTEXITCODE){throw 'Dalaran collision test build failed'}

# Apply twice to a connection-local copy, preserving the live server data.
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$migration=[IO.File]::ReadAllText("$repo/sql/updates/world/2026_10_12_11_dalaran_portal_landing.sql")
$sql=@"
CREATE TEMPORARY TABLE landing_copy AS SELECT * FROM spell_target_position WHERE id IN (228327,230156);
CREATE TEMPORARY TABLE spell_target_position LIKE landing_copy;
INSERT INTO spell_target_position SELECT * FROM landing_copy;
$migration
$migration
SELECT target_map,target_position_x,target_position_y,target_position_z,target_orientation FROM spell_target_position WHERE id=228327;
SELECT COUNT(*) FROM spell_target_position a JOIN landing_copy b USING(id) WHERE a.id=230156 AND a.target_map=b.target_map AND a.target_position_x=b.target_position_x AND a.target_position_y=b.target_position_y AND a.target_position_z=b.target_position_z AND a.target_orientation=b.target_orientation;
SELECT displayId,size FROM gameobject_template WHERE entry=246854;
SELECT position_x,position_y,position_z,orientation,PhaseId FROM gameobject WHERE guid=129023 AND id=246854;
"@
$previousPassword=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Isolated landing SQL test failed'}
} finally {$env:MYSQL_PWD=$previousPassword}
if($rows.Count -ne 4){throw 'Missing portal/city data'}
$point=$rows[0].Split([char]9)
if($point[0] -ne '0' -or $rows[1] -ne '1'){throw 'Wrong map or changed finale'}
$display=$rows[2].Split([char]9)
$city=$rows[3].Split([char]9)
if($display[0] -ne '35175' -or [double]$display[1] -ne 1 -or [double]$city[0] -ne -11088.3 -or [double]$city[1] -ne -1993.4 -or [math]::Abs([double]$city[2]-741.034) -gt 0.001 -or [math]::Abs([double]$city[3]-5.60251) -gt 0.00001 -or $city[4] -ne '7714 7552'){throw 'City transform changed; update the collision fixture from verified data'}
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/DalaranPortalLandingTest.exe" "$repo/build-extractors/bin/Release/" $point[1] $point[2] $point[3]
 if($LASTEXITCODE){throw 'Dalaran landing has no safe collision surface'}
} finally {$env:PATH=$previousPath}
'PASS: idempotent isolated portal migration; cinematic destination preserved.'
