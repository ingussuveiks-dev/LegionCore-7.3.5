param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/halak-presentation-data",[switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/halak-presentation"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/halak-presentation-data","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-HalakPresentationData.ps1" -VerifyLiveData:$VerifyLiveData
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $sections=[ordered]@{
 'CreditPlayer'=@('Player* GetScriptCreditPlayer(WorldObject* object)','Unit* GetScriptCreditCaster(')
 'PlayerResolver'=@('namespace SmartInvokerPlayer','ObjectList* SmartScript::GetTargets(')
 'PlayerTarget'=@('        case SMART_TARGET_ACTION_INVOKER_PLAYER:','        case SMART_TARGET_POSITION:')
 'InvokerTarget'=@('        case SMART_TARGET_NONE:','        case SMART_TARGET_ACTION_INVOKER_VEHICLE:')
 'ConversationAction'=@('        case SMART_ACTION_SUMMON_CONVERSATION:','        case SMART_ACTION_SUMMON_ADD_PLR_PERSONNAL_VISIBILE:')
 'CriteriaAction'=@('        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:','        case SMART_ACTION_SUMMON_CONVERSATION:')
 'UpdateEvent'=@('        //called from Update tick','        case SMART_EVENT_HEALTH_PCT:')
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
 'TalkAction'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'TextTimer'=@('    if (mUseTextTimer && me)','void SmartScript::FillScript(')
 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $target=$s.Substring($s.IndexOf('ObjectList* SmartScript::GetTargets('));$target=$target.Substring(0,$target.IndexOf('ObjectList* SmartScript::GetWorldObjectsInDist('))
 [IO.File]::WriteAllText("$out/TargetTail.inc",$target.Substring($target.LastIndexOf('    if (GetBaseObject() && GetBaseObject()->HasQuestObjectScope())')))
 $owner=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Section $owner 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const' 'Player* Unit::GetAffectingPlayer() const'))
 $chat=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/ChatAdmission.inc",(Section $chat '    if (!source || !source->IsInWorld())' '    CreatureTextGroup const& textGroupContainer'))
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $constants=@([regex]::Matches($all,'\bSMART_EVENT_[A-Z_]+\b')|ForEach-Object {$_.Value}|Where-Object {$_ -notlike 'SMART_EVENT_FLAG_*'}|Sort-Object -Unique)
 $enum=@($constants|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*(\d+)\s*,");if(!$m.Success){throw "Missing native event enum $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 $flags=@([regex]::Matches($all,'\bSMART_EVENT_FLAG_[A-Z_]+\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $extra='';foreach($flag in $flags){$m=[regex]::Match($header,"\b$flag\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing flag $flag"};$extra+="constexpr uint32 $flag=$($m.Groups[1].Value -replace '//.*','');`n"}
 foreach($name in @('SMART_TARGET_SELF','SMART_TARGET_VICTIM','SMART_TARGET_NONE','SMART_TARGET_ACTION_INVOKER','SMART_TARGET_ACTION_INVOKER_PLAYER','SMART_ACTION_SUMMON_CONVERSATION','SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA')){$match=[regex]::Match($header,"\b$name\s*=\s*(\d+)\s*,");if(!$match.Success){throw "Missing native constant $name"};$extra+="constexpr uint32 $name=$($match.Groups[1].Value);"}
 $action=[regex]::Match($header,'SMART_ACTION_TALK\s*=\s*(\d+)\s*,').Groups[1].Value
 [IO.File]::WriteAllText("$out/EventConstants.inc","enum SMART_EVENT {$enum}; constexpr uint32 SMART_ACTION_TALK=$action; $extra")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/HalakPresentationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`" `"$data/texts.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native acquisition combat speaker regression failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile HalakPresentationCatalog.cpp -ProbeName HalakPresentationCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv" -Raw
  foreach($record in @("SPELL`t79961`tHoly Shock","SPELL`t192648`tHoly Light","SPELL`t195272`tAvenging Wrath","TEXT`t96953`tThe Light... no...","CONVERSATION_LINE`t1769`t97442`t0`tThe soul of Halahk")){if(!$native.Contains($record)){throw "Matching26972 Halak payload differs: $record"}}
  'PASS native26972 retained three combat spells, death96953 and real741 line1769/Broadcast97442'
 }
 if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_halak_presentation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Halak linked native presentation differs'}}finally{$env:PYTHONPATH=$old}}
}
