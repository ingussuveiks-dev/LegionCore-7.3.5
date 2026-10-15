param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-bones"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($segment in @(
 @('NextQuest','Quest const* Player::GetNextQuest(','bool Player::CanSeeStartQuest('),
 @('Race','bool Player::SatisfyQuestRace(','bool Player::SatisfyQuestReputation('),
 @('KillCreditGO','void Player::KillCreditGO(','bool Player::TalkedToCreature('),
 @('CanRewardQuest','bool Player::CanRewardQuest(Quest const* quest, bool msg)','bool Player::CanRewardQuest(Quest const* quest, uint32 reward'),
 @('CanCompleteQuest','bool Player::CanCompleteQuest(','bool Player::CanCompleteRepeatableQuest('),
 @('PreviousQuest','bool Player::SatisfyQuestPreviousQuest(','bool Player::SatisfyQuestClass('),
 @('KilledMonsterCredit','void Player::KilledMonsterCredit(','void Player::KilledPlayerCredit('),
 @('GroupCredit','void Player::RewardPlayerAndGroupAtEvent(','bool Player::IsAtGroupRewardDistance('),
 @('QuestObjectiveSatisfy','void Player::QuestObjectiveSatisfy(','void Player::SetQuestStatus(')
)){
 $a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing core handler $($segment[0])"}
 [IO.File]::WriteAllText("$output/$($segment[0]).inc",$core.Substring($a,$b-$a))
}
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$loader.IndexOf('        if (qinfo->PrevQuestID)');$b=$loader.IndexOf('        if (qinfo->ExclusiveGroup)',$a)
[IO.File]::WriteAllText("$output/QuestPrevLoader.inc",$loader.Substring($a,$b-$a))
$fixture=Get-Content "$PSScriptRoot/StormheimHeartHandoffTest.cpp" -Raw
$fixture=$fixture.Substring(0,$fixture.IndexOf('int main()'))
$fixture=$fixture.Replace('uint32 id=0,special=0;', 'uint32 id=0,special=0,StartScript=0;')
$fixture=$fixture.Replace('static ObjectGuid const Empty;', 'uint64 GetLowPart(){return value;}uint64 GetHighPart(){return 0;}template<HighGuid H>static ObjectGuid Create(uint64 n){return ObjectGuid(n);}template<HighGuid H>static ObjectGuid Create(uint32,uint32,uint64 n){return ObjectGuid(n);}static ObjectGuid const Empty;')
$fixture=$fixture.Replace('virtual ~WorldObject()=default;', 'virtual ~WorldObject()=default;Creature* FindNearestCreature(uint32,float){return nullptr;}float GetVisibilityRange(){return 100;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {', 'struct Unit:WorldObject {bool IsPlayer(){return ToPlayer()!=nullptr;}')
$fixture=$fixture.Replace('struct Creature:Unit {', 'struct Creature:Unit {uint32 GetInstanceId(){return 0;}')
$fixture=$fixture.Replace('if(!castResult){auras.insert(a);', 'conversationAttempts+=a==192170;if(!castResult){auras.insert(a);')
$fixture=$fixture.Replace('uint32 team=ALLIANCE,packets=0', 'uint32 conversationAttempts=0,team=ALLIANCE,packets=0')
[IO.File]::WriteAllText("$output/HeartHandoffFixture.inc",$fixture)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_bones.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Bones.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_bones()')))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectKillCredit(');$b=$effects.IndexOf('void Spell::EffectQuestFail(', $a)
if($a -lt 0 -or $b -lt 0){throw 'Missing native credit effect'}
[IO.File]::WriteAllText("$output/NativeCredit.inc",$effects.Substring($a,$b-$a))
$conversation=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
$a=$conversation.IndexOf('bool Conversation::GenerateDynamicFieldData(')
[IO.File]::WriteAllText("$output/Conversation.inc",$conversation.Substring($a))
$a=$conversation.IndexOf('    uint32 duration = 0;', $conversation.IndexOf('bool Conversation::CreateConversation('));$b=$conversation.IndexOf('    SetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME', $a)
[IO.File]::WriteAllText("$output/ConversationLifetime.inc",$conversation.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimBonesTest.cpp`" /Fe`"$output/StormheimBones.exe`" /Fo`"$output/StormheimBones.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/StormheimBones.exe`"`r`n"
[IO.File]::WriteAllText("$output/run-StormheimBones.cmd",$cmd)
& "$output/run-StormheimBones.cmd"
if($LASTEXITCODE){throw 'Speaking of Bones production regression failed'}

if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe"
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$binary.Contains('spell_stormheim_bones_butchering')){throw 'Canonical Release lacks the Bones spell script'}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Cannot find actual Release scripts dependency'}
 $library=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if(!(Test-Path $library) -or (Get-Item $library).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Final Release is older than its actual scripts link input'}
 'PASS: canonical Release contains Bones registration and uses the checked scripts/Release/scripts.lib final-link input.'
}
