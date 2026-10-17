$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimShieldsData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-shields"
$native=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
$a=$native.IndexOf('bool WorldObject::_IsWithinDist(');$b=$native.IndexOf('bool WorldObject::IsWithinLOSInMap(',$a)
[IO.File]::WriteAllText("$out/ShieldNativeRange.inc",$native.Substring($a,$b-$a).Replace('WorldObject','RangeObject'))
$objects=Get-Content "$repo/src/server/game/Entities/Object/ObjectDefines.h" -Raw;$units=Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw
$size=[regex]::Match($objects,'#define DEFAULT_WORLD_OBJECT_SIZE\s+([\d.]+f)').Groups[1].Value
$distance=[regex]::Match($objects,'#define INTERACTION_DISTANCE\s+([\d.]+f)').Groups[1].Value
$reach=[regex]::Match($units,'#define DEFAULT_COMBAT_REACH\s+([\d.]+f)').Groups[1].Value
if(!$size -or !$distance -or !$reach){throw 'Native interaction constants missing'}
[IO.File]::WriteAllText("$out/ShieldReach.inc","constexpr float ShieldObjectSize=$size,ShieldInteractionDistance=$distance,ShieldPlayerReach=$reach;")
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimShieldsCatalog','StormheimShieldApproach','StormheimHuntingFloor')){
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
 $rows=@(& "$out/StormheimShieldsCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE
 $rows | Set-Content "$out/native-lines.txt";$rows
 if($code){throw 'Native shields catalog failed'}
 $groups=@(
 @('npc 293461 2788.86 1345.5 67.6698','npc 338652 2659.46 1517.76 30.3969','object 115401 2712.52 1641.37 18.7454','object 115389 2784.25 1476.16 24.8708','object 115385 2675.02 1356.35 47.5821'),
 @('object 115377 2658.06 1427.62 32.433','object 115394 2853.91 1523.14 13.0343','object 115380 2728.16 1465.83 26.0828','npc 293461 2788.86 1345.5 67.6698','npc 338652 2659.46 1517.76 30.3969'))
 $floors=@();foreach($anchors in $groups){$anchors | Set-Content "$out/anchors.txt";$rows=@(& "$out/StormheimHuntingFloor.exe" $runtime "$out/anchors.txt");$code=$LASTEXITCODE;$floors+=$rows;$rows;$floors | Set-Content "$out/floors.txt";if($code){throw 'Shield/giver floor check failed'}}
 $all=@()
 foreach($target in @('2712.52 1641.37 18.7454','2784.25 1476.16 24.8708','2675.02 1356.35 47.5821','2658.06 1427.62 32.433','2853.91 1523.14 13.0343','2728.16 1465.83 26.0828','2659.46 1517.76 30.3969')){
  @('2788.86 1345.5 67.6698',$target) | Set-Content "$out/ground.txt"
  $rows=@(& "$out/StormheimShieldApproach.exe" $runtime "$out/ground.txt" 65535 1220 walk-swim);$code=$LASTEXITCODE
  $all+="target $target";$all+=$rows;$rows;$all | Set-Content "$out/geometry.txt"
  if($code){throw 'Shield/giver ground approach incomplete; keep diagnostics'}
 }
}finally{$env:PATH=$previousPath}
