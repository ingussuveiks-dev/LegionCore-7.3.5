$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHelheimDisguiseData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-helheim-disguise"
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
$compilerArgs+='/Fe"'+$output+'/HelheimDisguiseGround.exe"';$compilerArgs+='/Fo"'+$output+'/HelheimDisguiseGround.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'HelheimDisguise native ground build failed'}
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 foreach($pair in @(@('366.422 369.746 27.5921','455.686 500 5.13701'),@('455.686 500 5.13701','454.412 565.988 0.083255'),@('454.412 565.988 0.083255','327.851 574.399 6.34934'))) {
  $pair | Set-Content "$output/ground.txt"
  & "$output/HelheimDisguiseGround.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt" 1463
  if($LASTEXITCODE){throw 'Helheim cage/Colborn approaches are not connected'}
 }
}finally{$env:PATH=$previousPath}
'PASS: native map1463 ground approaches connect Colborn -> approach side -> exact exit334 -> permanent Ashildir fallback. Guard collision/visibility and actual client passage remain unverified.'
