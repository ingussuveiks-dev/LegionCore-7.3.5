param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/malithar-cleanup-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/malithar-cleanup"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $events=Section $s 'void SmartScript::ProcessEvent(' 'void SmartScript::InitTimer('
 $sections=[ordered]@{
 'FindLinkedEvent'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'EventDispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'ActionPrefix'=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')
 'TalkAction'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'CastAction'=@('        case SMART_ACTION_CAST:','        case SMART_ACTION_CAST_CUSTOM:')
 'DataAction'=@('        case SMART_ACTION_SET_DATA:','        case SMART_ACTION_MOVE_FORWARD:')
 'PhaseAction'=@('        case SMART_ACTION_SET_EVENT_PHASE:','        case SMART_ACTION_INC_EVENT_PHASE:')
 'Reset'=@('void SmartScript::OnReset()','void SmartScript::ResetBaseObject()')
 'TextTimer'=@('    if (mUseTextTimer && me)','void SmartScript::FillScript(')
 'InitTimer'=@('void SmartScript::InitTimer(','void SmartScript::UpdateTimer(')
 'UpdateTimer'=@('void SmartScript::UpdateTimer(','bool SmartScript::CheckTimer(')
 }
 $eventSections=[ordered]@{
 'EventPrefix'=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
 'WaypointEvent'=@('        case SMART_EVENT_WAYPOINT_REACHED:','        case SMART_EVENT_SUMMON_DESPAWNED:')
 'DataSetEvent'=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')
 'TextOverEvent'=@('        case SMART_EVENT_TEXT_OVER:','        case SMART_EVENT_DATA_SET:')
 'NoParamEvents'=@('        case SMART_EVENT_AGGRO:','        case SMART_EVENT_IS_BEHIND_TARGET:')
 'ICEvent'=@('        case SMART_EVENT_UPDATE:','        case SMART_EVENT_HEALTH_PCT:')
 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 foreach($name in $eventSections.Keys){$v=Section $events $eventSections[$name][0] $eventSections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $cs=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $predicates=[ordered]@{'AuraCondition'=@('        case CONDITION_AURA:','        case CONDITION_ITEM:');'MapCondition'=@('        case CONDITION_MAPID:','        case CONDITION_AREAID:');'AliveCondition'=@('        case CONDITION_ALIVE:','        case CONDITION_HP_VAL:');'ScenarioCondition'=@('        case CONDITION_SCENARION_STEP:','        case CONDITION_REALM_ACHIEVEMENT:')}
 foreach($name in $predicates.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $cs $predicates[$name][0] $predicates[$name][1]))}
 $binding=[regex]::Match($cs,'ConditionSourceInfo sourceInfo\(unit, baseObject\);');if(!$binding.Success){throw 'Native initial source22 target binding changed'};[IO.File]::WriteAllText("$out/SmartTargetBinding.inc",$binding.Value)
 $phase=$s.Substring($s.IndexOf('bool SmartScript::IsInPhase('));[IO.File]::WriteAllText("$out/PhaseMethods.inc",$phase);$all+=$phase
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $names=@([regex]::Matches($all,'\b(?:SMART_EVENT_|SMART_ACTION_|SMART_TARGET_|SMART_SCRIPT_TYPE_|SMARTCAST_|CAST_FORCE_TARGET_SELF)[A-Z_0-9]*\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $eventNames=@($names|Where-Object {$_ -like 'SMART_EVENT_*' -and $_ -notlike 'SMART_EVENT_FLAG_*' -and $_ -ne 'SMART_EVENT_PHASE_COUNT'})
 $enum=@($eventNames|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing enum $_"};"$_=$($m.Groups[1].Value -replace '//.*','')"}) -join ','
 $decl="enum SMART_EVENT : uint32 {$enum};`n"
 foreach($name in @($names|Where-Object {$_ -notin $eventNames})){$m=[regex]::Match($header,"\b$name\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing constant $name"};$decl+="constexpr uint32 $name=$($m.Groups[1].Value -replace '//.*','');`n"}
 [IO.File]::WriteAllText("$out/Constants.inc",$decl)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MalitharCleanupTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`" `"$data/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Malithar cleanup regression failed'}
}
