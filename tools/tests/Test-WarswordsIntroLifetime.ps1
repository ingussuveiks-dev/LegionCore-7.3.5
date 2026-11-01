param([switch]$Baseline,[switch]$ScopeOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$text,[string]$start){$a=$text.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $text.Length){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$text.Substring($a,$b-$a)}
function Shape([string]$text){($text -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "661a25685:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-lifetime source missing'}
$actor=Block $source 'class npc_vigfus_bladewind_last :';$oldActor=Block $old 'class npc_vigfus_bladewind_last :'
$destructor=Block $actor '~npc_vigfus_bladewind_lastAI()'
if((Shape $destructor) -cne '~npc_vigfus_bladewind_lastAI()override{introLifetime.reset();RemoveIntroConversation();}'){throw 'Lifetime retirement must precede owned843 removal'}
$move=Block $actor 'void MoveInLineOfSight('; $oldMove=Block $oldActor 'void MoveInLineOfSight('
$handoff=@'
me->RemoveAura(205424);
                    if (lifetime.expired() || generation != introGeneration || !introPending)
                        return;
                    player = ObjectAccessor::GetPlayer(*me, playerGuid);
                    if (!IsIntroTarget(player, 100.0f) || !IsIntroHelya(ObjectAccessor::GetCreature(*me, helyaGuid)))
                    {
                        CancelIntro(generation);
                        return;
                    }
                    introPending = false;
                    AttackStart(player);
'@
if(!$move.Contains($handoff)){throw 'Actual aura-removal reentry must be checked before consuming pending/engaging'}
$normalMove=$move.Replace($handoff,"introPending = false;`n                    me->RemoveAura(205424);`n                    AttackStart(player);")
$normalMove=$normalMove.Replace('std::weak_ptr<bool> lifetime = introLifetime;','').Replace('[this, lifetime, generation, playerGuid, helyaGuid]','[this, generation, playerGuid, helyaGuid]').Replace('lifetime.expired() || ','')
$normalMove=$normalMove.Replace("                player = ObjectAccessor::GetPlayer(*me, playerGuid);`n                helya = ObjectAccessor::GetCreature(*me, helyaGuid);",'')
if((Shape $normalMove) -cne (Shape $oldMove) -or ([regex]::Matches($move,'lifetime.expired\(\) \|\| generation')).Count -ne 4){throw 'Other native24/7 timing/cast/receipt/recipient/intro behavior changed'}
$normalized=$source.Replace('#include <memory>','').Replace($destructor,'').Replace('std::shared_ptr<bool> introLifetime = std::make_shared<bool>(false);','').Replace($move,$oldMove)
if((Shape $normalized) -cne (Shape $old)){throw 'Other boss/combat/credit/loot/bubble/lifecycle bodies changed'}
if($ScopeOnly){'PASS exact AI lifetime/owned843 destructor and native24/7 closure guards with post-cast/post-aura original-GUID revalidation; all other gameplay unchanged';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/warswords-intro-lifetime$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -ExportOnly -LifetimeBaseline:$Baseline -OutputDirectory $out
$support=(Get-Content "$PSScriptRoot/WarswordsFinalIntroTest.cpp" -Raw).Replace("`r",'');$support=$support.Substring(0,$support.IndexOf('struct Fixture'))
$support=$support.Replace('    std::vector<Delayed> callbacks;',"    std::vector<Delayed> callbacks;`n    FunctionProcessor m_Functions;`n    std::function<void()> onAuraRemove;")
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
$support=$support.Replace((Block $support 'void AddDelayedEvent('),(Block $unit 'void AddDelayedEvent('))
$support=$support.Replace((Block $support 'void Advance('),'void Advance(uint32 diff) { elapsed += diff; if (onElapsed) onElapsed(diff); m_Functions.Update(diff); }')
$support=$support.Replace('bubble = false;','bubble = false; if (onAuraRemove) onAuraRemove();')
$support=$support.Replace('++attacks;','++attacks; ++engagementRequests;')
[IO.File]::WriteAllText("$out/Support.inc",$support)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DLIFETIME_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run-lifetime.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /DWARSWORDS_LATE_BLESSING $mode /I`"$out`" `"$PSScriptRoot/WarswordsIntroLifetimeTest.cpp`" /Fe`"$out/lifetime.exe`" /Fo`"$out/lifetime.obj`" /Fd`"$out/lifetime.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/lifetime.exe`"`nexit /b %errorlevel%`n")
& "$out/run-lifetime.cmd";if($LASTEXITCODE){throw 'Native final Vigfus lifetime regression failed'}
