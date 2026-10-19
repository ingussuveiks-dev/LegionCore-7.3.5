param([switch]$Offensive, [switch]$Baseline, [switch]$LostArmy)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
if($LostArmy -and $Offensive){throw 'Select one native probe'}
if($Baseline -and !$Offensive -and !$LostArmy){throw 'Select a supported native baseline'}
$stem=if($LostArmy){'LostArmyLootNative'}elseif($Offensive){'RockawayOffensiveNative'}else{'RockawayChannelNative'}
$directory=if($LostArmy){'lost-army-native'}elseif($Offensive){'rockaway-offensive-native'}else{'rockaway-channel-native'}
$output=Join-Path $repo "build-extractors/runtime-test/$directory"
if($Baseline){$output+='-baseline'}
New-Item -ItemType Directory -Force -Path $output | Out-Null
if($Offensive -or $LostArmy){
 $questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
 $query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
 if(!$query){throw 'Native quest loader SELECT could not be extracted'}
 if($LostArmy){[IO.File]::WriteAllText("$output/NativeLostArmyQuest.inc",$query+' " WHERE ID IN (44923,45088)"')}
 else{[IO.File]::WriteAllText("$output/NativeRockawayQuest.inc",$query+' " WHERE ID=44746"')}
 if($Baseline -and $Offensive){
  $oldHeader=& git -C $repo show 'a788f5d2:src/server/scripts/Legion/rockaway_coast.h'
  if($LASTEXITCODE){throw 'Pinned Rockaway baseline unavailable'}
  [IO.File]::WriteAllText("$output/RockawayBaseline.inc",($oldHeader -join "`n"))
 }
}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$gameLib=$libs | Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'} | Select-Object -First 1
if(!$gameLib){throw 'Actual game final-link input is missing'}
$runtime=(Resolve-Path "$repo/build-extractors/bin/Release").Path
$nativeSources=@('AI/CoreAI/CombatAI.cpp','AI/CoreAI/UnitAI.cpp','AI/CreatureAI.cpp','AI/ScriptedAI/ScriptedCreature.cpp','Spells/SpellInfo.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp','Spells/Auras/SpellAuras.cpp','Spells/Auras/SpellAuraEffects.cpp','Entities/Creature/Creature.cpp','Entities/Creature/TemporarySummon.cpp','Entities/Unit/Unit.cpp','Maps/Map.cpp','Globals/ObjectMgr.cpp','Scripting/ScriptMgr.cpp','Globals/ScriptsData.cpp')
if($Offensive -or $LostArmy){$nativeSources+=@('Entities/Player/Player.cpp','Entities/Unit/StatSystem.cpp','Conditions/ConditionMgr.cpp','Server/WorldSession.cpp','Server/Protocol/Opcodes.cpp','Globals/QuestData.cpp','Quests/QuestDef.cpp')}
if($LostArmy){$nativeSources+=@('Loot/LootMgr.cpp','Groups/Group.cpp','Entities/Player/KillRewarder.cpp','Entities/Item/Item.cpp')}
foreach($path in $nativeSources){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw "Actual game library predates $path; build Release first"}
}
if((Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item "$runtime/worldserver.exe").LastWriteTimeUtc){throw 'Canonical executable predates its game library'}
if($LostArmy -and $Baseline){
 $baselineLibrary="$output/game-baseline.lib"
 if(!(Test-Path $baselineLibrary)){
  if((& git -C $repo rev-parse --short=8 HEAD) -ne '0fbd08e4' -or (& git -C $repo diff -- src/server/game)){throw 'Baseline library must be captured from clean pinned0fbd08e4 game sources'}
  Copy-Item -LiteralPath $gameLib -Destination $baselineLibrary
  (Get-FileHash $baselineLibrary).Hash | Set-Content "$output/baseline-library.sha256"
 }
 if((Get-FileHash $baselineLibrary).Hash -ne (Get-Content "$output/baseline-library.sha256")){throw 'Pinned baseline library changed'}
 $libs=$libs | ForEach-Object {if($_ -eq $gameLib){$baselineLibrary}else{$_}}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline -and $Offensive){$compilerArgs+='/DROCKAWAY_OFFENSIVE_BASELINE'}
