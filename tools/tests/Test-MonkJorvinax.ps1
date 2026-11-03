param([switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
. "$PSScriptRoot/ChenEscortSpeechPredecessor.ps1"
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-jorvinax"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Prior([string]$path){$s=(& git -C $repo show "7bf4d7a50:$path") -join "`n";if($LASTEXITCODE){throw "Pinned Jorvinax predecessor missing: $path"};$s}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=Prior $path
 $hasPresentation=$source.Contains('if (!MonkChenRescue::Source(me)) _presentationFunctions.AddDelayedEvent(16000,')
 if($hasPresentation){& "$PSScriptRoot/Test-MonkChenPresentation.ps1" -ScopeOnly;$source=(& git -C $repo show "6cb25d055:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-presentation actor missing'}}
 $actor=Block $source 'class npc_master_hait :';$previous=Block $old 'class npc_master_hait :'
 $arrival=Block $actor 'if (id == 8)';$oldArrival=Block $previous 'if (id == 8)'
 $bridge=@'
if (MonkChenRoutes::Source(me))
                    {
                        if (InstanceScript* script = me->GetInstanceScript())
                            script->SetGuidData(MonkJorvinax::ArrivalData, me->GetGUID());
                        return;
                    }
'@
 $body=$oldArrival.Substring($oldArrival.IndexOf('if (Player*'))
 if((Shape $arrival) -cne (Shape ('if (id == 8) {'+$bridge+$body+'}'))){throw 'Hait native final-node bridge changed'}
 if((Shape $actor.Replace($arrival,$oldArrival)) -cne (Shape $previous)){throw 'Other Hait gameplay, native text, combat, clocks or initial actor changed'}
 if((Shape $source.Replace($actor,$previous).Replace('#include "jorvinax_defeat.h"','')) -cne (Shape $old)){throw 'Other Monk actors changed'}
 $hp='src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_rescue.h';$header=(Get-Content "$repo/$hp" -Raw).Replace("`r",'');$header=Get-ChenPreEscortSource $hp $header;$prior=Prior $hp
 $h=$header.Replace('bool genuineBossDefeat = false;','').Replace('genuineBossDefeat = false;','').Replace((Block $header 'bool EscortStage('),'')
 $h=$h.Replace('(scenario->GetCurrentStep() > 5 && !EscortStage(scenario))','scenario->GetCurrentStep() > 5').Replace('if (EscortStage(scenario) && EscortLeaf()','if (scenario->GetCurrentStep() == 5 && EscortLeaf()').Replace('bool handoff = handoffPending && EscortStage(scenario) && EscortLeaf()','bool handoff = handoffPending && lastStep == 5 && EscortLeaf()').Replace('(escort ? progress->GetCurrentStep() != 4 : !EscortStage(progress))','progress->GetCurrentStep() != (escort ? 4u : 5u)')
 if((Shape $h) -cne (Shape $prior)){throw 'Other original Chen receipt or authored route behavior changed'}
 $ip='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$actual=(Get-Content "$repo/$ip" -Raw).Replace("`r",'');$priorInstances=Prior $ip
 if($hasPresentation){$actual=(& git -C $repo show "6cb25d055:$ip") -join "`n";if($LASTEXITCODE){throw 'Certified pre-presentation instance missing'}}
 $normal=$actual.Replace('#include "../../Monk/Intro/jorvinax_defeat.h"','').Replace(', jorvinax(&chenRescue)','').Replace('MonkJorvinax::State jorvinax;','').Replace(' jorvinax.Death(actor, killer);','').Replace('if (type == MonkJorvinax::ArrivalData) jorvinax.Arrival(value);','').Replace(' jorvinax.Update(diff);','')
 if((Shape $normal) -cne (Shape $priorInstances)){throw 'Other instance/base/shared behavior changed'}
 if($ScopeOnly){'PASS exact real-Jorvinax instance death/update and native Hait victory bridge; genuine6-only escort continuation, all other original actor/instance/timer behavior retained';return}
 & "$PSScriptRoot/Test-MonkChenRoutes.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Current scoped native host export failed'}
 [IO.File]::WriteAllText("$out/Jorvinax.inc",(Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/jorvinax_defeat.h" -Raw).Replace('#include "chen_rescue.h"','#include "Rescue.inc"'))
 [IO.File]::WriteAllText("$out/HaitMovement.inc",(Block $actor 'void MovementInform('))
 [IO.File]::WriteAllText("$out/HaitMovementBaseline.inc",(Block $previous 'void MovementInform('))
 $smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $a=$smart.IndexOf('Player* GetScriptCreditPlayer(');$b=$smart.IndexOf('Unit* GetScriptCreditCaster(',$a);if($a -lt 0 -or $b -le $a){throw 'Native owner credit helper missing'}
 [IO.File]::WriteAllText("$out/SmartOwner.inc",$smart.Substring($a,$b-$a))
 $a=$smart.IndexOf('        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:');$b=$smart.IndexOf('        case SMART_ACTION_SUMMON_CONVERSATION:',$a);if($a -lt 0 -or $b -le $a){throw 'Native205 missing'}
 [IO.File]::WriteAllText("$out/Smart205.inc",$smart.Substring($a,$b-$a))
 if($ExportOnly){return}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run-jorvinax.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkJorvinaxTest.cpp`" /Fe`"$out/jorvinax.exe`" /Fo`"$out/jorvinax.obj`" /Fd`"$out/jorvinax.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/jorvinax.exe`"`nexit /b %errorlevel%`n")
 & "$out/run-jorvinax.cmd";if($LASTEXITCODE){throw 'Actual Jorvinax defeat ASAN regression failed'}
}
