$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/highmountain-aviash"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/HighmountainAviashGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/AviashGeometry.exe"'
$compilerArgs+='/Fo"'+$output+'/AviashGeometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-geometry.cmd",$cmd)
& "$output/build-geometry.cmd"
if($LASTEXITCODE){throw 'Aviash geometry probe build failed'}



$source=Get-Content "$repo/src/server/scripts/Legion/highmountain_aviash_route.h" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-aviash-native-2026-10-12.json" -Raw | ConvertFrom-Json
$landing=[regex]::Match($source,'Position const Landing = \{([\d.]+)f,([\d.]+)f,([\d.]+)f,([\d.]+)f')
$values=@($native.landing.LocX,$native.landing.LocY,$native.landing.LocZ,$native.landing_orientation_radians)
for($i=0;$i -lt 4;$i++){if(!$landing.Success -or [Math]::Abs([double]$landing.Groups[$i+1].Value-$values[$i]) -gt 0.00001){throw 'Landing differs from native WorldSafeLocs 5363'}}
$start=[regex]::Match($source,'Position const Start = \{([\d.]+)f,([\d.]+)f,([\d.]+)f,')
$route=$source.Substring($source.IndexOf('Position const Route[]'))
$points=@($start)+@([regex]::Matches($route,'\{([\d.]+)f,([\d.]+)f,([\d.]+)f,'))
if(!$start.Success -or $points.Count -ne 9){throw 'Unexpected Aviash route'}
$points | ForEach-Object {"$($_.Groups[1].Value) $($_.Groups[2].Value) $($_.Groups[3].Value)"} | Set-Content "$output/flight.txt"
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/AviashGeometry.exe" "$repo/build-extractors/bin/Release" "$output/flight.txt"
 if($LASTEXITCODE){throw 'Aviash flight collision check failed'}
} finally {$env:PATH=$previousPath}
# Reuse the ground navigation probe to check the native landing -> quest giver.
& "$PSScriptRoot/Test-HighmountainSkyhornRitualGeometry.ps1"
$evidence=Get-Content "$repo/docs/audits/highmountain-aviash-native-2026-10-12.json" -Raw | ConvertFrom-Json
$p=$evidence.landing
@("$($p.LocX) $($p.LocY) $($p.LocZ)",'4171.37 4398.75 660.976') | Set-Content "$output/ground.txt"
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$repo/build-extractors/runtime-test/highmountain-skyhorn-ritual/AudienceGeometry.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt"
 if($LASTEXITCODE){throw 'Aviash landing cannot reach Lasan'}
 $return=[regex]::Match($source,'Position const Return = \{([\d.]+)f,([\d.]+)f,([\d.]+)f,')
 if(!$return.Success){throw 'Missing recovery point'}
 $p=$evidence.witchqueen_landing
 @("$($return.Groups[1].Value) $($return.Groups[2].Value) $($return.Groups[3].Value)","$($p.LocX) $($p.LocY) $($p.LocZ)") | Set-Content "$output/return.txt"
 & "$repo/build-extractors/runtime-test/highmountain-skyhorn-ritual/AudienceGeometry.exe" "$repo/build-extractors/bin/Release" "$output/return.txt"
 if($LASTEXITCODE){throw 'Aviash recovery point is not on reachable ground'}
} finally {$env:PATH=$previousPath}
