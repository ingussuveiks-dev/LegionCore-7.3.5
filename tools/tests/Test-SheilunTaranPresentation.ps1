param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp';$header='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_taran_intro.h'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$intro=(Get-Content "$repo/$header" -Raw).Replace("`r",'')
$old=(& git -C $repo show "d52af1bc7:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-timer source missing'}
$oldIntro=(& git -C $repo show "d52af1bc7:$header") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-timer opening helper missing'}
$normal=$source.Replace('        // These callbacks capture this AI, so they must not outlive it.','').Replace('        // Keep the older combat/rescue queue on its original UpdateAI clock.','').Replace('        FunctionProcessor _presentationFunctions;','').Replace('_presentationFunctions.AddDelayedEvent(4000,','me->AddDelayedEvent(4000,').Replace('_presentationFunctions.AddDelayedEvent(5000,','me->AddDelayedEvent(5000,')
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Sheilun delta exceeds two AI-owned presentation timers and appended processor'}
$normal=$intro.Replace('    // Preserve the former Unit timer clock, including stun and suspended AI.','').Replace('    _presentationFunctions.Update(diff);','')
if(($normal -replace '\s','') -cne ($oldIntro -replace '\s','')){throw 'Opening helper delta exceeds one presentation lifecycle clock'}
$actor=Block $source 'struct npc_tazhan_chuAI :';$oldActor=Block $old 'struct npc_tazhan_chuAI :'
if(([regex]::Matches($actor,'_presentationFunctions.AddDelayedEvent')).Count -ne 2 -or ([regex]::Matches($intro,'_presentationFunctions.Update\(diff\)')).Count -ne 1){throw 'Unexpected queue consumers/clock count'}
if($ScopeOnly){'PASS exact whole-file d52af1bc7 certificate: only two4s/5s capture owners, appended AI processor and one lifecycle clock; original reset, rescue/combat queue, events, paths, spells, casts, gossip, wounds, credits and other actors retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-taran-presentation$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$selected=if($Baseline){$oldActor}else{$actor}
# Full actual AI except independently tested opening/wound endpoints. Presentation
# uses the exact actual lifecycle prefix, ahead of the opening's early return.
$life=Block $(if($Baseline){$oldIntro}else{$intro}) 'void UpdateLifecycle('
$boundary=if($Baseline){'    RecoverWound();'}else{'    _presentationFunctions.Update(diff);'}
$life=$life.Substring(0,$life.IndexOf($boundary)+$boundary.Length)+"`n}"
$selected=$selected.Replace('#include "sheilun_taran_intro.h"',"void CancelIntroHeal() { }`n$life").Replace('#include "sheilun_companion_wounds.h"','bool _pendingWoundHeal = false; void RecoverWound() { }')
[IO.File]::WriteAllText("$out/Actor.inc",$selected+';')
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$eh=(Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw).Replace('#include "Define.h"','').Replace('#include "Duration.h"','').Replace('TC_GAME_API ','');$es=(Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw).Replace('#include "EventMap.h"','').Replace('#include "Random.h"','')
[IO.File]::WriteAllText("$out/EventMap.inc",$eh+$es)
$cs=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $cs $pair[1]))}
[IO.File]::WriteAllText("$out/SafeLOS.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'void CreatureAI::MoveInLineOfSight_Safe('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SheilunTaranPresentationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Taran presentation ownership regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sheilun_taran_presentation_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Taran timer Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
