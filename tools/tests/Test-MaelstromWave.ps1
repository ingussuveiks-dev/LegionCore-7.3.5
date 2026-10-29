param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/maelstrom-wave-data",[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/maelstrom-wave"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $sections=[ordered]@{
 'FindLinkedEvent'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'EventDispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'EventPrefix'=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
 'ActionPrefix'=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')
 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
 'UpdateEvent'=@('        case SMART_EVENT_UPDATE:', '        case SMART_EVENT_UPDATE_OOC:')
 'HealthEvent'=@('        case SMART_EVENT_HEALTH_PCT:', '        case SMART_EVENT_TARGET_HEALTH_PCT:')
 'SpellHitEvent'=@('        case SMART_EVENT_SPELLHIT:', '        case SMART_EVENT_OOC_LOS:')
 'RecalcTimer'=@('void SmartScript::RecalcTimer(', 'void SmartScript::UpdateTimer(')
 'UpdateTimer'=@('void SmartScript::UpdateTimer(', 'bool SmartScript::CheckTimer(')
 'UpdateLoop'=@('void SmartScript::OnUpdate(', '    if (!mStoredEvents.empty())')
 'NoParamEvents'=@('        case SMART_EVENT_AGGRO:','        case SMART_EVENT_IS_BEHIND_TARGET:')
 'CastAction'=@('        case SMART_ACTION_CAST:','        case SMART_ACTION_CAST_CUSTOM:')
 'CreditAction'=@('        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:','        case SMART_ACTION_SUMMON_CONVERSATION:')
 'CreditOwner'=@('Player* GetScriptCreditPlayer(','Unit* GetScriptCreditCaster(')
 'PlayerTarget'=@('        case SMART_TARGET_CLOSEST_PLAYER:','        case SMART_TARGET_OWNER_OR_SUMMONER:')
 }
 $all='';foreach($name in $sections.Keys){$subject=if($name -eq 'UpdateLoop'){$s.Substring($s.IndexOf('void SmartScript::OnUpdate('))}else{$s};$v=Section $subject $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $summon=Section $s '        case SMART_ACTION_SUMMON_CREATURE:' '        case SMART_ACTION_SUMMON_GO:'
 [IO.File]::WriteAllText("$out/PositionSummon.inc",(Section $summon '            if (e.GetTargetType() == SMART_TARGET_POSITION)' '            break;'))
 $cs=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $predicates=[ordered]@{'NearCondition'=@('        case CONDITION_NEAR_CREATURE:','        case CONDITION_NEAR_GAMEOBJECT:');'ScenarioCondition'=@('        case CONDITION_SCENARION_STEP:','        case CONDITION_REALM_ACHIEVEMENT:');'ConditionGroups'=@('bool ConditionMgr::IsObjectMeetToConditionList(','bool ConditionMgr::CanHaveSourceGroupSet(')}
 foreach($name in $predicates.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $cs $predicates[$name][0] $predicates[$name][1]))}
 foreach($pair in @(@('SmartTargetBinding','ConditionSourceInfo sourceInfo\(unit, baseObject\);'),@('Negation','if \(NegativeCondition\)\s+condMeets = !condMeets;'))){$m=[regex]::Match($cs,$pair[1]);if(!$m.Success){throw "Native condition binding changed $($pair[0])"};[IO.File]::WriteAllText("$out/$($pair[0]).inc",$m.Value)}
 $grid=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp" -Raw
 $gh=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.h" -Raw
 [IO.File]::WriteAllText("$out/NearCreatureClass.inc",(Section $gh '    class NearestCreatureEntryWithLiveStateInObjectRangeCheck' '    class TC_GAME_API AnyPlayerInObjectRangeCheck'))
 [IO.File]::WriteAllText("$out/NearCreature.inc",(Section $grid 'NearestCreatureEntryWithLiveStateInObjectRangeCheck::NearestCreatureEntryWithLiveStateInObjectRangeCheck' 'AnyPlayerInObjectRangeCheck::'))
 [IO.File]::WriteAllText("$out/NearPlayerClass.inc",(Section $gh '    class NearestPlayerInObjectRangeCheck' '    class NearestPlayerNotGMInObjectRangeCheck'))
 [IO.File]::WriteAllText("$out/NearPlayer.inc",(Section $grid 'NearestPlayerInObjectRangeCheck::NearestPlayerInObjectRangeCheck' 'NearestPlayerNotGMInObjectRangeCheck::'))
 $cc=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
 [IO.File]::WriteAllText("$out/SelectPlayer.inc",(Section $cc 'Player* Creature::SelectNearestPlayer(float distance) const' 'Player* Creature::SelectNearestPlayerNotGM'))
 $h=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.h" -Raw
 [IO.File]::WriteAllText("$out/ClosestCreature.inc",(Section $h 'inline Creature* GetClosestCreatureWithEntry(' 'inline GameObject* GetClosestGameObjectWithEntry('))
 $ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 [IO.File]::WriteAllText("$out/JustDied.inc",(Section $ai 'void SmartAI::JustDied(Unit* killer)' 'void SmartAI::KilledUnit('))
 $u=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $guard=Section $u '    // Prevent killing unit twice (and giving reward from kill twice)' '    volatile uint32 entryorguid'
 [IO.File]::WriteAllText("$out/DeathGuard.inc",$guard)
 if($u.IndexOf('setDeathState(JUST_DIED)', $u.IndexOf('void Unit::Kill(')) -gt $u.IndexOf('JustDied(this)', $u.IndexOf('void Unit::Kill('))){throw 'Death state must precede AI callback'}
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $names=@([regex]::Matches($all,'\b(?:SMART_EVENT_|SMART_ACTION_|SMART_TARGET_|SMART_SCRIPT_TYPE_|SMARTCAST_|CAST_FORCE_)[A-Z_0-9]*\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $eventNames=@($names|Where-Object {$_ -like 'SMART_EVENT_*' -and $_ -notlike 'SMART_EVENT_FLAG_*'})
 $enum=@($eventNames|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing enum $_"};"$_=$($m.Groups[1].Value -replace '//.*','')"}) -join ','
 $decl="enum SMART_EVENT : uint32 {$enum};`n";foreach($name in @($names|Where-Object {$_ -notin $eventNames})){$m=[regex]::Match($header,"\b$name\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing constant $name"};$decl+="constexpr uint32 $name=$($m.Groups[1].Value -replace '//.*','');`n"};[IO.File]::WriteAllText("$out/Constants.inc",$decl)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MaelstromWaveTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`" `"$data/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Maelstrom wave dispatch regression failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MaelstromWaveCatalog.cpp -ProbeName MaelstromWaveCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv";foreach($line in @("SCENARIO_EVENT`t49264`t1038`t2`t46930`t46931","SCENARIO_EVENT`t49331`t1038`t3`t46938`t46939","SCENARIO_EVENT`t46799`t1038`t4`t46946`t46947","EFFECT`t205790`t0`t0`t205`t0`t266`t0`t0`t1`t0`t0")){if($native -notcontains $line){throw "Native Maelstrom metadata differs $line"}}
  'PASS matching26972 Maelstrom1038 waves/capture/exit and actual266 artifact-selection spell205790'
 }
}
