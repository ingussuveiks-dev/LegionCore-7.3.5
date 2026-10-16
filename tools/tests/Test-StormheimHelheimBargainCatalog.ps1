$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/stormheim-helheim-bargain"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/StormheimHelheimBargainCatalog.cpp"'
$compilerArgs+='/Fe"'+$output+'/HelheimBargainCatalog.exe"';$compilerArgs+='/Fo"'+$output+'/HelheimBargainCatalog.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-ground.cmd",$cmd)
& "$output/build-ground.cmd"
if($LASTEXITCODE){throw 'Helheim bargain native catalog build failed'}
$previousPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";$rows=& "$output/HelheimBargainCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/";if($LASTEXITCODE){throw 'Helheim bargain native DB2 reader failed'};$rows | Set-Content "$output/native-lines.txt";$rows}finally{$env:PATH=$previousPath}
$hashes=@{}
foreach($name in @('CreatureModelData','CreatureDisplayInfo','Spell','GameObjectDisplayInfo','ConversationLine','BroadcastText','SoundKit')){$hashes[$name]=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/$name.db2" -Algorithm SHA256).Hash}
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"

$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@("SELECT 'voice',COUNT(*) FROM broadcast_text WHERE ID=98487 AND SoundEntriesID2=52998 AND Text1='Unworthy! You will kneel before addressing the great Helya!' AND ConditionID=0; SELECT 'menu',COUNT(*) FROM broadcast_text WHERE ID IN(99442,99443) AND ConditionID=0; SELECT 'line-overlay',COUNT(*) FROM conversation_line WHERE ID=4747; SELECT 'spell-overlay',COUNT(*) FROM spell_effect WHERE SpellID=212898;" | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Bargain hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("voice`t1","menu`t2","line-overlay`t0","spell-overlay`t0")){if($rows -notcontains $expected){throw "Missing live hotfix evidence $expected"}}
$rows | Set-Content "$output/hotfix-lines.txt"
'PASS: installed Hotfix supplies female Helya broadcast98487/voice52998 and menu99442/99443; no4747 or212898 effect overrides.'
