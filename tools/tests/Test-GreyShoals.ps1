$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/grey-shoals"
New-Item -ItemType Directory -Force $out | Out-Null
$definition=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
$enums=''
foreach($name in @('QuestObjectiveType','QuestStatus','__QuestSpecialFlags','QuestObjectiveFlags')){
 $match=[regex]::Match($definition,"(?s)enum $name\b.*?\n\};")
 if(!$match.Success){throw "Missing native enum $name"}
 $enums+=$match.Value+"`n"
}
[IO.File]::WriteAllText("$out/GreyEnums.inc",$enums)
function Export([string]$path,[string]$start,[string]$end,[string]$name){
 $source=Get-Content "$repo/$path" -Raw
 $a=$source.IndexOf($start);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Missing production boundary $name"}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
Export 'src/server/scripts/Legion/zone_azsuna.cpp' 'class go_grey_shoals_supplies' 'void AddSC_azsuna()' 'GreySupplies'
Export 'src/server/game/Entities/Player/Player.cpp' 'bool Player::HasQuestObjectiveComplete(' 'bool Player::CanCompleteRepeatableQuest(' 'GreyCompletion'
Export 'src/server/game/Entities/Player/Player.cpp' 'void Player::RewardPlayerAndGroupAtEvent(' 'bool Player::IsAtGroupRewardDistance(' 'GreyGroupEvent'
Export 'src/server/game/Spells/SpellEffects.cpp' 'void Spell::EffectKillCredit(SpellEffIndex' 'void Spell::EffectQuestFail(' 'GreySpellCredit'
$native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$native.IndexOf('        case GAMEOBJECT_TYPE_GOOBER:                        //10')
$b=$native.IndexOf('        case GAMEOBJECT_TYPE_CAMERA:',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native goober use branch'}
[IO.File]::WriteAllText("$out/GreyGooberUse.inc",$native.Substring($a,$b-$a))
$a=$native.IndexOf('case GO_ACTIVATED:')
$a=$native.IndexOf('case GAMEOBJECT_TYPE_GOOBER:',$a)
$b=$native.IndexOf('case GAMEOBJECT_TYPE_CHEST:',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native goober activated-state update'}
[IO.File]::WriteAllText("$out/GreyGooberActivated.inc",$native.Substring($a,$b-$a))
Export 'src/server/game/Entities/GameObject/GameObjectData.cpp' 'uint32 GameObjectTemplate::GetAutoCloseTime() const' 'uint32 GameObjectTemplate::GetLootId() const' 'GreyAutoClose'
# Execute the actual generic objective mutation, including its quest status and
# raid admission, rather than modeling a second progress counter.
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$source.IndexOf('void Player::KilledMonsterCredit(')
$a=$source.IndexOf('        Quest const* qInfo = sQuestDataStore->GetQuestTemplate(questid);',$a)
$b=$source.IndexOf('void Player::KilledPlayerCredit()',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native kill objective boundary'}
[IO.File]::WriteAllText("$out/GreyKillObjective.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /I`"$out`" `"$PSScriptRoot/GreyShoalsTest.cpp`" /Fe`"$out/GreyShoals.exe`" /Fo`"$out/GreyShoals.obj`"`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/GreyShoals.exe`"`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Grey Shoals production interaction/progress regression failed'}
'PASS: extracted supply handler, native goober activation/timer, spell134, group-event dispatch, generic objective mutation and completion. World services and spell preparation/hit invocation are fixtures; Unit::Kill, full GO::Use/Update, packets and client are not executed.'
