param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-HallsAegisData.ps1"
& "$PSScriptRoot/Test-StormheimSalvation.ps1" *> "$PSScriptRoot/../../.codex/aegis-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/halls-aegis"
$upstream="$repo/build-extractors/runtime-test/stormheim-salvation"
Get-ChildItem "$upstream/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$upstream/SalvationFixture.inc" -Raw
$f=$f.Replace('struct WorldObject:Position {','struct WorldObject:Position {FakeMap* mapObject=nullptr;uint32 instanceId=0;')
$f=$f.Replace('uint32 GetInstanceId(){return 0;}','uint32 GetInstanceId(){return instanceId;}')
$f=$f.Replace('FakeMap* GetMap()const{static FakeMap m;return &m;}','FakeMap* GetMap()const{return mapObject;}')
$f=$f.Replace('struct GameObject:WorldObject {','struct GameObject:WorldObject {void Use(Unit*);uint32 m_cooldownTime=0;FakeMap* GetMap(){return mapObject;}InstanceScript* instanceScript=nullptr;InstanceScript* GetInstanceScript(){return instanceScript;}uint32 GetTypeId()override{return 5;}bool IsWithinDistInMap(WorldObject const* p,float n){return mapObject==p->mapObject&&GetExactDist(p)<=n;}')
$f=$f.Replace('struct GameObjectTemplate {','struct GameObjectTemplate {float size=1;std::string name="Aegis";uint32 GetCooldown(){return 0;}')
$f=$f.Replace('uint32 gossipID=18790;', 'uint32 gossipID=18790,conditionID1=0;')
$f=$f.Replace('struct GameObjectAI {','struct GameObjectAI {GameObject* go=nullptr;explicit GameObjectAI(GameObject* g=nullptr):go(g){if(g)g->ai=this;}')
$f=$f.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {bool IsPlayer(){return ToPlayer()!=nullptr;}')
$f=$f.Replace('GameObject* GetGameObjectIfCanInteractWith(ObjectGuid,int){return nullptr;}','GameObject* GetGameObjectIfCanInteractWith(ObjectGuid,GameobjectTypes)const;')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {uint32 prepared=0,sent=0;void PrepareGossipMenu(GameObject*,uint32){++prepared;}void SendPreparedGossip(GameObject*){++sent;}')
[IO.File]::WriteAllText("$out/AegisFixture.inc",$f)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('GameObject* Player::GetGameObjectIfCanInteractWith(');$b=$core.IndexOf('bool Player::IsUnderWater(',$a)
[IO.File]::WriteAllText("$out/NativeGOInteract.inc",$core.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Legion/HallsofValor/halls_of_valor_aegis.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','');[IO.File]::WriteAllText("$out/Aegis.inc",$source.Substring(0,$source.IndexOf('void AddSC_halls_of_valor_aegis()')))
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$source.IndexOf('void GameObject::Use(Unit* user)');$b=$source.IndexOf('    switch (GetGoType())',$a);$c=$source.IndexOf('        case GAMEOBJECT_TYPE_QUESTGIVER:',$b);$d=$source.IndexOf('        case GAMEOBJECT_TYPE_TRAP:',$c)
[IO.File]::WriteAllText("$out/NativeQuestgiverUse.inc",$source.Substring($a,$b-$a)+"    switch (GetGoType()) {`n"+$source.Substring($c,$d-$c)+"default:return;`n}}`n")
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/HallsAegisTest.cpp`" /Fe`"$out/Aegis.exe`" /Fo`"$out/Aegis.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Aegis.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Aegis production/native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$bytes.Contains('go_halls_of_valor_quest_aegis')){throw 'Canonical Release lacks Aegis registration'}
 'PASS: canonical Release contains the new Aegis registration.'
}
