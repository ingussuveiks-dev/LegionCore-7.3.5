param([switch]$Baseline,[switch]$CreationBaseline,[switch]$HandoffBaseline,[switch]$SurrenderBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $CreationBaseline -or $HandoffBaseline -or $SurrenderBaseline) -and $VerifyReleaseBinary){throw 'Baseline cannot verify current Release'}
if(@($Baseline,$CreationBaseline,$HandoffBaseline,$SurrenderBaseline | Where-Object {$_}).Count -gt 1){throw 'Choose one baseline'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-finale"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "675d6f7a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Magnar source missing'}}
if($CreationBaseline){$source=(& git -C $repo show "65f7a845:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Magnar creation source missing'}}
if($HandoffBaseline){$source=(& git -C $repo show "f91fbfc7:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Magnar handoff source missing'}}
if($SurrenderBaseline){$source=(& git -C $repo show "fc099666:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Magnar surrender source missing'}}
$source=$source.Substring($source.IndexOf('struct npc_magnar_icebreakerAI :'))
$first=$source.IndexOf('        SummonList summons;');$last=$source.IndexOf('        void Reset()',$first)
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/Reset.inc",(Block $source 'void Reset()'))
[IO.File]::WriteAllText("$out/Eligible.inc",$(if($Baseline){''}else{(Block $source 'bool IsEncounterPlayer(')+(Block $source 'Player* GetEncounterPlayer(')}))
[IO.File]::WriteAllText("$out/Before.inc",$(if($Baseline){Block $source 'void DamageTaken('}else{''}))
[IO.File]::WriteAllText("$out/After.inc",$(if($Baseline){''}else{Block $source 'void DamageTakenAfterScaling('}))
[IO.File]::WriteAllText("$out/Conversation.inc",$(if($Baseline -or $CreationBaseline){''}else{Block $source 'bool StartFinaleConversation('}))
[IO.File]::WriteAllText("$out/Handoff.inc",$(if($source.Contains('void UpdateFinale(')){(Block $source 'uint32 GetFinaleHandoffDelay(')+(Block $source 'void UpdateFinale(')}else{''}))
$source=Get-Content "$PSScriptRoot/WarswordsRetreatTest.cpp" -Raw
$header=$source.Substring(0,$source.IndexOf('struct FirstBoss :'))
$header="#include <set>`n"+$header.Replace('bool IsPlayer() const;','bool IsPlayer() const; bool operator<(ObjectGuid const& other) const { return std::less<Unit*>{}(unit,other.unit); }')
$header=$header.Replace('uint32 Deal(Creature* victim, uint32 damage);','uint32 Deal(Creature* victim, uint32 damage); void CreateConversation(uint32 identity);')
$header=$header.Replace('bool IsPlayer() const;', 'void Clear() { unit = nullptr; } bool IsPlayer() const;')
$header=$header.Replace('ObjectGuid GetCharmerOrOwnerGUID() const', 'ObjectGuid GetGUID() const { return {const_cast<Unit*>(this)}; } ObjectGuid GetCharmerOrOwnerGUID() const')
$header=$header.Replace('struct Map { } instanceMap, unrelatedMap;',@'
struct Player;
struct PlayerReference { Player* player; Player* getSource() const { return player; } };
struct Map { std::vector<PlayerReference> players; auto const& GetPlayers() const { return players; } } instanceMap, unrelatedMap;
'@)
$header=$header.Replace('UNIT_FIELD_FLAGS = 0,','UNIT_FIELD_FLAGS = 0, REACT_DEFENSIVE = 1, UNIT_FLAG_NOT_SELECTABLE = 0x2000000,')
$header=$header.Replace('uint32 identity = 944, step = 4;','uint32 identity = 909, step = 4;')
$header=$header.Replace('assert(identity == 40043);','assert(identity == 39191);').Replace('identity == 46982','identity == 45784')
$header=$header.Replace('    bool CanContact() { return contact; }','    bool world = true; uint32 instance = 42; bool IsInWorld() const { return world; } uint32 GetInstanceId() const { return instance; } bool CanContact() { return contact; }')
$header=$header.Replace('uint32 entry = 98007, mapId = 1511','uint32 entry = 96034, mapId = 1495')
$header=$header.Replace('Map* GetMap() const { return &instanceMap; }','Map* map = &instanceMap; Map* GetMap() const { return map; } float GetDistance(Player* player) const { return player->distance; }')
$header=$header.Replace('void RemoveAura(uint32 identity) { assert(identity == 128264 || identity == 195996 || identity == 205424); if (identity == 128264) ++removed; }','void RemoveAura(uint32 identity) { assert(identity == 128749 || identity == 191883); if (identity == 128749) ++removed; else { ++pentaRemovals; penta = false; auto callback = onPentaRemoval; if (callback) callback(); } }')
$header=$header.Replace('    std::vector<uint32> points;', '    uint32 clears = 0, idles = 0; bool chase = true; void Clear() { ++clears; chase = false; } void MoveIdle() { ++idles; } std::vector<uint32> points;')
$header=$header.Replace('    void StopAttack() { ++stops; }', @'
    bool moving = true, attacking = true, casting = true, delayedCast = true, penta = true;
    uint32 interrupts = 0, splineStops = 0, pentaRemovals = 0;
    std::function<void()> onInterrupt, onPentaRemoval;
    bool isMoving() const { return moving; }
    void AttackStop() { attacking = false; ++stops; }
    void InterruptNonMeleeSpells(bool withDelayed) { assert(withDelayed); ++interrupts; casting = delayedCast = false; auto callback = onInterrupt; if (callback) callback(); }
    void StopAttack(bool clearMove = false, bool interruptSpells = false);
'@)
$header=$header.Replace('void DespawnOrUnsummon(uint32 delay) { assert(delay == 7000); ++despawns; }','void DespawnOrUnsummon(uint32 delay = 0) { assert(delay == 0 || delay == 6000); ++despawns; }')
$header=$header.Replace('struct ObjectAccessor', 'struct Conversation; struct ObjectAccessor').Replace('    static Player* GetPlayer(', '    static Conversation* GetConversation(Unit const&, ObjectGuid guid); static Player* GetPlayer(')
$header=$header.Replace('    bool contact = true,', '    bool creditWorks = true; std::function<void()> onCredit; bool contact = true,')
$header=$header.Replace('        ++credits;', '        ++credits; if (onCredit) onCredit(); if (!creditWorks) return;')
$header=$header.Replace('    Player* FindNearestPlayer(float range)',@'
    uint32 conversations = 0, conversationAttempts = 0, retiredConversations = 0, faction = 14;
    bool conversationWorks = true, world = true, despawning = false;
    std::function<void()> onConversation, onCreationFailure;
    bool IsInWorld() const { return world; }
    bool IsDespawn() const { return despawning; }
    void setFaction(uint32 value) { faction = value; }
    Player* FindNearestPlayer(float range)
'@)
$header=$header.Replace('uint64 health = 100000, maximum','uint64 healthValue = 100000, maximum').Replace('return health;','return healthValue;').Replace('100.f * health / maximum','100.f * healthValue / maximum').Replace('health = value;','healthValue = value;')
$header+=@'
namespace Movement
{
    struct MoveSplineInit
    {
        Creature& actor;
        explicit MoveSplineInit(Creature& target) : actor(target) { }
        void Stop(bool first, bool second) { assert(!first && second); ++actor.splineStops; actor.moving = false; }
    };
}
#include "StopAttack.inc"
enum class HighGuid { Conversation };
struct Generator { uint32 next = 0; uint32 Generate() { return ++next; } } generator;
struct ObjectMgr { template<HighGuid> Generator* GetGenerator() { return &generator; } } objectManager;
auto sObjectMgr = &objectManager;
uint32 warningCount = 0;
#define TC_LOG_WARN(...) (++warningCount)
struct Conversation
{
    inline static std::set<Conversation*> allocations;
    inline static uint32 failedDeletes = 0;
    Creature* actor = nullptr;
    bool created = false, removed = false;
    ObjectGuid GetGUID() const { return {reinterpret_cast<Unit*>(const_cast<Conversation*>(this))}; }
    Conversation() { allocations.insert(this); }
    ~Conversation() { if (!created) ++failedDeletes; allocations.erase(this); }
    bool CreateConversation(uint32, uint32 identity, Unit* caster, void*, Unit const&)
    {
        assert(identity == 504);
        actor = caster->ToCreature();
        assert(actor);
        ++actor->conversationAttempts;
        if (!actor->conversationWorks)
        {
            auto callback = actor->onCreationFailure;
            if (callback) callback();
            return false;
        }
        ++actor->conversations;
        created = true;
        auto callback = actor->onConversation;
        if (callback) callback();
        return true;
    }
    void Remove() { assert(created && !removed); removed = true; ++actor->retiredConversations; }
    static void Cleanup() { while (!allocations.empty()) delete *allocations.begin(); }
};
Conversation* ObjectAccessor::GetConversation(Unit const&, ObjectGuid guid)
{
    auto* candidate = reinterpret_cast<Conversation*>(guid.unit);
    return Conversation::allocations.count(candidate) && !candidate->removed ? candidate : nullptr;
}
struct ConversationData { uint32 entry, id, idx, textId, unk1, unk2; };
struct ConversationDataStore
{
    bool available = true;
    std::vector<ConversationData> lines;
    auto const* GetConversationData(uint32 entry) const { assert(entry == 504); return available ? &lines : nullptr; }
    void Reset() { available = true; lines = {{504,1341,0,0,0,0},{504,1342,1,8618,0,1},{504,1343,2,23424,0,0},{504,1344,3,34064,0,1}}; }
} conversationDataStore;
auto* sConversationDataStore = &conversationDataStore;
#include "UnitConversation.inc"
'@
[IO.File]::WriteAllText("$out/Harness.inc",$header)
[IO.File]::WriteAllText("$out/Deal.inc",(Block $source 'uint32 Unit::Deal('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/UnitConversation.inc",(Block $source 'void Unit::CreateConversation('))
foreach($entry in @(@('OwnerPlayer','Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'),@('Health','uint64 Unit::GetHealth(Unit* victim) const'),@('MaxHealth','uint64 Unit::GetMaxHealth(Unit* victim) const'),@('SetHealthScal','void Unit::SetHealthScal('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$first=$source.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);',$source.IndexOf('uint32 Unit::DealDamage('));$hook=$source.IndexOf('    if (Creature* creature = victim->ToCreature())',$first)
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",$source.Substring($first,$hook-$first)+(Block $source.Substring($hook) 'if (Creature* creature = victim->ToCreature())'))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/StopAttack.inc",(Block $source 'void Creature::StopAttack('))
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block $source 'float Creature::GetHealthMultiplierForTarget('))
$source=Get-Content "$repo/src/common/Utilities/Util.h" -Raw
[IO.File]::WriteAllText("$out/CalculatePct.inc","template <class T, class U>`n"+(Block $source 'T CalculatePct('))
$source=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $source 'enum CriteriaTypes')+';')

$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($CreationBaseline){'/DCREATION_BASELINE'}elseif($HandoffBaseline){'/DHANDOFF_BASELINE'}elseif($SurrenderBaseline){'/DSURRENDER_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/MagnarFinaleTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Magnar native finale failed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $rows=@(@'
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE entry=96034;
SELECT 'spawn',guid,id,map,phaseId FROM creature WHERE id=96034;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax,LevelScalingDeltaMin,LevelScalingDeltaMax,LevelScalingDuration FROM creature_template_scaling WHERE Entry=96034;
SELECT 'smart',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN (96034,-361400);
SELECT 'handoff',id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=504 ORDER BY idx;
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Magnar live metadata read failed'}
 }finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("binding`t96034`t`tnpc_magnar_icebreaker","spawn`t361400`t96034`t1495`t6999","scaling`t96034`t98`t110`t0`t0`t100","smart`t0")){if($rows -notcontains $expected){throw "Magnar live metadata differs: $expected"}}
 foreach($expected in @("handoff`t1341`t0`t0`t0`t0","handoff`t1342`t1`t8618`t0`t1","handoff`t1343`t2`t23424`t0`t0","handoff`t1344`t3`t34064`t0`t1")){if($rows -notcontains $expected){throw "Magnar live handoff schedule differs: $expected"}}
 if(@($rows | Where-Object {$_ -like "handoff`t*"}).Count -ne 4){throw 'Magnar live handoff requires exactly four indexed lines'}
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Magnar criteria catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($expected in @("EARTH_CRITERION`t3`t29010`t92`t46427`t1","EARTH_CRITERION`t4`t28549`t92`t45784`t1","EARTH_CRITERION`t5`t28550`t92`t45785`t0","DURATION`t128749`t32`t6000`t6000`t0","EFFECT`t128749`t0`t6`t23`t0`t128750`t1`t0","EFFECT`t128749`t1`t6`t33`t0`t0`t1`t0")){if($native -notcontains $expected){throw "Native Magnar stage/aura differs: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_magnar_finale_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Magnar finale missing'}}finally{$env:PYTHONPATH=$previous}
}
