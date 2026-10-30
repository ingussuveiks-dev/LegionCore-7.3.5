param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){
 $first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"}
 $last=$source.IndexOf('{',$first)+1;$depth=1
 while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last}
 if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)
}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "89231609d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned transition baseline missing'}
$original=Block $pinned 'struct npc_horrific_aberrationAI : ScriptedAI'
$actor=Block $source 'struct npc_horrific_aberrationAI : ScriptedAI'
if(!$Baseline){
 $expectedUpdate=Block $original 'void UpdateAI('
 $start=$expectedUpdate.IndexOf('                    case EVENT_3:');$end=$expectedUpdate.IndexOf('                        break;',$start)+'                        break;'.Length
 if($start -lt 0 -or $end -le $start){throw 'Original companion-loss event missing'}
 $expectedUpdate=$expectedUpdate.Remove($start,$end-$start)
 if(($expectedUpdate -replace '\s','') -cne ((Block $actor 'void UpdateAI(') -replace '\s','')){throw 'Ordinary aberration combat changed beyond moving event3'}
 $normalized=$actor.Replace((Block $actor 'void Reset('),(Block $original 'void Reset('))
 $normalized=$normalized.Replace((Block $normalized 'void DamageTakenAfterScaling('),(Block $original 'void DamageTaken('))
 $normalized=$normalized.Replace((Block $normalized 'void UpdateAI('),(Block $original 'void UpdateAI('))
 $normalized=$normalized.Replace((Block $normalized 'void UpdateLifecycle('),'')
 $normalized=$normalized.Replace('bool outroDone = false;','bool outroDone;').Replace('uint32 _outroTimer = 0;','').Replace('bool _outroFinished = false;','').Replace('uint64 _outroGeneration = 0;','')
 if(($normalized -replace '\s','') -cne ($original -replace '\s','')){throw 'Unrelated death/owner/reset/actor behavior changed'}
}
if($ScopeOnly){'PASS exact transition-only scope; retained owner/death callback unchanged';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired binary'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/silver-hand-aberration-transition"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
if($Baseline){$actor=$original}
$actor=$actor.Replace((Block $actor 'void JustDied('),'')
[IO.File]::WriteAllText("$out/Actor.inc",$actor+';')
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
foreach($pair in @(@('Health','uint64 Unit::GetHealth(Unit* victim) const'),@('MaxHealth','uint64 Unit::GetMaxHealth(Unit* victim) const'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $core $pair[1]))}
$first=$core.IndexOf('    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));');$last=$core.IndexOf('    // A player charmed by an NPC',$first)
if($first -lt 0 -or $last -le $first){throw 'Native final damage bridge missing'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",$core.Substring($first,$last-$first))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block $creature 'float Creature::GetHealthMultiplierForTarget('))
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$events=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw
[IO.File]::WriteAllText("$out/EventMap.inc",(Block $events 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
$events=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
$eventMethods=foreach($signature in @('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time,','void EventMap::CancelEvent(','uint32 EventMap::ExecuteEvent(')){Block $events $signature}
[IO.File]::WriteAllText("$out/EventMapMethods.inc",($eventMethods -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $definition /I`"$out`" `"$PSScriptRoot/SilverHandAberrationTransitionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Aberration transition native/ASAN failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_aberration_transition_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Selected Release transition missing'}}finally{$env:PYTHONPATH=$previous}
}
