param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimFamiliarFateData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimBargain.ps1" *> "$PSScriptRoot/../../.codex/familiar-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-familiar-fate"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-helheim-bargain/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/BargainFixture.inc" -Raw
$f=$f.Replace('uint32 GetItemCount(uint32 id){return bags[id];}','uint32 GetItemCount(uint32 id,bool=false)const{auto i=bags.find(id);return inventoryVisible&&i!=bags.end()?i->second:0;}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {uint32 visibilityUpdates=0;void UpdateVisibilityOf(Creature*){++visibilityUpdates;}bool inventoryVisible=true;bool HasQuestForItem(uint32)const;bool InBattleground()const{return false;}int32 GetQuestObjectiveData(Quest const* q,int8 i)const{return const_cast<Player*>(this)->GetQuestObjectiveData(q,i);}')
$f=$f.Replace('GetQuestSlotQuestId(uint8 index){','GetQuestSlotQuestId(uint8 index)const{').Replace('GetGroup(){return group;}','GetGroup()const{return group;}').Replace('int GetDifficultyID(){','int GetDifficultyID()const{')
$f=$f.Replace('struct Quest {','struct Quest {uint32 ItemDrop[4]={},ItemDropQuantity[4]={};')
$f=$f.Replace('struct SpellInfo {','struct SpellInfo {uint32 MiscValue=97982;')
$f=$f.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {bool IsPlayer(){return ToPlayer()!=nullptr;}')
$f=$f.Replace('struct Creature:Unit {','struct Creature:Unit {bool IsWithinDistInMap(WorldObject* p,float distance){return map==p->map&&GetExactDist(p)<distance+11.25f+1.5f;}')
[IO.File]::WriteAllText("$out/FamiliarFixture.inc",$f)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('bool Player::HasQuestForItem(');$b=$core.IndexOf('bool Player::HasQuestForCreature(',$a);[IO.File]::WriteAllText("$out/HasQuestForItem.inc",$core.Substring($a,$b-$a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectKillCredit(');$b=$effects.IndexOf('void Spell::EffectQuestFail(',$a);[IO.File]::WriteAllText("$out/SpellGroupCredit.inc",$effects.Substring($a,$b-$a))
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$smart.IndexOf('        case SMART_ACTION_CALL_KILLEDMONSTER:');$b=$smart.IndexOf('        case SMART_ACTION_SET_INST_DATA:',$a);[IO.File]::WriteAllText("$out/SmartGroupCredit.inc",$smart.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_familiar_fate.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','');[IO.File]::WriteAllText("$out/FamiliarFate.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_familiar_fate()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimFamiliarFateTest.cpp`" /Fe`"$out/FamiliarFate.exe`" /Fo`"$out/FamiliarFate.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/FamiliarFate.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Familiar Fate production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_familiar_binding','spell_stormheim_familiar_shard','player_stormheim_familiar_fate','A Familiar Fate: ignored generic binding credit','A Familiar Fate: ignored generic binding talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1;if(!$relative){throw "Missing actual $name link input"};$lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative));if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name input"}}
 'PASS: canonical Release Familiar Fate registrations/guards and actual scripts/game link inputs.'
}
