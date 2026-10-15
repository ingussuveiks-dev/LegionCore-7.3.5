$ErrorActionPreference='Stop'
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
$compilerArgs+='"'+$repo+'/tools/tests/StormheimFinalJudgmentCatalog.cpp"'
$compilerArgs+='/Fe"'+$output+'/FinalJudgmentCatalog.exe"';$compilerArgs+='/Fo"'+$output+'/FinalJudgmentCatalog.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Final Judgment native catalog build failed'}
$previousPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";$rows=& "$output/FinalJudgmentCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/";if($LASTEXITCODE){throw 'Final Judgment native DB2 reader failed'};$rows | Set-Content "$output/native-lines.txt";$rows}finally{$env:PATH=$previousPath}
@{CreatureModelData=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/CreatureModelData.db2" -Algorithm SHA256).Hash;CreatureDisplayInfo=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/CreatureDisplayInfo.db2" -Algorithm SHA256).Hash;Spell=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/Spell.db2" -Algorithm SHA256).Hash;Map=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/Map.db2" -Algorithm SHA256).Hash} | ConvertTo-Json | Set-Content "$output/native-inputs.json"
