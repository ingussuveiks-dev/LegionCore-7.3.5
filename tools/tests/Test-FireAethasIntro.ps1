param([switch]$VerifyLiveData,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-aethas-intro"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/fire-aethas-intro-data","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-FireAethasIntroData.ps1" -VerifyLiveData:$VerifyLiveData
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $sections=[ordered]@{
 'FindLinkedEvent'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'EventDispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'EventPrefix'=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
 'ActionPrefix'=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')

 'DataSetEvent'=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')

 'UpdateEvent'=@('        //called from Update tick','        case SMART_EVENT_HEALTH_PCT:')

 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')

 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $constants=@([regex]::Matches($all,'\bSMART_EVENT_[A-Z_]+\b')|ForEach-Object {$_.Value}|Where-Object {$_ -notlike 'SMART_EVENT_FLAG_*'}|Sort-Object -Unique)
 $enum=@($constants|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*(\d+)\s*,");if(!$m.Success){throw "Missing native event enum $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 $flags=@([regex]::Matches($all,'\bSMART_EVENT_FLAG_[A-Z_]+\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $extra='';foreach($flag in $flags){$m=[regex]::Match($header,"\b$flag\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing flag $flag"};$extra+="constexpr uint32 $flag=$($m.Groups[1].Value -replace '//.*','');`n"}
 $action=[regex]::Match($header,'SMART_ACTION_TALK\s*=\s*(\d+)\s*,').Groups[1].Value
 [IO.File]::WriteAllText("$out/Constants.inc","enum SMART_EVENT {$enum}; constexpr uint32 SMART_ACTION_TALK=$action; $extra")
 $source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $cases=(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')
 [IO.File]::WriteAllText("$out/ActorConditions.inc",$cases)
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 [IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
 [IO.File]::WriteAllText("$out/SmartEventLookup.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForSmartEvent(' 'ConditionList ConditionMgr::GetConditionsForNpcVendorEvent('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data="$repo/build-extractors/runtime-test/fire-aethas-intro-data"
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/FireAethasIntroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/test.exe`" `"$data/actors.tsv`" `"$data/rules.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Fire Aethas battle intro guard failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FireBossCatalog.cpp -ProbeName FireBossCatalog -OutputFile "$out/native.tsv"
 }finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv" -Raw
  foreach($line in @("AETHAS_SUMMON_PRODUCER_COUNT`t0","EFFECT`t196561`t0`t0`t219`t0`t728`t0`t0`t1`t0`t0","EFFECT`t196520`t0`t0`t219`t0`t727`t0`t0`t18`t0`t0")){
   if($native -notmatch ('(?m)^'+[regex]::Escape($line)+'\r?$')){throw "Matching26972 Aethas native differs: $line"}
  }
 }
}
