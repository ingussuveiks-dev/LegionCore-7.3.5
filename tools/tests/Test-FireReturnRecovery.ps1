param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-return-recovery"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing native block $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/fire-return-recovery-data") -Test {
 & "$PSScriptRoot/Test-FireReturnRecoveryData.ps1" -VerifyLiveData:$VerifyLiveData
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $sections=[ordered]@{
 'FindLinkedEvent'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'EventDispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'EventPrefix'=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
 'ActionPrefix'=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')

 'NoParamEvents'=@('        case SMART_EVENT_AGGRO:','        case SMART_EVENT_IS_BEHIND_TARGET:')
 'WaypointEvent'=@('        case SMART_EVENT_WAYPOINT_REACHED:', '        case SMART_EVENT_SUMMON_DESPAWNED:')
 'StartPathAction'=@('        case SMART_ACTION_WP_START:', '        case SMART_ACTION_WP_PAUSE:')
 'DataSetEvent'=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')

 'UpdateEvent'=@('        //called from Update tick','        case SMART_EVENT_HEALTH_PCT:')

 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')

 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $cases=(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')
 [IO.File]::WriteAllText("$out/ActorConditions.inc",$cases)
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 [IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
 [IO.File]::WriteAllText("$out/SmartEventLookup.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForSmartEvent(' 'ConditionList ConditionMgr::GetConditionsForNpcVendorEvent('))
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 [IO.File]::WriteAllText("$out/SetData.inc",(Block $source 'void SmartAI::SetData('))
 [IO.File]::WriteAllText("$out/StartPath.inc",(Block $source 'bool SmartAI::StartPath('))
 [IO.File]::WriteAllText("$out/MovementInform.inc",(Block $source 'void SmartAI::MovementInform('))
 [IO.File]::WriteAllText("$out/MovepointReached.inc",(Block $source 'void SmartAI::MovepointReached('))
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $summon=Section $source '        case SMART_ACTION_SUMMON_CREATURE:' '        case SMART_ACTION_SUMMON_GO:'
 [IO.File]::WriteAllText("$out/PositionSummon.inc",(Block $summon 'if (e.GetTargetType() == SMART_TARGET_POSITION)'))
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $enums='';foreach($name in @('SMART_EVENT','SMART_ACTION','SMARTAI_TARGETS','SmartScriptType','SmartEventFlags','eSmartAI')){$enums+=(Block $source "enum $name`r`n")+';'}
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.h" -Raw
 $enums+=(Block $source 'enum SmartEscortState')+';'
 $source=Get-Content "$repo/src/server/game/Movement/MotionMaster.h" -Raw
 foreach($name in @('POINT_MOTION_TYPE','FOLLOW_MOTION_TYPE')){$m=[regex]::Match($source,"\b$name\s*=\s*(\d+)");if(!$m.Success){throw "Missing native motion $name"};$enums+="constexpr uint32 $name=$($m.Groups[1].Value);"}
 $source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
 $m=[regex]::Match($source,'\bUNIT_STATE_EVADE\s*=\s*(0x[0-9A-Fa-f]+)');if(!$m.Success){throw 'Missing native evade state'};$enums+="constexpr uint32 UNIT_STATE_EVADE=$($m.Groups[1].Value);"
 [IO.File]::WriteAllText("$out/NativeEnums.inc",$enums)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data="$repo/build-extractors/runtime-test/fire-return-recovery-data"
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion/ClassHalls`" `"$PSScriptRoot/FireReturnRecoveryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/test.exe`" `"$data/actors.tsv`" `"$data/conditions.tsv`" `"$data/path.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Fire return recovery regression failed'}
}
