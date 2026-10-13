$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/highmountain-underking"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/HighmountainUnderkingGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/UnderkingGeometry.exe"'
$compilerArgs+='/Fo"'+$output+'/UnderkingGeometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-geometry.cmd",$cmd)
& "$output/build-geometry.cmd"
if($LASTEXITCODE){throw 'Highmountain Underking geometry probe build failed'}

$source=Get-Content "$repo/src/server/scripts/Legion/highmountain_underking.cpp" -Raw
$point=[regex]::Match($source,'Position const CaveMayla = \{([\d.]+)f,([\d.]+)f,([\d.]+)f,')
if(!$point.Success){throw 'Missing cave meeting coordinate'}
$positions=@("$($point.Groups[1].Value) $($point.Groups[2].Value) $($point.Groups[3].Value)","3611.37 3927.83 907.282")
$positions | Set-Content "$output/endpoint.txt"
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/UnderkingGeometry.exe" "$repo/build-extractors/bin/Release" "$output/endpoint.txt"
 if($LASTEXITCODE){throw 'Highmountain Underking terrain check failed'}
} finally {$env:PATH=$previousPath}
