param([string]$SourceFile="$PSScriptRoot/../../src/server/game/Maps/Map.cpp")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath("$PSScriptRoot/../../build-extractors/runtime-test/map-tile-ownership")
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=[IO.File]::ReadAllText($SourceFile)
$handlers=''
foreach($name in @('LoadMap','LoadMapImpl','UnloadMapImpl','LoadMapAndVMap')) {
 $start=$source.IndexOf("void Map::$name(")
 if($start -lt 0){throw "Missing handler $name"}
 $end=$source.IndexOf('{',$start)+1;$depth=1
 while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
 if($depth){throw 'Unbalanced handler'}
 $handlers+=$source.Substring($start,$end-$start)+"`n"
}
# Exercise the production grid-unload ownership branch without unrelated object cleanup.
$start=$source.IndexOf('bool Map::UnloadGrid(')
$start=$source.IndexOf('    int gx =',$start)
$end=$source.IndexOf('    #ifdef TRINITY_DEBUG',$start)
if($start -lt 0 -or $end -lt 0){throw 'Missing grid tile cleanup'}
$handlers+="void Map::UnloadGridTiles(int x, int y) {`n"+$source.Substring($start,$end-$start)+"}`n"
[IO.File]::WriteAllText("$output/MapTileHandlers.inc",$handlers)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/MapTileOwnershipTest.cpp" /Fe"$output/MapTileOwnershipTest.exe" /Fo"$output/MapTileOwnershipTest.obj"
if errorlevel 1 exit /b 1
"$output/MapTileOwnershipTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Map tile ownership regression failed'}
