param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fire-boss-health-data",[switch]$VerifyNativeCatalog,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-boss-health"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
 & "$PSScriptRoot/Test-FireBossHealthData.ps1" -VerifyLiveData:$VerifyLiveData
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $sections=[ordered]@{
 'FindLinkedEvent'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'EventDispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'EventPrefix'=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
 'ActionPrefix'=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')
 'TargetSelfVictim'=@('        case SMART_TARGET_SELF:','        case SMART_TARGET_HOSTILE_SECOND_AGGRO:')
 'DataSetEvent'=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')
 'Timers'=@('void SmartScript::InitTimer(', 'bool SmartScript::CheckTimer(')
 'TimerEvent'=@('        //called from Update tick','        case SMART_EVENT_HEALTH_PCT:')
 'WaypointEvent'=@('        case SMART_EVENT_WAYPOINT_REACHED:', '        case SMART_EVENT_SUMMON_DESPAWNED:')
 'HealthEvent'=@('        case SMART_EVENT_HEALTH_PCT:','        case SMART_EVENT_TARGET_HEALTH_PCT:')
 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
 'NoParamEvents'=@('        case SMART_EVENT_AGGRO:','        case SMART_EVENT_IS_BEHIND_TARGET:')
 'TextOverEvent'=@('        case SMART_EVENT_TEXT_OVER:','        case SMART_EVENT_DATA_SET:')
 'TalkAction'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'TextTimer'=@('    if (mUseTextTimer && me)','void SmartScript::FillScript(')
 }
 $sections['TargetInvoker']=@('        case SMART_TARGET_NONE:','        case SMART_TARGET_ACTION_INVOKER_VEHICLE:')
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $chat=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/ChatAdmission.inc",(Section $chat '    if (!source || !source->IsInWorld())' '    CreatureTextGroup const& textGroupContainer'))
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $constants=@([regex]::Matches($all,'\bSMART_EVENT_[A-Z_]+\b')|ForEach-Object {$_.Value}|Where-Object {$_ -notlike 'SMART_EVENT_FLAG_*'}|Sort-Object -Unique)
 $enum=@($constants|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*(\d+)\s*,");if(!$m.Success){throw "Missing native event enum $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 $flags=@([regex]::Matches($all,'\bSMART_EVENT_FLAG_[A-Z_]+\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $extra='';foreach($flag in $flags){$m=[regex]::Match($header,"\b$flag\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing flag $flag"};$extra+="constexpr uint32 $flag=$($m.Groups[1].Value -replace '//.*','');`n"}
 $extra+='constexpr uint32 SMART_ACTION_CAST=11,SMART_ACTION_CAST_CUSTOM=210,SMARTCAST_INTERRUPT_PREVIOUS=1,SMARTCAST_TRIGGERED=2,UNIT_STATE_CASTING=1,SMART_SCRIPT_TYPE_TIMED_ACTIONLIST=9;'
 $extra+='constexpr uint32 SMART_TARGET_SELF=1,SMART_TARGET_VICTIM=2,SMART_TARGET_NONE=0,SMART_TARGET_ACTION_INVOKER=7;'
 $action=[regex]::Match($header,'SMART_ACTION_TALK\s*=\s*(\d+)\s*,').Groups[1].Value
 [IO.File]::WriteAllText("$out/EventConstants.inc","enum SMART_EVENT {$enum}; constexpr uint32 SMART_ACTION_TALK=$action; $extra")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/FireBossHealthTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`" `"$data/texts.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Fire boss health regression failed'}
 & "$PSScriptRoot/Test-FireBossHealthCasts.ps1" -DataDirectory $data -AddressSanitizer
 if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_bilaal_combat_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical shared native threshold retry path differs'}
 }finally{$env:PYTHONPATH=$old}}
 if($VerifyNativeCatalog){
  $old=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FireBossCatalog.cpp -ProbeName FireBossCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv" -Raw
  foreach($line in @("SPELL`t196327`tPhoenix Reborn","EFFECT`t196327`t0`t0`t2`t0`t0`t0`t0`t53`t16`t0","MISC`t196327`t0`t0`t1500","SCENARIO_EVENT`t47380`t957`t3`t45964`t45965")){
   if($native -notmatch ('(?m)^'+[regex]::Escape($line)+'\r?$')){throw "Matching26972 Fire boss native differs: $line"}
  }
 }
}
