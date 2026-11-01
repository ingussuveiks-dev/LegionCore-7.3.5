param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-pickup-native")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $source=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
 $query=[regex]::Match($source,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
 if(!$query){throw 'Actual native quest query missing'}
 [IO.File]::WriteAllText("$out/NativeFuZanQuest.inc",$query+' " WHERE ID=42765"')
 [IO.File]::WriteAllText("$out/NativeLostArmyQuest.inc",$query+' " WHERE ID IN(44923,45088)"')
 $source=[IO.File]::ReadAllText("$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp")
 $start=$source.IndexOf('class scene_monk_artifact_equipment');$end=$source.IndexOf('class scene_xalatath_pickup',$start)
 if($start -lt 0 -or $end -le $start){throw 'Actual Monk scene constructor missing'}
 [IO.File]::WriteAllText("$out/NativeMonkScene.inc",$source.Substring($start,$end-$start))
 $source=[IO.File]::ReadAllText("$PSScriptRoot/SheilunAcquisitionNative.cpp")
 $start=$source.IndexOf('struct NativeReturnSession');$end=$source.IndexOf('static void CheckReturn(',$start)
 if($start -lt 0 -or $end -le $start){throw 'Private native map/session fixture missing'}
 [IO.File]::WriteAllText("$out/NativeReturnSession.inc",$source.Substring($start,$end-$start))
 $inputs=@($PSCommandPath,"$PSScriptRoot/FuZanPickupNative.cpp","$PSScriptRoot/FuZanWaveEntryNative.cpp","$PSScriptRoot/FuZanBelphiarNative.cpp","$PSScriptRoot/FuZanTempleIntroNative.cpp","$PSScriptRoot/MasterLootNative.cpp","$PSScriptRoot/LostArmyLootNative.cpp","$PSScriptRoot/RockawayOffensiveNative.cpp","$PSScriptRoot/RockawayChannelNative.cpp",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_belphiar.h",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_summerpetal.h",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_wave_entry.h","$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_victory_recipient.h",
 "$repo/src/server/game/Entities/Object/QuestObjectScope.h","$repo/src/server/game/AI/SmartScripts/SmartAI.h",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_temple_intro.h",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_temple_sanctuary_door.h",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/instance_temple_of_the_jade_serpent.cpp",
 "$repo/src/server/game/Entities/Unit/Unit.cpp","$repo/src/server/game/Entities/Creature/Creature.cpp",
 "$repo/src/server/game/Entities/GameObject/GameObject.cpp","$repo/src/server/game/Entities/Player/Player.cpp",
 "$repo/src/server/game/Entities/EventObject/EventObject.cpp","$repo/src/server/game/Globals/ConversationData.cpp","$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp",
 "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp","$repo/src/server/game/AI/SmartScripts/SmartScript.cpp",
 "$repo/src/server/game/Scenarios/Scenario.cpp","$repo/src/server/game/Spells/Spell.cpp","$repo/src/server/game/Spells/SpellEffects.cpp",
 "$out/NativeFuZanQuest.inc","$out/NativeLostArmyQuest.inc","$PSScriptRoot/FuZanDefenseNative.cpp","$PSScriptRoot/SheilunAcquisitionNative.cpp","$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp","$repo/src/server/game/Handlers/SceneHandler.cpp","$repo/src/server/game/Handlers/NPCHandler.cpp","$repo/src/server/game/Handlers/LootHandler.cpp","$out/NativeMonkScene.inc","$out/NativeReturnSession.inc")
 $inputs+=@("$repo/src/server/game/Maps/PhaseMgr.cpp","$repo/src/server/game/Conditions/ConditionMgr.cpp","$repo/src/server/game/Globals/ObjectMgr.cpp","$repo/src/server/game/Entities/Object/Object.cpp")
 $hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash -LiteralPath $path).Hash}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
 $includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
 $libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
 $game=$libs|Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'}|Select-Object -First 1
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$game -or !$scripts -or $group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Native Release ABI/final-link inputs differ'}
 $gameHash=(Get-FileHash $game).Hash;$scriptsHash=(Get-FileHash $scripts).Hash
 $language=switch($group.ClCompile.LanguageStandard){'stdcpp17'{'/std:c++17'}'stdcpp20'{'/std:c++20'}'stdcpplatest'{'/std:c++latest'}default{throw 'Native C++ language standard differs'}}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $extra=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include')|ForEach-Object {Join-Path $repo $_}
 $args=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB','/DFUZAN_INTRO_LIVE')
 $args+=@('/DFUZAN_BELPHIAR_LIVE','/DFUZAN_SUMMERPETAL_LIVE','/DFUZAN_WAVE_ENTRY_LIVE');$args+='/DFUZAN_DEFENSE_CRITERIA_LIVE'
 $args+=($includes+$extra+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)|ForEach-Object {'/I"'+$_+'"'})
 $args+='"'+$PSScriptRoot+'/FuZanPickupNative.cpp"'
 $args+=@('/Fe"'+$out+'/probe.exe"','/Fo"'+$out+'/probe.obj"','/link','/DEBUG','/MAP:"'+$out+'/probe.map"','/PDB:"'+$out+'/probe.pdb"')
 $args+=($libs|ForEach-Object {'"'+$_+'"'})
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n")
 & "$out/build.cmd" *> "$out/build.log";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 15;throw 'Native pickup build failed'}
 $runtime="$repo/build-extractors/bin/Release";$run="$out/run-$(Get-Date -Format yyyyMMdd-HHmmss)"
 New-Item -ItemType Directory -Force "$run/logs"|Out-Null
 $config=[regex]::Replace([IO.File]::ReadAllText("$runtime/worldserver.conf"),'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'/logs"'))
 $config=[regex]::Replace($config,'(?m)^Logger\.sql\.sql\s*=.*$','Logger.sql.sql=4,Console DBErrors')
 [IO.File]::WriteAllText("$run/worldserver.conf",$config);$prior=$env:PATH
 try{$env:PATH="$runtime;$prior";& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/native-diagnostics" -RuntimeDirectories @($run) -Test {
  Push-Location $runtime
  try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$code=$LASTEXITCODE}finally{Pop-Location}
  @{exit=$code;stdout=(Get-FileHash "$run/stdout.log").Hash;stderr=(Get-FileHash "$run/stderr.log").Hash}|ConvertTo-Json|Set-Content "$run/process-result.json"
  Get-Content "$run/stdout.log"|Where-Object {$_ -match '^(CASE|PASS|EXPECTED_BROKEN)'}
  if($code -or (Get-Item "$run/stderr.log").Length){Get-Content "$run/stderr.log" -Tail 20;throw "Native pickup exit=$code or stderr"}
  $lines=@(Get-Content "$run/stdout.log")
  if(@($lines|Where-Object {$_ -match '^CASE native pickup .* PASS$'}).Count -ne 22 -or @($lines|Where-Object {$_ -match '^PASS22 native pickup/scene/equipment/return cases\.'}).Count -ne 1){throw 'Native pickup case count/footer differs'}
  if((Test-Path "$run/logs/DBErrors.log") -and (Get-Item "$run/logs/DBErrors.log").Length){throw 'Native pickup emitted DBErrors'}
 }}finally{$env:PATH=$prior}
 foreach($path in $inputs){if((Get-FileHash -LiteralPath $path).Hash -ne $hashes[$path]){throw 'Native pickup input changed during execution'}}
 if((Get-FileHash $game).Hash -ne $gameHash -or (Get-FileHash $scripts).Hash -ne $scriptsHash){throw 'Native final-link input changed during execution'}
 @{inputs=$hashes;game=$gameHash;scripts=$scriptsHash;executable=(Get-FileHash "$out/probe.exe").Hash;run=$run;native_core_release_not_instrumented=$true;live_data=$true}|ConvertTo-Json -Depth 4|Set-Content "$out/native-inputs.json"
 'PASS actual native Fu Zan pickup/scene/equipment/return execution and diagnostics; read-only pools, earlier entry/stage2 controlled, no authenticated save/client traversal'
}
