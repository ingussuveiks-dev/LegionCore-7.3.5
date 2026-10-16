$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-blightcaller"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimBlightcallerCatalog','StormheimStemTideGeometry')){
 $compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
 $compilerArgs+='"'+$PSScriptRoot+'/'+$test+'.cpp"';$compilerArgs+='/Fe"'+$out+'/'+$test+'.exe"';$compilerArgs+='/Fo"'+$out+'/'+$test+'.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/build.cmd",$cmd);& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $test build failed"}
}
$previousPath=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $rows=@(& "$out/StormheimBlightcallerCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE
 $rows | Set-Content "$out/native-lines.txt";$rows
 if($code){throw 'Blightcaller native DB2/faction reader failed'}
 $routes=,@('4343.36 2834.09 1.83185','4369.01 2792.6 1.25984')
 $geometry=@()
 foreach($route in $routes){
  $route | Set-Content "$out/ground.txt"
  $ground=@(& "$out/StormheimStemTideGeometry.exe" $runtime "$out/ground.txt" 8192);$groundExit=$LASTEXITCODE
  $ground | Set-Content "$out/geometry-ground-only.txt"
  if($groundExit -ne 6){throw 'Ground-only shore baseline changed; review native statuses'}
  $rows=@(& "$out/StormheimStemTideGeometry.exe" $runtime "$out/ground.txt" 8192 1220 walk-swim);$code=$LASTEXITCODE
  $geometry+=$rows;$rows
  $geometry | Set-Content "$out/geometry.txt"
  if($code){throw 'Shore Nathanos to paratrooper walking/swimming approach incomplete'}
 }
 $geometry | Set-Content "$out/geometry.txt"
}finally{$env:PATH=$previousPath}
