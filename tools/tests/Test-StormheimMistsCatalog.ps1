$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimMistsData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-mists"
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
foreach($name in @('StormheimMistsCatalog','StormheimGiftApproach')){
 $compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'});$compilerArgs+='/I"'+$out+'"';$compilerArgs+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$compilerArgs+='/Fe"'+$out+'/'+$name+'.exe"';$compilerArgs+='/Fo"'+$out+'/'+$name+'.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $name compilation failed"}
}
$previous=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $rows=@(& "$out/StormheimMistsCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE;$rows | Set-Content "$out/native.tsv";$rows
 if($code){throw 'Native Mists data failed'}
 $spawns=@(Get-Content "$out/world.tsv" | Where-Object {$_ -like "spawn`t*"} | ForEach-Object {,$_.Split("`t")})
 $all=@();$count=0
 foreach($start in $spawns){foreach($end in $spawns){if($start[1] -eq $end[1]){continue}
  @(($start[4..6] -join ' '),($end[4..6] -join ' ')) | Set-Content "$out/route.txt"
  $rows=@(& "$out/StormheimGiftApproach.exe" $runtime "$out/route.txt" 65535 1220 walk-swim);$code=$LASTEXITCODE
  $all+="route $($start[1]) -> $($end[1])";$all+=$rows
  [IO.File]::WriteAllLines("$out/geometry.txt",[string[]]$all)
  if($code -or !($rows -match 'staticLOS=1') -or !($rows -match '^PASS: Source approach')){throw 'Mists directed corridor/endpoint check failed; retain geometry.txt'}
  $count++
 }}
 if($count -ne 20){throw 'Missing Mists directed routes'}
 'PASS:20 directed native walk/swim corridors between Havi and all four enemies, complete endpoints and static endpoint LOS; full body/dynamic/client movement remains unverified.'
}finally{$env:PATH=$previous}
