$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-light-way"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$source=Get-Content "$repo/src/server/game/Globals/ObjectMgr.cpp" -Raw
$a=$source.IndexOf('        if (std::vector<ItemSpecOverrideEntry const*> const* itemSpecOverrides =', $source.IndexOf('void ObjectMgr::LoadItemTemplates('));$b=$source.IndexOf('        // Items that have no specializations set',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native item class-mask builder missing'}
[IO.File]::WriteAllText("$output/NativeItemClassMask.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='/I"'+$output+'"';$compilerArgs+='"'+$repo+'/tools/tests/StormheimLightWayCatalog.cpp"'
$compilerArgs+='/Fe"'+$output+'/LightWayCatalog.exe"';$compilerArgs+='/Fo"'+$output+'/LightWayCatalog.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Light Way native catalog build failed'}
$previousPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";$rows=& "$output/LightWayCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/";if($LASTEXITCODE){throw 'Light Way native DB2 reader failed'};$rows | Set-Content "$output/native-lines.txt";$rows}finally{$env:PATH=$previousPath}
$hashes=@{}
foreach($name in @('CreatureModelData','CreatureDisplayInfo','Item','ItemSparse','ItemEffect','ItemSpec','ItemSpecOverride','ChrSpecialization','QuestPackageItem','Spell','SoundKit')){$hashes[$name]=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/$name.db2" -Algorithm SHA256).Hash}
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"
