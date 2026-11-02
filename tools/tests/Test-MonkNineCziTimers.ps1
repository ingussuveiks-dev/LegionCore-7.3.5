param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$text,[string]$start){$a=$text.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $text.Length){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$text.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "aa1513802:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-timer source missing'}
if($source.Contains('#include "chen_rescue.h"')){
 & "$PSScriptRoot/Test-MonkChenRescue.ps1" -ScopeOnly
 $preRescue=(& git -C $repo show "6b6fae36c:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-rescue actor missing'}
 $source=$source.Replace((Block $source 'class npc_monk_chen_porter :'),(Block $preRescue 'class npc_monk_chen_porter :')).Replace('#include "InstanceScript.h"','').Replace('#include "chen_rescue.h"','')
}
$actor=Block $source 'class npc_nine_czi :';$oldActor=Block $old 'class npc_nine_czi :'
$addition=@'

        // These callbacks use this AI. The Unit queue can outlive an AI swap.
        FunctionProcessor _presentationFunctions;

        void UpdateLifecycle(uint32 diff) override
        {
            if (me->getDeathState() == ALIVE || me->getDeathState() == CORPSE)
                _presentationFunctions.Update(diff);
        }
'@
$normalized=$actor.Replace($addition,'').Replace('_presentationFunctions.AddDelayedEvent(', 'me->AddDelayedEvent(').Replace("                    ObjectGuid const playerGuid = pl->GetGUID();`n",'').Replace('[this, playerGuid]','[this, pl]').Replace('Talk(8, playerGuid);','Talk(8, pl->GetGUID());')
if(($normalized -replace '\s','') -cne ($oldActor -replace '\s','')){throw 'Other Nine Czi admission/spells/movement/credit/reset/combat changed'}
$outside=$source.Replace($actor,$oldActor).Replace('#include "FunctionProcessor.h"','').Replace('#include "ObjectAccessor.h"','')
$chen=Block $source 'class npc_monk_chen_porter :';$oldChen=Block $old 'class npc_monk_chen_porter :'
if(($chen -replace '\s','') -cne ($oldChen -replace '\s','')){& "$PSScriptRoot/Test-MonkChenTimers.ps1" -ScopeOnly;if(!$?){throw 'Successor Chen scope failed'};$outside=$outside.Replace($chen,$oldChen)}
$hait=Block $source 'class npc_master_hait :';$oldHait=Block $old 'class npc_master_hait :'
if(($hait -replace '\s','') -cne ($oldHait -replace '\s','')){& "$PSScriptRoot/Test-MonkHaitTimers.ps1" -ScopeOnly;if(!$?){throw 'Successor Hait scope failed'};$outside=$outside.Replace($hait,$oldHait)}
if($source.Contains('#include "monk_hait_click.h"')){foreach($include in @('Player.h','Group.h','QuestData.h','Scenario.h','ScenarioMgr.h','SpellMgr.h','SpellScript.h','Timer.h','monk_hait_click.h')){$outside=$outside.Replace('#include "'+$include+'"','')};$outside=$outside.Replace('RegisterSpellScript(spell_monk_hait_click);','')}
if(($outside -replace '\s','') -cne ($old -replace '\s','')){throw 'Other Monk actors changed'}
if(([regex]::Matches($actor,'_presentationFunctions.AddDelayedEvent\(')).Count -ne 4 -or $actor.Contains('me->AddDelayedEvent(') -or $actor.Contains('[this, pl]')){throw 'Four actor-owned callbacks and immutable player GUID required'}
if($ScopeOnly){'PASS exact four Nine Czi timer owners/one immutable GUID; other native gameplay/actors unchanged';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Broken counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-nine-czi-timers$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldActor}else{$actor})+';')
[IO.File]::WriteAllText("$out/DeathState.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum DeathState')+';')
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$cs=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $cs $pair[1]))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/MonkNineCziTimersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Nine Czi timer regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_monk_nine_czi_timers_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Nine Czi Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
