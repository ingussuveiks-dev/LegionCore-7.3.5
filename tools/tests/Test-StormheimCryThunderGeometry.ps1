$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-cry-thunder"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/StormheimCryThunderGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/CryThunderGeometry.exe"'
$compilerArgs+='/Fo"'+$output+'/CryThunderGeometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-flight-geometry.cmd",$cmd)
& "$output/build-flight-geometry.cmd"
if($LASTEXITCODE){throw 'Cry Thunder geometry probe build failed'}



$previousPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH"; & "$output/CryThunderGeometry.exe" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Cry Thunder collision check failed'}}finally{$env:PATH=$previousPath}

$groundCmd=$cmd.Replace('/tools/tests/StormheimCryThunderGeometry.cpp','/tools/tests/HighmountainSkyhornRitualGeometry.cpp').Replace('CryThunderGeometry.exe','CryThunderGroundGeometry.exe').Replace('CryThunderGeometry.obj','CryThunderGroundGeometry.obj')
[IO.File]::WriteAllText("$output/build-ground.cmd",$groundCmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Cry Thunder ground probe build failed'}
@('3070.21 2249.24 223.3','3066.67 2246.5 224.916') | Set-Content "$output/ground.txt"
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH"; & "$output/CryThunderGroundGeometry.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt";if($LASTEXITCODE){throw 'Landing cannot reach Huginn'}}finally{$env:PATH=$previousPath}
'PASS: actual landing -> Huginn native ground route.'
