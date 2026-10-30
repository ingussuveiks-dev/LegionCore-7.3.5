param([switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/blood-prison-guard-condition"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/blood-prison-guard-data","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-BloodPrisonGuardData.ps1" -VerifyLiveData:$VerifyLiveData
 function Section([string]$source,[string]$start,[string]$end){$a=$source.IndexOf($start);$b=$source.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Missing native guard section $start"};$source.Substring($a,$b-$a)}
 $source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/SourceInfo.inc",(Section $source 'ConditionSourceInfo::ConditionSourceInfo(' 'Condition::Condition()'))
 [IO.File]::WriteAllText("$out/ConditionPrefix.inc",(Section $source 'bool Condition::Meets(' '    switch (ConditionType)'))
 [IO.File]::WriteAllText("$out/NearCondition.inc",(Section $source '        case CONDITION_NEAR_CREATURE:' '        case CONDITION_NEAR_GAMEOBJECT:'))
 [IO.File]::WriteAllText("$out/ScenarioCondition.inc",(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:'))
 [IO.File]::WriteAllText("$out/ConditionTail.inc",(Section $source '    if (NegativeCondition)' 'uint32 Condition::GetSearcherTypeMaskForCondition()'))
 [IO.File]::WriteAllText("$out/ConditionList.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 $source=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp" -Raw
 [IO.File]::WriteAllText("$out/Nearest.inc",(Section $source 'NearestCreatureEntryWithLiveStateInObjectRangeCheck::NearestCreatureEntryWithLiveStateInObjectRangeCheck(' 'AnyPlayerInObjectRangeCheck::'))
 $source=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.h" -Raw
 [IO.File]::WriteAllText("$out/Closest.inc",(Section $source 'inline Creature* GetClosestCreatureWithEntry(' 'inline GameObject* GetClosestGameObjectWithEntry('))
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $line=@($source -split "`n"|Where-Object {$_ -like '*ConditionSourceInfo info = ConditionSourceInfo(unit ? unit : GetBaseObject(), GetBaseObject());*'})
 if($line.Count -ne 1){throw 'Missing native raw-invoker/source condition context'};[IO.File]::WriteAllText("$out/EventInfo.inc",$line[0].TrimEnd("`r")+"`n")
 $data="$repo/build-extractors/runtime-test/blood-prison-guard-data"
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/BloodPrisonGuardConditionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/test.exe`" `"$data/baseline-conditions.tsv`" `"$data/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Blood guard condition investigation failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile BloodPrisonCatalog.cpp -ProbeName BloodPrisonCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv";if($native -notcontains "CRITERION`t3`t46754`t29494`t92`t48683" -or $native -notcontains "EFFECT`t203481`t0`t0`t28`t102955`t64`t0`t47`t0"){throw 'Native Blood940 stage3/keystone context differs'}
 }
 if($VerifyReleaseBinary){$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_blood_guard_condition_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Actual Blood guard condition Release regression failed'}}finally{$env:PYTHONPATH=$previous}}
}
