$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimWagerData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-wager"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimWagerCatalog','StormheimStemTideGeometry','StormheimHuntingFloor')){
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
 & "$out/StormheimStemTideGeometry.exe" --self-test
 if($LASTEXITCODE){throw 'Native corridor status validation failed'}
 $rows=@(& "$out/StormheimWagerCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE
 $rows | Set-Content "$out/native-lines.txt";$rows
 if($code){throw 'Wager native DB2 reader failed'}
 @('npc 371030 2953.34 2057.69 202.631','npc 338664 2969.44 2047.61 201.939','npc 290799 2827.81 2341.43 144.987','npc 293764 2339.84 2043.03 170.825','object 25167470 3905.85 2223.91 322.099') | Set-Content "$out/anchors.txt"
 $rows=@(& "$out/StormheimHuntingFloor.exe" $runtime "$out/anchors.txt");$code=$LASTEXITCODE
 $rows | Set-Content "$out/floors.txt";$rows
 if($code){throw 'Wager native source/giver floor check failed'}
 $all=@()
 foreach($target in @('2827.81 2341.43 144.987','2339.84 2043.03 170.825','3905.85 2223.91 322.099')){
  @('2969.44 2047.61 201.939',$target) | Set-Content "$out/ground.txt"
  # The musken hunting grounds are across water. Ground-only rejects a valid
  # walk/swim route. Keep the real terrain flags and reject partial paths.
  $routeArgs=@($runtime,"$out/ground.txt",65535,1220)
  if($target -eq '2339.84 2043.03 170.825'){$routeArgs+='walk-swim'}
  $rows=@(& "$out/StormheimStemTideGeometry.exe" @routeArgs);$code=$LASTEXITCODE
  $all+="target $target";$all+=$rows;$rows;$all | Set-Content "$out/geometry.txt"
  if($code){throw 'Wager ground approach incomplete; retain diagnostic result'}
 }
}finally{$env:PATH=$previousPath}
