$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimUnworthyTaskData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-unworthy-task"
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $out
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($part in @(
 @('NextQuest','Quest const* Player::GetNextQuest(','bool Player::CanSeeStartQuest('),
 @('Race','bool Player::SatisfyQuestRace(','bool Player::SatisfyQuestReputation('),
 @('KillCreditGO','void Player::KillCreditGO(','bool Player::TalkedToCreature('),
 @('CanRewardQuest','bool Player::CanRewardQuest(Quest const* quest, bool msg)','bool Player::CanRewardQuest(Quest const* quest, uint32 reward'),
 @('CanCompleteQuest','bool Player::CanCompleteQuest(','bool Player::CanCompleteRepeatableQuest('),
 @('QuestStatus','QuestStatus Player::GetQuestStatus(','QuestStatus Player::GetDailyQuestStatus('),
 @('PreviousQuest','bool Player::SatisfyQuestPreviousQuest(','bool Player::SatisfyQuestClass('),
 @('KilledMonsterCredit','void Player::KilledMonsterCredit(','void Player::KilledPlayerCredit('),
 @('GroupCredit','void Player::RewardPlayerAndGroupAtEvent(','bool Player::IsAtGroupRewardDistance('),
 @('QuestObjectiveSatisfy','void Player::QuestObjectiveSatisfy(','void Player::SetQuestStatus(')
)){
 $a=$core.IndexOf($part[1]);$b=$core.IndexOf($part[2],$a)
 if($a -lt 0 -or $b -lt $a){throw "Missing $($part[0])"}
 [IO.File]::WriteAllText("$out/$($part[0]).inc",$core.Substring($a,$b-$a))
}
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$loader.IndexOf('        if (qinfo->PrevQuestID)');$b=$loader.IndexOf('        if (qinfo->ExclusiveGroup)',$a)
[IO.File]::WriteAllText("$out/QuestPrevLoader.inc",$loader.Substring($a,$b-$a))
$fixture=Get-Content "$PSScriptRoot/StormheimHeartHandoffTest.cpp" -Raw
$fixture=$fixture.Substring(0,$fixture.IndexOf('int main()'))
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {bool IsPlayer(){return ToPlayer()!=nullptr;}')
$fixture=$fixture.Replace('bool IsWeekly()const','bool IsDaily()const{return false;}bool IsWorld()const{return false;}bool IsWeekly()const')
$fixture=$fixture.Replace('int GetQuestStatus(uint32 q){auto s=getQuestStatus(q);return s?s->Status:QUEST_STATUS_NONE;}','QuestStatus GetQuestStatus(uint32)const;bool WorldQuestCompleted(uint32)const{return false;}')
$fixture=$fixture.Replace('bool IsQuestWeekRewarded(uint32){','bool IsQuestWeekRewarded(uint32)const{').Replace('bool IsQuestSeasonalRewarded(uint32){','bool IsQuestSeasonalRewarded(uint32)const{').Replace('bool IsQuestDailyRewarded(uint32){','bool IsQuestDailyRewarded(uint32)const{')
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {uint32 GetTypeId(){return ToPlayer()?4:0;}bool HasAura(uint32){return false;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {struct Threat {Unit* getHostilTarget(){return nullptr;}} threat;Threat& getThreatManager(){return threat;}bool isPet(){return false;}Unit* GetOwner(){return nullptr;}')
$fixture=$fixture.Replace('uint32 GetTeam(){return team;}','uint32 GetSkillValue(int){return 0;}bool IsInArmyTraining(){return false;}struct Army {std::vector<int> currentUnits[1];} armyTrainingInfo;uint32 GetAuraCount(uint32){return 0;}uint32 GetTeam(){return team;}')
$fixture=$fixture.Replace('struct FakeMap {','struct FakeMap {template<class... T>void ScriptsStart(T...){}')
$fixture=$fixture.Replace('Player* ToPlayer()override{return this;} std::function','void SendPreparedGossip(GameObject*){}void PrepareGossipMenu(GameObject*,uint32){}struct BG {void EventPlayerUsedGO(Player*,GameObject*){}};BG* GetBattleground(){return nullptr;}void SendDirectMessage(int){}Player* ToPlayer()override{return this;} std::function')
$fixture=$fixture.Replace('struct GameObject:WorldObject {','struct GameObjectTemplate {uint32 entry=241778,type=10;struct {uint32 pageID=0,gossipID=0,eventID=0,questID=39837,linkedTrap=0,customAnim=0,spell=0;} goober;uint32 GetAutoCloseTime()const{return 3;}};struct GameObject:WorldObject {GameObjectTemplate info;uint32 animations=0;uint32 GetDBTableGUIDLow(){return 25164548;}GameObjectTemplate const* GetGOInfo(){return &info;}void Use(Unit*);void EventInform(uint32){}void TriggeringLinkedGameObject(uint32,Unit*){}void SetFlag(int,uint32 f){flags|=f;}void SetLootState(int,Unit*){}void SendCustomAnim(uint32){++animations;}uint32 GetGoAnimProgress(){return 255;}void SetGoState(uint32 n){state=n;++animations;}uint32 m_cooldownTime=0;FakeMap* GetMap();')
# Forward declaration for the real GO-to-map relationship.
$fixture=$fixture.Replace('struct Unit;struct Player;','struct FakeMap;struct Unit;struct Player;')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$fixture)
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$start=$go.IndexOf('void GameObject::Use(Unit* user)')
$a=$go.IndexOf('        case GAMEOBJECT_TYPE_GOOBER:', $start);$b=$go.IndexOf('        case GAMEOBJECT_TYPE_CAMERA:', $a)
[IO.File]::WriteAllText("$out/Goober.inc",$go.Substring($a,$b-$a))
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$spell.IndexOf('SpellCastResult Spell::CanOpenLock(');$b=$spell.IndexOf('void Spell::SetSpellValue(',$a)
[IO.File]::WriteAllText("$out/CanOpenLock.inc",$spell.Substring($a,$b-$a))
$conditions=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$conditions.IndexOf('        case CONDITION_QUESTREWARDED:');$b=$conditions.IndexOf('        case CONDITION_QUEST_NONE:',$a)
[IO.File]::WriteAllText("$out/QuestConditions.inc",$conditions.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimUnworthyTaskTest.cpp`" /Fe`"$out/Unworthy.exe`" /Fo`"$out/Unworthy.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Unworthy.exe`" `"$out/cage.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Unworthy Task native regression failed'}
