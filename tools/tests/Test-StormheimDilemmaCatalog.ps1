$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimDilemmaData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-dilemma"
$native=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
$a=$native.IndexOf('bool WorldObject::_IsWithinDist(');$b=$native.IndexOf('bool WorldObject::IsWithinLOSInMap(',$a)
[IO.File]::WriteAllText("$out/ShieldNativeRange.inc",$native.Substring($a,$b-$a).Replace('WorldObject','RangeObject'))
$objects=Get-Content "$repo/src/server/game/Entities/Object/ObjectDefines.h" -Raw;$units=Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw
$size=[regex]::Match($objects,'#define DEFAULT_WORLD_OBJECT_SIZE\s+([\d.]+f)').Groups[1].Value
$distance=[regex]::Match($objects,'#define INTERACTION_DISTANCE\s+([\d.]+f)').Groups[1].Value
$reach=[regex]::Match($units,'#define DEFAULT_COMBAT_REACH\s+([\d.]+f)').Groups[1].Value
if(!$size -or !$distance -or !$reach){throw 'Missing native reach constants'}
[IO.File]::WriteAllText("$out/ShieldReach.inc","constexpr float ShieldObjectSize=$size,ShieldInteractionDistance=$distance,ShieldPlayerReach=$reach;")
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$r.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimDilemmaCatalog','StormheimGiftApproach')){
 $args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $args+=($includes | ForEach-Object {'/I"'+$_+'"'});$args+='/I"'+$out+'"';$args+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$args+='/Fe"'+$out+'/'+$name+'.exe"';$args+='/Fo"'+$out+'/'+$name+'.obj"';$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $name compilation failed"}
}
$previous=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $rows=@(& "$out/StormheimDilemmaCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE;$rows | Set-Content "$out/native.tsv";$rows
 if($code){throw 'Native Dilemma data failed'}
 $routes=@();foreach($line in (Get-Content "$out/world.tsv" | Where-Object {$_ -like "spawn`t*"})){$p=$line.Split("`t");if($p[2] -ne '93603'){$routes+=,@('2370.99 1838.8 30.6481',($p[4..6] -join ' '))}}
 if($routes.Count -ne 34){throw 'Missing native opponent approach targets'}
 $all=@();foreach($route in $routes){
  $route | Set-Content "$out/route.txt";$rows=@(& "$out/StormheimGiftApproach.exe" $runtime "$out/route.txt" 65535 1220 walk-swim);$code=$LASTEXITCODE
  $all+=('route '+($route -join ' -> '));$all+=$rows;$all | Set-Content "$out/geometry.txt";$rows
  if($code){throw 'Dilemma opponent approach failed; retain raw diagnostics'}
 }
}finally{$env:PATH=$previous}
