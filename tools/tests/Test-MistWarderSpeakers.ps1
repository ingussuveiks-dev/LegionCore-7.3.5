param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/mist-warder-speakers-data",[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/mist-warder-speakers"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
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
 'HealthEvent'=@('        case SMART_EVENT_HEALTH_PCT:','        case SMART_EVENT_TARGET_HEALTH_PCT:')
 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
 'NoParamEvents'=@('        case SMART_EVENT_AGGRO:','        case SMART_EVENT_IS_BEHIND_TARGET:')
 'TextOverEvent'=@('        case SMART_EVENT_TEXT_OVER:','        case SMART_EVENT_DATA_SET:')
 'SpellHitEvent'=@('        case SMART_EVENT_SPELLHIT:','        case SMART_EVENT_OOC_LOS:')
 'DieAction'=@('        case SMART_ACTION_DIE:','        case SMART_ACTION_SET_IN_COMBAT_WITH_ZONE:')
 'TalkAction'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'TextTimer'=@('    if (mUseTextTimer && me)','void SmartScript::FillScript(')
 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 [IO.File]::WriteAllText("$out/JustDied.inc",(Section $ai 'void SmartAI::JustDied(Unit* killer)' 'void SmartAI::KilledUnit(Unit* victim)'))
 [IO.File]::WriteAllText("$out/SpellHit.inc",(Section $ai 'void SmartAI::SpellHit(Unit* unit,' 'void SmartAI::SpellHitTarget(Unit* target,'))
 $chat=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/ChatAdmission.inc",(Section $chat '    if (!source || !source->IsInWorld())' '    CreatureTextGroup const& textGroupContainer'))
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $constants=@([regex]::Matches($all,'\bSMART_EVENT_[A-Z_]+\b')|ForEach-Object {$_.Value}|Where-Object {$_ -notlike 'SMART_EVENT_FLAG_*'}|Sort-Object -Unique)
 $enum=@($constants|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*(\d+)\s*,");if(!$m.Success){throw "Missing native event enum $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 $flags=@([regex]::Matches($all,'\bSMART_EVENT_FLAG_[A-Z_]+\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $extra='';foreach($flag in $flags){$m=[regex]::Match($header,"\b$flag\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing flag $flag"};$extra+="constexpr uint32 $flag=$($m.Groups[1].Value -replace '//.*','');`n"}
 $extra+='constexpr uint32 SMART_TARGET_SELF=1,SMART_TARGET_VICTIM=2,SMART_ACTION_DIE=37;'
 $action=[regex]::Match($header,'SMART_ACTION_TALK\s*=\s*(\d+)\s*,').Groups[1].Value
 [IO.File]::WriteAllText("$out/EventConstants.inc","enum SMART_EVENT {$enum}; constexpr uint32 SMART_ACTION_TALK=$action; $extra")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MistWarderSpeakersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`" `"$data/texts.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Mist Warder speaker regression failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawWardstoneCatalog.cpp -ProbeName TalonclawWardstoneCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv"
  if($native -notcontains "SCENARIO_EVENT`t48069`t973`t2`t46184`t46185"){throw 'Native Mist Warder stage differs'}
  foreach($id in @(103865,135117,103866)){if(!@($native|Where-Object {$_ -like "TEXT`t$id`t*"}).Count){throw "Native Mist Warder text missing $id"}}
  if(!@($native|Where-Object {$_ -like "EFFECT`t3355`t0`t0`t6`t12`t*"}).Count){throw 'Native Freezing Trap stun aura differs'}
  'PASS matching26972 Mist Warder own three text groups,48069 scenario973/2 and original3355 freezing aura'
 }
}
