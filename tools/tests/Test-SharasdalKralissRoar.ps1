param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
if((Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp" -Raw).Contains('namespace SharasdalLady')){
 & "$PSScriptRoot/Test-SharasdalLady.ps1" -ScopeOnly
 if(!$?){throw 'Current Lady source certificate failed'}
 $source=(& git -C $repo show "d7610f9d1:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-Lady roar scope missing'}
}
$old=(& git -C $repo show "267d273dc:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned native roar source missing'}
$npc=Block $source 'class npc_kraliss :';$oldNPC=Block $old 'class npc_kraliss :'
$legacy=@'
                        me->AddDelayedEvent(1000, [this]() -> void
                        {
                            DoCast(209748);
                        });
'@
$new='                        events.RescheduleEvent(EVENT_3, 1000);'
$branch=@'
                    case EVENT_3:
                        DoCast(209748);
                        break;
'@
if(!$npc.Contains($new) -or !$npc.Contains($branch.Replace("`r",''))){throw 'Roar event3 queue/dispatch missing'}
$normal=$npc.Replace($new,$legacy.Replace("`r",'')).Replace($branch.Replace("`r",''),'').Replace('// 209748','// 209748 с m_Functions')
if(($normal -replace '\s','') -cne ($oldNPC -replace '\s','')){throw 'Other Kraliss reset/combat/target/spells/death/credit changed'}
if(($source.Replace($npc,$oldNPC) -replace '\s','') -cne ($old -replace '\s','')){throw 'Other native actors/wound/gossip/wall/Lady/pickup changed'}
if($ScopeOnly){'PASS only Kraliss1s roar queue changes from Unit raw-AI capture to existing AI EventMap3; native18s/20s/target/reset/death/other actors retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-kraliss-roar"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldNPC}else{$npc})+';')
$header=(Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw).Replace('#include "Define.h"','').Replace('#include "Duration.h"','').Replace('TC_GAME_API ','')
$implementation=(Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw).Replace('#include "EventMap.h"','').Replace('#include "Random.h"','')
[IO.File]::WriteAllText("$out/EventMap.inc",$header+$implementation)
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
[IO.File]::WriteAllText("$out/Victim.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'bool CreatureAI::UpdateVictim()'))
$creatureSource=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/AIDestroy.inc",(Block $creatureSource 'bool Creature::AIM_Destroy()'))
$corpse=Block $creatureSource 'case CORPSE:'
if(!$corpse.Contains('Unit::Update(diff);') -or !$corpse.Contains('AI()->UpdateCorpseAI(diff);') -or $corpse.Contains('i_AI->UpdateAI(diff);')){throw 'Native corpse/Unit callback boundary changed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalKralissRoarTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native Kraliss roar regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_kraliss_roar_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native Kraliss roar implementation missing'}}finally{$env:PYTHONPATH=$previous}
}
