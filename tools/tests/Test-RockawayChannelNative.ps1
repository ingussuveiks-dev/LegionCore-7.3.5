param([switch]$Offensive, [switch]$Baseline, [switch]$LostArmy, [switch]$MasterLoot, [switch]$RollCompletion, [switch]$GroupLifecycle, [string]$CharacterScratch, [string]$AuthScratch, [string]$ScratchUser, [string]$ScratchPassword)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
if($LostArmy -and $Offensive){throw 'Select one native probe'}
if($MasterLoot -and ($LostArmy -or $Offensive)){throw 'Select one native probe'}
if($RollCompletion -and ($MasterLoot -or $LostArmy -or $Offensive)){throw 'Select one native probe'}
if($GroupLifecycle -and ($RollCompletion -or $MasterLoot -or $LostArmy -or $Offensive)){throw 'Select one native probe'}
if($GroupLifecycle -and $CharacterScratch -notmatch '^codex_roll_lifecycle_[a-f0-9]{16}$'){throw 'Use the lifecycle wrapper with an isolated scratch database'}
if($GroupLifecycle -and $AuthScratch -notmatch '^codex_roll_auth_[a-f0-9]{16}$'){throw 'Use the lifecycle wrapper with an isolated auth database'}
$lootProbe=$LostArmy -or $MasterLoot -or $RollCompletion -or $GroupLifecycle
if($Baseline -and !$Offensive -and !$lootProbe){throw 'Select a supported native baseline'}
$stem=if($GroupLifecycle){'GroupRollLifecycleNative'}elseif($RollCompletion){'RollCompletionNative'}elseif($MasterLoot){'MasterLootNative'}elseif($LostArmy){'LostArmyLootNative'}elseif($Offensive){'RockawayOffensiveNative'}else{'RockawayChannelNative'}
$directory=if($GroupLifecycle){'group-roll-lifecycle-native'}elseif($RollCompletion){'roll-completion-native'}elseif($MasterLoot){'master-loot-native'}elseif($LostArmy){'lost-army-native'}elseif($Offensive){'rockaway-offensive-native'}else{'rockaway-channel-native'}
$output=Join-Path $repo "build-extractors/runtime-test/$directory"
if($Baseline){$output+='-baseline'}
New-Item -ItemType Directory -Force -Path $output | Out-Null
if($Offensive -or $lootProbe){
 $questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
 $query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
 if(!$query){throw 'Native quest loader SELECT could not be extracted'}
 if($lootProbe){[IO.File]::WriteAllText("$output/NativeLostArmyQuest.inc",$query+' " WHERE ID IN (44923,45088)"')}
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
if($Offensive -or $lootProbe){$nativeSources+=@('Entities/Player/Player.cpp','Entities/Unit/StatSystem.cpp','Conditions/ConditionMgr.cpp','Server/WorldSession.cpp','Server/Protocol/Opcodes.cpp','Globals/QuestData.cpp','Quests/QuestDef.cpp')}
if($lootProbe){$nativeSources+=@('Loot/LootMgr.cpp','Groups/Group.cpp','Entities/Player/KillRewarder.cpp','Entities/Item/Item.cpp')}
if($MasterLoot -or $RollCompletion -or $GroupLifecycle){$nativeSources+=@('Handlers/LootHandler.cpp','Server/Packets/LootPackets.cpp')}
foreach($path in $nativeSources){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw "Actual game library predates $path; build Release first"}
}
if((Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item "$runtime/worldserver.exe").LastWriteTimeUtc){throw 'Canonical executable predates its game library'}
if($lootProbe -and $Baseline){
 $baselinePin=if($GroupLifecycle){'77d92ffe'}elseif($RollCompletion){'183d54fb'}elseif($MasterLoot){'d7c70c62'}else{'0fbd08e4'}
 $baselineLibrary="$output/game-baseline.lib"
 if(!(Test-Path $baselineLibrary)){
  if((& git -C $repo rev-parse --short=8 HEAD) -ne $baselinePin -or (& git -C $repo diff -- src/server/game)){throw "Baseline library must be captured from clean pinned$baselinePin game sources"}
  Copy-Item -LiteralPath $gameLib -Destination $baselineLibrary
  (Get-FileHash $baselineLibrary).Hash | Set-Content "$output/baseline-library.sha256"
 }
 if((Get-FileHash $baselineLibrary).Hash -ne (Get-Content "$output/baseline-library.sha256")){throw 'Pinned baseline library changed'}
 $libs=$libs | ForEach-Object {if($_ -eq $gameLib){$baselineLibrary}else{$_}}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$language=switch($group.ClCompile.LanguageStandard){'stdcpp14'{'/std:c++14'} 'stdcpp17'{'/std:c++17'} 'stdcpp20'{'/std:c++20'} 'stdcpplatest'{'/std:c++latest'} default{throw 'Unsupported generated native language standard'}}
if($group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Native runner expects the generated Release /MD runtime'}
$compilerArgs=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline -and $Offensive){$compilerArgs+='/DROCKAWAY_OFFENSIVE_BASELINE'}
if($Baseline -and $LostArmy){$compilerArgs+='/DLOST_ARMY_NATIVE_BASELINE'}
if($Baseline -and $MasterLoot){$compilerArgs+='/DMASTER_LOOT_NATIVE_BASELINE'}
if($Baseline -and $RollCompletion){$compilerArgs+='/DROLL_COMPLETION_NATIVE_BASELINE'}
if($Baseline -and $GroupLifecycle){$compilerArgs+='/DGROUP_ROLL_LIFECYCLE_BASELINE'}
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
if($GroupLifecycle){
 $connection=[regex]::Match($config,'(?m)^CharacterDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
 if($connection.Count -ne 5){throw 'Unexpected character connection format'}
 if($ScratchUser -notmatch '^codex_roll_[a-f0-9]{16}$' -or $ScratchPassword -notmatch '^[a-f0-9]{32}$'){throw 'Unexpected scratch-only account'}
 $connection[2]=$ScratchUser;$connection[3]=$ScratchPassword;$connection[4]=$CharacterScratch
 $config=[regex]::Replace($config,'(?m)^CharacterDatabaseInfo\s*=.*$',('CharacterDatabaseInfo = "'+($connection -join ';')+'"'))
 $auth=[regex]::Match($config,'(?m)^LoginDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
 if($auth.Count -ne 5){throw 'Unexpected auth connection format'}
 $auth[2]=$ScratchUser;$auth[3]=$ScratchPassword;$auth[4]=$AuthScratch
 $config=[regex]::Replace($config,'(?m)^LoginDatabaseInfo\s*=.*$',('LoginDatabaseInfo = "'+($auth -join ';')+'"'))
 $config+="`nNativeRollScratchDatabase = `"$CharacterScratch`"`nNativeRollScratchAuth = `"$AuthScratch`"`n"
}
[IO.File]::WriteAllText("$run/worldserver.conf",$config)
Push-Location "$repo/build-extractors/bin/Release"
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";$rows=& "$output/$stem.exe" "$run/worldserver.conf" "$repo/build-extractors/bin/Release/";$result=$LASTEXITCODE;$rows | Set-Content "$output/native-lines.txt";$rows | Where-Object {$_ -match '^PASS:'};if($result){$rows | Select-Object -Last 16;throw "Native execution failed (exit $result)"}}finally{$env:PATH=$previousPath;Pop-Location}
$run | Set-Content "$output/latest-run.txt"
if((Get-Item "$run/DBErrors.log").Length){Get-Content "$run/DBErrors.log" -Tail 8;throw 'Native channel test emitted DB/script errors'}
$hashes=@{}
$inputs=@(Get-ChildItem "$runtime/dbc/enUS/Spell*.db2")
$inputs+=@('Map','Difficulty','CreatureDisplayInfo','CreatureModelData','ChrClasses','Faction','FactionTemplate','SummonProperties','ConversationLine','BroadcastText','SoundKit','SoundKitEntry') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}
if($lootProbe){$inputs+=@('Item','ItemSparse','ItemEffect','ItemCurrencyCost','CurrencyTypes','QuestObjective','QuestV2CliTask','ChrRaces') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}}
$hashes['src/server/scripts/Legion/rockaway_coast.h']=(Get-FileHash "$repo/src/server/scripts/Legion/rockaway_coast.h").Hash
$hashes['tools/tests/RockawayChannelNative.cpp']=(Get-FileHash "$repo/tools/tests/RockawayChannelNative.cpp").Hash
$hashes['tools/tests/Test-RockawayChannelNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-RockawayChannelNative.ps1").Hash
if($Offensive){$hashes['tools/tests/RockawayOffensiveNative.cpp']=(Get-FileHash "$repo/tools/tests/RockawayOffensiveNative.cpp").Hash;$hashes['NativeRockawayQuest.inc']=(Get-FileHash "$output/NativeRockawayQuest.inc").Hash}
if($lootProbe){$hashes['tools/tests/LostArmyLootNative.cpp']=(Get-FileHash "$repo/tools/tests/LostArmyLootNative.cpp").Hash;$hashes['tools/tests/Test-LostArmyNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-LostArmyNative.ps1").Hash;$hashes['tools/tests/RockawayOffensiveNative.cpp']=(Get-FileHash "$repo/tools/tests/RockawayOffensiveNative.cpp").Hash;$hashes['NativeLostArmyQuest.inc']=(Get-FileHash "$output/NativeLostArmyQuest.inc").Hash}
if($MasterLoot -or $RollCompletion -or $GroupLifecycle){$hashes['tools/tests/MasterLootNative.cpp']=(Get-FileHash "$repo/tools/tests/MasterLootNative.cpp").Hash;$hashes['tools/tests/Test-MasterLootNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-MasterLootNative.ps1").Hash}
if($RollCompletion -or $GroupLifecycle){$hashes['tools/tests/RollCompletionNative.cpp']=(Get-FileHash "$repo/tools/tests/RollCompletionNative.cpp").Hash;$hashes['tools/tests/Test-RollCompletionNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-RollCompletionNative.ps1").Hash}
if($GroupLifecycle){$hashes['tools/tests/GroupRollLifecycleNative.cpp']=(Get-FileHash "$repo/tools/tests/GroupRollLifecycleNative.cpp").Hash;$hashes['tools/tests/Test-GroupRollLifecycleNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-GroupRollLifecycleNative.ps1").Hash}
$hashes['native-lines.txt']=(Get-FileHash "$output/native-lines.txt").Hash
$inputs+=Get-ChildItem "$runtime/gt" -File
foreach($inputFile in $inputs){$hashes[$inputFile.FullName.Substring($runtime.Length+1).Replace('\','/')]=(Get-FileHash $inputFile.FullName -Algorithm SHA256).Hash}
$hashes['actual-final-link-game.lib']=(Get-FileHash $gameLib -Algorithm SHA256).Hash
$hashes['canonical-worldserver.exe']=(Get-FileHash "$runtime/worldserver.exe" -Algorithm SHA256).Hash
if($Baseline -and $Offensive){$hashes['pinned-a788f5d2-rockaway-header']=(Get-FileHash "$output/RockawayBaseline.inc").Hash}
if($Baseline -and $lootProbe){$hashes["linked-pinned-$baselinePin-game.lib"]=(Get-FileHash $baselineLibrary).Hash}
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"
Copy-Item -LiteralPath "$output/native-inputs.json" -Destination "$run/native-inputs.json"
Copy-Item -LiteralPath "$output/native-lines.txt" -Destination "$run/native-lines.txt"
'PASS: actual final-link library/source freshness and relevant DB2/GT/library/executable hashes recorded. Private world pool is read-only; runtime configuration and personal progress are unchanged.'
