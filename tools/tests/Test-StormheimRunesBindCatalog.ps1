$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimRunesBindData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-runes-bind"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimRunesBindCatalog','StormheimStemTideGeometry','StormheimHuntingFloor')){
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
 $rows=@(& "$out/StormheimRunesBindCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE
 $rows | Set-Content "$out/native-lines.txt";$rows
 if($code){throw 'Runes Bind native DB2 reader failed'}
 @('object 115167 3407.49 982.872 101.165','object 115143 3555.14 1001.38 103.702','object 115176 3427.26 887.076 88.9982','object 115191 3473.61 797.974 78.8167','npc 338670 3416.96 977.193 101.426') | Set-Content "$out/anchors.txt"
 $rows=@(& "$out/StormheimHuntingFloor.exe" $runtime "$out/anchors.txt");$code=$LASTEXITCODE
 $rows | Set-Content "$out/floors.txt";$rows
 if($code){throw 'Runes Bind native source/giver floor check failed'}
 $all=@()
 foreach($target in @('3555.14 1001.38 103.702','3427.26 887.076 88.9982','3473.61 797.974 78.8167','3416.96 977.193 101.426')){
  @('3407.49 982.872 101.165',$target) | Set-Content "$out/ground.txt"
  $rows=@(& "$out/StormheimStemTideGeometry.exe" $runtime "$out/ground.txt" 65535 1220 walk-swim);$code=$LASTEXITCODE
  $all+="target $target";$all+=$rows;$rows;$all | Set-Content "$out/geometry.txt"
  if($code){throw 'Runes Bind ground approach incomplete; retain diagnostic result'}
 }
}finally{$env:PATH=$previousPath}
