$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-salvation"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimSalvationCatalog','StormheimStemTideGeometry','StormheimSalvationGeometry')){
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
 $rows=& "$out/StormheimSalvationCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/"
 if($LASTEXITCODE){throw 'Salvation native DB2 reader failed'}
 $rows | Set-Content "$out/native-lines.txt";$rows
 @('npc 338656 2490.15 948.437 241.446','platform 341218 2488.31 945.134 596.224','npc 291417 2449.32 907.818 596.224','platform 0 2492.08 957.37 241.44','npc 338669 2497.89 956.938 241.529') | Set-Content "$out/anchors.txt"
 $rows=& "$out/StormheimSalvationGeometry.exe" "$repo/build-extractors/bin/Release" "$out/anchors.txt"
 if($LASTEXITCODE){throw 'Salvation native floor anchors failed'}
 $rows | Set-Content "$out/floors.txt";$rows
 $routes=@(@('2488.31 945.13 596.224','2449.32 907.818 596.224'),@('2492.08 957.37 241.44','2497.89 956.938 241.529'))
 $geometry=@();$routeIndex=0
 foreach($route in $routes){
  ++$routeIndex
  $route | Set-Content "$out/ground.txt"
  $rows=& "$out/StormheimStemTideGeometry.exe" "$repo/build-extractors/bin/Release" "$out/ground.txt" 8192
  $geometry+= "route$routeIndex $($route -join ' -> ')";$geometry+= $rows;$rows
  if($LASTEXITCODE){throw 'Salvation ground route incomplete; inspect Detour status before claiming disconnected terrain'}
 }
 $geometry | Set-Content "$out/geometry.txt"
}finally{$env:PATH=$previousPath}
$hashes=@{}
foreach($name in @('Spell','SpellEffect','CreatureDisplayInfo','CreatureModelData','QuestLineXQuest','FactionTemplate','Vehicle','VehicleSeat')){$hashes[$name]=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/$name.db2" -Algorithm SHA256).Hash}
$hashes | ConvertTo-Json | Set-Content "$out/native-inputs.json"
