$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimFinalJudgmentData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-final-judgment"
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
$compilerArgs+='/Fe"'+$output+'/FinalJudgmentGround.exe"';$compilerArgs+='/Fo"'+$output+'/FinalJudgmentGround.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'FinalJudgment native ground build failed'}
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 foreach($pair in @(@('3532.89 887.594 54.7112','3626.52 791.312 38.7806'),@('3626.52 791.312 38.7806','3639.2 777.75 40.2274'),@('3626.52 791.312 38.7806','3630.62 759.943 38.7806'),@('3626.52 791.312 38.7806','3655.53 784.203 39.1235'))) {
  $pair | Set-Content "$output/ground.txt"
  & "$output/FinalJudgmentGround.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt"
  if($LASTEXITCODE){throw 'Haustvald confrontation approaches are not connected'}
 }
 @('343.62 354.5 30.54','341.621 356.556 30.5468') | Set-Content "$output/helheim-ground.txt"
 & "$output/FinalJudgmentGround.exe" "$repo/build-extractors/bin/Release" "$output/helheim-ground.txt" 1463
 if($LASTEXITCODE){throw 'Helheim landing does not connect to the questgiver'}
}finally{$env:PATH=$previousPath}
'PASS: native ground approaches connect source Ashildir -> Faljar -> captive spirit, and map1463 landing -> turn-in Ashildir; client arrival/collision/cinematic remain unverified.'
