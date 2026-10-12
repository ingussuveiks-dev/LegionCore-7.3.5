param()
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output="$repo/build-extractors/runtime-test/map-object-arrival"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=[IO.File]::ReadAllText("$repo/src/server/game/Maps/Map.cpp")
$start=$source.IndexOf('void Map::SendInitSelf(Player* player)')
if($start -lt 0){throw 'Missing production map arrival handler'}
$end=$source.IndexOf('{',$start)+1;$depth=1
while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
if($depth){throw 'Unbalanced production handler'}
[IO.File]::WriteAllText("$output/SendInitSelf.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/MapObjectArrivalTest.cpp" /Fe"$output/MapObjectArrivalTest.exe" /Fo"$output/MapObjectArrivalTest.obj"
if errorlevel 1 exit /b 1
"$output/MapObjectArrivalTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Map object arrival regression failed'}
$add=$source.Substring($source.IndexOf('bool Map::AddPlayerToMap('))
$add=$add.Substring(0,$add.IndexOf('template<class T>'))
if($add.IndexOf('player->ClearClient();') -gt $add.IndexOf('SendInitSelf(player);')){throw 'Initial city visibility tracking discarded after creating the player'}
'PASS: client visibility is cleared before initial objects are sent.'
