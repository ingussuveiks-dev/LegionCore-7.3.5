param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$text,[string]$start){$a=$text.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $text.Length){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$text.Substring($a,$b-$a)}
function Shape([string]$text){($text -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "ad164392f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-Hait source missing'}
$actor=Block $source 'class npc_master_hait :';$oldActor=Block $old 'class npc_master_hait :'
$helper=Block $actor 'static void AttackDemon('
$expectedHelper=@'
static void AttackDemon(Creature* demon, ObjectGuid haitGuid, uint8 remaining = 10)
        {
            Creature* hait = ObjectAccessor::GetCreature(*demon, haitGuid);
            if (!demon->IsAlive() || !hait || !hait->IsAlive())
                return;
            if (CreatureAI* ai = demon->AI())
            {
                ai->AttackStart(hait);
                return;
            }
            if (!remaining)
            {
                TC_LOG_ERROR("scripts", "Monk intro: demon %s has no AI for its delayed attack after 10 retries", demon->GetGUID().ToString().c_str());
                return;
            }
            demon->AddDelayedEvent(500, [demon, haitGuid, remaining]() -> void
            {
                AttackDemon(demon, haitGuid, remaining - 1);
            });
        }
'@
if((Shape $helper) -cne (Shape $expectedHelper)){throw 'Hait finite original-GUID attack helper changed'}
$addition=@'

        FunctionProcessor _presentationFunctions;

        void UpdateLifecycle(uint32 diff) override
        {
            if (me->getDeathState() == ALIVE || me->getDeathState() == CORPSE)
                _presentationFunctions.Update(diff);
        }
'@
$normalized=$actor.Replace($helper,'').Replace($addition,'').Replace('_presentationFunctions.AddDelayedEvent(', 'me->AddDelayedEvent(')
$normalized=[regex]::Replace($normalized,'sCreatureTextMgr->SendChat\(chuan, (\d)\);',{param($m)'chuan->AI()->Talk('+$m.Groups[1].Value+');'})
$normalized=$normalized.Replace('ObjectGuid const chuanGuid = chuan->GetGUID();','').Replace('ObjectGuid const haitGuid = me->GetGUID();','').Replace('[demon, chuanGuid]','[demon, chuan]').Replace('demon->AddDelayedEvent(23000, [demon, chuanGuid, haitGuid]','chuan->AddDelayedEvent(23000, [this, demon, chuan]')
$normalized=[regex]::Replace($normalized,'if \(Creature\* chuan = ObjectAccessor::GetCreature\(\*demon, chuanGuid\)\)\s+(chuan->CastSpell\(demon, 227140\);)','$1')
$normalized=[regex]::Replace($normalized,'if \(Creature\* chuan = ObjectAccessor::GetCreature\(\*demon, chuanGuid\)\)\s*\{\s*(chuan->SetFlag\(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC\);\s*chuan->CastSpell\(chuan, 185627\);)\s*\}','$1')
$normalized=[regex]::Replace($normalized,'if \(Creature\* hait = ObjectAccessor::GetCreature\(\*demon, haitGuid\)\)\s*sCreatureTextMgr->SendChat\(hait, 4\);','Talk(4);').Replace('AttackDemon(demon, haitGuid);','demon->AI()->AttackStart(me);')
if((Shape $normalized) -cne (Shape $oldActor)){throw 'Other Hait actions/admission/timers/casts/flags/vehicle/credit/motion changed'}
$outside=$source.Replace($actor,$oldActor).Replace('#include "ObjectAccessor.h"','')
$outside=$outside.Replace("if (me->getDeathState() == ALIVE || me->getDeathState() == CORPSE)`n                _presentationFunctions.Update(diff);",'_presentationFunctions.Update(diff);')
if((Shape $outside) -cne (Shape $old)){throw 'Other Monk actors beyond native timer death clocks changed'}
if(([regex]::Matches($source,'if \(me->getDeathState\(\) == ALIVE \|\| me->getDeathState\(\) == CORPSE\)')).Count -ne 3){throw 'All three moved AI clocks must retain native ALIVE/CORPSE dispatch'}
if(([regex]::Matches($actor,'_presentationFunctions.AddDelayedEvent\(')).Count -ne 7 -or $actor.Contains('me->AddDelayedEvent(') -or $actor.Contains('[this, demon')){throw 'Seven owned self-AI callbacks/full foreign GUIDs required'}
if($ScopeOnly){'PASS exact seven Hait AI timers/five native Chuan texts/two foreign GUID callbacks with vehicle-owned cleanup and finite actual-AI attack retry; other native gameplay/actors unchanged';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Broken counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-hait-timers$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldActor}else{$actor})+';')
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
[IO.File]::WriteAllText("$out/TalkForward.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'void CreatureAI::Talk(uint8 id, ObjectGuid WhisperGuid)'))
[IO.File]::WriteAllText("$out/DeathState.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum DeathState')+';')
$cs=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $cs $pair[1]))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/MonkHaitTimersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Hait lifetime regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_monk_hait_timers_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Hait Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
