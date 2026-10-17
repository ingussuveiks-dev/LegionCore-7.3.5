$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-impalement"
& "$PSScriptRoot/Test-StormheimImpalement.ps1" *> "$repo/.codex/impalement-lifecycle-upstream.log"
& "$PSScriptRoot/Test-StormheimImpalementData.ps1" -Live
$sql="SELECT 'template',type,flags,Data5,Data10,Data20 FROM gameobject_template WHERE entry=251257; SELECT 'spawns',COUNT(*),SUM(spawntimesecs=180 AND animprogress=255) FROM gameobject WHERE id=251257 AND map=1220; SELECT 'linked',COUNT(*) FROM linked_respawn WHERE guid IN(SELECT guid FROM gameobject WHERE id=251257); SELECT 'pool',COUNT(*) FROM pool_gameobject WHERE guid IN(SELECT guid FROM gameobject WHERE id=251257); SELECT 'spawn',guid,spawntimesecs,animprogress FROM gameobject WHERE id=251257 ORDER BY guid;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Launcher lifecycle World query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content "$out/lifecycle-world.tsv"
foreach($line in @("template`t10`t262144`t1`t0`t0","spawns`t17`t17","linked`t0","pool`t0")){if($rows -notcontains $line){throw "Native launcher fixture no longer matches live source: $line"}}
$source=Get-Content "$PSScriptRoot/StormheimImpalementTest.cpp" -Raw
$source=$source.Substring(0,$source.IndexOf('int main()'))
$source=$source.Replace('struct ObjectGuid {','enum class HighGuid {GameObject};struct ObjectGuid {explicit operator bool()const{return value!=0;}template<HighGuid>static ObjectGuid Create(uint32,uint32,uint32 id){return {id};}')
$source=$source.Replace('struct Template {','struct Template {uint32 GetXpLevel()const{return 0;}bool IsDespawnAtAction()const;struct {uint32 consumable=0;} chest;')
$source=$source.Replace('uint32 open=43;','uint32 open=43,spell=0,consumable=1,cooldown=0;')
$source=$source.Replace('uint32 GetDBTableGUIDLow(){return 0;}','uint32 GetDBTableGUIDLow(){return m_DBTableGuid;}')
$source=$source.Replace('bool isSpawned(){return spawned;}','bool isSpawned()const;')
$source=$source.Replace('struct GameObject {','struct GameObject {' + @'
 uint32& m_lootState=state;time_t m_respawnTime=0;uint32 m_respawnDelayTime=180,m_DBTableGuid=115492,m_usetimes=0;
 bool m_spawnedByDefault=true,m_isUpdate=false;std::vector<ObjectGuid> m_unique_users;std::vector<int> m_SkillupList;
 uint32 animations=0,visibility=0,saves=0,goState=0;struct {uint32 clears=0;void clear(){++clears;}} loot;
 LifecycleMap mapState;struct SpawnData {uint32 areaId=7608;} data;
 void Tick();LifecycleMap* GetMap(){return &mapState;}SpawnData* GetGOData(){return &data;}
 uint32 GetGoAnimProgress(){return 255;}uint32 GetGoState(){return goState;}void SetGoState(uint32 s){goState=s;}
 void SendObjectDeSpawnAnim(ObjectGuid){++animations;}void UpdateObjectVisibility(){++visibility;}void SaveRespawnTime(){++saves;}
 void SetUInt32Value(uint32,uint32 v){flags=v;}uint32 GetSpellId(){return 0;}ObjectGuid GetOwnerGUID(){return {};}
 void SetRespawnTime(uint32 delay){m_respawnTime=delay?GameTime::GetGameTime()+delay:0;}void Delete(){assert(false);}
 Unit* GetOwner(){return nullptr;}void ResetDoorOrButton(){assert(false);}uint32 GetPlayerFromArea(uint32){return lifecycleWorld.players;}
'@)
$source=$source.Replace('void SetLootState(uint32 s,Unit*)','void SetLootState(uint32 s,Unit* = nullptr)')
$source=$source.Replace('bool IsCreature(){return false;}','bool IsCreature(){return false;}bool IsPlayer(){return ToPlayer()!=nullptr;}')
$source=$source.Replace('Player* ToPlayer()override{return this;}','void CastSpell(Player*,uint32,bool){assert(false);}void RemoveGameObject(GameObject*,bool){assert(false);}void SendDirectMessage(int){assert(false);}Player* ToPlayer()override{return this;}')
# Services used by the native timer branch are explicit fixture boundaries.
$services=@'
#include <ctime>
enum {GO_READY=1,GO_STATE_READY=1,GO_NOT_READY=0,GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_FISHINGNODE=17,GAMEOBJECT_TYPE_DOOR=0,GAMEOBJECT_TYPE_BUTTON=1,GO_FLAG_NODESPAWN=32,DAY=86400,MINUTE=60,CONFIG_RESPAWN_FROM_PLAYER_ENABLED=1,CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY=2,CONFIG_RESPAWN_FROM_PLAYER_COUNT=3};
namespace GameTime {time_t now=1000;time_t GetGameTime(){return now;}}
struct LifecycleWorld {bool adaptive=false,save=true;uint32 players=0;bool getBoolConfig(int c){return c==CONFIG_RESPAWN_FROM_PLAYER_ENABLED?adaptive:save;}uint32 getIntConfig(int){return 10;}} lifecycleWorld;
auto sWorld=&lifecycleWorld;
uint32 urand(uint32 low,uint32 high){assert(low<=12&&high>=12);return 12;}
struct LifecycleMap {uint32 adds=0;bool instance=false;time_t GetLinkedRespawnTime(auto){return 0;}void AddToMap(void*){++adds;}bool Instanceable(){return instance;}};
struct LifecyclePool {template<class T>uint32 IsPartOfAPool(uint32){return 0;}template<class T>void UpdatePool(uint32,uint32){assert(false);}} lifecyclePool;auto sPoolMgr=&lifecyclePool;
namespace WorldPackets {namespace GameObject {struct FishNotHooked {int Write(){return 0;}};}}
'@
$at=$source.IndexOf('struct GameObject {')
$source=$source.Insert($at,$services)
$source=$source.Replace('std::string GetScriptName(uint32 id)', 'ObjectGuid GetLinkedRespawnGuid(ObjectGuid){assert(false);return {};}std::string GetScriptName(uint32 id)')
[IO.File]::WriteAllText("$out/ImpalementLifecycleFixture.inc",$source)
$core=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
function ExtractBlock([string]$text,[string]$marker,[int]$start=0){
 $a=$text.IndexOf($marker,$start);if($a -lt 0){throw "Missing native block: $marker"};$open=$text.IndexOf('{',$a);$depth=1;$b=$open+1
 while($depth -gt 0 -and $b -lt $text.Length){if($text[$b] -eq '{'){$depth++};if($text[$b] -eq '}'){$depth--};$b++}
 if($depth){throw "Unterminated native block: $marker"};return $text.Substring($a,$b-$a)
}
$update=$core.IndexOf('void GameObject::Update(')
[IO.File]::WriteAllText("$out/ImpalementDeactivate.inc",(ExtractBlock $core '        case GO_JUST_DEACTIVATED:' $update))
$ready=$core.IndexOf('        case GO_READY:',$update)
[IO.File]::WriteAllText("$out/ImpalementRespawn.inc",(ExtractBlock $core '            if (m_respawnTime > 0)' $ready))
[IO.File]::WriteAllText("$out/ImpalementSpawned.inc",(ExtractBlock $core 'bool GameObject::isSpawned() const'))
$template=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
[IO.File]::WriteAllText("$out/ImpalementDespawnAtAction.inc",(ExtractBlock $template 'bool GameObjectTemplate::IsDespawnAtAction() const').Replace('GameObjectTemplate::','Template::'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimImpalementLifecycleTest.cpp`" /Fe`"$out/Lifecycle.exe`" /Fo`"$out/Lifecycle.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Lifecycle.exe`"`r`n"
[IO.File]::WriteAllText("$out/run-lifecycle.cmd",$cmd);& "$out/run-lifecycle.cmd"
if($LASTEXITCODE){throw 'Native launcher deactivation/respawn regression failed'}
