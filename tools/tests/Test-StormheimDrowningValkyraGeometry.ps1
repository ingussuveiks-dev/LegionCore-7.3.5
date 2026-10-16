$ErrorActionPreference='Stop'

$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-drowning-valkyra"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/StormheimSunderedGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/ValkyraGround.exe"';$compilerArgs+='/Fo"'+$output+'/ValkyraGround.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Valkyra native ground build failed'}
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 $failures=0
 foreach($row in (Get-Content "$output/spawns.tsv")) {
  $parts=$row.Split("`t");$pair=@('327.851 574.399 6.34934',($parts[1..3] -join ' '))
  $pair | Set-Content "$output/ground.txt"
  $rows = & "$output/ValkyraGround.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 1463
  $rows | Tee-Object -FilePath "$output/geometry-$($parts[0]).txt"
  if($LASTEXITCODE){++$failures}
 }
 if($failures){throw "$failures Valkyra walk/swim approaches remain disconnected"}
}finally{$env:PATH=$previousPath}
'PASS: native map1463 walk/swim approaches connect actual Ashildir97480 -> all ten original Drowning Valkyra within its production10yd limit. Actual client collision, visibility and interaction remain unverified.'
