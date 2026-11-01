param([switch]$VerifyLiveData,[switch]$Baseline,[switch]$FaultInjection,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-temple-intro-native")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
if($Baseline -and $VerifyLiveData){throw 'Counterfactual cannot certify live registrations'}
if($Baseline -and $FaultInjection){throw 'Counterfactual and repaired fault injections are separate runs'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $quest=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
 $query=[regex]::Match($quest,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
 if(!$query){throw 'Missing actual native quest query'}
 [IO.File]::WriteAllText("$out/NativeFuZanQuest.inc",$query+' " WHERE ID=42765"')
 [IO.File]::WriteAllText("$out/NativeLostArmyQuest.inc",$query+' " WHERE ID IN(44923,45088)"')
 $inputs=@($PSCommandPath,"$PSScriptRoot/FuZanTempleIntroNative.cpp","$PSScriptRoot/MasterLootNative.cpp","$PSScriptRoot/LostArmyLootNative.cpp","$PSScriptRoot/RockawayOffensiveNative.cpp","$PSScriptRoot/RockawayChannelNative.cpp",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_temple_intro.h",
 "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/instance_temple_of_the_jade_serpent.cpp",
 "$repo/src/server/game/Entities/Conversation/Conversation.cpp","$repo/src/server/game/Entities/Conversation/Conversation.h",
 "$repo/src/server/game/Globals/ConversationData.cpp","$repo/src/server/game/Globals/ConversationData.h",
 "$repo/src/server/game/Spells/Spell.cpp","$repo/src/server/game/Spells/SpellEffects.cpp",
 "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp","$repo/src/server/game/AI/SmartScripts/SmartScript.cpp",
 "$out/NativeFuZanQuest.inc","$out/NativeLostArmyQuest.inc")
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
 $args=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 if($VerifyLiveData){$args+='/DFUZAN_INTRO_LIVE'}
 if($Baseline){$args+='/DFUZAN_INTRO_BASELINE'}
 if($FaultInjection){$args+='/DFUZAN_INTRO_FAULTS'}
 $args+=($includes+$extra+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)|ForEach-Object {'/I"'+$_+'"'})
 $args+='"'+$PSScriptRoot+'/FuZanTempleIntroNative.cpp"'
 $args+=@('/Fe"'+$out+'/probe.exe"','/Fo"'+$out+'/probe.obj"','/link','/DEBUG','/MAP:"'+$out+'/probe.map"','/PDB:"'+$out+'/probe.pdb"')
 $args+=($libs|ForEach-Object {'"'+$_+'"'})
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n")
 & "$out/build.cmd" *> "$out/build.log";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 20;throw 'Native Fu Zan intro build failed'}
 $runtime="$repo/build-extractors/bin/Release";$run="$out/run-$(Get-Date -Format yyyyMMdd-HHmmss)"
 New-Item -ItemType Directory -Force "$run/logs"|Out-Null
 $config=Get-Content "$runtime/worldserver.conf" -Raw
 $config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'/logs"'))
 [IO.File]::WriteAllText("$run/worldserver.conf",$config)
 $prior=$env:PATH
 try{
  $env:PATH="$runtime;$prior"
  try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/native-diagnostics" -RuntimeDirectories @($run) -Test {
   Push-Location $runtime
   try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$code=$LASTEXITCODE}finally{Pop-Location}
   @{exit=$code;stdout=(Get-FileHash "$run/stdout.log").Hash;stderr=(Get-FileHash "$run/stderr.log").Hash}|ConvertTo-Json|Set-Content "$run/process-result.json"
   Get-Content "$run/stdout.log"|Where-Object {$_ -match '^(CASE|PASS|EXPECTED_BROKEN)'}
   if($code -or ((Get-Item "$run/stderr.log").Length -and !$FaultInjection)){Get-Content "$run/stderr.log" -Tail 25;throw "Native intro exit=$code or stderr"}
   if((Test-Path "$run/logs/DBErrors.log") -and (Get-Item "$run/logs/DBErrors.log").Length){throw 'Native intro emitted DBErrors'}
  }}catch{
   if(!$FaultInjection -or $_.Exception.Message -notlike 'New runtime diagnostics require investigation:*'){throw}
   # These are deliberate missing-actor/interrupted-story counterfactuals,
   # paired with the ordinary18 clean cases. Keep every original diagnostic.
   $diagnostics=Get-Content "$out/native-diagnostics/diagnostics-after.json" -Raw|ConvertFrom-Json
   $messages=@($diagnostics.ServerDiagnostics)
   $patterns=@('Fu Zan: native intro cast acknowledgement timed out for player [0-9]+; bounded retry pending','Fu Zan: native intro cast retries exhausted; no completion bypass \(player [0-9]+\)','Fu Zan: accepted intro conversation disappeared before completion for player [0-9]+; bounded replay pending')
   if(!$diagnostics.Completed -or @($diagnostics.ChangedCrashes).Count -or @($diagnostics.AppendedDBErrors).Count -or $messages.Count -ne 6){throw 'Fault injection has unexplained exit/crash/DB/severity evidence'}
   foreach($pattern in $patterns){if(@($messages|Where-Object {$_ -match $pattern}).Count -ne 2){throw "Expected fault diagnostic missing or duplicated: $pattern"}}
   foreach($message in $messages){if(!@($patterns|Where-Object {$message -match $_}).Count){throw "Unexplained fault diagnostic: $message"}}
   $stderrLines=@(Get-Content "$run/stderr.log"|Where-Object {$_ -match '\S'})
   if($stderrLines.Count -ne 6 -or (Get-Content "$run/process-result.json" -Raw|ConvertFrom-Json).exit -ne 0){throw 'Fault injection stderr/exit differs'}
   foreach($line in $stderrLines){if($line -notmatch '^ERROR \[scripts.ai\]' -or !@($patterns|Where-Object {$line -match $_}).Count){throw "Unexplained fault stderr: $line"}}
   $messages|Set-Content "$out/expected-fault-diagnostics.txt"
   $messages|ForEach-Object {"EXPECTED_FAULT $_"}
   'PASS explicit native fault diagnostics reviewed; original logs retained, paired ordinary clean18 required'
  }
 }finally{$env:PATH=$prior}
 foreach($path in $inputs){if((Get-FileHash -LiteralPath $path).Hash -ne $hashes[$path]){throw 'Native Fu Zan source input changed during execution'}}
 if((Get-FileHash $game).Hash -ne $gameHash -or (Get-FileHash $scripts).Hash -ne $scriptsHash){throw 'Native final-link input changed during execution'}
 @{inputs=$hashes;game=$gameHash;scripts=$scriptsHash;executable=(Get-FileHash "$out/probe.exe").Hash;run=$run;native_core_release_not_instrumented=$true;live_data=[bool]$VerifyLiveData;baseline=[bool]$Baseline;explicit_fault_injection=[bool]$FaultInjection}|ConvertTo-Json -Depth 4|Set-Content "$out/native-inputs.json"
 'PASS native intro execution/exit/stdout/stderr/post-test diagnostics; private metadata shadows and readonly pools, no client/authenticated save/full-chain claim'
}
