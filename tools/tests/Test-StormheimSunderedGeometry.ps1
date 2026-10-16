$ErrorActionPreference='Stop'

$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-sundered"
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
$compilerArgs+='/Fe"'+$output+'/SunderedGround.exe"';$compilerArgs+='/Fo"'+$output+'/SunderedGround.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Sundered native ground build failed'}
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 $failures=0
 foreach($pair in @(@('327.851 574.399 6.34934','231.622 378.858 6.28104'),@('327.851 574.399 6.34934','109.45 449.408 -14.6794'),@('327.851 574.399 6.34934','112.366 704.08 17.823'))) {
  $pair | Set-Content "$output/ground.txt"
  $rows = & "$output/SunderedGround.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 1463
  $rows | Tee-Object -FilePath "$output/geometry-$($pair[1].Split(' ')[0]).txt"
  if($LASTEXITCODE){++$failures}
 }
 if($failures){throw "$failures Sundered walk/swim approaches remain disconnected"}
}finally{$env:PATH=$previousPath}
'PASS: native map1463 walk/swim approaches connect actual Ashildir97480 -> all three original Sundered fragments within its production10yd limit. Actual client collision, visibility and interaction remain unverified.'
