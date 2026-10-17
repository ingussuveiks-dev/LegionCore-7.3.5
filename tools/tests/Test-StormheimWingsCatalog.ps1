$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimWingsData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-wings"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimWingsCatalog','StormheimWingsCaveAccess','StormheimStemTideGeometry','StormheimHuntingFloor')){
 $compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
 $compilerArgs+='"'+$PSScriptRoot+'/'+$test+'.cpp"';$compilerArgs+='/Fe"'+$out+'/'+$test+'.exe"';$compilerArgs+='/Fo"'+$out+'/'+$test+'.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/build.cmd",$cmd);& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $test build failed"}
}
$previousPath=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $rows=@(& "$out/StormheimWingsCatalog.exe" "$runtime");$code=$LASTEXITCODE
 $rows | Set-Content "$out/native-lines.txt";$rows
 if($code){throw 'Wings native DB2 reader failed'}
 @('object 115480 2033.69 2186.11 465.141','object 115466 2133.51 2373.01 490.76','object 115473 2058.21 2475.53 484.165','object 115476 1998 2383.88 482.131','npc 294030 2182.78 2433.03 485.375') | Set-Content "$out/anchors.txt"
 $rows=@(& "$out/StormheimHuntingFloor.exe" $runtime "$out/anchors.txt");$code=$LASTEXITCODE
 $rows | Set-Content "$out/floors.txt";$rows
 if($code){throw 'Wings native source/giver floor check failed'}
 $rows=@(& "$out/StormheimWingsCaveAccess.exe" $runtime "$PSScriptRoot/data/stormheim-wings-cave.txt");$code=$LASTEXITCODE
 $rows | Set-Content "$out/cave-access.txt";$rows
 if($code){throw "Wings cave collision approach failed ($code)"}
 $samples=Get-Content "$PSScriptRoot/data/stormheim-wings-cave.txt"
 @($samples[0],$samples[-1]) | Set-Content "$out/cave-gap.txt"
 $rejected=@(& "$out/StormheimWingsCaveAccess.exe" $runtime "$out/cave-gap.txt");$code=$LASTEXITCODE
 if($code -ne 4){throw "Missing cave locomotion was not rejected ($code)"}
 $global:LASTEXITCODE=0
 $all=@()
 # The mountain has several disconnected navmesh islands. Require an actual
 # complete corridor to each surveyed manual locomotion join, then replay its
 # supported walk/jump/fall samples. A partial direct query is never a pass.
 foreach($target in @(@('hrafsir','2133.51','2373.01','490.76'),@('erilar','2058.21','2475.53','484.165'),@('hridmogir','1998','2383.88','482.131'),@('vethir','1977.46','2551.44','489.014'))){
  $rows=@(& "$out/StormheimWingsCaveAccess.exe" $runtime "$PSScriptRoot/data/stormheim-wings-$($target[0]).txt" $target[1] $target[2] $target[3]);$code=$LASTEXITCODE
  $all+="target $($target -join ' ')";$all+=$rows;$rows;$all | Set-Content "$out/geometry.txt"
  if($code){throw "Wings supported approach failed: $($target[0]) ($code)"}
 }
 & "$out/StormheimStemTideGeometry.exe" --self-test
 if($LASTEXITCODE){throw 'Native partial/exhausted/truncated query self-test failed'}
}finally{$env:PATH=$previousPath}
