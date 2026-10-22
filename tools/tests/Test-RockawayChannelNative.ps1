param([switch]$CorpseRestart, [switch]$Offensive, [switch]$Baseline, [switch]$LostArmy, [switch]$MasterLoot, [switch]$RollCompletion, [switch]$GroupLifecycle, [switch]$QuestTurnIn, [switch]$RaidFragments, [switch]$TrialCriteria, [switch]$TrialTurnIn, [switch]$TrialInstance, [switch]$TrialExit, [switch]$TrialChest, [switch]$TrialReentry, [switch]$TrialGarm, [switch]$TrialHelya, [switch]$TrialHelyaPhase3, [switch]$TrialHelyaPhase1, [switch]$TrialHelyaTaint, [switch]$TrialHelyaGrimelord, [switch]$TrialHelyaMariner, [switch]$TrialHelyaControl, [switch]$TrialHelyaMistwatcher, [string]$CharacterScratch, [string]$AuthScratch, [string]$ScratchUser, [string]$ScratchPassword)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
if($LostArmy -and $Offensive){throw 'Select one native probe'}
if($MasterLoot -and ($LostArmy -or $Offensive)){throw 'Select one native probe'}
if($RollCompletion -and ($MasterLoot -or $LostArmy -or $Offensive)){throw 'Select one native probe'}
if($GroupLifecycle -and ($RollCompletion -or $MasterLoot -or $LostArmy -or $Offensive)){throw 'Select one native probe'}
if(($GroupLifecycle -or $QuestTurnIn -or $RaidFragments -or $TrialCriteria) -and $CharacterScratch -notmatch '^codex_roll_lifecycle_[a-f0-9]{16}$'){throw 'Use the lifecycle wrapper with an isolated scratch database'}
if(($GroupLifecycle -or $QuestTurnIn -or $RaidFragments -or $TrialCriteria) -and $AuthScratch -notmatch '^codex_roll_auth_[a-f0-9]{16}$'){throw 'Use the lifecycle wrapper with an isolated auth database'}
if($QuestTurnIn -and ($GroupLifecycle -or $RollCompletion -or $MasterLoot -or $LostArmy -or $Offensive)){throw 'Select one native probe'}
if($RaidFragments -and ($QuestTurnIn -or $GroupLifecycle -or $RollCompletion -or $MasterLoot -or $LostArmy -or $Offensive)){throw 'Select one native probe'}
$scratchProbe=$GroupLifecycle -or $QuestTurnIn -or $RaidFragments -or $TrialCriteria
$lootProbe=$LostArmy -or $MasterLoot -or $RollCompletion -or $GroupLifecycle -or $QuestTurnIn -or $RaidFragments -or $TrialCriteria
if($Baseline -and !$Offensive -and !$lootProbe){throw 'Select a supported native baseline'}
if($TrialCriteria -and ($RaidFragments -or $QuestTurnIn -or $GroupLifecycle -or $RollCompletion -or $MasterLoot -or $LostArmy -or $Offensive)){throw 'Select one current native probe'}
if($TrialTurnIn -and (!$TrialCriteria -or ($Baseline -and !$TrialInstance))){throw 'Select the current Trial turn-in wrapper'}
if($TrialInstance -and !$TrialTurnIn){throw 'Use the Trial instance wrapper'}
if($TrialExit -and (!$TrialInstance)){throw 'Use the Trial exit wrapper'}
if($TrialChest -and (!$TrialInstance -or $TrialExit)){throw 'Use the Trial chest wrapper'}
if($TrialGarm -and !$TrialReentry){throw 'Use the Trial Guarm wrapper'}
if($TrialHelya -and !$TrialGarm){throw 'Use the Trial Helya wrapper'}
if($TrialReentry -and (!$TrialInstance -or $TrialChest -or $TrialExit)){throw 'Use the Trial reentry wrapper'}
$stem=if($CorpseRestart){'CorpseRestartNative'}elseif($TrialHelyaControl){'TrialHelyaControlNative'}elseif($TrialHelyaMistwatcher){'TrialHelyaMistwatcherNative'}elseif($TrialHelyaMariner){'TrialHelyaMarinerNative'}elseif($TrialHelyaGrimelord){'TrialHelyaGrimelordNative'}elseif($TrialHelyaTaint){'TrialHelyaTaintNative'}elseif($TrialHelyaPhase1){'TrialHelyaPhase1Native'}elseif($TrialHelyaPhase3){'TrialHelyaPhase3Native'}elseif($TrialHelya){'TrialHelyaNative'}elseif($TrialGarm){'TrialGarmNative'}elseif($TrialReentry){'TrialRaidReentryNative'}elseif($TrialChest){'TrialRaidChestNative'}elseif($TrialExit){'TrialRaidExitNative'}elseif($TrialInstance){'TrialRaidInstanceNative'}elseif($TrialTurnIn){'TrialRaidTurnInNative'}elseif($TrialCriteria){'TrialRaidCriteriaNative'}elseif($RaidFragments){'RaidFragmentsNative'}elseif($QuestTurnIn){'LostArmyTurnInNative'}elseif($GroupLifecycle){'GroupRollLifecycleNative'}elseif($RollCompletion){'RollCompletionNative'}elseif($MasterLoot){'MasterLootNative'}elseif($LostArmy){'LostArmyLootNative'}elseif($Offensive){'RockawayOffensiveNative'}else{'RockawayChannelNative'}
$directory=if($CorpseRestart){'corpse-restart-native'}elseif($TrialHelyaControl){'trial-helya-control-native'}elseif($TrialHelyaMistwatcher){'trial-helya-mistwatcher-native'}elseif($TrialHelyaMariner){'trial-helya-mariner-native'}elseif($TrialHelyaGrimelord){'trial-helya-grimelord-native'}elseif($TrialHelyaTaint){'trial-helya-taint-native'}elseif($TrialHelyaPhase1){'trial-helya-phase1-native'}elseif($TrialHelyaPhase3){'trial-helya-phase3-native'}elseif($TrialHelya){'trial-helya-native'}elseif($TrialGarm){'trial-garm-native'}elseif($TrialReentry){'trial-raid-reentry-native'}elseif($TrialChest){'trial-raid-chest-native'}elseif($TrialExit){'trial-raid-exit-native'}elseif($TrialInstance){'trial-raid-instance-native'}elseif($TrialTurnIn){'trial-raid-turnin-native'}elseif($TrialCriteria){'trial-raid-criteria-native'}elseif($RaidFragments){'raid-fragments-native'}elseif($QuestTurnIn){'lost-army-turnin-native'}elseif($GroupLifecycle){'group-roll-lifecycle-native'}elseif($RollCompletion){'roll-completion-native'}elseif($MasterLoot){'master-loot-native'}elseif($LostArmy){'lost-army-native'}elseif($Offensive){'rockaway-offensive-native'}else{'rockaway-channel-native'}
$output=Join-Path $repo "build-extractors/runtime-test/$directory"
if($Baseline){$output+='-baseline'}
New-Item -ItemType Directory -Force -Path $output | Out-Null
if($Offensive -or $lootProbe){
 $questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
 $query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
 if(!$query){throw 'Native quest loader SELECT could not be extracted'}
 if($TrialTurnIn){[IO.File]::WriteAllText("$output/NativeLostArmyQuest.inc",$query+' " WHERE ID IN (44721,44729,44771,44868,44923,45088)"')}
 elseif($TrialCriteria){[IO.File]::WriteAllText("$output/NativeLostArmyQuest.inc",$query+' " WHERE ID IN (44729,44868,44923,45088)"')}
 elseif($lootProbe){[IO.File]::WriteAllText("$output/NativeLostArmyQuest.inc",$query+' " WHERE ID IN (44923,45088)"')}
 else{[IO.File]::WriteAllText("$output/NativeRockawayQuest.inc",$query+' " WHERE ID=44746"')}
 if($Baseline -and $Offensive){
  $oldHeader=& git -C $repo show 'a788f5d2:src/server/scripts/Legion/rockaway_coast.h'
  if($LASTEXITCODE){throw 'Pinned Rockaway baseline unavailable'}
  [IO.File]::WriteAllText("$output/RockawayBaseline.inc",($oldHeader -join "`n"))
 }
}
if($TrialTurnIn){
 $loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
 $a=$loader.IndexOf('        if (qinfo->PrevQuestID)');$b=$loader.IndexOf('        if (qinfo->LimitTime)',$a)
 $c=$loader.IndexOf('        if (qinfo->NextQuestIdChain)');$d=$loader.IndexOf('        for (uint8 j = 0; j < QUEST_REWARD_CURRENCY_COUNT;',$c)
 if($a -lt 0 -or $b -le $a -or $c -lt 0 -or $d -le $c){throw 'Native quest link loader boundaries changed'}
 [IO.File]::WriteAllText("$output/NativeTrialQuestLinks.inc",$loader.Substring($a,$b-$a)+$loader.Substring($c,$d-$c))
}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
if($TrialReentry){
 $calendar=if($Baseline){(& git -C $repo show '79705202:src/server/game/Handlers/CalendarHandler.cpp') -join "`n"}else{Get-Content "$repo/src/server/game/Handlers/CalendarHandler.cpp" -Raw}
 $method=$calendar.IndexOf('void WorldSession::HandleCalendarGetCalendar(')
 $a=$calendar.IndexOf('    for (', $calendar.IndexOf('packet.Events.push_back(eventInfo);',$method))
 $b=$calendar.IndexOf('    SendPacket(packet.Write());',$a)
 if($method -lt 0 -or $a -lt $method -or $b -le $a -or $calendar.Substring($a,$b-$a) -notmatch 'packet.RaidLockouts.push_back'){throw 'Native calendar bind enumeration boundaries changed'}
 [IO.File]::WriteAllText("$output/NativeCalendarBindings.inc",$calendar.Substring($a,$b-$a))
}
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$gameLib=$libs | Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'} | Select-Object -First 1
if(!$gameLib){throw 'Actual game final-link input is missing'}
$runtime=(Resolve-Path "$repo/build-extractors/bin/Release").Path
$nativeSources=@('AI/CoreAI/CombatAI.cpp','AI/CoreAI/UnitAI.cpp','AI/CreatureAI.cpp','AI/ScriptedAI/ScriptedCreature.cpp','Spells/SpellInfo.cpp','Spells/Spell.cpp','Spells/SpellEffects.cpp','Spells/Auras/SpellAuras.cpp','Spells/Auras/SpellAuraEffects.cpp','Entities/Creature/Creature.cpp','Entities/Creature/TemporarySummon.cpp','Entities/Unit/Unit.cpp','Maps/Map.cpp','Globals/ObjectMgr.cpp','Scripting/ScriptMgr.cpp','Globals/ScriptsData.cpp')
if($CorpseRestart -or $TrialHelyaControl){$nativeSources+='Entities/Corpse/Corpse.cpp'}
if($CorpseRestart){$nativeSources+=@('Globals/ObjectAccessor.cpp','Handlers/MiscHandler.cpp','Handlers/NPCHandler.cpp','Entities/Object/Object.cpp','Server/Packets/MiscPackets.cpp','Server/Packets/NPCPackets.cpp')}
if($TrialHelyaControl){$nativeSources+=@('Spells/SpellMgr.cpp','Handlers/NPCHandler.cpp','Server/WorldSocket.cpp','Server/WorldSession.h','Handlers/LoginQueryHolder.cpp','Handlers/MovementHandler.cpp','Server/Packets/MovementPackets.h','Server/Packets/MiscPackets.cpp','Server/Packets/WorldStatePackets.cpp','Server/Packets/ScenarioPackets.cpp','Server/Packets/AchievementPackets.cpp','Scenarios/Scenario.cpp','Maps/MapInstanced.cpp','Scenarios/ScenarioMgr.cpp','Entities/Player/PlayerScenario.cpp','Globals/ObjectMgr.h','Scenarios/ScenarioResume.h','Handlers/CharacterHandler.cpp','Server/Packets/CharacterPackets.cpp','World/World.cpp','../shared/Networking/Socket.h')}
if($Offensive -or $lootProbe){$nativeSources+=@('Entities/Player/Player.cpp','Entities/Unit/StatSystem.cpp','Conditions/ConditionMgr.cpp','Server/WorldSession.cpp','Server/Protocol/Opcodes.cpp','Globals/QuestData.cpp','Quests/QuestDef.cpp')}
if($lootProbe){$nativeSources+=@('Loot/LootMgr.cpp','Groups/Group.cpp','Entities/Player/KillRewarder.cpp','Entities/Item/Item.cpp')}
if($MasterLoot -or $RollCompletion -or $GroupLifecycle){$nativeSources+=@('Handlers/LootHandler.cpp','Server/Packets/LootPackets.cpp')}
if($QuestTurnIn){$nativeSources+=@('Handlers/QuestHandler.cpp','Entities/Player/CollectionMgr.cpp','Entities/Item/Container/Bag.cpp','Server/Packets/QuestPackets.cpp')}
if($RaidFragments){$nativeSources+=@('Entities/GameObject/GameObject.cpp','Instances/InstanceScript.cpp','Handlers/LootHandler.cpp')}
if($TrialReentry){$nativeSources+=@('Maps/MapInstanced.cpp','Maps/MapManager.cpp','Instances/InstanceSaveMgr.cpp','DataStores/DBCEnums.h','Entities/Player/Player.h','Groups/Group.h','Handlers/CalendarHandler.cpp','Server/Packets/CalendarPackets.cpp')}
if($TrialGarm){$nativeSources+=@('Entities/Vehicle/Vehicle.cpp','Entities/EventObject/EventObject.cpp','Globals/EventObjectData.cpp','Handlers/NPCHandler.cpp','Handlers/MovementHandler.cpp','Server/Packets/NPCPackets.cpp','Server/Packets/MovementPackets.cpp')}
if($TrialChest){$nativeSources+=@('Entities/Player/Player.h','Entities/GameObject/GameObject.cpp','Entities/Object/Object.cpp','Handlers/LootHandler.cpp','Handlers/SpellHandler.cpp','Server/Packets/GameObjectPackets.cpp')}
if($TrialExit){$nativeSources+=@('Handlers/NPCHandler.cpp','Handlers/MovementHandler.cpp','Server/Packets/NPCPackets.cpp','Globals/GossipData.cpp','Entities/Creature/GossipDef.cpp','Maps/MapManager.cpp')}
if($TrialCriteria){$nativeSources+=@('Achievements/AchievementMgr.cpp','Instances/InstanceScript.cpp','Handlers/QuestHandler.cpp','Server/Packets/QuestPackets.cpp')}
if($TrialCriteria){
 $trialScripts=$libs | Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'} | Select-Object -First 1
 if(!$trialScripts){throw 'Actual scripts final-link input missing'}
 $trialScriptPaths=@('boss_tov_odyn.cpp','boss_tov_helya.cpp','trial_of_valor.h')
 if($TrialInstance){$trialScriptPaths+=@('instance_trial_of_valor.cpp','trial_of_valor.cpp')};if($TrialGarm){$trialScriptPaths+='boss_tov_garm.cpp'}
 foreach($path in $trialScriptPaths){
  if((Get-Item "$repo/src/server/scripts/Legion/TrialOfValor/$path").LastWriteTimeUtc -gt (Get-Item $trialScripts).LastWriteTimeUtc){throw "Final-link scripts library predates $path"}
 }
 if((Get-Item $trialScripts).LastWriteTimeUtc -gt (Get-Item "$runtime/worldserver.exe").LastWriteTimeUtc){throw 'Canonical executable predates scripts library'}
}
foreach($path in $nativeSources){
 if(!($Baseline -and $lootProbe) -and (Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw "Actual game library predates $path; build Release first"}
}
if(!$Baseline -and (Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item "$runtime/worldserver.exe").LastWriteTimeUtc){throw 'Canonical executable predates its game library'}
if($lootProbe -and $Baseline){
 $baselinePin=if($TrialHelyaControl){'3f102023'}elseif($TrialHelyaMistwatcher){'da4fdfa8'}elseif($TrialHelyaMariner){'8422b836'}elseif($TrialHelyaGrimelord){'11c9c866'}elseif($TrialHelyaTaint){'48b57e05'}elseif($TrialHelyaPhase1){'1ef0e154'}elseif($TrialHelyaPhase3){'3f88a859'}elseif($TrialHelya){'174b0df5'}elseif($TrialGarm){'70419558'}elseif($TrialReentry){'79705202'}elseif($TrialChest){'f24b5547'}elseif($TrialExit){'9d0ea556'}elseif($TrialInstance){'69f70aeb'}elseif($TrialCriteria){'8b23c3c5'}elseif($RaidFragments){'9b0179a1'}elseif($QuestTurnIn){'197c304f'}elseif($GroupLifecycle){'77d92ffe'}elseif($RollCompletion){'183d54fb'}elseif($MasterLoot){'d7c70c62'}else{'0fbd08e4'}
 $baselineLibrary="$output/game-baseline.lib"
 if(!(Test-Path $baselineLibrary)){
  if((& git -C $repo rev-parse --short=8 HEAD) -ne $baselinePin -or (& git -C $repo diff -- src/server/game)){throw "Baseline library must be captured from clean pinned$baselinePin game sources"}
  Copy-Item -LiteralPath $gameLib -Destination $baselineLibrary
  (Get-FileHash $baselineLibrary).Hash | Set-Content "$output/baseline-library.sha256"
 }
 if((Get-FileHash $baselineLibrary).Hash -ne (Get-Content "$output/baseline-library.sha256")){throw 'Pinned baseline library changed'}
 $libs=$libs | ForEach-Object {if($_ -eq $gameLib){$baselineLibrary}else{$_}}
}
if($TrialInstance -and $Baseline){
 $oldScripts="$output/scripts-baseline.lib"
 if(!(Test-Path $oldScripts) -or (Get-FileHash $oldScripts).Hash -ne (Get-Content "$output/scripts-baseline.sha256")){throw 'Capture the pinned original scripts library before editing'}
 $libs=$libs | ForEach-Object {if($_ -eq $trialScripts){$oldScripts}else{$_}}
}
if($Baseline -and $lootProbe){
 # Historical libraries contain the old Player/Group bind-array layout.
 # Keep current enum definitions, but compile the fixture with that pinned
 # array bound. Mixing a three-slot game library with four-slot headers (or
 # newly rebuilt scripts) would test invalid object offsets.
 $pinnedEnums=(& git -C $repo show "${baselinePin}:src/server/game/DataStores/DBCEnums.h") -join "`n"
 if($LASTEXITCODE){throw 'Pinned native ABI header unavailable'}
 $bound=[regex]::Match($pinnedEnums,'const uint8 MAX_BOUND = (\d+);').Groups[1].Value
 if(!$bound){throw 'Pinned bind-array ABI missing'}
 $abiHeader=[regex]::Replace((Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw),'const uint8 MAX_BOUND = \d+;',"const uint8 MAX_BOUND = $bound;")
 [IO.File]::WriteAllText("$output/BaselineDBCEnums.h",$abiHeader)
 if(!$TrialInstance){
  $abiScripts="$repo/build-extractors/runtime-test/native-abi-79705202/scripts.lib"
  if(!(Test-Path $abiScripts) -or (Get-FileHash $abiScripts).Hash -ne (Get-Content "$repo/build-extractors/runtime-test/native-abi-79705202/scripts.sha256")){throw 'Capture the pre-bind-layout79705202 scripts library before rebuilding'}
  $libs=$libs | ForEach-Object {if($_ -match 'scripts[\\/]Release[\\/]scripts.lib$'){$abiScripts}else{$_}}
 }
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$language=switch($group.ClCompile.LanguageStandard){'stdcpp14'{'/std:c++14'} 'stdcpp17'{'/std:c++17'} 'stdcpp20'{'/std:c++20'} 'stdcpplatest'{'/std:c++latest'} default{throw 'Unsupported generated native language standard'}}
if($group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Native runner expects the generated Release /MD runtime'}
$compilerArgs=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline -and $lootProbe){$compilerArgs+='/FI"'+$output+'/BaselineDBCEnums.h"'}
if($Baseline -and $Offensive){$compilerArgs+='/DROCKAWAY_OFFENSIVE_BASELINE'}
if($Baseline -and $LostArmy){$compilerArgs+='/DLOST_ARMY_NATIVE_BASELINE'}
if($Baseline -and $MasterLoot){$compilerArgs+='/DMASTER_LOOT_NATIVE_BASELINE'}
if($Baseline -and $RollCompletion){$compilerArgs+='/DROLL_COMPLETION_NATIVE_BASELINE'}
if($Baseline -and $GroupLifecycle){$compilerArgs+='/DGROUP_ROLL_LIFECYCLE_BASELINE'}
if($Baseline -and $QuestTurnIn){$compilerArgs+='/DLOST_ARMY_TURNIN_BASELINE'}
if($Baseline -and $RaidFragments){$compilerArgs+='/DRAID_FRAGMENTS_BASELINE'}
if($Baseline -and $TrialCriteria){$compilerArgs+='/DTRIAL_CRITERIA_BASELINE'}
if($Baseline -and $TrialHelyaControl){$compilerArgs+='/DTRIAL_HELYA_CONTROL_BASELINE'}elseif($Baseline -and $TrialHelyaMistwatcher){$compilerArgs+='/DTRIAL_HELYA_MISTWATCHER_BASELINE'}elseif($Baseline -and $TrialHelyaMariner){$compilerArgs+='/DTRIAL_HELYA_MARINER_BASELINE'}elseif($Baseline -and $TrialHelyaGrimelord){$compilerArgs+='/DTRIAL_HELYA_GRIMELORD_BASELINE'}elseif($Baseline -and $TrialHelyaTaint){$compilerArgs+='/DTRIAL_HELYA_TAINT_BASELINE'}elseif($Baseline -and $TrialHelyaPhase1){$compilerArgs+='/DTRIAL_HELYA_PHASE1_BASELINE'}elseif($Baseline -and $TrialHelyaPhase3){$compilerArgs+='/DTRIAL_HELYA_PHASE3_BASELINE'}elseif($Baseline -and $TrialHelya){$compilerArgs+='/DTRIAL_HELYA_BASELINE'}elseif($Baseline -and $TrialGarm){$compilerArgs+='/DTRIAL_GARM_BASELINE'}elseif($Baseline -and $TrialReentry){$compilerArgs+='/DTRIAL_REENTRY_BASELINE'}elseif($Baseline -and $TrialChest){$compilerArgs+='/DTRIAL_CHEST_BASELINE'}elseif($Baseline -and $TrialExit){$compilerArgs+='/DTRIAL_EXIT_BASELINE'}elseif($Baseline -and $TrialInstance){$compilerArgs+='/DTRIAL_INSTANCE_BASELINE'}
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
if($scratchProbe){
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
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH";$rows=@()
 $phases=if($CorpseRestart){@('write','read')}else{@('run')}
 foreach($phase in $phases){
  $phaseRows=@(& "$output/$stem.exe" "$run/worldserver.conf" "$repo/build-extractors/bin/Release/");$result=$LASTEXITCODE
  $phaseRows | Set-Content "$run/$phase-lines.txt";$rows+=$phaseRows;$rows | Set-Content "$output/native-lines.txt"
  $phaseRows | Where-Object {$_ -match '^PASS:'}
  if($result){$phaseRows | Select-Object -Last 16;throw "Native execution failed ($phase exit $result)"}
 }
}finally{$env:PATH=$previousPath;Pop-Location}
$run | Set-Content "$output/latest-run.txt"
if((Get-Item "$run/DBErrors.log").Length){Get-Content "$run/DBErrors.log" -Tail 8;throw 'Native channel test emitted DB/script errors'}
if($TrialHelya -and (Select-String -LiteralPath "$run/Server.log" -Pattern '\bERROR\s+\[')){throw 'Native Helya probe emitted a server error; resolve fixture/runtime prerequisites before accepting the result'}
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
if($QuestTurnIn){$hashes['tools/tests/LostArmyTurnInNative.cpp']=(Get-FileHash "$repo/tools/tests/LostArmyTurnInNative.cpp").Hash;$hashes['tools/tests/Test-GroupRollLifecycleNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-GroupRollLifecycleNative.ps1").Hash;$inputs+=@('QuestPackageItem','ItemSpec','ItemSpecOverride','ChrSpecialization','TransmogSet','TransmogSetItem','ItemModifiedAppearance') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}}
if($TrialCriteria){foreach($path in @('TrialRaidCriteriaNative.cpp','LostArmyTurnInNative.cpp','Test-TrialRaidCriteriaNative.ps1','Test-GroupRollLifecycleNative.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash};$inputs+=@('Criteria','CriteriaTree','ModifierTree','DungeonEncounter','Achievement') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}}
if($RaidFragments){foreach($path in @('RaidFragmentsNative.cpp','LostArmyTurnInNative.cpp','Test-RaidFragmentsNative.ps1','Test-GroupRollLifecycleNative.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash};$inputs+=@('GameObjects','DungeonEncounter') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}}
if($TrialCriteria){
 $hashes['actual-final-link-scripts.lib']=(Get-FileHash $trialScripts).Hash
 $trialScriptPaths=@('boss_tov_odyn.cpp','boss_tov_helya.cpp','trial_of_valor.h')
 if($TrialInstance){$trialScriptPaths+=@('instance_trial_of_valor.cpp','trial_of_valor.cpp')};if($TrialGarm){$trialScriptPaths+='boss_tov_garm.cpp'}
 foreach($path in $trialScriptPaths){$hashes["src/server/scripts/Legion/TrialOfValor/$path"]=(Get-FileHash "$repo/src/server/scripts/Legion/TrialOfValor/$path").Hash}
}
if($TrialTurnIn){
 $hashes['tools/tests/TrialRaidTurnInNative.cpp']=(Get-FileHash "$repo/tools/tests/TrialRaidTurnInNative.cpp").Hash
 $hashes['tools/tests/Test-TrialRaidTurnInNative.ps1']=(Get-FileHash "$repo/tools/tests/Test-TrialRaidTurnInNative.ps1").Hash
 $hashes['NativeTrialQuestLinks.inc']=(Get-FileHash "$output/NativeTrialQuestLinks.inc").Hash
}
if($TrialInstance){
 foreach($path in @('TrialRaidInstanceNative.cpp','Test-TrialRaidInstanceNative.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash}
 if($Baseline){$hashes["linked-pinned-$baselinePin-scripts.lib"]=(Get-FileHash $oldScripts).Hash}
}
if($TrialExit){
 foreach($path in @('TrialRaidExitNative.cpp','Test-TrialRaidExitNative.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path" ).Hash}
 $inputs+=Get-Item "$runtime/dbc/enUS/WorldSafeLocs.db2"
}
if($TrialChest){
 foreach($path in $nativeSources){$hashes["src/server/game/$path"]=(Get-FileHash "$repo/src/server/game/$path").Hash}
 foreach($path in @('TrialRaidChestNative.cpp','Test-TrialRaidChestNative.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path" ).Hash}
 $inputs+=@('Lock','GameObjects','GameObjectDisplayInfo') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"}
}
if($TrialReentry){
 $hashes['NativeCalendarBindings.inc']=(Get-FileHash "$output/NativeCalendarBindings.inc").Hash
 foreach($path in $nativeSources){$hashes["src/server/game/$path"]=(Get-FileHash "$repo/src/server/game/$path").Hash}
 foreach($path in @('TrialRaidReentryNative.cpp','Test-TrialRaidReentryNative.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash}
 $inputs+=Get-Item "$runtime/dbc/enUS/MapDifficulty.db2"
}
if($TrialGarm){$inputs+=Get-Item "$runtime/dbc/enUS/WorldSafeLocs.db2";foreach($path in @('TrialGarmNative.cpp','Test-TrialGarmNative.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash}}
if($TrialHelya){foreach($path in @('TrialHelyaNative.cpp','Test-TrialHelyaNative.ps1','Test-TrialHelyaFuryRelease.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash}}
if($TrialHelyaPhase1){foreach($path in @('TrialHelyaPhase1Native.cpp','Test-TrialHelyaPhase1Native.ps1','Test-TrialHelyaPoolData.ps1','Test-TrialHelyaPoolRelease.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path" ).Hash};$hashes['sql/updates/world/2026_10_19_01_helya_bilewater_pool_scale.sql']=(Get-FileHash "$repo/sql/updates/world/2026_10_19_01_helya_bilewater_pool_scale.sql").Hash}
if($TrialHelyaTaint){foreach($path in @('TrialHelyaTaintNative.cpp','Test-TrialHelyaTaintNative.ps1','Test-TrialHelyaTaintRelease.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path" ).Hash}}
if($TrialHelyaControl -or $TrialHelyaMistwatcher -or $TrialHelyaMariner -or $TrialHelyaGrimelord -or $TrialHelyaTaint -or $TrialHelyaPhase1 -or $TrialHelyaPhase3){foreach($path in @('TrialHelyaPhase3Native.cpp','Test-TrialHelyaPhase3Native.ps1','Test-TrialHelyaBreathRelease.ps1','Test-TrialHelyaCorrosionData.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash}}
if($TrialHelyaGrimelord){$hashes['sql/updates/world/2026_10_19_02_helya_fetid_rot.sql']=(Get-FileHash "$repo/sql/updates/world/2026_10_19_02_helya_fetid_rot.sql").Hash;foreach($path in @('TrialHelyaGrimelordNative.cpp','Test-TrialHelyaGrimelordNative.ps1','Test-TrialHelyaGrimelordData.ps1','Test-TrialHelyaGrimelordRelease.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash}}
if($TrialHelyaMariner){foreach($path in @('TrialHelyaMarinerNative.cpp','Test-TrialHelyaMarinerNative.ps1','TrialHelyaGrimelordNative.cpp')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path" ).Hash}}
if($TrialHelyaMistwatcher){foreach($path in @('TrialHelyaMistwatcherNative.cpp','Test-TrialHelyaMistwatcherNative.ps1','Test-TrialHelyaMistwatcherRelease.ps1','TrialHelyaGrimelordNative.cpp')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path" ).Hash}}
if($TrialHelyaControl){foreach($path in @('TrialHelyaControlNative.cpp','TrialHelyaControlLoginNative.inc','TrialGraveyardNative.inc','TrialGhostReturnNative.inc','TrialGhostInstanceNative.inc','TrialGhostUnloadedNative.inc','TrialSpiritGraveyardNative.inc','TrialSpiritFarNative.inc','TrialSpiritFailureNative.inc','TrialSpiritLoginNative.inc','TrialSpiritSessionNative.inc','TrialSpiritQueryNative.inc','TrialSpiritInstanceLinkNative.inc','TrialSpiritLinkedCancelNative.inc','TrialSpiritEarlyLoadingNative.inc','TrialSpiritMapLoadingNative.inc','TrialSpiritLoadingOrderNative.inc','TrialScenarioInitializationNative.inc','TrialScenarioRelogNative.inc','TrialScenarioReconnectNative.inc','TrialScenarioGroupNative.inc','ScenarioResumeTest.cpp','Test-ScenarioResume.ps1','Test-ScenarioInitializationRelease.ps1','Test-SpiritGraveyardRelease.ps1','Test-GhostReturnRelease.ps1','Test-TrialHelyaControlNative.ps1','Test-TrialHelyaControlRelease.ps1','Test-TrialHelyaControlLoginRelease.ps1','Test-LogoutTeleportRelease.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash};$inputs+=@('AreaTable','Scenario','ScenarioStep','LFGDungeons') | ForEach-Object {Get-Item "$runtime/dbc/enUS/$_.db2"};$hashes['tools/research/ScenarioInitializationCatalog.cpp']=(Get-FileHash "$repo/tools/research/ScenarioInitializationCatalog.cpp").Hash;$hashes['sql/updates/world/2026_10_20_13_trial_of_valor_graveyard.sql']=(Get-FileHash "$repo/sql/updates/world/2026_10_20_13_trial_of_valor_graveyard.sql").Hash}
if($CorpseRestart){
 foreach($path in @('CorpseRestartNative.cpp','CorpseReclaimNative.inc','SpiritHealerNative.inc','Test-CorpseRestartNative.ps1','Test-CorpseRestartRelease.ps1','Test-SpiritHealerRelease.ps1')){$hashes["tools/tests/$path"]=(Get-FileHash "$repo/tools/tests/$path").Hash}
 foreach($path in @('src/server/game/Globals/ObjectAccessor.cpp','src/server/game/World/World.cpp','src/server/game/Handlers/NPCHandler.cpp','src/server/database/Database/Implementation/CharacterDatabase.cpp')){$hashes[$path]=(Get-FileHash "$repo/$path").Hash}
}
$hashes['native-lines.txt']=(Get-FileHash "$output/native-lines.txt").Hash
$inputs+=Get-ChildItem "$runtime/gt" -File
foreach($inputFile in $inputs){$hashes[$inputFile.FullName.Substring($runtime.Length+1).Replace('\','/')]=(Get-FileHash $inputFile.FullName -Algorithm SHA256).Hash}
$hashes['actual-final-link-game.lib']=(Get-FileHash $gameLib -Algorithm SHA256).Hash
$hashes['canonical-worldserver.exe']=(Get-FileHash "$runtime/worldserver.exe" -Algorithm SHA256).Hash
if($Baseline -and $Offensive){$hashes['pinned-a788f5d2-rockaway-header']=(Get-FileHash "$output/RockawayBaseline.inc").Hash}
if($Baseline -and $lootProbe){$hashes["linked-pinned-$baselinePin-game.lib"]=(Get-FileHash $baselineLibrary).Hash}
if($Baseline -and $lootProbe){$hashes['BaselineDBCEnums.h']=(Get-FileHash "$output/BaselineDBCEnums.h").Hash;if($abiScripts){$hashes['linked-ABI-compatible-79705202-scripts.lib']=(Get-FileHash $abiScripts).Hash}}
$hashes | ConvertTo-Json | Set-Content "$output/native-inputs.json"
Copy-Item -LiteralPath "$output/native-inputs.json" -Destination "$run/native-inputs.json"
Copy-Item -LiteralPath "$output/native-lines.txt" -Destination "$run/native-lines.txt"
'PASS: actual final-link library/source freshness and relevant DB2/GT/library/executable hashes recorded. Private world pool is read-only; runtime configuration and personal progress are unchanged.'
