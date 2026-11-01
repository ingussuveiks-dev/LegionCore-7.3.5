param([switch]$ScopeOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$text,[string]$start){$a=$text.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $text.Length){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$text.Substring($a,$b-$a)}
function Shape([string]$text){(($text -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
$path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "bb877ffc3:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-click source missing'}
$actor=Block $source 'class npc_master_hait :';$oldActor=Block $old 'class npc_master_hait :'
$ctor=Block $actor 'npc_master_haitAI(Creature*';$hook=Block $actor 'void OnSpellClick('
$members=@'
        std::shared_ptr<uint8> _clickLifetime = std::make_shared<uint8>();
        std::weak_ptr<uint8> _pendingClick;
'@
$expectedCtor=@'
npc_master_haitAI(Creature* creature) : ScriptedAI(creature)
        {
            if (me->GetMapId() == 1014 && me->GetDBTableGUIDLow() == 362105)
                me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }
'@
if((Shape $ctor) -cne (Shape $expectedCtor) -or (Shape $hook) -cne 'voidOnSpellClick(Unit*)override{}' -or !$actor.Contains($members)){throw 'Unexpected actor admission/lease/hook change'}
$normal=$actor.Replace($ctor,(Block $oldActor 'npc_master_haitAI(Creature*')).Replace($hook,(Block $oldActor 'void OnSpellClick(')).Replace($members,'')
if((Shape $normal) -cne (Shape $oldActor)){throw 'Click change altered existing actor timers/actions/vehicle/movement'}
$outside=$source.Replace($actor,$oldActor)
foreach($include in @('Player.h','Group.h','QuestData.h','Scenario.h','ScenarioMgr.h','SpellMgr.h','SpellScript.h','Timer.h','monk_hait_click.h')){$outside=$outside.Replace('#include "'+$include+'"','')}
$outside=$outside.Replace('RegisterSpellScript(spell_monk_hait_click);','')
if((Shape $outside) -cne (Shape $old)){throw 'Click change altered other Monk actors'}
if($ScopeOnly){'PASS exact initial actor ctor/quest-actor leases/no-op hook plus spell registration; existing actor behavior unchanged';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-hait-click"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/ActorAdmission.inc",$ctor+"`n"+$members+"`n"+$hook)
[IO.File]::WriteAllText("$out/OldHook.inc",(Block $oldActor 'void OnSpellClick(').Replace('override',''))
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_hait_click.h" "$out/Click.inc"
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$sources=@{
 'NativeClick'=@('Entities/Unit/Unit.cpp','bool Unit::HandleSpellClick(')
 'NativePersonal'=@('Spells/SpellEffects.cpp','void Spell::EffectKillCreditPersonal(')
 'NativeCredit'=@('Spells/SpellEffects.cpp','void Spell::EffectKillCredit(')
 'NativeEvent'=@('Spells/SpellEffects.cpp','void Spell::EffectSendEvent(')
 'NativeGroup'=@('Entities/Player/Player.cpp','void Player::RewardPlayerAndGroupAtEvent(')
 'NativeDistance'=@('Entities/Player/Player.cpp','bool Player::IsAtGroupRewardDistance(')
 'NativeDelayed'=@('Entities/Player/Player.cpp','void Player::AddQuestDelayedEvent(')
 'NativeOriginalGuid'=@('Spells/SpellScript.cpp','ObjectGuid SpellScript::GetOriginalTargetGUID() const')
}
foreach($key in $sources.Keys){[IO.File]::WriteAllText("$out/$key.inc",(Block (Get-Content "$repo/src/server/game/$($sources[$key][0])" -Raw) $sources[$key][1]))}
$timer=Get-Content "$repo/src/common/Utilities/Timer.h" -Raw
[IO.File]::WriteAllText("$out/Timer.inc",(Block $timer 'inline uint32 getMSTimeDiff(')+"`n"+(Block $timer 'inline uint32 GetMSTimeDiffToNow('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkHaitClickTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Hight click regression failed'}
