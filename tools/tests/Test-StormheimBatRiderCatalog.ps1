$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimBatRiderData.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/stormheim-batrider"
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$r.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimBatRiderCatalog')){
 $args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $args+=($includes | ForEach-Object {'/I"'+$_+'"'});$args+='/I"'+$out+'"';$args+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$args+='/Fe"'+$out+'/'+$name+'.exe"';$args+='/Fo"'+$out+'/'+$name+'.obj"';$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $name compilation failed"}
}
$old=$env:PATH
try{$runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH";$rows=@(& "$out/StormheimBatRiderCatalog.exe" $runtime);$code=$LASTEXITCODE;$rows | Set-Content "$out/native.tsv";$rows;if($code){throw 'Native Bat-Rider catalog/flight collision failed'}}finally{$env:PATH=$old}