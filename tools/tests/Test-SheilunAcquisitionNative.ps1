param([switch]$Baseline,[switch]$VerifyLiveData,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-acquisition-native$(if($Baseline){'-baseline'})"}
$out=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){
 $at=$source.IndexOf($start);if($at -lt 0){throw "Missing $start"}
 $end=$source.IndexOf('{',$at)+1;$depth=1
 while($depth -and $end -lt $source.Length){$depth+=([int]($source[$end] -eq '{'))-([int]($source[$end] -eq '}'));++$end}
 if($depth){throw "Unclosed $start"};$source.Substring($at,$end-$at)
}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp" -Raw
[IO.File]::WriteAllText("$out/SheilunGo.inc",(Block $source 'class go_art_sheylun :')+';')
$priorSource=(& git -C $repo show ab99b2d2d:src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp) -join "`n"
if($LASTEXITCODE){throw 'Pinned actual pre-acquisition death source missing'}
[IO.File]::WriteAllText("$out/PinnedSheilunDeath.inc",(Block (Block $priorSource 'class npc_aspersiy :') 'void JustDied('))
$questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Missing actual native quest query'}
[IO.File]::WriteAllText("$out/NativeSheilunQuest.inc",$query+' " WHERE ID=41003"')
[IO.File]::WriteAllText("$out/NativeLostArmyQuest.inc",$query+' " WHERE ID IN(44923,45088)"')
$playerSource=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$saveBody=Block $playerSource 'void Player::SaveToDB('
$updateBody=$saveBody.Substring($saveBody.IndexOf('// Update query'))
$savePosition=Block $updateBody 'if (!IsBeingTeleported())'
$saveElse=Block $updateBody.Substring($updateBody.IndexOf($savePosition)+$savePosition.Length) 'else'
[IO.File]::WriteAllText("$out/NativeSheilunSavePosition.inc",$savePosition+"`n"+$saveElse)
$sessionSource=Get-Content "$repo/src/server/game/Server/WorldSession.cpp" -Raw
$logoutBody=Block $sessionSource 'void WorldSession::LogoutPlayer('
$before=$logoutBody.IndexOf('sScriptMgr->OnPlayerBeforeLogout(_player)');$saveAt=$logoutBody.IndexOf('_player->SaveToDB()');$after=$logoutBody.IndexOf('sScriptMgr->OnPlayerLogout(_player)')
if($before -lt 0 -or $before -ge $saveAt -or $saveAt -ge $after){throw 'Native logout must dispatch before-save and retain its old after-save notification'}
$pinnedSession=(& git -C $repo show c3c6e4f8c:src/server/game/Server/WorldSession.cpp) -join "`n"
if($LASTEXITCODE){throw 'Pinned pre-hook logout source missing'}
$oldLogout=Block $pinnedSession 'void WorldSession::LogoutPlayer('
if($oldLogout.Contains('OnPlayerBeforeLogout') -or $oldLogout.IndexOf('_player->SaveToDB()') -ge $oldLogout.IndexOf('sScriptMgr->OnPlayerLogout(_player)')){throw 'Pinned pre-hook save-order counterfactual changed'}
$currentApi=Block (Get-Content "$repo/src/server/game/Scripting/ScriptMgr.h" -Raw) 'class TC_GAME_API PlayerScript :'
$pinnedApi=(& git -C $repo show c3c6e4f8c:src/server/game/Scripting/ScriptMgr.h) -join "`n"
if($LASTEXITCODE){throw 'Pinned PlayerScript ABI missing'}
$oldApi=Block $pinnedApi 'class TC_GAME_API PlayerScript :'
$withoutHook=$currentApi -replace '(?s)\s*// Logout has begun; called before the final character save\..*?virtual void OnBeforeLogout\(Player\* /\*player\*/\) \{ \}',''
if(($withoutHook -replace '\s','') -cne ($oldApi -replace '\s','') -or $currentApi.LastIndexOf('virtual ') -ne $currentApi.IndexOf('virtual void OnBeforeLogout')){throw 'Before-logout hook must append without changing existing PlayerScript slots'}
$migrationPath="$repo/sql/pending/world/2026_11_01_01_sheilun_manual_acquisition.sql"
if(!(Test-Path $migrationPath)){$migrationPath="$repo/sql/updates/world/2026_11_01_01_sheilun_manual_acquisition.sql"}
$inputPaths=@($PSCommandPath,"$PSScriptRoot/SheilunAcquisitionNative.cpp","$PSScriptRoot/MasterLootNative.cpp","$PSScriptRoot/LostArmyLootNative.cpp","$PSScriptRoot/RockawayOffensiveNative.cpp","$PSScriptRoot/RockawayChannelNative.cpp",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp","$repo/src/server/game/Spells/SpellScript.cpp","$repo/src/server/game/Spells/SpellScript.h","$repo/src/server/game/Maps/Map.cpp",
 "$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/Entities/Player/PlayerScenario.cpp","$repo/src/server/game/Scripting/ScriptMgr.h","$repo/src/server/game/Scripting/ScriptMgr.cpp","$repo/src/server/game/Server/WorldSession.cpp",
 $migrationPath,"$out/SheilunGo.inc","$out/NativeSheilunQuest.inc","$out/NativeLostArmyQuest.inc","$out/PinnedSheilunDeath.inc","$out/NativeSheilunSavePosition.inc")+
 @(Get-ChildItem "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun" -Filter '*.h' -File|Select-Object -ExpandProperty FullName)
