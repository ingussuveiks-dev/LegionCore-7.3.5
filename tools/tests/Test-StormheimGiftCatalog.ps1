$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimGiftData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-gift"
$native=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
$a=$native.IndexOf('bool WorldObject::_IsWithinDist(');$b=$native.IndexOf('bool WorldObject::IsWithinLOSInMap(',$a)
[IO.File]::WriteAllText("$out/ShieldNativeRange.inc",$native.Substring($a,$b-$a).Replace('WorldObject','RangeObject'))
$objects=Get-Content "$repo/src/server/game/Entities/Object/ObjectDefines.h" -Raw;$units=Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw
$size=[regex]::Match($objects,'#define DEFAULT_WORLD_OBJECT_SIZE\s+([\d.]+f)').Groups[1].Value
$distance=[regex]::Match($objects,'#define INTERACTION_DISTANCE\s+([\d.]+f)').Groups[1].Value
$reach=[regex]::Match($units,'#define DEFAULT_COMBAT_REACH\s+([\d.]+f)').Groups[1].Value
if(!$size -or !$distance -or !$reach){throw 'Missing native interaction constants'}
[IO.File]::WriteAllText("$out/ShieldReach.inc","constexpr float ShieldObjectSize=$size,ShieldInteractionDistance=$distance,ShieldPlayerReach=$reach;")
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimGiftCatalog','StormheimGiftApproach')){
 $compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'});$compilerArgs+='/I"'+$out+'"'
 $compilerArgs+='"'+$PSScriptRoot+'/'+$test+'.cpp"';$compilerArgs+='/Fe"'+$out+'/'+$test+'.exe"';$compilerArgs+='/Fo"'+$out+'/'+$test+'.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/build.cmd",$cmd);& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $test build failed"}
}
$previousPath=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $rows=@(& "$out/StormheimGiftCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE
 $rows | Set-Content "$out/native-lines.txt";$rows
 if($code){throw "Native Gift catalog failed ($code)"}
 $routes=@(@('2370.99 1838.8 30.6481','2427.45 1653.27 -2.90057'),@('2427.45 1653.27 -2.90057','2396.54 1637.76 -5.11749'),@('2396.54 1637.76 -5.11749','2402.16 1659.65 -4.54072'),@('2402.16 1659.65 -4.54072','2370.99 1838.8 30.6481'),@('2370.99 1838.8 30.6481','2612.2 1740.14 16.6921'))
 $all=@();foreach($route in $routes){
  $route | Set-Content "$out/route.txt"
  $rows=@(& "$out/StormheimGiftApproach.exe" $runtime "$out/route.txt" 65535 1220 walk-swim);$code=$LASTEXITCODE
  $all+=('route '+($route -join ' -> '));$all+=$rows;$all | Set-Content "$out/surface-geometry.txt";$rows
  if($code){throw 'Gift native surface interaction corridor failed; retain diagnostics'}
 }
}finally{$env:PATH=$previousPath}
