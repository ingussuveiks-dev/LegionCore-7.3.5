param([switch]$ScopeOnly,[switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
function Adapt([string]$s,[string]$a,[string]$b){if(!$s.Contains($a)){throw "Combat host boundary changed: $a"};$s.Replace($a,$b)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "585592236:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned combat actor missing'}
$normal=$source
foreach($pair in @(@('erunak','AttackStart(lady)'),@('adel','AttackStart(lady)'),@('garsh','Talk(9)'))){$name=$pair[0];$action=$pair[1];$prefix=' '*28;$added="${prefix}if (CreatureAI* content = $name"+'->GetDisabledAI() ? static_cast<CreatureAI*>('+$name+'->GetDisabledAI()) : '+$name+"->AI())`n"+$prefix+'    content->'+$action+';';$normal=Adapt $normal $added ($prefix+$name+'->AI()->'+$action+';')}
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other actors/progression/spell/target/cast ordering/timers changed'}
foreach($name in @('instance_sharasdal.cpp','sharasdal_lower_ascent.h','sharasdal_gauntlet_arrival.h','sharasdal_upper_wall.h','sharasdal_erunak_entry.h','sharasdal_erunak_upper.h')){$relative='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/'+$name;$prior=(& git -C $repo show "585592236:$relative") -join "`n";if($LASTEXITCODE -or (($prior -replace '\s','') -cne ((Get-Content "$repo/$relative" -Raw) -replace '\s',''))){throw "Other instance/member policy changed: $name"}}
if($ScopeOnly){'PASS only three optional Sharasdal combat AI messages select available original suspended/current content; casts/targets/periods/routes/instance/progression/loot and all other actors retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-combat-ai$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalGauntletArrival.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Native Lady/quest/AI host preparation failed'}
[IO.File]::WriteAllText("$out/Actor.inc",(Block $(if($Baseline){$old}else{$source}) 'class npc_lady_zitrin :')+';')
$hostSource=Get-Content "$out/NativeHost.inc" -Raw
$hostSource=Adapt $hostSource '#include <algorithm>' "#include <algorithm>`n#include <cstdint>`n#include <chrono>`nusing uint16=uint16_t;using Milliseconds=std::chrono::milliseconds;"
$hostSource=Adapt $hostSource ',EVENT_1=1,EVENT_2=2,EVENT_3=3,EVENT_4=4' ''
$hostSource=Adapt $hostSource 'struct EventMap{void Reset(){}void RescheduleEvent(uint32,uint32){}void Update(uint32){}uint32 ExecuteEvent(){return 0;}};' "uint32 urand(uint32,uint32);`n#include `"EventMap.inc`""
$hostSource=Adapt $hostSource 'bool UpdateVictim(){return false;}void DoCast(uint32){}' 'bool hasVictim=true;bool UpdateVictim(){return hasVictim;}void DoCast(uint32 id){me->CastSpell(me,id);}'
$hostSource=Adapt $hostSource 'std::vector<uint32> speech;void Talk' 'std::vector<Unit*> attacks;std::vector<uint32> speech;void Talk'
$hostSource=Adapt $hostSource 'void AttackStart(Unit*){}' 'void AttackStart(Unit* target){attacks.push_back(target);}'
$hostSource=Adapt $hostSource 'std::vector<uint32> casts;void CastSpell(Unit*,uint32 id){casts.push_back(id);}' 'std::vector<uint32> casts;std::vector<Unit*> targets;std::function<void()> onCast;void CastSpell(Unit* target,uint32 id){casts.push_back(id);targets.push_back(target);if(onCast)onCast();}'
$hostSource=Adapt $hostSource 'Creature* erunak=nullptr;' 'CreatureAI*& i_AI=primary;bool AIM_Destroy();Creature* erunak=nullptr;'
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$header=(Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw).Replace('#include "Define.h"','').Replace('#include "Duration.h"','').Replace('TC_GAME_API ','');$implementation=(Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw).Replace('#include "EventMap.h"','').Replace('#include "Random.h"','');[IO.File]::WriteAllText("$out/EventMap.inc",$header+$implementation)
[IO.File]::WriteAllText("$out/AIDestroy.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'bool Creature::AIM_Destroy()'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalCombatAITest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Current Lady/native combat speech regression failed'}
if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_combat_ai_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Selected canonical combat AI implementation missing'}}finally{$env:PYTHONPATH=$prior}}
