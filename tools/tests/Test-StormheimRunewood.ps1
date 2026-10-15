param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-runewood"
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
$fixture=$fixture.Replace('static ObjectGuid const Empty;', 'void Clear(){value=0;}static ObjectGuid const Empty;')
$fixture=$fixture.Replace('struct WorldObject {', 'struct Position {float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};struct WorldObject:Position {')
$fixture=[regex]::Replace($fixture,'float GetPositionX\(\)\{return 3442.69f;\}float GetPositionY\(\)\{return 2103.1f;\}float GetPositionZ\(\)\{return 277.243f;\}float GetOrientation\(\)\{return 1.57336f;\}','')
$fixture=$fixture.Replace('struct Creature:Unit {', 'struct Creature:Unit {void SetFacingTo(float angle){o=angle;}')
$fixture=$fixture.Replace('float GetExactDist(WorldObject* o){return o->distance;}', 'using Position::GetExactDist;float GetExactDist(WorldObject* object){return Position::GetExactDist(object);}')
$fixture=$fixture.Replace('return creatures.count(g)?creatures[g]:nullptr;', 'return creatures.count(g)&&creatures[g]->world?creatures[g]:nullptr;')
$fixture=$fixture.Replace('assert(n>0);if(q->id', 'assert(n>0||q->id==38778);if(q->id')
$fixture=$fixture.Replace('int CastSpell(Player*,uint32 a,bool){if(!castResult){auras.insert(a);if(castHook)castHook();}return castResult;}', 'int CastSpell(Player* target,uint32 id,bool triggered){return Unit::CastSpell(target,id,triggered);}')
$fixture=$fixture.Replace('struct Unit:WorldObject {', 'struct Unit:WorldObject {void RemoveAllMinionsByFilter(uint32,uint8){}void RemoveGameObject(uint32,bool){}')
$fixture=[regex]::Replace($fixture,'struct SpellInfo \{[^}]*\};','struct SpellInfo {uint32 Id=184579,TriggerSpell=0;SpellInfo* GetEffect(int,int)const{return const_cast<SpellInfo*>(this);}};')
$fixture=$fixture.Replace('uint32 GetTeam(){return team;}', 'bool isInFlight(){return false;}uint32 GetTeam(){return team;}')
$fixture=$fixture.Replace('bool TalkedToCreature(uint32,ObjectGuid){return false;}', 'bool TalkedToCreature(uint32,ObjectGuid);')
$fixture=$fixture.Replace('void Clear(){value=0;}', 'void Clear(){value=0;}explicit operator bool()const{return value!=0;}')
[IO.File]::WriteAllText("$output/RunewoodFixture.inc",$fixture)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_runewood.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Runewood.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_runewood()')))
$auras=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp" -Raw
$a=$auras.IndexOf('void AuraEffect::HandleSummonController(');$b=$auras.IndexOf('void AuraEffect::HandleModNextSpell(', $a)
[IO.File]::WriteAllText("$output/Controller.inc",$auras.Substring($a,$b-$a))
$a=$core.IndexOf('bool Player::TalkedToCreature(');$b=$core.IndexOf('void Player::MoneyChanged(', $a)
[IO.File]::WriteAllText("$output/Talked.inc",$core.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimRunewoodTest.cpp`" /Fe`"$output/StormheimRunewood.exe`" /Fo`"$output/StormheimRunewood.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/StormheimRunewood.exe`"`r`n"
[IO.File]::WriteAllText("$output/run-StormheimRunewood.cmd",$cmd)
& "$output/run-StormheimRunewood.cmd"
if($LASTEXITCODE){throw 'Runewood production regression failed'}

if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe"
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('spell_stormheim_runewood_turn','Turn the Keys: ignored generic kill credit','Turn the Keys: ignored generic talk credit')){if(!$binary.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Cannot find actual Release scripts dependency'}
 $library=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if(!(Test-Path $library) -or (Get-Item $library).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Final Release is older than its actual scripts link input'}
 $game=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
 if(!$game){throw 'Missing actual game final-link dependency'}
 $game=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $game))
 if((Get-Item $game).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Stale final game library link'}
 'PASS: canonical Release contains Runewood script and both generic credit guards; checked actual scripts/game Release library inputs.'
}
