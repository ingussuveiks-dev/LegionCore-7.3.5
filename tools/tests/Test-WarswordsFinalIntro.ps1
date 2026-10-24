param([switch]$Baseline,[switch]$HelyaBaseline,[switch]$TargetBaseline,[switch]$BubbleBaseline,[switch]$CommunionBaseline,[switch]$ConversationBaseline,[switch]$TimingBaseline,[switch]$CleanupBaseline,[switch]$BlessingBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(@($Baseline,$HelyaBaseline,$TargetBaseline,$BubbleBaseline,$CommunionBaseline,$ConversationBaseline,$TimingBaseline,$CleanupBaseline,$BlessingBaseline|Where-Object {$_}).Count -gt 1 -or (($Baseline -or $HelyaBaseline -or $TargetBaseline -or $BubbleBaseline -or $CommunionBaseline -or $ConversationBaseline -or $TimingBaseline -or $CleanupBaseline -or $BlessingBaseline) -and $VerifyReleaseBinary)){throw 'Select one baseline without repaired Release verification'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-final-intro"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "9dd043ad:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned final intro baseline'}}
if($HelyaBaseline){$source=(& git -C $repo show "a5a82529:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned Helya baseline'}}
if($TargetBaseline){$source=(& git -C $repo show "f6b173ea:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned Helya target baseline'}}
if($BubbleBaseline){$source=(& git -C $repo show "05cd027c:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned bubble reset baseline'}}
if($CommunionBaseline){$source=(& git -C $repo show "a617a973:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned communion baseline'}}
if($ConversationBaseline){$source=(& git -C $repo show "638dfa31:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned conversation baseline'}}
if($TimingBaseline){$source=(& git -C $repo show "46ae22fd:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned dialogue timing baseline'}}
if($CleanupBaseline){$source=(& git -C $repo show "2d87fcfb:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned dialogue cleanup baseline'}}
if($BlessingBaseline){$source=(& git -C $repo show "20ce5e37:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned premature blessing baseline'}}
$lateBlessing=$source.Contains('bool IsIntroHelya(')
if($lateBlessing){
 $header=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw
 [IO.File]::WriteAllText("$out/NativeFunctionProcessor.inc",(Block $header 'class TC_COMMON_API FunctionProcessor')+';')
 $functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeFunctionMethods.inc",$functions.Substring($functions.IndexOf('FunctionProcessor::FunctionProcessor()')))
}
$source=$source.Substring($source.IndexOf('class npc_vigfus_bladewind_last :'))
$first=$source.IndexOf('        EventMap events;');$last=$source.IndexOf('        void Reset()',$first)
if($first -lt 0 -or $last -le $first){throw 'Final intro member fields missing'}
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
foreach($name in @('Reset','MoveInLineOfSight')){[IO.File]::WriteAllText("$out/$name.inc",(Block $source "void $name("))}
[IO.File]::WriteAllText("$out/StartConversation.inc",$(if($source.Contains('bool StartIntroConversation(')){Block $source 'bool StartIntroConversation('}else{''}))
[IO.File]::WriteAllText("$out/RemoveConversation.inc",$(if($source.Contains('void RemoveIntroConversation(')){Block $source 'void RemoveIntroConversation('}else{''}))
foreach($entry in @(@('IsIntroHelya','bool IsIntroHelya('),@('CancelIntro','void CancelIntro('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",$(if($source.Contains($entry[1])){Block $source $entry[1]}else{''}))}
$helper=if($Baseline){''}else{Block $source 'bool IsIntroTarget('}
[IO.File]::WriteAllText("$out/IsIntroTarget.inc",$helper)
$retry=if($Baseline){''}else{Block $source 'void UpdateIntro('}
[IO.File]::WriteAllText("$out/UpdateIntro.inc",$retry)
$lifecycle=if($source.Contains('void UpdateLifecycle(')){Block $source 'void UpdateLifecycle('}else{''}
[IO.File]::WriteAllText("$out/UpdateLifecycle.inc",$lifecycle)
$tick=''
if(!$Baseline -and !$HelyaBaseline){$update=Block $source 'void UpdateAI(';$tick=$update.Substring($update.IndexOf('{')+1);$tick=$tick.Substring(0,$tick.IndexOf('            UpdateIntro(diff);'))}
[IO.File]::WriteAllText("$out/VisualTick.inc",$tick)
foreach($entry in @(@('EncounterReady','bool IsEncounterReady('),@('AttackStart','void AttackStart('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",$(if($source.Contains($entry[1])){Block $source $entry[1]}else{''}))}
$update=Block $source 'void UpdateAI('
$first=$update.IndexOf('{')+1;$last=$update.IndexOf('            if (spectreActive')
$combat=if($last -gt $first){$update.Substring($first,$last-$first)}else{''}
[IO.File]::WriteAllText("$out/CombatTick.inc",$combat)
$source=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw
[IO.File]::WriteAllText("$out/GetPlayer.inc",(Block $source 'Player* ObjectAccessor::GetPlayer('))
[IO.File]::WriteAllText("$out/GetConversation.inc",(Block $source 'Conversation* ObjectAccessor::GetConversation('))
[IO.File]::WriteAllText("$out/GetCreature.inc",(Block $source 'Creature* ObjectAccessor::GetCreature('))
$source=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/ObjectInMap.inc",(Block $source 'template<class T> static T* GetObjectInMap('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($HelyaBaseline){'/DHELYA_BASELINE'}elseif($TargetBaseline){'/DTARGET_BASELINE'}elseif($BubbleBaseline){'/DBUBBLE_BASELINE'}elseif($CommunionBaseline){'/DCOMMUNION_BASELINE'}elseif($ConversationBaseline){'/DCONVERSATION_BASELINE'}elseif($TimingBaseline){'/DTIMING_BASELINE'}elseif($CleanupBaseline){'/DCLEANUP_BASELINE'}elseif($BlessingBaseline){'/DBLESSING_BASELINE'}elseif($lateBlessing){'/DWARSWORDS_LATE_BLESSING'}else{''}
$test=if($lateBlessing){'WarswordsLateBlessingTest.cpp'}else{'WarswordsFinalIntroTest.cpp'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/$test`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Final intro native regression failed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind_last';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=98602;SELECT 'helya',guid,id,map,PhaseId FROM creature WHERE id=99043;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Final intro binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("binding`t98602`tnpc_vigfus_bladewind_last","spawn`t361283`t98602`t1511`t7003","helya`t361286`t99043`t1511`t7003")){if($rows -notcontains $expected){throw "Final intro binding differs: $expected"}}
 if($rows.Count -ne 3){throw 'Additional shared final boss or Helya spawns require review'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_final_intro_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical final intro missing'}}finally{$env:PYTHONPATH=$previous}
}
