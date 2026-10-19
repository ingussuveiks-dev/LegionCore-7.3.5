$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/rockaway-channel-native"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$gameLib=$libs | Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'} | Select-Object -First 1
if(!$gameLib){throw 'Actual game final-link input is missing'}
$runtime=(Resolve-Path "$repo/build-extractors/bin/Release").Path
$nativeSources=@('AI/CoreAI/CombatAI.cpp','AI/CoreAI/UnitAI.cpp','AI/CreatureAI.cpp','AI/ScriptedAI/ScriptedCreature.cpp','Spells/SpellInfo.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp','Spells/Auras/SpellAuras.cpp','Spells/Auras/SpellAuraEffects.cpp','Entities/Creature/Creature.cpp','Entities/Creature/TemporarySummon.cpp','Entities/Unit/Unit.cpp','Maps/Map.cpp','Globals/ObjectMgr.cpp','Scripting/ScriptMgr.cpp','Globals/ScriptsData.cpp')
foreach($path in $nativeSources){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw "Actual game library predates $path; build Release first"}
}
if((Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item "$runtime/worldserver.exe").LastWriteTimeUtc){throw 'Canonical executable predates its game library'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='/I"'+$output+'"';$compilerArgs+='"'+$repo+'/tools/tests/RockawayChannelNative.cpp"'
$compilerArgs+='/Fe"'+$output+'/RockawayChannelNative.exe"';$compilerArgs+='/Fo"'+$output+'/RockawayChannelNative.obj"';$compilerArgs+='/link';$compilerArgs+='/DEBUG';$compilerArgs+='/MAP';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build.cmd",$cmd)
& "$output/build.cmd"
if($LASTEXITCODE){throw 'Rockaway native channel build failed'}
$previousPath=$env:PATH
$run=Join-Path $output ('run-'+(Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory -Force $run | Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'"'))
[IO.File]::WriteAllText("$run/worldserver.conf",$config)
Push-Location "$repo/build-extractors/bin/Release"
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";$rows=& "$output/RockawayChannelNative.exe" "$run/worldserver.conf" "$repo/build-extractors/bin/Release/";$result=$LASTEXITCODE;$rows | Set-Content "$output/native-lines.txt";$rows | Where-Object {$_ -match '^PASS:'};if($result){$rows | Select-Object -Last 16;throw 'Native channel execution failed'}}finally{$env:PATH=$previousPath;Pop-Location}
$run | Set-Content "$output/latest-run.txt"
if((Get-Item "$run/DBErrors.log").Length){Get-Content "$run/DBErrors.log" -Tail 8;throw 'Native channel test emitted DB/script errors'}
$hashes=@{}
$inputs=@(Get-ChildItem "$runtime/dbc/enUS/Spell*.db2")
$inputs+=@('Map','Difficulty','CreatureDisplayInfo','CreatureModelData','ChrClasses','Faction','FactionTemplate','SummonProperties','ConversationLine','BroadcastText','SoundKit','SoundKitEntry') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}
$hashes['src/server/scripts/Legion/rockaway_coast.h']=(Get-FileHash "$repo/src/server/scripts/Legion/rockaway_coast.h").Hash
$hashes['tools/tests/RockawayChannelNative.cpp']=(Get-FileHash "$repo/tools/tests/RockawayChannelNative.cpp").Hash
$hashes['tools/tests/Test-RockawayChannelNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-RockawayChannelNative.ps1").Hash
$hashes['native-lines.txt']=(Get-FileHash "$output/native-lines.txt").Hash
$inputs+=Get-ChildItem "$runtime/gt" -File
foreach($inputFile in $inputs){$hashes[$inputFile.FullName.Substring($runtime.Length+1).Replace('\','/')]=(Get-FileHash $inputFile.FullName -Algorithm SHA256).Hash}
$hashes['actual-final-link-game.lib']=(Get-FileHash $gameLib -Algorithm SHA256).Hash
$hashes['canonical-worldserver.exe']=(Get-FileHash "$runtime/worldserver.exe" -Algorithm SHA256).Hash
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"
'PASS: actual final-link library/source freshness and relevant DB2/GT/library/executable hashes recorded. Private world pool is read-only; runtime configuration and personal progress are unchanged.'
