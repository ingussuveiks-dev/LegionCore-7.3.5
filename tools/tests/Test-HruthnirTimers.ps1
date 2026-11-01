param([switch]$Baseline,[switch]$ScopeOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$text,[string]$start){$a=$text.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $text.Length){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$text.Substring($a,$b-$a)}
function Shape([string]$text){($text -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "e4a7e933a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-timer source missing'}
$actor=Block $source 'class npc_hrutnir :';$oldActor=Block $old 'class npc_hrutnir :'
$life=Block $actor 'void UpdateLifecycle('
$expectedLife=@'
void UpdateLifecycle(uint32 diff) override {
 if (me->getDeathState() == ALIVE || me->getDeathState() == CORPSE) _presentationFunctions.Update(diff);
}
'@
if((Shape $life) -cne (Shape $expectedLife)){throw 'Native ALIVE/CORPSE clock changed'}
$normalized=$source.Replace('#include "FunctionProcessor.h"','').Replace('FunctionProcessor _presentationFunctions;','').Replace($life,'').Replace('_presentationFunctions.AddDelayedEvent(', 'me->AddDelayedEvent(').Replace('sCreatureTextMgr->SendChat(targ, 5);','targ->AI()->Talk(5);')
if((Shape $normalized) -cne (Shape $old)){throw 'Other Hruthnir/Magnar/gameplay/guarded callback behavior changed'}
if(([regex]::Matches($actor,'_presentationFunctions.AddDelayedEvent\(')).Count -ne 6 -or ([regex]::Matches($actor,'me->AddDelayedEvent\(')).Count -ne 2){throw 'Exactly six owned callbacks and two existing guarded Unit callbacks required'}
if($ScopeOnly){'PASS exact six Hruthnir AI-owned callbacks/native target text/ALIVE-CORPSE clock; all admission, combat, Magnar tokens/scene and other actors unchanged';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/hruthnir-timers$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$selected=if($Baseline){$oldActor}else{$actor}
$movement=Block $selected 'void MovementInform('
# Later guarded chamber branches are independently retained by HruthnirGossip.
# Compile the complete earlier movement branches without replacing their bodies.
foreach($case in @('case 6:','case 7:')){$movement=$movement.Replace((Block $movement $case),'')}
$methods=(Block $selected 'void EnterEvadeMode(')+"`n"+$movement+"`n"+(Block $selected 'void MoveInLineOfSight(')+"`n"+(Block $selected 'void DamageTaken(')
if(!$Baseline){$methods+='FunctionProcessor _presentationFunctions;'+$life}
[IO.File]::WriteAllText("$out/Methods.inc",$methods)
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
[IO.File]::WriteAllText("$out/Talk.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'void CreatureAI::Talk(uint8 id, ObjectGuid WhisperGuid)'))
[IO.File]::WriteAllText("$out/DeathState.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum DeathState')+';')
$native=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $native $pair[1]))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/HruthnirTimersTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Hruthnir timer regression failed'}