$inputHashes=[ordered]@{}
foreach($inputPath in $inputPaths){$inputHashes[[IO.Path]::GetFullPath($inputPath)]=(Get-FileHash -LiteralPath $inputPath).Hash}
foreach($inputPath in $inputHashes.Keys){
 $relative=if($inputPath.StartsWith($out+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){'generated/'+[IO.Path]::GetFileName($inputPath)}else{[IO.Path]::GetRelativePath($repo,$inputPath)}
 $snapshot=Join-Path "$out/input-snapshot" $relative
 New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($snapshot))|Out-Null
 Copy-Item -LiteralPath $inputPath -Destination $snapshot
 if((Get-FileHash -LiteralPath $snapshot).Hash -ne $inputHashes[$inputPath]){throw 'Native input snapshot differs before compilation'}
}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
$libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$game=$libs|Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'}|Select-Object -First 1
$scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
if(!$game -or !$scripts){throw 'Missing original final-link libraries'}
$gameHash=(Get-FileHash $game).Hash;$scriptsHash=(Get-FileHash $scripts).Hash
if($group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Native Release ABI must be /MD'}
$language=switch($group.ClCompile.LanguageStandard){'stdcpp17'{'/std:c++17'}'stdcpp20'{'/std:c++20'}'stdcpplatest'{'/std:c++latest'}default{throw 'Review native language standard'}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$extra=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include')|ForEach-Object {Join-Path $repo $_}
$args=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline){$args+='/DSHEILUN_SOURCE_BASELINE'}
if($VerifyLiveData){$args+='/DSHEILUN_LIVE_DATA'}
$args+=($includes+$extra+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)|ForEach-Object {'/I"'+$_+'"'})
$args+='"'+$PSScriptRoot+'/SheilunAcquisitionNative.cpp"'
$args+=@('/Fe"'+$out+'/probe.exe"','/Fo"'+$out+'/probe.obj"','/link','/DEBUG','/MAP:"'+$out+'/probe.map"','/PDB:"'+$out+'/probe.pdb"')
$args+=($libs|ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build.cmd" *> "$out/build.log"
if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 20;throw 'Native acquisition build failed'}
$runtime="$repo/build-extractors/bin/Release"
$run="$out/run-$(Get-Date -Format yyyyMMdd-HHmmss)"
New-Item -ItemType Directory -Force "$run/logs"|Out-Null
$config=Get-Content "$runtime/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$run/worldserver.conf",$config)
$prior=$env:PATH
try{
 $env:PATH="$runtime;$prior"
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/native-diagnostics-$(Get-Date -Format yyyyMMdd-HHmmss)" -RuntimeDirectories @($run) -Test {
  Push-Location $runtime
  try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$exit=$LASTEXITCODE}finally{Pop-Location}
  Get-Content "$run/stdout.log"|Where-Object {$_ -match '^(CASE|PASS|EXPECTED_BROKEN)'}
  if($exit -or (Get-Item "$run/stderr.log").Length){Get-Content "$run/stderr.log" -Tail 20;throw "Native acquisition exit=$exit or stderr"}
  if((Test-Path "$run/logs/DBErrors.log") -and (Get-Item "$run/logs/DBErrors.log").Length){throw 'Native acquisition emitted DBErrors; inspect retained evidence'}
 }
}finally{$env:PATH=$prior}
foreach($inputPath in $inputHashes.Keys){if((Get-FileHash -LiteralPath $inputPath).Hash -ne $inputHashes[$inputPath]){throw "Native compilation input changed during the test: $inputPath"}}
if((Get-FileHash $game).Hash -ne $gameHash -or (Get-FileHash $scripts).Hash -ne $scriptsHash){throw 'Native final-link library changed during the test'}
@{baseline=[bool]$Baseline;live_world_data_no_production_overrides=[bool]$VerifyLiveData;native_core_release_not_instrumented=$true;game=$gameHash;scripts=$scriptsHash;inputs=$inputHashes;executable=(Get-FileHash "$out/probe.exe").Hash;run=$run}|ConvertTo-Json -Depth 4|Set-Content "$out/native-inputs.json"
'PASS native acquisition execution/exit/stdout/stderr and post-test diagnostics; native artifact XP reads empty temporary shadow under READ ONLY, no character data writes/client traversal claim'
