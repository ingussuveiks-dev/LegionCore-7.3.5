param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/artifact-boss-speakers-data",[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-boss-speakers"
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
 'TalkAction'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'TextTimer'=@('    if (mUseTextTimer && me)','void SmartScript::FillScript(')
 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $s $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $chat=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/ChatAdmission.inc",(Section $chat '    if (!source || !source->IsInWorld())' '    CreatureTextGroup const& textGroupContainer'))
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $constants=@([regex]::Matches($all,'\bSMART_EVENT_[A-Z_]+\b')|ForEach-Object {$_.Value}|Where-Object {$_ -notlike 'SMART_EVENT_FLAG_*'}|Sort-Object -Unique)
 $enum=@($constants|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*(\d+)\s*,");if(!$m.Success){throw "Missing native event enum $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 $flags=@([regex]::Matches($all,'\bSMART_EVENT_FLAG_[A-Z_]+\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $extra='';foreach($flag in $flags){$m=[regex]::Match($header,"\b$flag\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing flag $flag"};$extra+="constexpr uint32 $flag=$($m.Groups[1].Value -replace '//.*','');`n"}
 $extra+='constexpr uint32 SMART_TARGET_SELF=1,SMART_TARGET_VICTIM=2;'
 $action=[regex]::Match($header,'SMART_ACTION_TALK\s*=\s*(\d+)\s*,').Groups[1].Value
 [IO.File]::WriteAllText("$out/EventConstants.inc","enum SMART_EVENT {$enum}; constexpr uint32 SMART_ACTION_TALK=$action; $extra")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ArtifactBossSpeakersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/original.tsv`" `"$data/repaired.tsv`" `"$data/texts.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native artifact boss speaker regression failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ArtifactBossSpeakersCatalog.cpp -ProbeName ArtifactBossSpeakersCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv"
  foreach($line in @("SCENARIO_EVENT`t48421`t991`t7`t46620`t46621","SCENARIO_EVENT`t51391`t1122`t4`t48469`t48470")){if($native -notcontains $line){throw "Actual26972 boss metadata differs: $line"}}
  foreach($id in @(104575,105388,105390,105391,113373,113374)){if(!@($native|Where-Object {$_ -like "TEXT`t$id`t*"}).Count){throw "Native boss text missing $id"}}
  'PASS matching26972 six text payloads/sound IDs and native991/7 Farthing/1122/4 Balaadur event metadata'
 }
}
