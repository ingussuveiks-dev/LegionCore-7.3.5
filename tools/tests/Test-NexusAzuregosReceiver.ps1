#requires -Version 7.0
param([switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[switch]$WithPriestHealing)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/nexus-azuregos-receiver/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 if($VerifyLiveData -and (Test-Path "$repo/sql/updates/world/2026_10_30_33_priest_azuregos_healing.sql")){$WithPriestHealing=$true}
 & "$PSScriptRoot/Test-NexusAzuregosData.ps1" -OutputDirectory $out -VerifyLiveData:$VerifyLiveData -WithPriestHealing:$WithPriestHealing
 function Block($s,$a){$i=$s.IndexOf($a);if($i -lt 0){throw "Missing $a"};$j=$s.IndexOf('{',$i)+1;$depth=1;while($depth -and $j -lt $s.Length){if($s[$j] -eq '{'){++$depth};if($s[$j] -eq '}'){--$depth};++$j};if($depth){throw "Unclosed $a"};$s.Substring($i,$j-$i)}
 function Section($s,$a,$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max($i,0));if($i -lt 0 -or $j -le $i){throw "Missing $a/$b"};$s.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $parts=[ordered]@{
 'FindLink.inc'=Block $s 'SmartScriptHolder SmartScript::FindLinkedEvent('
 'Dispatch.inc'=Block $s 'void SmartScript::ProcessEventsFor('
 'EventPrefix.inc'=Section $s 'void SmartScript::ProcessEvent(' '    switch (e.GetEventType())'
 'ActionPrefix.inc'=Section $s 'void SmartScript::ProcessAction(' '    switch (e.GetActionType())'
 'UpdateEvents.inc'=Section $s '        case SMART_EVENT_LINK: //special handling' '        case SMART_EVENT_HEALTH_PCT:'
 'DataEvent.inc'=Section $s '        case SMART_EVENT_DATA_SET:' '        case SMART_EVENT_PASSENGER_REMOVED:'
 'DataAction.inc'=Block $s '        case SMART_ACTION_SET_DATA:'
 'ListAction.inc'=Block $s '        case SMART_ACTION_CALL_TIMED_ACTIONLIST:'
 'RemoveAction.inc'=Block $s '        case SMART_ACTION_REMOVEAURASFROMSPELL:'
 'CastAction.inc'=Block $s '        case SMART_ACTION_CAST:'
 'LinkTail.inc'=Section $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('
 'InitTimer.inc'=Block $s 'void SmartScript::InitTimer('
 'RecalcTimer.inc'=Block $s 'void SmartScript::RecalcTimer('
 'UpdateTimer.inc'=Block $s 'void SmartScript::UpdateTimer('
 'SetList.inc'=Block $s 'void SmartScript::SetScript9('
 'TimedTick.inc'=Section $s '    bool needCleanup = true;' '    if (!mRemIDs.empty())'
 }
 $ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 $parts['AISetData.inc']=Block $ai 'void SmartAI::SetData('
 $parts['AISetList.inc']=Block $ai 'void SmartAI::SetScript9('
 $c=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $parts['ConditionSource.inc']=Block $c 'ConditionSourceInfo::ConditionSourceInfo('
 $parts['ConditionPrefix.inc']=Section $c 'bool Condition::Meets(' '    switch (ConditionType)'
 $parts['NearCondition.inc']=Section $c '        case CONDITION_NEAR_CREATURE:' '        case CONDITION_NEAR_GAMEOBJECT:'
 $parts['ConditionTail.inc']=Section $c '    if (NegativeCondition)' 'uint32 Condition::GetSearcherTypeMaskForCondition()'
 $parts['ConditionGroups.inc']=Block $c 'bool ConditionMgr::IsObjectMeetToConditionList('
 $parts['GetConditions.inc']=Block $c 'ConditionList ConditionMgr::GetConditionsForSmartEvent('
 $parts['EventConditions.inc']=Block $c 'bool ConditionMgr::IsObjectMeetingSmartEventConditions('
 if($WithPriestHealing){$parts['CriteriaCondition.inc']=Block $c '        case CONDITION_CRITERIA_TREE:';$parts['ScenarioCondition.inc']=Block $c '        case CONDITION_SCENARION_STEP:';$parts['HealthEvent.inc']=Block $s '        case SMART_EVENT_HEALTH_PCT:'}
 $g=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp" -Raw;$h=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.h" -Raw
 $parts['NearClass.inc']=Section $h '    class NearestCreatureEntryWithLiveStateInObjectRangeCheck' '    class TC_GAME_API AnyPlayerInObjectRangeCheck'
 $parts['NearBody.inc']=Section $g 'NearestCreatureEntryWithLiveStateInObjectRangeCheck::NearestCreatureEntryWithLiveStateInObjectRangeCheck' 'AnyPlayerInObjectRangeCheck::'
 $h=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.h" -Raw;$parts['Closest.inc']=Section $h 'inline Creature* GetClosestCreatureWithEntry(' 'inline GameObject* GetClosestGameObjectWithEntry('
 $all='';foreach($key in $parts.Keys){[IO.File]::WriteAllText("$out/$key",$parts[$key]);$all+=$parts[$key]}
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $names=@([regex]::Matches($all+' SMART_EVENT_DEATH SMART_SCRIPT_TYPE_TIMED_ACTIONLIST','\b(?:SMART_EVENT_|SMART_ACTION_|SMART_TARGET_|SMARTCAST_|SMART_SCRIPT_TYPE_|CAST_FORCE_)[A-Z_0-9]*\b')|ForEach-Object{$_.Value}|Sort-Object -Unique)
 $events=@($names|Where-Object{$_ -like 'SMART_EVENT_*' -and $_ -notlike 'SMART_EVENT_FLAG_*'})
 $constants='';$ev=@();foreach($name in $names){$m=[regex]::Match($header,"\b$name\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Native constant missing $name"};$value=$m.Groups[1].Value -replace '//.*','';if($name -in $events){$ev+="$name=$value"}else{$constants+="constexpr uint32 $name=$value;`n"}}
 [IO.File]::WriteAllText("$out/Constants.inc","enum SMART_EVENT : uint32 {$($ev -join ',')};`n$constants")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/NexusAzuregosReceiverTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/events.tsv`" `"$out/baseline-conditions.tsv`" `"$out/conditions.tsv`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual native Azuregos receiver regression failed'}
 if($WithPriestHealing){[IO.File]::WriteAllText("$out/healing.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/NexusAzuregosHealingTest.cpp`" /Fe`"$out/healing.exe`" /Fo`"$out/healing.obj`" /Fd`"$out/healing.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/healing.exe`" `"$out/priest-baseline-events.tsv`" `"$out/priest-events.tsv`" `"$out/priest-baseline-conditions.tsv`" `"$out/priest-conditions.tsv`" `"$out/priest-source-health.tsv`"`n");& "$out/healing.cmd";if($LASTEXITCODE){throw 'Actual native Priest healing regression failed'}}
 if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile NexusAzuregosCatalog.cpp -ProbeName NexusAzuregosCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}}
 if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_nexus_azuregos_receiver_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Actual native Azuregos receiver PE failed'}}finally{$env:PYTHONPATH=$prior}}
}
