$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimMurkyData.ps1"
& "$PSScriptRoot/Test-StormheimJournals.ps1" *> "$repo/.codex/murky-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-murky"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-journals/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$f=$f.Replace('struct WorldObject {','struct WorldObject {uint32 GetTypeId(){return ToPlayer()?4:3;}')
$f=$f.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {bool failAura=false;bool HasAuraEffect(uint32 id,uint32){for(auto const& a:roots)if(a.first.first==id)return true;return false;}void SetAuraStack(uint32,Unit*,uint32);Aura* AddAura(uint32 id,Unit* t){if(failAura)return nullptr;return &t->roots[{id,GetGUID().value}];}bool isPet(){return ownerPlayer!=nullptr;}Unit* GetOwner(){return ownerPlayer?reinterpret_cast<Unit*>(ownerPlayer):nullptr;}struct Threat{Unit* getHostilTarget(){return nullptr;}};Threat& getThreatManager(){static Threat t;return t;}')
$f=$f.Replace('struct Aura {','struct Aura {void ModStackAmount(uint32){}')
$f=$f.Replace('struct CreatureTemplate {','struct CreatureTemplate {uint32 faction=14;')
$f=$f.Replace('struct Creature:Unit {','struct Creature:Unit {bool isInCombat(){return combat;}uint32 getFaction(){return 14;}void RestoreFaction(){}bool IsVisible(){return visible;}')
$f=$f.Replace('struct SpellInfo {','struct SpellInfo {struct Misc{struct Data{uint32 SchoolMask=1;}MiscData;}misc;Misc* GetMisc()const{return const_cast<Misc*>(&misc);}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {InventoryResult unequip=0;InventoryResult CanUnequipItems(uint32,uint32){return unequip;}bool TakeQuestSourceItem(uint32,bool);void DestroyItemCount(uint32 id,uint32 count,bool,bool=true){uint32 take=std::min(count,bags[id]);bags[id]-=take;count-=take;bank[id]-=std::min(count,bank[id]);}')
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$f)
function Export-Murky([string]$source,[string]$start,[string]$end,[string]$name){$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw "Missing native $name"};[IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))}
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Export-Murky $player 'bool Player::TakeQuestSourceItem(' 'bool Player::GetQuestRewardStatus(' 'MurkyTakeSource'
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Export-Murky $smart 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction(' 'MurkyEvents'
Export-Murky $smart 'void SmartScript::OnReset()' 'void SmartScript::ResetBaseObject()' 'MurkyReset'
Export-Murky $smart 'void SmartScript::ProcessEvent(SmartScriptHolder&' '    switch (e.GetEventType())' 'MurkyEventGate'
Export-Murky $smart 'void SmartScript::ProcessAction(SmartScriptHolder&' '    switch (e.GetActionType())' 'MurkyActionGate'
foreach($p in @(@('MurkySpellHit','        case SMART_EVENT_SPELLHIT_TARGET:','        case SMART_EVENT_OOC_LOS:'),@('MurkyCredit','        case SMART_ACTION_CALL_KILLEDMONSTER:','        case SMART_ACTION_SET_INST_DATA:'),@('MurkyAura','        case SMART_ACTION_ADD_AURA:','        case SMART_ACTION_ACTIVATE_GOBJECT:'),@('MurkyDespawnAction','        case SMART_ACTION_FORCE_DESPAWN:','        case SMART_ACTION_SET_INGAME_PHASE_MASK:'))){Export-Murky $smart $p[1] $p[2] $p[0]}
$a=$smart.IndexOf('    if (e.link && e.link != e.event_id)');$b=$smart.IndexOf('void SmartScript::InstallTemplate(',$a)
[IO.File]::WriteAllText("$out/MurkyLink.inc",$smart.Substring($a,$b-$a))
$ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
foreach($p in @(@('MurkyDespawn','void SmartAI::UpdateDespawn(','void SmartAI::Reset()'),@('MurkyRespawn','void SmartAI::JustRespawned()','int SmartAI::Permissible('),@('MurkyStartDespawn','void SmartAI::SetDespawnTime(','void SmartAI::OnSpellClick('))){Export-Murky $ai $p[1] $p[2] $p[0]}
foreach($name in @('MurkyDespawn','MurkyRespawn','MurkyStartDespawn')){(Get-Content "$out/$name.inc" -Raw).Replace('SmartAI::','MurkyAI::') | Set-Content "$out/$name.inc"}
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
Export-Murky $unit 'void Unit::SetAuraStack(' 'void Unit::SendPlaySpellVisualKit(' 'MurkyAuraStack'
$cond=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Export-Murky $cond 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded()' 'MurkyConditionDispatch'
foreach($p in @(@('MurkyConditionAura','        case CONDITION_AURA:','        case CONDITION_ITEM:'),@('MurkyConditionQuest','        case CONDITION_QUESTTAKEN:','        case CONDITION_QUEST_COMPLETE:'),@('MurkyConditionType','        case CONDITION_OBJECT_ENTRY:','        case CONDITION_TYPE_MASK:'))){Export-Murky $cond $p[1] $p[2] $p[0]}
$native=Get-Content "$repo/src/server/game/Spells/SpellInfo.cpp" -Raw
foreach($p in @(@('TorchTarget','SpellCastResult SpellInfo::CheckTarget(','SpellCastResult SpellInfo::CheckExplicitTarget('),@('TorchExplicit','SpellCastResult SpellInfo::CheckExplicitTarget(','bool SpellInfo::CheckTargetCreatureType('),@('TorchCreatureType','bool SpellInfo::CheckTargetCreatureType(','SpellSchoolMask SpellInfo::GetSchoolMask('),@('TorchDeadTarget','bool SpellInfo::IsRequiringDeadTarget()','bool SpellInfo::CanBeUsedInCombat()'))){Export-Murky $native $p[1] $p[2] $p[0]}
$spells=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
Export-Murky $spells 'bool WorldObjectSpellAreaTargetCheck::operator()' 'WorldObjectSpellBetweenTargetCheck::WorldObjectSpellBetweenTargetCheck' 'TorchArea'
$defines=Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw
$attributes=[regex]::Matches(((Get-Content "$out/TorchTarget.inc" -Raw)+' SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS SPELL_ATTR2_CAN_TARGET_DEAD'),'SPELL_ATTR\d+_[A-Z_]+') | ForEach-Object Value | Sort-Object -Unique
$constants=foreach($name in $attributes){if($name -like '*_CU_*'){continue};$match=[regex]::Match($defines,"\b$($name)\s*=\s*(0x[0-9A-Fa-f]+)");if(!$match.Success){throw "Missing native $name"};$index=[regex]::Match($name,'ATTR(\d+)').Groups[1].Value;"constexpr Attribute $name{$index,$($match.Groups[1].Value)};"}
[IO.File]::WriteAllLines("$out/TorchAttributes.inc",$constants)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimMurkyTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Murky native progression regression failed'}
