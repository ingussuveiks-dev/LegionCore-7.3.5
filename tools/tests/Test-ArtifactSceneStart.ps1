param([switch]$VerifyLiveData,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-scene-start"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/artifact-scene-start-data") -Test {
 & "$PSScriptRoot/Test-ArtifactSceneStartData.ps1" -VerifyLiveData:$VerifyLiveData
 function Section([string]$text,[string]$start,[string]$end){$a=$text.IndexOf($start);$b=$text.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Missing native $start"};$text.Substring($a,$b-$a)}
 $source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $cases=(Section $source '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')
 [IO.File]::WriteAllText("$out/Conditions.inc",$cases)
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 $source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
 [IO.File]::WriteAllText("$out/SpellConditionBridge.inc",(Section $source '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/InvokerCast.inc",(Section $source '        case SMART_ACTION_INVOKER_CAST:' '        case SMART_ACTION_ADD_AURA:'))
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 [IO.File]::WriteAllText("$out/CastFlags.inc",(Section $source 'enum SmartCastFlags' '// one line in DB'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/ArtifactSceneStartTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n"
 foreach($spell in @(190435,196023,200152,199502,217814)){$cmd+="`"$out/test.exe`" `"$repo/build-extractors/runtime-test/artifact-scene-start-data/rules-$spell.tsv`" $spell`nif not %errorlevel%==0 exit /b %errorlevel%`n"}
 $cmd+="exit /b 0`n";[IO.File]::WriteAllText("$out/run.cmd",$cmd)
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual delayed scene-start source17 regression failed'}
 if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_artifact_scene_start_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical artifact scene source17 bridge differs'}
 }finally{$env:PYTHONPATH=$old}}
}
