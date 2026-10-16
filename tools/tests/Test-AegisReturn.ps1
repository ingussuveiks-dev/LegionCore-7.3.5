$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/aegis-return"
& "$PSScriptRoot/Test-AegisReturnData.ps1"
& "$PSScriptRoot/Test-HallsAegis.ps1" *> "$repo/.codex/aegis-return-upstream.log"
$upstream="$repo/build-extractors/runtime-test/halls-aegis"
Get-ChildItem "$upstream/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/AegisFixture.inc" -Raw
$fixture=$fixture.Replace('struct WorldObject:Position {','struct WorldObject:Position {float GetDistance(WorldObject const* other){return GetExactDist(other);}')
[IO.File]::WriteAllText("$out/AegisFixture.inc",$fixture)
$prefix=Get-Content "$PSScriptRoot/HallsAegisTest.cpp" -Raw
$prefix=$prefix.Substring(0,$prefix.IndexOf('void Setup('))
$prefix=$prefix.Replace('struct ScriptMgr{','struct EventObject;struct ScriptMgr{bool IsEOMeetConditions(Player*,EventObject*){return true;}bool OnEventObject(Player*,EventObject*,bool);')
$prefix=$prefix.Replace('return (GetEntry()==97988||GetEntry()==251991)&&q==40072;','return GetEntry()==251992&&q==43349;')
[IO.File]::WriteAllText("$out/Fixture.inc",$prefix)
$event=Get-Content "$repo/src/server/game/Entities/EventObject/EventObject.cpp" -Raw
$a=$event.IndexOf('void EventObject::Update(');$b=$event.IndexOf('void EventObject::MoveInLineOfSight(',$a)
[IO.File]::WriteAllText("$out/EventUpdate.inc",$event.Substring($a,$b-$a))
$a=$event.IndexOf('bool EventObject::ActivatedForPlayer(')
[IO.File]::WriteAllText("$out/Activated.inc",$event.Substring($a))
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$smart.IndexOf('class SmartEventObject :');$b=$smart.IndexOf('void AddSC_SmartScripts()',$a)
[IO.File]::WriteAllText("$out/SmartEventObject.inc",$smart.Substring($a,$b-$a))
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$smart.IndexOf('        case SMART_ACTION_CALL_KILLEDMONSTER:');$b=$smart.IndexOf('        case SMART_ACTION_SET_INST_DATA:',$a)
[IO.File]::WriteAllText("$out/Action33.inc",$smart.Substring($a,$b-$a))
$a=$smart.IndexOf('WorldObject* SmartScript::GetBaseObject()');$b=$smart.IndexOf('bool SmartScript::IsUnit(',$a)
[IO.File]::WriteAllText("$out/SmartBase.inc",$smart.Substring($a,$b-$a))
$a=$smart.IndexOf('        case SMART_TARGET_NONE:');$b=$smart.IndexOf('        case SMART_TARGET_ACTION_INVOKER_VEHICLE:',$a)
[IO.File]::WriteAllText("$out/InvokerTarget.inc",$smart.Substring($a,$b-$a))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('        else if (GameObject* go = source->ToGameObject())',$core.IndexOf('void Player::PrepareGossipMenu('));$b=$core.IndexOf('        if (canTalk)',$a)
[IO.File]::WriteAllText("$out/GOOption.inc",$core.Substring($a,$b-$a).Replace('        else if','        if'))
$condition=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$condition.IndexOf('        case CONDITION_QUESTREWARDED:');$b=$condition.IndexOf('        case CONDITION_QUESTTAKEN:',$a)
[IO.File]::WriteAllText("$out/QuestRewardedCondition.inc",$condition.Substring($a,$b-$a))
$phase=Get-Content "$repo/src/server/game/Maps/PhaseMgr.cpp" -Raw
$a=$phase.IndexOf('void PhaseUpdateData::AddQuestUpdate(');$b=$phase.IndexOf('void PhaseUpdateData::AddScenarioUpdate(',$a)
[IO.File]::WriteAllText("$out/QuestPhaseUpdate.inc",$phase.Substring($a,$b-$a))
# Actual reward schedules this unconditionally after m_RewardedQuests insertion;
# UPDATE_PHASE_SHIFT is not tested on this server path.
$a=$core.IndexOf('    // Must come after the insert in m_RewardedQuests');$b=$core.IndexOf('    // update area quests',$a)
$rewardPhase=$core.Substring($a,$b-$a)
if(!$rewardPhase.Contains('phaseUdateData.AddQuestUpdate(quest_id);') -or !$rewardPhase.Contains('GetPhaseMgr().NotifyConditionChanged(phaseUdateData);') -or $rewardPhase.Contains('HasFlag')){throw 'Native reward phase notification changed; review its admission'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/AegisReturnTest.cpp`" /Fe`"$out/Return.exe`" /Fo`"$out/Return.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Return.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Aegis return native regression failed'}
