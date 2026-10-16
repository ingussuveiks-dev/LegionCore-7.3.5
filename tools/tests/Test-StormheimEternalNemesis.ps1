$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimEternalNemesisData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimBargain.ps1" -VerifyReleaseBinary *> "$PSScriptRoot/../../.codex/nemesis-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-eternal-nemesis"
$previous="$repo/build-extractors/runtime-test/stormheim-helheim-bargain"
Get-ChildItem "$previous/*.inc" | Copy-Item -Destination $output
$fixture=Get-Content "$previous/BargainFixture.inc" -Raw
$fixture=$fixture.Replace('struct Position {', 'struct Position {void Relocate(float a,float b,float c,float d){x=a;y=b;z=c;o=d;}')
$fixture=$fixture.Replace('HighGuid {Player,Creature,Pet,Vehicle,GameObject}', 'HighGuid {Player,Creature,Pet,Vehicle,GameObject,Conversation}')
$fixture=$fixture.Replace('uint64 value=0;', 'uint64 GetLowPart(){return value;}uint64 GetHighPart(){return 0;}template<HighGuid H>static ObjectGuid Create(uint64 n){return ObjectGuid(n);}uint64 value=0;')
$fixture=$fixture.Replace('struct WorldObject:Position {','struct WorldObject:Position {virtual uint32 GetTypeId(){return 5;}virtual GameObject* ToGameObject(){return nullptr;}Position const& GetPosition(){return *this;}float GetVisibilityRange(){return 100;}Creature* FindNearestCreature(uint32,float){return nullptr;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {','using Object=WorldObject;struct Unit:WorldObject {')
$fixture=$fixture.Replace('struct GameObject:WorldObject {','struct GameObject:WorldObject {GameObject* ToGameObject()override{return this;}')
$fixture=$fixture.Replace('bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestRace(q,b)&&SatisfyQuestPreviousQuest(q,b);}', 'bool CanTakeQuest(Quest const*,bool);bool SatisfyQuestStatus(Quest const*,bool);')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {void AddQuestAndCheckCompletion(Quest const*,Object*);void AddQuest(Quest const* q,Object*){Accept(q->id);}void DestroyItem(uint32,uint32,bool){}')
$fixture=$fixture.Replace('return s?s->Status:QUEST_STATUS_NONE;', 'return s?s->Status:(m_RewardedQuests.count(q)?QUEST_STATUS_REWARDED:QUEST_STATUS_NONE);')
foreach($name in @('ExclusiveGroup','Class','Level','Skill','Reputation','Timed','NextChain','PrevChain','Day','Week','Seasonal','Conditions')){$fixture=$fixture.Replace('struct Player:Unit {',"struct Player:Unit {bool SatisfyQuest$name(Quest const*,bool=false){return true;}")}
$fixture=$fixture.Replace('struct SmartGameObjectAI:GameObjectAI {','struct SmartGameObjectAI:GameObjectAI {NativeEvents* GetScript();bool QuestAccept(Player*,Quest const*)override;')
$fixture=$fixture.Replace('virtual bool GossipSelect(Player*,uint32,uint32){return false;}', 'virtual bool QuestAccept(Player*,Quest const*){return false;}virtual bool GossipSelect(Player*,uint32,uint32){return false;}')
[IO.File]::WriteAllText("$output/NemesisFixture.inc",$fixture)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($pair in @(@('Accept','void Player::AddQuestAndCheckCompletion(','void Player::AddQuest('),@('CanTake','bool Player::CanTakeQuest(','bool Player::CanAddQuest('),@('Status','bool Player::SatisfyQuestStatus(','bool Player::SatisfyQuestConditions('))){$a=$core.IndexOf($pair[1]);$b=$core.IndexOf($pair[2],$a);[IO.File]::WriteAllText("$output/$($pair[0]).inc",$core.Substring($a,$b-$a))}
$ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$ai.IndexOf('bool SmartGameObjectAI::QuestAccept(');$b=$ai.IndexOf('// Called when a player selects a quest reward',$a)
[IO.File]::WriteAllText("$output/NativeQuestAccept.inc",$ai.Substring($a,$b-$a))
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$smart.IndexOf('        case SMART_ACTION_SUMMON_CONVERSATION:');$b=$smart.IndexOf('        case SMART_ACTION_SUMMON_ADD_PLR_PERSONNAL_VISIBILE:',$a)
[IO.File]::WriteAllText("$output/NativeConversationAction.inc",$smart.Substring($a,$b-$a))
$a=$smart.IndexOf('        case SMART_EVENT_ACCEPTED_QUEST:');$b=$smart.IndexOf('        case SMART_EVENT_TRANSPORT_ADDCREATURE:',$a)
[IO.File]::WriteAllText("$output/NativeQuestEvent.inc",$smart.Substring($a,$b-$a))
$definitions=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
$a=$definitions.IndexOf('struct ConversationActor');$b=$definitions.IndexOf('typedef ',$a)
[IO.File]::WriteAllText("$output/NativeConversationStructs.inc",$definitions.Substring($a,$b-$a))
$conversation=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
$a=$conversation.IndexOf('bool Conversation::GenerateDynamicFieldData(')
[IO.File]::WriteAllText("$output/NativeConversationData.inc",$conversation.Substring($a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimEternalNemesisTest.cpp`" /Fe`"$output/EternalNemesis.exe`" /Fo`"$output/EternalNemesis.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/EternalNemesis.exe`" `"$output/world.tsv`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Eternal Nemesis native regression failed'}
