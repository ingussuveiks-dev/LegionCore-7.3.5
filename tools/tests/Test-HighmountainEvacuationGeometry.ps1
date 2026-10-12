$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/highmountain-evacuation"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/HighmountainEvacuationGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/EvacuationGeometry.exe"'
$compilerArgs+='/Fo"'+$output+'/EvacuationGeometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-geometry.cmd",$cmd)
& "$output/build-geometry.cmd"
if($LASTEXITCODE){throw 'Evacuation geometry probe build failed'}

$source=Get-Content "$repo/src/server/scripts/Legion/highmountain_evacuation.cpp" -Raw
$positions=foreach($name in @('RedPath','TarvimPath')) {
 $route=[regex]::Match($source,('PointsArray const '+$name+' = \{([\s\S]+?)\n\};'))
 if(!$route.Success){throw "Missing evacuation $name"}
 $points=[regex]::Matches($route.Groups[1].Value,'\{([\d.]+)f,([\d.]+)f,([\d.]+)f\}')
 $points.Count
 $index=0
 foreach($p in $points){
  $z=$p.Groups[3].Value
  # MoveSplineInit replaces the first point with the actual source spawn position.
  if($index++ -eq 0){$z=if($name -eq 'RedPath'){'898.375'}else{'915.923'}}
  "$($p.Groups[1].Value) $($p.Groups[2].Value) $z"
 }
}
$positions | Set-Content "$output/endpoint.txt"
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/EvacuationGeometry.exe" "$repo/build-extractors/bin/Release" "$output/endpoint.txt"
 if($LASTEXITCODE){throw 'Evacuation ground path check failed'}
} finally {$env:PATH=$previousPath}
