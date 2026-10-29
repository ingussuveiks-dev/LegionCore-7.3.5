param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/talonclaw-wardstone-data",[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-wardstone"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
 function Section([string]$s,[string]$a,[string]$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $a"};$s.Substring($i,$j-$i)}
 $cs=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $cases=(Section $cs '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $cs '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $cs '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $cs '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $cs '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $cs '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')+(Section $cs '        case CONDITION_OBJECT_ENTRY:' '        case CONDITION_TYPE_MASK:')
 [IO.File]::WriteAllText("$out/Conditions.inc",$cases)
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $cs 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 [IO.File]::WriteAllText("$out/ClickConditions.inc",(Section $cs 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent(' 'ConditionList ConditionMgr::GetConditionsForVehicleSpell('))
 [IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $cs 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
 $p=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/ClickVisibility.inc",(Section $p 'bool Player::canSeeSpellClickOn(' 'void Player::SendTalentsInfoData('))
 $u=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/HandleSpellClick.inc",(Section $u 'bool Unit::HandleSpellClick(' 'void Unit::EnterVehicle('))
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $parts=[ordered]@{
 'FindLinkedEvent'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
 'LinkTail'=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
 'EventDispatch'=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
 'EventPrefix'=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
 'ActionPrefix'=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')
 'LinkEvent'=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
 'SpellHitEvent'=@('        case SMART_EVENT_SPELLHIT:','        case SMART_EVENT_OOC_LOS:')
 'CastAction'=@('        case SMART_ACTION_CAST:','        case SMART_ACTION_CAST_CUSTOM:')
 'CreditAction'=@('        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:','        case SMART_ACTION_SUMMON_CONVERSATION:')
 'CreditOwner'=@('Player* GetScriptCreditPlayer(','Unit* GetScriptCreditCaster(')
 }
 $all='';foreach($name in $parts.Keys){$v=Section $s $parts[$name][0] $parts[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
 $binding=[regex]::Match($spell,'ConditionSourceInfo condInfo = ConditionSourceInfo\(m_caster\);\s*condInfo\.mConditionTargets\[1\] = m_targets\.GetObjectTarget\(\);');if(!$binding.Success){throw 'Source17 caster/actual object binding changed'};[IO.File]::WriteAllText("$out/CastBinding.inc",$binding.Value)
 $scenario=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
 [IO.File]::WriteAllText("$out/CriteriaSnapshot.inc",$scenario.Substring($scenario.IndexOf('void Scenario::UpdateAchievementCriteria(AchievementCachePtr cache)')))
 [IO.File]::WriteAllText("$out/CriteriaFilter.inc",(Section $scenario 'bool Scenario::CanUpdateCriteria(' 'Challenge* Scenario::GetChallenge('))
 $achievement=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
 $amount=[regex]::Match($achievement,'if \(!requiredAmount\)\s*requiredAmount = 1;');if(!$amount.Success){throw 'Native zero amount normalization changed'};[IO.File]::WriteAllText("$out/CriteriaAmount.inc",$amount.Value)
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $names=@([regex]::Matches($all,'\b(?:SMART_EVENT_|SMART_ACTION_|SMART_TARGET_|SMARTCAST_|CAST_FORCE_)[A-Z_0-9]*\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)
 $events=@($names|Where-Object {$_ -like 'SMART_EVENT_*' -and $_ -notlike 'SMART_EVENT_FLAG_*'});$decl='enum SMART_EVENT : uint32 {'+(@($events|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing $_"};"$_=$($m.Groups[1].Value -replace '//.*','')"}) -join ',')+'};'
 foreach($name in @($names|Where-Object {$_ -notin $events})){$m=[regex]::Match($header,"\b$name\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing $name"};$decl+="constexpr uint32 $name=$($m.Groups[1].Value -replace '//.*','');`n"};[IO.File]::WriteAllText("$out/Constants.inc",$decl)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/TalonclawWardstoneTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Wardstone admission/once replay failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawWardstoneCatalog.cpp -ProbeName TalonclawWardstoneCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv";foreach($line in @("EFFECT`t199728`t0`t0`t3`t0`t0`t0`t0`t25`t0`t0","MISC`t199728`t0`t0`t4000","WARDSTONE_AMOUNT`t3`t0`t46484","WARDSTONE_AMOUNT`t4`t3`t46187","SCENARIO_EVENT`t48069`t973`t2`t46184`t46185","EFFECT`t199823`t0`t2`t1`t0`t0`t0`t0`t87`t8`t0")){if($native -notcontains $line){throw "Native Wardstone payload differs $line"}}
  'PASS matching26972 real target25/dummy/4000ms Taking, native amount0->one receipt versus3, MistWarder48069 and original Wardstone-only instant-kill effect'
 }
}
