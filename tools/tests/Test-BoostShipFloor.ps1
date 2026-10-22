param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boost-ship-arrival"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$args+=($includes+@("$repo/src/common/collision/Models","$repo/src/common/collision/Management","$repo/src/common/collision","$repo/dep/g3dlite/include",'C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$args+='"'+$repo+'/tools/tests/BoostShipFloorTest.cpp"'
$args+='/Fe"'+$out+'/floor.exe"';$args+='/Fo"'+$out+'/floor.obj"'
$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl "+($args -join ' ')+"`r`nexit /b %errorlevel%`r`n"
[IO.File]::WriteAllText("$out/build.cmd",$cmd);& "$out/build.cmd"
if($LASTEXITCODE){throw 'Native ship floor probe failed to build'}
$previous=$env:PATH
try {$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";& "$out/floor.exe" "$repo/build-extractors/bin/Release/";if($LASTEXITCODE){throw 'Native boost deck collision check failed'}}finally{$env:PATH=$previous}
