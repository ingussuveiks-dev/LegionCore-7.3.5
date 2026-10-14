$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-will"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/StormheimWillGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/WillGeometry.exe"'
$compilerArgs+='/Fo"'+$output+'/WillGeometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-geometry.cmd",$cmd)
& "$output/build-geometry.cmd"
if($LASTEXITCODE){throw 'Valor geometry probe build failed'}



$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previousPassword=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=& C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute="SELECT 'object',guid,position_x,position_y,position_z FROM gameobject WHERE guid=114812 UNION ALL SELECT 'console',96139,3333.15,2209.16,315.999 UNION ALL SELECT 'npc',96129,3328.56,2197.37,312.742 UNION ALL SELECT 'npc',96129,3338.89,2196.61,313.129 UNION ALL SELECT 'npc',108263,3332.81,2197.72,312.897 UNION ALL SELECT 'record',id,target_position_x,target_position_y,target_position_z FROM spell_target_position WHERE id IN(190707,190708) UNION ALL SELECT 'light',guid,position_x,position_y,position_z FROM creature WHERE guid=146728274"
 if($LASTEXITCODE){throw 'Trial location export failed'}
 $rows | Set-Content "$output/locations.txt"
} finally {$env:MYSQL_PWD=$previousPassword}
$previousPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH"; & "$output/WillGeometry.exe" "$repo/build-extractors/bin/Release" "$output/locations.txt";if($LASTEXITCODE){throw 'Will static archive geometry failed'}}finally{$env:PATH=$previousPath}
