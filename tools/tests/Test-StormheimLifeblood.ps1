param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimLifebloodData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimBargain.ps1" *> "$PSScriptRoot/../../.codex/lifeblood-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-lifeblood"
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-helheim-bargain/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/BargainFixture.inc" -Raw
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {Position teleportPoint;uint32 teleportMap=0,teleports=0;bool teleportResult=true;WorldLocation& GetTeleportDest(){static WorldLocation d;d.map=teleportMap;static_cast<Position&>(d)=teleportPoint;return d;}bool TeleportTo(uint32 m,float x,float y,float z,float o){++teleports;if(!teleportResult)return false;teleportMap=m;teleportPoint={x,y,z,o};teleport=true;return true;}')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {float GetDistance(WorldObject* source){return Position::GetExactDist(source);}')
$fixture=$fixture.Replace('enum class HighGuid {Player,','enum class HighGuid {Conversation,Player,')
$fixture=$fixture.Replace('static ObjectGuid const Empty;', 'template<HighGuid H>static ObjectGuid Create(uint64 n){ObjectGuid g(n);g.high=H;return g;}uint64 GetLowPart(){return value;}uint64 GetHighPart(){return 0;}static ObjectGuid const Empty;')
$fixture=$fixture.Replace('struct WorldObject:Position {','struct WorldObject:Position {Creature* FindNearestCreature(uint32,float){return nullptr;}float GetVisibilityRange(){return 90;}Position const& GetPosition(){return *this;}')
$fixture=$fixture.Replace('struct SpellInfo {','struct SpellInfo {struct Effect {uint32 MiscValue=0;} effect;Effect* Effects[1]{&effect};')
$fixture=$fixture.Replace('struct Motion {','struct Motion {uint32 idles=0;void MoveIdle(){++idles;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct WorldLocation:Position {uint32 map=0;uint32 GetMapId(){return map;}};struct Unit:WorldObject {')
$fixture=$fixture.Replace('return id==0&&item?this:nullptr;', 'return id<=1&&item?this:nullptr;')
# Match ObjectAccessor's map-local lookup; a stranded actor must clean up on
# its own update, without dereferencing an old map during OnMapChanged.
$fixture=$fixture.Replace('Creature* GetCreature(WorldObject&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}', 'Creature* GetCreature(WorldObject& source,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world||i->second->map!=source.map?nullptr:i->second;}')
[IO.File]::WriteAllText("$out/LifebloodFixture.inc",$fixture)
foreach($module in @('lifeblood','helheim_bargain')){
 $source=Get-Content "$repo/src/server/scripts/Legion/stormheim_$module.cpp" -Raw
 $source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
 [IO.File]::WriteAllText("$out/$module.inc",$source.Substring(0,$source.IndexOf("void AddSC_stormheim_$module()")))
}
$eo=Get-Content "$repo/src/server/game/Entities/EventObject/EventObject.cpp" -Raw
$a=$eo.IndexOf('void EventObject::Update(');$b=$eo.IndexOf('void EventObject::MoveInLineOfSight(',$a)
[IO.File]::WriteAllText("$out/NativeEventUpdate.inc",$eo.Substring($a,$b-$a))
$a=$eo.IndexOf('bool EventObject::ActivatedForPlayer(')
[IO.File]::WriteAllText("$out/NativeEventActivated.inc",$eo.Substring($a))
$definitions=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
$a=$definitions.IndexOf('struct ConversationActor');$b=$definitions.IndexOf('typedef ',$a)
[IO.File]::WriteAllText("$out/NativeConversationStructs.inc",$definitions.Substring($a,$b-$a))
$conversation=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
$a=$conversation.IndexOf('bool Conversation::GenerateDynamicFieldData(')
[IO.File]::WriteAllText("$out/NativeConversationData.inc",$conversation.Substring($a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectSummonConversation(');$b=$effects.IndexOf('void Spell::EffectLearnGarrisonBuilding(',$a)
[IO.File]::WriteAllText("$out/NativeConversationSpell.inc",$effects.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimLifebloodTest.cpp`" /Fe`"$out/Lifeblood.exe`" /Fo`"$out/Lifeblood.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Lifeblood.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Paid in Lifeblood production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_lifeblood_ashildir','eventobject_stormheim_lifeblood_return','player_stormheim_lifeblood','Paid in Lifeblood: ignored generic exit credit','Paid in Lifeblood: ignored generic exit talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name input"}
 }
 'PASS: canonical Release Lifeblood registrations/guards and actual scripts/game final-link inputs.'
}
