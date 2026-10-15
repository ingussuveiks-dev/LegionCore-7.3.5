$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-judgment"
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
$compilerArgs+='/Fe"'+$output+'/JudgmentGround.exe"';$compilerArgs+='/Fo"'+$output+'/JudgmentGround.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Judgment native ground probe build failed'}
$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 foreach($point in @('3513.71 1113.86 153.034','3375.76 1272.61 126.355','3406.63 1153.15 136.178','3292.32 1078.66 126.649')) {
  @('3292.32 1078.66 126.649',$point) | Set-Content "$output/ground.txt"
  & "$output/JudgmentGround.exe" "$repo/build-extractors/bin/Release" "$output/ground.txt"
  if($LASTEXITCODE){throw "Iounn -> bowl ground check failed: $point"}
 }
 foreach($pair in @(@('3513.62 1102.43 157.74','3513.71 1113.86 153.034'),@('3377.39 1279.22 128.48','3375.76 1272.61 126.355'),@('3417.11 1162.67 139','3406.63 1153.15 136.178'))) {
  $pair | Set-Content "$output/king-ground.txt"
  & "$output/JudgmentGround.exe" "$repo/build-extractors/bin/Release" "$output/king-ground.txt"
  if($LASTEXITCODE){throw 'Ruler -> bowl ground connectivity failed'}
 }
} finally {$env:PATH=$previousPath}
'PASS: native Iounn -> three offering bowls/turn-in have connected ground approaches within the interaction ranges; model/ritual rendering remains client-unverified.'
