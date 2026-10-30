#requires -Version 7.0
param([switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[switch]$VerifyNativeCatalog,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/smart-linked-cast-retry/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 & "$PSScriptRoot/Test-WardstoneLinkedCastData.ps1" -OutputDirectory "$out/data" -VerifyLiveData:$VerifyLiveData
 function Block($s,$a){$i=$s.IndexOf($a);if($i -lt 0){throw "Missing $a"};$j=$s.IndexOf('{',$i)+1;$depth=1;while($depth -and $j -lt $s.Length){if($s[$j] -eq '{'){++$depth};if($s[$j] -eq '}'){--$depth};++$j};if($depth){throw "Unclosed $a"};$s.Substring($i,$j-$i)}
 function Section($s,$a,$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max($i,0));if($i -lt 0 -or $j -le $i){throw "Missing $a/$b"};$s.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $parts=[ordered]@{
 'RetryQueue.inc'=Block $s 'void SmartScript::QueueLinkedCastRetry('
 'RetryBegin.inc'=Block $s 'uint32 SmartScript::BeginLinkedCastAttempt('
 'RetryFinish.inc'=Block $s 'bool SmartScript::FinishLinkedCastAttempt('
 'RetryUpdate.inc'=Block $s 'void SmartScript::UpdateLinkedCastRetries('
 'RetryReset.inc'=Block $s 'void SmartScript::ResetLinkedCastRetries('
 'RetryCorpseUpdate.inc'=Block $s 'void SmartScript::OnCorpseUpdate('
 'RetryOnReset.inc'=Block $s 'void SmartScript::OnReset('
 'RetryFindLink.inc'=Block $s 'SmartScriptHolder SmartScript::FindLinkedEvent('
 'RetryDispatch.inc'=Block $s 'void SmartScript::ProcessEventsFor('
 'RetryEventPrefix.inc'=Section $s 'void SmartScript::ProcessEvent(' '    switch (e.GetEventType())'
 'RetryActionPrefix.inc'=Section $s 'void SmartScript::ProcessAction(' '    switch (e.GetActionType())'
 'RetryUpdateEvents.inc'=Section $s '        case SMART_EVENT_LINK: //special handling' '        case SMART_EVENT_HEALTH_PCT:'
 'RetrySpellHit.inc'=Section $s '        case SMART_EVENT_SPELLHIT_TARGET:' '        case SMART_EVENT_OOC_LOS:'
 'RetryCast.inc'=Block $s '        case SMART_ACTION_CAST:'
 'RetryLinkTail.inc'=Section $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('
 'RetryInitTimer.inc'=Block $s 'void SmartScript::InitTimer('
 'RetryRecalcTimer.inc'=Block $s 'void SmartScript::RecalcTimer('
 }
 $ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 $parts['RetryCorpseAI.inc']=Block $ai 'void SmartAI::UpdateCorpseAI('
 $creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
 $corpse=Section $creature '        case CORPSE:' '        case ALIVE:'
 $parts['RetryCorpseDispatch.inc']=Section $corpse '            Unit::Update(diff);' '            if (m_groupLootTimer'
 $baseAI=Get-Content "$repo/src/server/game/AI/CreatureAI.h" -Raw
 $parts['RetryCorpseDefault.inc']=Block $baseAI 'virtual void UpdateCorpseAI('
 $h=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.h" -Raw;$parts['RetryState.inc']=Section $h '        struct LinkedCastRetry' '        void IncPhase('
 $v=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.cpp" -Raw;$parts['RetryValidation.inc']=Section $v '        case SMART_ACTION_ADD_AURA:' '        case SMART_ACTION_CALL_AREAEXPLOREDOREVENTHAPPENS:'
 $all='';foreach($key in $parts.Keys){[IO.File]::WriteAllText("$out/$key",$parts[$key]);$all+=$parts[$key]}
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $names=@([regex]::Matches($all+' SMART_EVENT_RESET SMART_EVENT_SPELLHIT SMART_SCRIPT_TYPE_TIMED_ACTIONLIST SMART_SCRIPT_TYPE_GAMEOBJECT','\b(?:SMART_EVENT_|SMART_ACTION_|SMART_TARGET_|SMARTCAST_|SMART_SCRIPT_TYPE_|CAST_FORCE_)[A-Z_0-9]*\b')|ForEach-Object{$_.Value}|Sort-Object -Unique)
 $events=@($names|Where-Object{$_ -like 'SMART_EVENT_*' -and $_ -notlike 'SMART_EVENT_FLAG_*'});$constants='';$ev=@();foreach($name in $names){$m=[regex]::Match($header,"\b$name\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Native constant missing $name"};$value=$m.Groups[1].Value -replace '//.*','';if($name -in $events){$ev+="$name=$value"}else{$constants+="constexpr uint32 $name=$value;`n"}}
 $defines=Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw;$dead=[regex]::Match($defines,'\bSPELL_ATTR0_CASTABLE_WHILE_DEAD\s*=\s*(0x[0-9A-Fa-f]+)');if(!$dead.Success){throw 'Native dead-caster flag missing'}
 [IO.File]::WriteAllText("$out/RetryConstants.inc","enum SMART_EVENT : uint32 {$($ev -join ',')};`n$constants`nconstexpr uint32 SPELL_ATTR0_CASTABLE_WHILE_DEAD=$($dead.Groups[1].Value);`n")
 $update=Block $s 'void SmartScript::OnUpdate(';if($update -notmatch 'UpdateLinkedCastRetries\(diff\)') {throw 'Native OnUpdate lost retry dispatch'}
 $initialize=Block $s 'void SmartScript::OnInitialize(';if($initialize -notmatch 'ResetLinkedCastRetries\(\)'){throw 'Native initialization lost retry cleanup'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SmartLinkedCastRetryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/data/original-events.tsv`" `"$out/data/events.tsv`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual native linked-cast retry regression failed'}
 if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawWardstoneCatalog.cpp -ProbeName TalonclawWardstoneCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior};$native=[IO.File]::ReadAllLines("$out/native.tsv");foreach($id in @(187027,199770)){$attr=@($native|Where-Object{$_ -like "ATTR`t$id`t*"});if($attr.Count -ne 1){throw 'Native cast attributes missing'};$allowed=([uint32]$attr[0].Split("`t")[2] -band [Convert]::ToUInt32($dead.Groups[1].Value.Substring(2),16)) -ne 0;if($allowed -ne ($id -eq 187027)){throw 'Native corpse-caster admission differs'}};if($native -cnotcontains "MISC`t199823`t0`t4000`t0"){throw 'Native lightning duration/cast identity differs'}}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_linked_cast_retry_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual linked-cast retry PE failed'}}finally{$env:PYTHONPATH=$prior}}
}
