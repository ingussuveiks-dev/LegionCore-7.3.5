param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$Migration
)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/SouthshoreGuardGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/GuardGeometry.exe"'
$compilerArgs+='/Fo"'+$output+'/GuardGeometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-geometry.cmd",$cmd)
& "$output/build-geometry.cmd"
if($LASTEXITCODE){throw 'Southshore guard geometry probe build failed'}



if (!$Migration) {
    $Migration = "$repo/sql/updates/world/2026_11_25_03_southshore_brawl_guards.sql"
}
$source = Get-Content "$repo/src/server/game/Battlegrounds/Zones/BrawlBattlegroundSouthshoreVsTarrenMill.cpp" -Raw
$spawns = [regex]::Matches($source, '\{845(?:57|93), TEAM_(?:ALLIANCE|HORDE), \{([\d.]+)f, ([\d.]+)f, ([\d.]+)f, [\d.]+f\}, (?:0|OrchardPatrolPath)\}')
if ($spawns.Count -ne 4) { throw 'Expected four guard placements' }
$labels = 'Well', 'OppositeRoad', 'OrchardInside', 'PatrolSpawn'
$points = for ($i = 0; $i -lt $spawns.Count; ++$i) {
    $parts = $spawns[$i].Groups
    "$($labels[$i]) $($parts[1].Value) $($parts[2].Value) $($parts[3].Value)"
}
$sql = Get-Content $Migration -Raw
$route = [regex]::Matches($sql, '\(845930, (\d), ([\d.]+), ([\d.]+), ([\d.]+), \d+, 100, 0\)')
if ($route.Count -ne 2) { throw 'Expected a two-point walk' }
$turn = $route[0].Groups
$returnPoint = $route[1].Groups
$homePoint = "PatrolSpawn $($returnPoint[2].Value) $($returnPoint[3].Value) $($returnPoint[4].Value)"
if ($points[3] -ne $homePoint) { throw 'Patrol endpoint and starting location differ' }
$points += "PatrolTurn $($turn[2].Value) $($turn[3].Value) $($turn[4].Value)"
$points | Set-Content "$output/guards.txt"

$runtime = "$repo/build-extractors/bin/Release"
$oldPath = $env:PATH
try {
    $env:PATH = "$runtime;$oldPath"
    & "$output/GuardGeometry.exe" $runtime "$output/guards.txt"
    if ($LASTEXITCODE) { throw "Southshore map geometry check failed ($LASTEXITCODE)" }
} finally { $env:PATH = $oldPath }
