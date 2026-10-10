param([string]$RuntimeDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
New-Item -ItemType Directory -Force -Path "$repo/.codex" | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/HighmountainLifespringGeometry.cpp"'
$compilerArgs+='/Fe"'+$repo+'/.codex/hm-cave-geometry.exe"'
$compilerArgs+='/Fo"'+$repo+'/.codex/hm-cave-geometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`nif errorlevel 1 exit /b 1`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$repo/.codex/build-hm-geometry.cmd",$cmd)
& "$repo/.codex/build-hm-geometry.cmd"
if($LASTEXITCODE){throw 'Geometry probe build failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_08_highmountain_lifespring.sql" -Raw
$points=[regex]::Matches($migration,'(?m)^ (?:UNION ALL )?SELECT (\d+\.\d+)(?: AS x)?,(\d+\.\d+)(?: AS y)?,(\d+\.\d+)')
if($points.Count -ne 12){throw 'Expected twelve migration placements'}
$points | ForEach-Object {"$($_.Groups[1].Value) $($_.Groups[2].Value) $($_.Groups[3].Value)"} | Set-Content "$repo/.codex/hm-lifespring-positions.txt"
& "$repo/.codex/hm-cave-geometry.exe" ([IO.Path]::GetFullPath($RuntimeDirectory)) "$repo/.codex/hm-lifespring-positions.txt"
if($LASTEXITCODE){throw 'Crystal geometry or connected-path check failed'}