if($Baseline -and $LostArmy){$compilerArgs+='/DLOST_ARMY_NATIVE_BASELINE'}
$compilerArgs+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='/I"'+$output+'"';$compilerArgs+='"'+$repo+'/tools/tests/'+$stem+'.cpp"'
$compilerArgs+='/Fe"'+$output+'/'+$stem+'.exe"';$compilerArgs+='/Fo"'+$output+'/'+$stem+'.obj"';$compilerArgs+='/link';$compilerArgs+='/DEBUG';$compilerArgs+='/MAP';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
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
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";$rows=& "$output/$stem.exe" "$run/worldserver.conf" "$repo/build-extractors/bin/Release/";$result=$LASTEXITCODE;$rows | Set-Content "$output/native-lines.txt";$rows | Where-Object {$_ -match '^PASS:'};if($result){$rows | Select-Object -Last 16;throw "Native execution failed (exit $result)"}}finally{$env:PATH=$previousPath;Pop-Location}
$run | Set-Content "$output/latest-run.txt"
if((Get-Item "$run/DBErrors.log").Length){Get-Content "$run/DBErrors.log" -Tail 8;throw 'Native channel test emitted DB/script errors'}
$hashes=@{}
$inputs=@(Get-ChildItem "$runtime/dbc/enUS/Spell*.db2")
$inputs+=@('Map','Difficulty','CreatureDisplayInfo','CreatureModelData','ChrClasses','Faction','FactionTemplate','SummonProperties','ConversationLine','BroadcastText','SoundKit','SoundKitEntry') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}
if($LostArmy){$inputs+=@('Item','ItemSparse','ItemEffect','ItemCurrencyCost','CurrencyTypes','QuestObjective','QuestV2CliTask','ChrRaces') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}}
$hashes['src/server/scripts/Legion/rockaway_coast.h']=(Get-FileHash "$repo/src/server/scripts/Legion/rockaway_coast.h").Hash
$hashes['tools/tests/RockawayChannelNative.cpp']=(Get-FileHash "$repo/tools/tests/RockawayChannelNative.cpp").Hash
$hashes['tools/tests/Test-RockawayChannelNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-RockawayChannelNative.ps1").Hash
if($Offensive){$hashes['tools/tests/RockawayOffensiveNative.cpp']=(Get-FileHash "$repo/tools/tests/RockawayOffensiveNative.cpp").Hash;$hashes['NativeRockawayQuest.inc']=(Get-FileHash "$output/NativeRockawayQuest.inc").Hash}
if($LostArmy){$hashes['tools/tests/LostArmyLootNative.cpp']=(Get-FileHash "$repo/tools/tests/LostArmyLootNative.cpp").Hash;$hashes['tools/tests/Test-LostArmyNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-LostArmyNative.ps1").Hash;$hashes['tools/tests/RockawayOffensiveNative.cpp']=(Get-FileHash "$repo/tools/tests/RockawayOffensiveNative.cpp").Hash;$hashes['NativeLostArmyQuest.inc']=(Get-FileHash "$output/NativeLostArmyQuest.inc").Hash}
$hashes['native-lines.txt']=(Get-FileHash "$output/native-lines.txt").Hash
$inputs+=Get-ChildItem "$runtime/gt" -File
foreach($inputFile in $inputs){$hashes[$inputFile.FullName.Substring($runtime.Length+1).Replace('\','/')]=(Get-FileHash $inputFile.FullName -Algorithm SHA256).Hash}
$hashes['actual-final-link-game.lib']=(Get-FileHash $gameLib -Algorithm SHA256).Hash
$hashes['canonical-worldserver.exe']=(Get-FileHash "$runtime/worldserver.exe" -Algorithm SHA256).Hash
if($Baseline -and $Offensive){$hashes['pinned-a788f5d2-rockaway-header']=(Get-FileHash "$output/RockawayBaseline.inc").Hash}
if($Baseline -and $LostArmy){$hashes['linked-pinned-0fbd08e4-game.lib']=(Get-FileHash $baselineLibrary).Hash}
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"
Copy-Item -LiteralPath "$output/native-inputs.json" -Destination "$run/native-inputs.json"
Copy-Item -LiteralPath "$output/native-lines.txt" -Destination "$run/native-lines.txt"
'PASS: actual final-link library/source freshness and relevant DB2/GT/library/executable hashes recorded. Private world pool is read-only; runtime configuration and personal progress are unchanged.'
