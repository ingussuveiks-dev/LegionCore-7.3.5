param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-temple-admission",[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$out/data","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-FuZanTempleAdmissionData.ps1" -OutputDirectory "$out/data" -VerifyLiveData:$VerifyLiveData
 function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Missing native $start"};$s.Substring($a,$b-$a)}
 $source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $predicates=(Section $source '        case CONDITION_OBJECT_ENTRY:' '        case CONDITION_TYPE_MASK:')+
  (Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+
  (Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+
  (Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+
  (Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')+
  (Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')
 [IO.File]::WriteAllText("$out/Predicates.inc",$predicates)
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 $smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $sections=[ordered]@{
  FindLinkedEvent=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
  LinkTail=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
  EventDispatch=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
  EventPrefix=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
  ActionPrefix=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')
  LosEvent=@('        case SMART_EVENT_OOC_LOS:','        case SMART_EVENT_IC_LOS:')
  LinkEvent=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
 }
 $all='';foreach($name in $sections.Keys){$s=Section $smart $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$s);$all+=$s}
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $names=@([regex]::Matches($all,'\bSMART_EVENT_[A-Z_]+\b')|ForEach-Object Value|Where-Object {$_ -notlike 'SMART_EVENT_FLAG_*'}|Sort-Object -Unique)
 $enum=@($names|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*(\d+)\s*,");if(!$m.Success){throw "Missing event $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 $extra='';foreach($flag in @([regex]::Matches($all,'\bSMART_EVENT_FLAG_[A-Z_]+\b')|ForEach-Object Value|Sort-Object -Unique)){$m=[regex]::Match($header,"\b$flag\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing flag $flag"};$extra+="constexpr uint32 $flag=$($m.Groups[1].Value -replace '//.*','');`n"}
 $conditionHeader=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.h" -Raw
 foreach($name in @([regex]::Matches($predicates,'\bCONDITION_[A-Z_]+\b')|ForEach-Object Value|Sort-Object -Unique)){$m=[regex]::Match($conditionHeader,"\b$name\s*=\s*(\d+)");if(!$m.Success){throw "Missing condition $name"};$extra+="constexpr uint32 $name=$($m.Groups[1].Value);`n"}
 $quests=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
 $statuses=@('QUEST_STATUS_NONE','QUEST_STATUS_COMPLETE','QUEST_STATUS_INCOMPLETE','QUEST_STATUS_REWARDED')
 $questEnum=@($statuses|ForEach-Object {$m=[regex]::Match($quests,"\b$_\s*=\s*(\d+)");if(!$m.Success){throw "Missing quest enum $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 [IO.File]::WriteAllText("$out/Constants.inc","enum SMART_EVENT {$enum}; enum QuestStatus {$questEnum}; $extra")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/FuZanTempleAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/data/smart_scripts.tsv`" `"$out/data/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Fu Zan temple admission regression failed'}
}
