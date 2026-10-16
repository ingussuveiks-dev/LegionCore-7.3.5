$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-stem-tide"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimStemTideCatalog','StormheimStemTideGeometry')){
 $compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
 $compilerArgs+='"'+$PSScriptRoot+'/'+$test+'.cpp"';$compilerArgs+='/Fe"'+$out+'/'+$test+'.exe"';$compilerArgs+='/Fo"'+$out+'/'+$test+'.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/build.cmd",$cmd);& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $test build failed"}
}
$previousPath=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$out/StormheimStemTideGeometry.exe" --self-test
 if($LASTEXITCODE){throw 'Geometry status regression failed'}
 $rows=& "$out/StormheimStemTideCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/"
 if($LASTEXITCODE){throw 'Stem the Tide native DB2 reader failed'}
 $rows | Set-Content "$out/native-lines.txt";$rows
 $routes=@(@('2875.26 1208.98 188.763','2713.4 970.118 205.88'),@('2713.4 970.118 205.88','2601.57 981.316 216.77'),@('2601.57 981.316 216.77','2467.32 1060.44 216.77'),@('2467.32 1060.44 216.77','2490.15 948.437 241.446'))
 $geometry=@();$routeIndex=0
 $routes[0] | Set-Content "$out/ground.txt"
 $reproduction=& "$out/StormheimStemTideGeometry.exe" "$repo/build-extractors/bin/Release" "$out/ground.txt" 1024
 if($LASTEXITCODE -ne 7 -or !($reproduction -match 'status=0x40000060.*out_of_nodes=1 partial=1')){throw 'Original route must reproduce node exhaustion, not disconnected terrain'}
 $reproduction | Set-Content "$out/geometry-original-budget.txt";$reproduction
 foreach($route in $routes){
  ++$routeIndex
  $route | Set-Content "$out/ground.txt"
  $rows=& "$out/StormheimStemTideGeometry.exe" "$repo/build-extractors/bin/Release" "$out/ground.txt" 8192
  $geometry+= "route$routeIndex $($route -join ' -> ')";$geometry+= $rows;$rows
  if($LASTEXITCODE){throw 'Stem ground route incomplete; inspect Detour status before claiming disconnected terrain'}
 }
 $geometry | Set-Content "$out/geometry.txt"
}finally{$env:PATH=$previousPath}
$hashes=@{}
foreach($name in @('ItemSparse','ItemEffect','Spell','SpellEffect','CreatureDisplayInfo','CreatureModelData','QuestLineXQuest','GameObjectDisplayInfo','SpellMisc','SpellCastTimes','SpellRange')){$hashes[$name]=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/$name.db2" -Algorithm SHA256).Hash}
$hashes | ConvertTo-Json | Set-Content "$out/native-inputs.json"
