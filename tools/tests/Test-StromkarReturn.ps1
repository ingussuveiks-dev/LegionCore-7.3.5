param([switch]$AddressSanitizer,[string]$RulesFile,[switch]$LifetimeBaseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-return"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 if($RulesFile -and !(Test-Path -LiteralPath $RulesFile -PathType Leaf)){throw "Return rules export missing: $RulesFile"}
 function Section([string]$s,[string]$a,[string]$b){$start=$s.IndexOf($a);$end=$s.IndexOf($b,[Math]::Max(0,$start));if($start -lt 0 -or $end -le $start){throw "Missing native $a"};$s.Substring($start,$end-$start)}
 function Block([string]$s,[string]$a){$start=$s.IndexOf($a);if($start -lt 0){throw "Missing native $a"};$end=$s.IndexOf('{',$start)+1;$depth=1;while($depth){if($s[$end] -eq '{'){++$depth};if($s[$end] -eq '}'){--$depth};++$end};$s.Substring($start,$end-$start)}
 $s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $cases=(Section $s '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $s '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $s '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $s '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $s '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $s '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')+(Section $s '        case CONDITION_OBJECT_ENTRY:' '        case CONDITION_TYPE_MASK:')+(Section $s '        case CONDITION_QUEST_OBJECTIVE_DONE:' '        case CONDITION_INSTANCE_INFO:')
 [IO.File]::WriteAllText("$out/Conditions.inc",$cases)
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Block $s 'bool ConditionMgr::IsObjectMeetToConditionList('))
 $s=Get-Content "$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp" -Raw
 [IO.File]::WriteAllText("$out/AreaConditions.inc",(Block $s 'bool AreaTrigger::CheckActionConditions('))
 [IO.File]::WriteAllText("$out/AreaLifetime.inc",(Section $s '    if (GetDuration() != -1)' '    if (_activationDelay)'))
 [IO.File]::WriteAllText("$out/AreaCast.inc",(Section $s '        case AT_ACTION_TYPE_CAST_SPELL:' '        case AT_ACTION_TYPE_ADD_STACK:'))
 [IO.File]::WriteAllText("$out/AreaSelfCaster.inc",(Section $s '        if ((spellInfo->Effects[EFFECT_0]->TargetA.GetTarget()' '    if (action.action->targetFlags & AT_TARGET_FLAG_CASTER_AURA_TARGET)'))
 [IO.File]::WriteAllText("$out/AreaTrigger.h",'// Native area services are explicit fixture endpoints.')
 $summonSource=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
 [IO.File]::WriteAllText("$out/SummonLifetime.inc",(Section $summonSource '        case TEMPSUMMON_MANUAL_DESPAWN:' '        case TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT:'))
 $controller=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/stromkar_return.h" -Raw
 if($LifetimeBaseline){$controller=(git -C $repo show 'f43bcaef3:src/server/scripts/Legion/ClassHalls/stromkar_return.h') -join "`n"}
 [IO.File]::WriteAllText("$out/ReturnController.inc",$controller)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 if($LifetimeBaseline){$asan+=' /DLIFETIME_BASELINE'}
 $ruleArg=if($RulesFile){' "'+[IO.Path]::GetFullPath($RulesFile)+'"'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/StromkarReturnTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"$ruleArg`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Stromkar return controller failed'}
}
