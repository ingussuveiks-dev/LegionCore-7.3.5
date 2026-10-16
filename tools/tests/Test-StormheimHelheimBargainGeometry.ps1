$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHelheimBargainData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-helheim-bargain"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/StormheimJudgmentGeometry.cpp"'
$compilerArgs+='/Fe"'+$output+'/HelheimBargainGround.exe"';$compilerArgs+='/Fo"'+$output+'/HelheimBargainGround.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'HelheimBargain native ground build failed'}
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 foreach($pair in @(@('454.412 565.988 0.083255','484.451 717.054 23.3577'),@('484.451 717.054 23.3577','490.429 721.396 23.2744'))) {
  $pair | Set-Content "$output/ground.txt"
  & "$output/HelheimBargainGround.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 1463
  if($LASTEXITCODE){throw 'Helheim throne/altar approaches are not connected'}
 }
}finally{$env:PATH=$previousPath}
'PASS: native map1463 ground approaches connect exit334 -> throne approach -> questgiver altar. Actual client collision, visibility and interaction remain unverified.'
