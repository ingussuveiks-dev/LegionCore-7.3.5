param([switch]$VerifyNativeCatalog,[string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/desmond-search-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/desmond-search"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $start"};$text.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $events=Section $s 'void SmartScript::ProcessEvent(' 'void SmartScript::InitTimer('
 $sections=[ordered]@{
 'FindLinkedEvent'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'EventDispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'ActionPrefix'=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')
 'CastAction'=@('        case SMART_ACTION_CAST:','        case SMART_ACTION_CAST_CUSTOM:')
 'CloseAction'=@('        case SMART_ACTION_CLOSE_GOSSIP:','        case SMART_ACTION_EQUIP:')
 }
 $eventSections=[ordered]@{
 'EventPrefix'=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
 'SelectEvent'=@('        case SMART_EVENT_GOSSIP_SELECT:','        case SMART_EVENT_DUMMY_EFFECT:')
 'NoParamEvents'=@('        case SMART_EVENT_AGGRO:','        case SMART_EVENT_IS_BEHIND_TARGET:')
 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 foreach($name in $eventSections.Keys){$v=Section $events $eventSections[$name][0] $eventSections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $cs=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $predicates=[ordered]@{'QuestCondition'=@('        case CONDITION_QUESTTAKEN:','        case CONDITION_QUEST_COMPLETE:');'PlayerCondition'=@('        case CONDITION_OBJECT_ENTRY:','        case CONDITION_TYPE_MASK:');'AliveCondition'=@('        case CONDITION_ALIVE:','        case CONDITION_HP_VAL:')}
 foreach($name in $predicates.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $cs $predicates[$name][0] $predicates[$name][1]))}
 $binding=[regex]::Match($cs,'ConditionSourceInfo sourceInfo\(unit, baseObject\);');if(!$binding.Success){throw 'Native initial source22 target binding changed'};[IO.File]::WriteAllText("$out/SmartTargetBinding.inc",$binding.Value)
 $effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/PersonalCredit.inc",(Section $effects 'void Spell::EffectKillCreditPersonal(' 'void Spell::EffectKillCredit('))
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $names=@([regex]::Matches($all,'\b(?:SMART_EVENT_|SMART_ACTION_|SMART_TARGET_|SMARTCAST_|CAST_FORCE_TARGET_SELF)[A-Z_0-9]*\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $eventNames=@($names|Where-Object {$_ -like 'SMART_EVENT_*' -and $_ -notlike 'SMART_EVENT_FLAG_*'})
 $enum=@($eventNames|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing enum $_"};"$_=$($m.Groups[1].Value -replace '//.*','')"}) -join ','
 $decl="enum SMART_EVENT : uint32 {$enum};`n"
 foreach($name in @($names|Where-Object {$_ -notin $eventNames})){$m=[regex]::Match($header,"\b$name\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing constant $name"};$decl+="constexpr uint32 $name=$($m.Groups[1].Value -replace '//.*','');`n"}
 [IO.File]::WriteAllText("$out/Constants.inc",$decl)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/DesmondSearchTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`" `"$data/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Desmond search regression failed'}
 if($VerifyNativeCatalog){$oldConfig=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SubtletyContactCatalog.cpp -ProbeName SubtletyContactCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$oldConfig}
  $native=Get-Content "$out/native.tsv"
  if($native -notcontains "CONTACT`t208755`t90`t105332" -or $native -notcontains "SPELL`t208755`tSearching" -or $native -notcontains "EFFECT`t208755`t0`t0`t90`t0`t105332`t0`t0`t1`t0" -or $native -notcontains "MISC`t208755`t0`t1500`t1" -or @($native|Where-Object {$_ -like "EFFECT`t208755`t*"}).Count -ne 1){throw 'Actual26972 Searching spell/cast/credit/target differs'}
  'PASS actual26972 Searching208755:1500ms, sole effect90/105332 self1, no trigger credit'
 }

}
