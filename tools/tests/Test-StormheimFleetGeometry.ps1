$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-fleet"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/StormheimFleetGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/FleetGeometry.exe"'
$compilerArgs+='/Fo"'+$output+'/FleetGeometry.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-geometry.cmd",$cmd)
& "$output/build-geometry.cmd"
if($LASTEXITCODE){throw 'Stormheim rounds geometry probe build failed'}

$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_fleet.cpp" -Raw
$points=@()
foreach($name in @('LocalDeck','LocalParachute')){
 $value=[regex]::Match($source,"Position const $name=\{([^}]+)\}").Groups[1].Value.Split(',')
 $points+='local '+(($value[0..2] | ForEach-Object {$_.Replace('f','').Trim()}) -join ' ')
}
foreach($name in @('LocalFlares','Boats')){
 $block=[regex]::Match($source,"Position const $name\[\]=\{([\s\S]*?)\};").Groups[1].Value
 foreach($row in [regex]::Matches($block,'\{([^}]+)\}')){
  $value=$row.Groups[1].Value.Split(',');$role=if($name -eq 'Boats'){'boat'}else{'local'}
  $points+=$role+' '+(($value[0..2] | ForEach-Object {$_.Replace('f','').Trim()}) -join ' ')
 }
}
$points+='launch -16 11 20.65'
$points | Set-Content "$output/points.txt"
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/FleetGeometry.exe" "$repo/build-extractors/bin/Release" "$output/points.txt"
 if($LASTEXITCODE){throw 'Fleet geometry regression failed'}
}finally{$env:PATH=$previousPath}