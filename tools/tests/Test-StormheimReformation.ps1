param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimReformationData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-reformation"
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $out
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($part in @(
 @('NextQuest','Quest const* Player::GetNextQuest(','bool Player::CanSeeStartQuest('),
 @('Race','bool Player::SatisfyQuestRace(','bool Player::SatisfyQuestReputation('),
 @('KillCreditGO','void Player::KillCreditGO(','bool Player::TalkedToCreature('),
 @('CanRewardQuest','bool Player::CanRewardQuest(Quest const* quest, bool msg)','bool Player::CanRewardQuest(Quest const* quest, uint32 reward'),
 @('CanCompleteQuest','bool Player::CanCompleteQuest(','bool Player::CanCompleteRepeatableQuest('),
 @('PreviousQuest','bool Player::SatisfyQuestPreviousQuest(','bool Player::SatisfyQuestClass('),
 @('KilledMonsterCredit','void Player::KilledMonsterCredit(','void Player::KilledPlayerCredit('),
 @('GroupCredit','void Player::RewardPlayerAndGroupAtEvent(','bool Player::IsAtGroupRewardDistance('),
 @('QuestObjectiveSatisfy','void Player::QuestObjectiveSatisfy(','void Player::SetQuestStatus(')
)){$a=$core.IndexOf($part[1]);$b=$core.IndexOf($part[2],$a);if($a -lt 0 -or $b -lt $a){throw "Missing $($part[0])"};[IO.File]::WriteAllText("$out/$($part[0]).inc",$core.Substring($a,$b-$a))}
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$loader.IndexOf('        if (qinfo->PrevQuestID)');$b=$loader.IndexOf('        if (qinfo->ExclusiveGroup)',$a)
[IO.File]::WriteAllText("$out/QuestPrevLoader.inc",$loader.Substring($a,$b-$a))
$fixture=Get-Content "$PSScriptRoot/StormheimHeartHandoffTest.cpp" -Raw
$fixture=$fixture.Substring(0,$fixture.IndexOf('int main()'))
$fixture=$fixture.Replace('uint32 GetInstanceId(){return 0;}', 'uint32 instance=0;uint32 GetInstanceId(){return instance;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {', 'struct Unit:WorldObject {bool IsPlayer(){return ToPlayer()!=nullptr;}')
$fixture=$fixture.Replace('struct QuestStatusData {', 'struct QuestStatusData {std::map<int8,int32> ObjectiveData;')
$fixture=$fixture.Replace('int32 GetQuestObjectiveData(uint32 q,uint32 e){return data[{q,e}];}', 'int32 GetQuestObjectiveData(uint32,uint32)const;')
$fixture=$fixture.Replace('int32 GetQuestObjectiveData(Quest const* q,int8 index){for(auto const& o:q->objectives)if(o.StorageIndex==index)return GetQuestObjectiveData(q->id,o.ObjectID);return 0;}', 'int32 GetQuestObjectiveData(Quest const*,int8)const;')
$fixture=$fixture.Replace('data[{q->id,o->ObjectID}]=n;', 'quests[q->id].ObjectiveData[o->StorageIndex]=n;')
$fixture=$fixture.Replace('int CastSpell(Unit* target,uint32 id,bool){casts.push_back(id);', 'std::vector<bool> triggeredCasts;int CastSpell(Unit* target,uint32 id,bool triggered){triggeredCasts.push_back(triggered);casts.push_back(id);')
$fixture=$fixture.Replace('struct Creature:Unit {', 'struct Creature:Unit {bool summoned=true;float x=3532.88f,y=887.547f,z=53.4053f;bool isSummon(){return summoned;}uint32 GetUInt32Value(int f){return fields[f];}float GetExactDist(float a,float b,float c){return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}')
[IO.File]::WriteAllText("$out/ReformationFixture.inc",$fixture)
$a=$core.IndexOf('int32 Player::GetQuestObjectiveData(uint32');$b=$core.IndexOf('void Player::SetQuestObjectiveData(',$a)
[IO.File]::WriteAllText("$out/ObjectiveGetters.inc",$core.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_reformation.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$out/Reformation.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_reformation()')))
$spells=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$spells.IndexOf('void Spell::SelectImplicitNearbyTargets(');$b=$spells.IndexOf('void Spell::SelectImplicitTargetsFromThreadList(',$a)
[IO.File]::WriteAllText("$out/NearbyTargets.inc",$spells.Substring($a,$b-$a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectKillCredit(');$b=$effects.IndexOf('void Spell::EffectQuestFail(',$a)
[IO.File]::WriteAllText("$out/NativeCredit.inc",$effects.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimReformationTest.cpp`" /Fe`"$out/Reformation.exe`" /Fo`"$out/Reformation.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Reformation.exe`" `"$out/destination.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Reformation native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe"
 $bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$bytes.Contains('npc_stormheim_reformation_bones')){throw 'Canonical executable lacks reformation binding'}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name final-link dependency"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical executable predates $name final-link input"}
 }
 'PASS: canonical Release binding and actual scripts/game final-link inputs.'
}
