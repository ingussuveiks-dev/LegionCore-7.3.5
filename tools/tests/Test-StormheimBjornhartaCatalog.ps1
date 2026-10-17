$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimBjornhartaData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-bjornharta"
New-Item -ItemType Directory -Force -Path $out | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($test in @('StormheimBjornhartaCatalog','StormheimStemTideGeometry','StormheimHuntingFloor')){
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
 $rows=@(& "$out/StormheimBjornhartaCatalog.exe" "$runtime/dbc/enUS/");$code=$LASTEXITCODE
 $rows | Set-Content "$out/native-lines.txt";$rows
 if($code){throw 'Bjornharta native DB2 reader failed'}
 # The shared floor probe requires five ordinary NPC anchors. Vydhar is a
 # stationary tree; its model origin is not a player foot position. Validate
 # actual approaches to him through the native route checks below.
 @('npc 292160 3063.41 989.272 134.25','npc 292126 3124.09 952.031 113.752','npc 292076 3218.59 900.555 114.055','npc 292119 3120.15 946.233 113.133','npc 292081 3212.96 861.625 114.01') | Set-Content "$out/anchors.txt"
 $rows=@(& "$out/StormheimHuntingFloor.exe" $runtime "$out/anchors.txt");$code=$LASTEXITCODE
 $rows | Set-Content "$out/floors.txt";$rows
 if($code){throw 'Bjornharta native source/giver floor check failed'}
 $all=@()
 foreach($target in @('3063.41 989.272 134.25','3124.09 952.031 113.752','3218.59 900.555 114.055')){
  @('3073.09 1056.88 127.021',$target) | Set-Content "$out/ground.txt"
  $rows=@(& "$out/StormheimStemTideGeometry.exe" $runtime "$out/ground.txt" 65535 1220 walk-swim);$code=$LASTEXITCODE
  $all+="target $target";$all+=$rows;$rows;$all | Set-Content "$out/geometry.txt"
  if($code){throw 'Bjornharta ground approach incomplete; retain diagnostic result'}
 }
}finally{$env:PATH=$previousPath}
