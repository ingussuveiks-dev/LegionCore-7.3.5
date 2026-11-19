// Time-driven real Creature AI,Spell/Aura,area/summon updates and evade/re-pull.
// Synthetic stationary party and health checkpoints;not client dungeon traversal.
#define LEGION_TEEMING_AI_SUPPORT_ONLY
#include "LegionTeemingAINative.cpp"
#include "AreaTrigger.h"
#include "AreaTriggerData.h"
#include "DynamicObject.h"
#include "TemporarySummon.h"
#include "GridNotifiersImpl.h"
#include "Spell.h"
#include "DisableMgr.h"
#ifdef DUNGEON_SIC_BASELINE
static constexpr uint32 SPELL_SIC_BATS=203163;
#include "SicBatBaseline.inc"
#endif
void AddSC_challenge_scripts();
void AddSC_generic_spell_scripts();

struct AllCombatObjects { bool operator()(WorldObject*) const { return true; } };
static std::list<WorldObject*> Objects(InstanceMap& map,Player& owner)
{
    std::list<WorldObject*> objects;AllCombatObjects check;
    Trinity::WorldObjectListSearcher<AllCombatObjects> search(&owner,objects,check);
    map.VisitAll(owner.GetPositionX(),owner.GetPositionY(),300.f,search);return objects;
}
static void Tick(InstanceMap& map,Player& owner,Player& member,uint32 diff,Creature* controller=nullptr)
{
    owner.Unit::Update(diff);member.Unit::Update(diff);
    auto objects=Objects(map,owner);
    for(auto object:objects)if(object->IsInWorld()&&!object->IsPreDelete()&&!object->IsPlayer())
    {
        if(object==controller)controller->Unit::Update(diff);else object->Update(diff);
    }
    map.MoveAllCreaturesInMoveList();map.MoveAllGameObjectsInMoveList();map.MoveAllDynamicObjectsInMoveList();map.MoveAllAreaTriggersInMoveList();map.RemoveAllObjectsInRemoveList();
}
static void ClearEphemeral(InstanceMap& map,Player& owner)
{
    for(auto object:Objects(map,owner))
    {
        if(auto actor=object->ToCreature())if(auto summon=actor->ToTempSummon())summon->UnSummon();
        if(auto area=object->ToAreaTrigger())area->Remove();
        if(auto dynamic=object->ToDynObject())dynamic->Remove();
    }
    map.RemoveAllObjectsInRemoveList();
}

static unsigned Areas(InstanceMap& map,Player& owner,uint32 spell,ObjectGuid caster)
{
    unsigned count=0;for(auto object:Objects(map,owner))if(auto area=object->ToAreaTrigger())if(area->GetSpellId()==spell&&area->GetCasterGUID()==caster)++count;return count;
}

static bool NativeFrenzyDeath(NativeAIActor& actor,bool hadOwnFrenzy);
static void HazardReceipts(uint32 entry,NativeAIActor& actor,InstanceMap& map,Player& owner,Player& member)
{
    auto pump=[&](uint32 duration){for(unsigned t=0;t<duration;t+=100)Tick(map,owner,member,100,&actor);};
    if(entry==98792)
    {
        Require(actor.CastSpell(&actor,201061,true)==SPELL_CAST_OK,"Actual native Frenzy cast failed");
        bool ended=false;for(unsigned elapsed=0;elapsed<25000;elapsed+=100){bool own=actor.HasAura(201061,actor.GetGUID());pump(100);if(NativeFrenzyDeath(actor,own)){ended=true;break;}}
        Check(ended,"Native5% Frenzy failed its actual self-damage death endpoint");
        actor.Respawn(true,0);actor.Respawn(false,0);Check(actor.IsAlive()&&actor.IsInWorld()&&!actor.HasAura(201061),"Native Frenzy corpse/respawn retained the dead attempt");
        std::cout<<"HAZARD entry=98792 controlled actual201061 cast,self-damage death and native corpse/respawn PASS;not a forced random potion selection\n";
    }
    if(entry==96574)
    {
        uint64 before=owner.GetHealth()+member.GetHealth();
        Require(actor.CastSpell(&owner,199805,true)==SPELL_CAST_OK,"Actual Crackle parent cast failed");pump(3000);
        Check(Areas(map,owner,199808,actor.GetGUID())>0,"Crackle did not reach its native missile/area trigger");
        Check(owner.HasAura(199818,actor.GetGUID())&&owner.GetHealth()+member.GetHealth()<before,"Actual Crackle area failed its native periodic player damage");
        Require(actor.CastSpell(&owner,199805,true)==SPELL_CAST_OK,"Repeated actual Crackle cast failed");pump(1000);
        std::vector<AreaTrigger*> fields;for(auto object:Objects(map,owner))if(auto area=object->ToAreaTrigger())if(area->GetSpellId()==199808&&area->GetCasterGUID()==actor.GetGUID())fields.push_back(area);
        Require(fields.size()>=2,"Crackle overlap fixture did not create two actual fields");fields.front()->Remove();pump(100);
        Check(owner.HasAura(199818,actor.GetGUID()),"An ending Crackle field removed another live field's damage aura");
        NativeAIActor foreign;
        Require(foreign.Create(190600,&map,1,96574,0,ALLIANCE,actor.GetPositionX(),actor.GetPositionY(),actor.GetPositionZ(),actor.GetOrientation()),"Second actual Crackle caster create failed");foreign.SetHomePosition(foreign);Require(map.AddToMap(static_cast<Creature*>(&foreign)),"Second actual Crackle caster publication failed");
        Require(foreign.CastSpell(&owner,199805,true)==SPELL_CAST_OK,"Second caster Crackle cast failed");pump(1000);
        Check(owner.HasAura(199818,foreign.GetGUID()),"Second Crackle caster lost its independent aura identity");
        for(auto object:Objects(map,owner))if(auto area=object->ToAreaTrigger())if(area->GetSpellId()==199808&&area->GetCasterGUID()==actor.GetGUID())area->Remove();map.RemoveAllObjectsInRemoveList();
        Check(!owner.HasAura(199818,actor.GetGUID())&&owner.HasAura(199818,foreign.GetGUID()),"Crackle cleanup removed a foreign caster's contribution");
        ClearEphemeral(map,owner);Check(!owner.HasAura(199818,actor.GetGUID()),"Native Crackle area exit/despawn retained its damage aura");
        Check(!owner.HasAura(199818,foreign.GetGUID()),"Second Crackle caster cleanup retained its contribution");
        std::cout<<"HAZARD Crackle native parent/missile/area/periodic player damage/removal PASS\n";
    }
    if(entry==96587)
    {
        uint64 before=owner.GetHealth()+member.GetHealth(),npcHealth=actor.GetHealth();
        Require(owner.CastSpell(&owner,191641,true)==SPELL_CAST_OK,"Counterfactual host-cast Nightmares failed");pump(1500);
        Check(Areas(map,owner,191641,owner.GetGUID())>0&&!Areas(map,owner,191641,actor.GetGUID())&&actor.GetHealth()==npcHealth,"Counterfactual original host-caster hazard identity not reproduced with real player condition");
        ClearEphemeral(map,owner);
        std::cout<<"COUNTERFACTUAL Nightmares host-cast native area loses its hostile original caster;party health change="<<int64(before)-int64(owner.GetHealth()+member.GetHealth())<<";not ordinary successful gameplay\n";
        for(uint32 aura:{191639u,193069u})
        {
            before=owner.GetHealth()+member.GetHealth();Require(actor.CastSpell(&owner,aura,true)==SPELL_CAST_OK,"Actual Nightmares parent aura failed");
            for(unsigned waited=0;waited<15000&&!Areas(map,owner,191641,actor.GetGUID());waited+=100)pump(100);
            pump(1000);
            Check(Areas(map,owner,191641,actor.GetGUID())>0&&!Areas(map,owner,191641,owner.GetGUID()),"Nightmares periodic aura lost actual hostile original caster");
            Check(owner.GetHealth()+member.GetHealth()<before&&actor.GetHealth()==npcHealth,"Nightmares hazard did not damage players while preserving its caster");
            owner.RemoveAurasDueToSpell(aura,actor.GetGUID());ClearEphemeral(map,owner);
        }
        std::cout<<"HAZARD Nightmares both actual aura ticks/player-anchor/hostile caster/player damage PASS\n";
    }
    if(entry==114624)
    {
        auto npcHealth=actor.GetHealth();Require(actor.CastSpell(&member,228320,true)==SPELL_CAST_OK,"Actual Volatile Charge parent failed");pump(1000);
        Player* marked=member.HasAura(228331,actor.GetGUID())?&member:owner.HasAura(228331,actor.GetGUID())?&owner:nullptr;
        Require(marked,"Actual Volatile Charge failed its player aura chain");Check(!actor.HasAura(228331),"Volatile Charge marked its NPC caster");
        auto before=owner.GetHealth()+member.GetHealth();marked->RemoveAurasDueToSpell(228331,actor.GetGUID());pump(300);
        Check(owner.GetHealth()+member.GetHealth()<before&&actor.GetHealth()==npcHealth,"Volatile Charge did not explode on actual players without harming its NPC caster");
        owner.RemoveAurasDueToSpell(228331,actor.GetGUID());member.RemoveAurasDueToSpell(228331,actor.GetGUID());
        std::cout<<"HAZARD Volatile Charge real player mark/native removal explosion/NPC exclusion PASS\n";
    }
    for(Player* player:{&owner,&member}){player->RemoveAllAuras();player->SetHealth(player->GetMaxHealth());}
    actor.CastStop();
}

static bool DamageThresholdReceipt(uint32 entry,NativeAIActor& actor,Player& owner,InstanceMap& map,Player& member)
{
    if(entry!=102253&&entry!=98792)return false;
    uint32 spell=188587;
    // Read the actual source constant below;the Scavenger's native potion ID
    // is pinned from boss_illysanna_ravencrest.cpp by the wrapper.
    if(entry==98792){auto value=std::getenv("LEGION_COMBAT_POTION");Require(value,"Actual potion metadata missing");spell=std::stoul(value);}
    actor.SetHealth(actor.GetMaxHealth()/2);
    owner.DealDamage(&actor,1,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
    auto cast=actor.GetCurrentSpell(CURRENT_GENERIC_SPELL);
    bool receipt=(cast&&cast->GetSpellInfo()->Id==spell)||actor.HasAura(spell);
    if(cast&&cast->GetSpellInfo()->Id==spell&&cast->GetCastTime()>0)
    {
        actor.InterruptNonMeleeSpells(false,spell);
        owner.DealDamage(&actor,1,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        cast=actor.GetCurrentSpell(CURRENT_GENERIC_SPELL);
        Check(cast&&cast->GetSpellInfo()->Id==spell,"Interrupted health ability consumed its next actual damage retry");
    }
    for(unsigned elapsed=0;elapsed<5000;elapsed+=100)Tick(map,owner,member,100,&actor);
    owner.DealDamage(&actor,1,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
    cast=actor.GetCurrentSpell(CURRENT_GENERIC_SPELL);
    Check(!cast||cast->GetSpellInfo()->Id!=spell,"Acknowledged health ability repeated within the same attempt");
    std::cout<<"THRESHOLD entry="<<entry<<" spell="<<spell<<" receipt="<<receipt<<'\n';
    return receipt;
}

static bool NativeFrenzyDeath(NativeAIActor& actor,bool hadOwnFrenzy)
{
    if(actor.IsAlive())return false;
    auto info=sSpellMgr->GetSpellInfo(201061);auto effect=info?info->GetEffect(EFFECT_2,actor.GetMap()->GetDifficultyID()):nullptr;
    Check(actor.GetEntry()==98792&&hadOwnFrenzy&&effect&&effect->ApplyAuraName==SPELL_AURA_PERIODIC_DAMAGE_PERCENT&&effect->BasePoints==5,"Combat actor died without its actual native5% Frenzy self-damage");
    std::cout<<"NATIVE_SELF_DAMAGE_DEATH entry=98792 guid="<<actor.GetGUIDLow()<<" spell=201061 percent=5;native authored endpoint,not a surviving90s rotation\n";
    return true;
}

struct DarkHuntObserver:SmartAI
{
    explicit DarkHuntObserver(Creature* actor):SmartAI(actor){}
    std::map<uint32,std::map<ObjectGuid,unsigned>> Hits;
    void SpellHitTarget(Unit* target,SpellInfo const* spell) override
    {
        SmartAI::SpellHitTarget(target,spell);
        if(spell->Id==201517||spell->Id==201564){++Hits[spell->Id][target->GetGUID()];std::cout<<"DARK_HIT spell="<<spell->Id<<" target="<<target->GetGUIDLow()<<" player="<<target->IsPlayer()<<'\n';}
    }
};
static void DarkHuntReceipts(NativeAIActor& actor,InstanceMap& map,Player& owner,Player& member)
{
    Require(actor.GetEntry()==99365&&dynamic_cast<SmartAI*>(actor.AI()),"Actual Stalker SmartAI missing");
    auto observer=new DarkHuntObserver(&actor);Require(actor.AIM_Initialize(observer),"Native forwarding SmartAI observer initialization failed");
    auto origin=actor.GetHomePosition();
    auto reset=[&]{actor.AI()->EnterEvadeMode();owner.CombatStop(true);member.CombatStop(true);for(unsigned t=0;t<30000&&(actor.IsInEvadeMode()||actor.isInCombat());t+=100)Tick(map,owner,member,100);Require(!actor.IsInEvadeMode()&&!actor.isInCombat(),"Native Stalker evade/home did not finish");actor.CastStop();owner.CastStop();member.CastStop();observer->Hits.clear();};
    Position goodOwner,goodMember;
    auto count=[&](uint32 spell){return observer->Hits[spell][owner.GetGUID()]+observer->Hits[spell][member.GetGUID()];};
    auto pump=[&](unsigned ms){for(unsigned t=0;t<ms;t+=100)Tick(map,owner,member,100);};
    auto start=[&]{static_cast<SmartAI*>(actor.AI())->SetCombatMove(false);actor.AI()->AttackStart(&owner);actor.AddThreat(&member,1.f);member.SetInCombatWith(&actor);};
    for(float distance:{0.f,1.f,2.f,20.f,60.f})
    {
        reset();
        unsigned used=0;for(Player* player:{&owner,&member})
        {
            float requested=distance==1?(player==&owner?20.f:0.f):distance==2?(player==&member?20.f:0.f):distance;
            bool placed=false;for(unsigned candidate=used;candidate<64;++candidate)
            {
                auto point=actor.GetNearPosition(requested,float(candidate)*float(M_PI)/32.f);
                player->UpdatePosition(point.GetPositionX(),point.GetPositionY(),point.GetPositionZ(),0,true);
                bool range=requested==0?actor.IsInRange(player,0,7):requested==60?!actor.IsInRange(player,0,40):actor.IsInRange(player,8,40);
                if(range&&actor.IsWithinLOSInMap(player)){placed=true;used=candidate+8;break;}
            }
            Require(placed,"Native terrain/LOS fixture has no eligible position in requested range");
            std::cout<<"DARK_POSITION requested="<<requested<<" player="<<player->GetGUIDLow()<<" actual="<<actor.GetDistance(player)<<" los="<<actor.IsWithinLOSInMap(player)<<'\n';
        }
        if(distance==20){goodOwner=owner.GetPosition();goodMember=member.GetPosition();}
        start();
        auto first=owner.GetHealth(),second=member.GetHealth();
        for(unsigned elapsed=0;elapsed<30000;elapsed+=100)Tick(map,owner,member,100);
        auto primary=observer->Hits[201517][owner.GetGUID()]+observer->Hits[201517][member.GetGUID()];auto child=observer->Hits[201564][owner.GetGUID()]+observer->Hits[201564][member.GetGUID()];
        std::cout<<"DARK_CASE range="<<distance<<" primary="<<primary<<" child="<<child<<" ownerDamage="<<first-owner.GetHealth()<<" memberDamage="<<second-member.GetHealth()<<" casterTravel="<<actor.GetExactDist(origin)<<'\n';
        if(distance==0||distance==60)Check(!primary&&!child,"Dark Hunt ignored actual player-range target admission");
        else Check(primary>0,"Native ranged SmartAI did not execute Dark Hunt");
        if(distance==20)Check(child>0&&owner.GetHealth()<first&&member.GetHealth()<second,"Native Dark Hunt did not execute both actual player hits and teleports");
    }
    auto fresh=[&]{reset();owner.UpdatePosition(goodOwner.GetPositionX(),goodOwner.GetPositionY(),goodOwner.GetPositionZ(),0,true);member.UpdatePosition(goodMember.GetPositionX(),goodMember.GetPositionY(),goodMember.GetPositionZ(),0,true);start();};
    auto queue=[&]{ObjectList targets{&owner,&member};Check(static_cast<SmartAI*>(actor.AI())->QueueDarkHunt(targets),"Native controller request refused actual Stalker scope");};
    auto firstHit=[&]{for(unsigned t=0;t<3000&&!count(201517);t+=100)pump(100);Require(count(201517)==1&&!count(201564),"Native first hit did not expose the real between-hit boundary");};
    fresh();ObjectList duplicate{&owner,&owner,&member};Check(static_cast<SmartAI*>(actor.AI())->QueueDarkHunt(duplicate),"Duplicate-target request rejected");ObjectList reversed{&member,&owner};Check(static_cast<SmartAI*>(actor.AI())->QueueDarkHunt(reversed),"Repeated pending request rejected");pump(4000);
    Check(count(201517)==1&&count(201564)==1,"Repeated/duplicate input duplicated or replaced a native pair");
    Check(observer->Hits[201517][owner.GetGUID()]==1&&observer->Hits[201564][member.GetGUID()]==1,"Pending request changed its actual selected target identities");
    std::cout<<"DARK_LIFECYCLE duplicate targets/pending request exactly-once PASS\n";

    {
        LifecyclePlayer outsider(map,973,goodOwner);outsider.SetMaxHealth(1000000000);outsider.SetHealth(1000000000);
        fresh();ObjectList three{&owner,&member,&outsider};Check(static_cast<SmartAI*>(actor.AI())->QueueDarkHunt(three),"Three-player native request rejected");pump(4000);
        Check(count(201517)==1&&count(201564)==1&&!observer->Hits[201517][outsider.GetGUID()]&&!observer->Hits[201564][outsider.GetGUID()],"Dark Hunt exceeded its actual two distinct selected players");
        fresh();ObjectList outsideParty{&outsider};Check(static_cast<SmartAI*>(actor.AI())->QueueDarkHunt(outsideParty),"Native nearby nonparty player request rejected");pump(4000);
        Check(observer->Hits[201517][outsider.GetGUID()]==1&&!count(201564),"Native player-range selector acquired an invented party-only gate");
        std::cout<<"DARK_LIFECYCLE three-player cap and legitimate nearby nonparty player PASS\n";
    }

    fresh();queue();firstHit();member.CombatStop(true);member.CastStop();member.GetMapRef().unlink();member.Unit::RemoveFromWorld();member.RemoveFromGrid();sObjectAccessor->RemoveObject(&member);pump(4000);
    Check(count(201517)==1&&!count(201564),"Missing second target received or duplicated a hit");
    sObjectAccessor->AddObject(&member);map.AddToGrid(&member,Cell(Trinity::ComputeCellCoord(member.GetPositionX(),member.GetPositionY())));member.Unit::AddToWorld();member.GetMapRef().link(&map,&member);
    std::cout<<"DARK_LIFECYCLE actual second-player unpublication/republication PASS\n";

    fresh();queue();firstHit();member.Unit::setDeathState(JUST_DIED);pump(4000);Check(!count(201564),"Dead second player received a child hit");member.Unit::setDeathState(ALIVE);member.SetHealth(member.GetMaxHealth());
    std::cout<<"DARK_LIFECYCLE native Unit death-state checkpoint rejects second target PASS;not a client death/corpse claim\n";

    for(unsigned cancellation=0;cancellation<3;++cancellation)
    {
        fresh();queue();firstHit();
        if(cancellation==0){actor.AI()->EnterEvadeMode();owner.CombatStop(true);member.CombatStop(true);}
        else if(cancellation==1)actor.AI()->Reset();
        else actor.AI()->OnCharmed(true);
        pump(2000);Check(!count(201564),"Canceled attempt retained its second native hit");
        std::cout<<"DARK_LIFECYCLE cancel="<<cancellation<<" between real hits PASS\n";
    }
    fresh();queue();actor.AI()->UpdateAI(1);actor.InterruptNonMeleeSpells(false,201517);pump(4000);Check(!count(201517)&&!count(201564),"Canceled pending first cast synthesized a hit/child");
    std::cout<<"DARK_LIFECYCLE native pending-cast interruption PASS;instant ability Counterspell interruptibility is separate\n";

    fresh();bool blocked=false;for(unsigned i=0;i<64;++i){owner.UpdatePosition(origin.GetPositionX()+20.f*std::cos(float(i)*float(M_PI)/32.f),origin.GetPositionY()+20.f*std::sin(float(i)*float(M_PI)/32.f),origin.GetPositionZ(),0,true);if(actor.IsInRange(&owner,8,40)&&!actor.IsWithinLOSInMap(&owner)){blocked=true;break;}}
    Require(blocked,"Native geometry has no blocked-cast fixture");member.UpdatePosition(owner.GetPositionX(),owner.GetPositionY(),owner.GetPositionZ(),0,true);ObjectList blockedTarget{&owner};Check(static_cast<SmartAI*>(actor.AI())->QueueDarkHunt(blockedTarget),"Blocked native request not admitted for normal cast validation");pump(9000);Check(!count(201517)&&!count(201564),"Dark Hunt bypassed a real line-of-sight rejection");
    std::cout<<"DARK_LIFECYCLE native blocked normal casts and finite timeout PASS\n";
    fresh();queue();pump(4000);Check(count(201517)==1&&count(201564)==1,"Native retry/re-entry failed after canceled and missing-target attempts");
    reset();std::cout<<"DARK_HUNT actual ranged timer,pair,normal spell checks,duplicate/missing/dead/canceled attempts and native home/re-entry PASS\n";
}

static void LoadCombatSpellConditions()
{
    auto known=[](uint32 id){return id&&sSpellStore.LookupEntry(id)&&sSpellMgr->GetSpellInfo(id);};
    std::set<uint32> ids;
    auto seeds=WorldDatabase.Query("SELECT id FROM combat_spell_seeds");Require(bool(seeds),"Source spell seed scope missing");
    do{if(known(seeds->Fetch()[0].GetUInt32()))ids.insert(seeds->Fetch()[0].GetUInt32());}while(seeds->NextRow());
    auto direct=WorldDatabase.Query("SELECT DISTINCT action_param1 FROM smart_scripts WHERE action_type=11");
    if(direct)do{ids.insert(direct->Fetch()[0].GetUInt32());}while(direct->NextRow());
    std::vector<std::pair<uint32,uint32>> links;
    for(auto query:{"SELECT ABS(spell_trigger),ABS(spell_effect) FROM spell_linked_spell","SELECT ABS(spell_id),ABS(spell_trigger) FROM spell_dummy_trigger","SELECT t.SpellId,a.spellId FROM areatrigger_template t JOIN areatrigger_actions a ON a.entry=t.entry"})
        if(auto rows=WorldDatabase.Query(query))do{links.emplace_back(rows->Fetch()[0].GetUInt32(),rows->Fetch()[1].GetUInt32());}while(rows->NextRow());
    bool changed=true;
    while(changed)
    {
        auto before=ids.size();auto current=ids;
        for(auto id:current)if(auto spell=sSpellMgr->GetSpellInfo(id))for(unsigned i=0;i<MAX_SPELL_EFFECTS;++i)if(auto effect=spell->GetEffect(SpellEffIndex(i),8))
        {
            if(known(effect->TriggerSpell))ids.insert(effect->TriggerSpell);
            if(effect->Effect==SPELL_EFFECT_DUMMY&&effect->BasePoints>10000&&known(uint32(effect->BasePoints)))ids.insert(uint32(effect->BasePoints));
        }
        for(auto link:links)if(ids.count(link.first)&&known(link.second))ids.insert(link.second);
        Require(ids.size()<10000,"Review unexpectedly broad spell closure");changed=ids.size()!=before;
    }
    std::string list;for(auto id:ids){if(!list.empty())list+=',';list+=std::to_string(id);}
    WorldDatabase.DirectExecute(("INSERT INTO conditions SELECT * FROM combat_spell_conditions WHERE SourceEntry IN("+list+")").c_str());
    std::cout<<"SPELL_CONDITION_CLOSURE ids="<<ids.size()<<'\n';
}

static void ApplyCombatMigrations()
{
    auto directory=std::getenv("LEGION_COMBAT_MIGRATIONS_DIR");Require(directory,"Explicit combat migration folder missing");
    for(auto table:{"spell_dummy_trigger","spell_linked_spell","smart_scripts","conditions"})
    {
        auto proof=WorldDatabase.Query(("SHOW CREATE TABLE "+std::string(table)).c_str());Require(proof&&proof->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Combat repair requires explicit temporary targets");
    }
    for(unsigned replay=0;replay<2;++replay)for(auto file:{"2026_11_19_02_hov_crackle.sql","2026_11_19_03_karazhan_volatile_charge.sql","2026_11_19_04_brh_sic_bats.sql"})
    {
        std::ifstream stream(std::string(directory)+'/'+file);Require(bool(stream),"Combat migration input unavailable");std::string line;
        while(std::getline(stream,line))if(!line.empty()&&line.rfind("--",0)!=0)WorldDatabase.DirectExecute(line.c_str());
    }
}

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_challenge_scripts();AddSC_generic_spell_scripts();sScriptMgr->SwapScriptContext(true);
        DisableMgr::LoadDisables();
        sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadEquipmentTemplates();sObjectMgr->LoadCreatureTemplateAddons();sObjectMgr->LoadScenarioData();sObjectMgr->LoadTempSummons();
        sAchievementMgr->LoadCriteriaList();sAchievementMgr->LoadAchievementCriteriaData();
        sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellPrcoCheck();sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellThreats();
        sCreatureTextMgr->LoadCreatureTexts();sScriptDataStore->LoadSpellScriptNames();
        Require(sScriptMgr->GetSpellScriptLoader(sObjectMgr->GetScriptId("spell_challengers_might")),"Actual Might aura script missing from combat bootstrap");
        sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();sAreaTriggerDataStore->LoadAreaTriggerScripts();
        HashMapHolder<AreaTrigger>::SetSize(200000);HashMapHolder<DynamicObject>::SetSize(200000);
        (sQuestDataStore->*Access(QuestVectorTag{})).resize(1,nullptr);sQuestDataStore->*Access(QuestMaxTag{})=0;
        auto modelData=std::getenv("LEGION_MAW_MODEL_DATA");Require(modelData,"Explicit ship models missing");
        for(uint32 display:{31852u,31853u})Require(MMAP::MMapFactory::createOrGetMMapManager()->loadGameObject(display,std::string(modelData)+'/'),"Actual ship navigation missing");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");ExecuteFixture();
        LoadCombatSpellConditions();ApplyCombatMigrations();WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellLinked();sSpellMgr->LoadSpellTriggered();
        sQuestDataStore->LoadQuests();
        sWaypointMgr->Load();sObjectMgr->LoadCreatures();sObjectMgr->LoadCreatureAddons();sFormationMgr->LoadCreatureFormations();sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");CharacterDatabase.Execute("SET SESSION TRANSACTION READ WRITE");
        for(auto table:{"creature_respawn","gameobject_respawn"})
        {
            auto seed=std::string("CREATE TEMPORARY TABLE combat_seed_")+table+" LIKE "+table;
            auto shadow=std::string("CREATE TEMPORARY TABLE ")+table+" LIKE combat_seed_"+table;
            CharacterDatabase.DirectExecute(seed.c_str());CharacterDatabase.Execute(seed.c_str());CharacterDatabase.DirectExecute(shadow.c_str());CharacterDatabase.Execute(shadow.c_str());
        }
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        for(auto row:sSpellStore)if(std::string(row->Name->Str[0])=="Crackle"||std::string(row->Name->Str[0])=="Sic Bats!")std::cout<<"NAMED id="<<row->ID<<" name="<<row->Name->Str[0]<<" desc="<<row->Description->Str[0]<<'\n';
        for(uint32 id:{199805u,199807u,199808u,199818u,198745u,191639u,191641u,191642u,193069u,2139u,228320u,228328u,228331u,228333u,238605u,203163u,227913u,201517u,201564u})if(auto spell=sSpellMgr->GetSpellInfo(id))
        {
            std::cout<<"META id="<<id<<" name="<<sSpellStore.LookupEntry(id)->Name->Str[0]<<" desc="<<sSpellStore.LookupEntry(id)->Description->Str[0]<<" minRange="<<spell->GetMinRange(false)<<" maxRange="<<spell->GetMaxRange(false)<<'\n';
            for(auto mode:{DIFFICULTY_NORMAL,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE})for(unsigned i=0;i<MAX_SPELL_EFFECTS;++i)if(auto effect=spell->GetEffect(SpellEffIndex(i),mode))if(effect->Effect)std::cout<<"EFFECT id="<<id<<" mode="<<unsigned(mode)<<" index="<<i<<" type="<<effect->Effect<<" aura="<<effect->ApplyAuraName<<" bp="<<effect->BasePoints<<" trigger="<<effect->TriggerSpell<<" target="<<effect->TargetA.GetTarget()<<" targetB="<<effect->TargetB.GetTarget()<<'\n';
        }
        if(std::getenv("LEGION_COMBAT_METADATA_ONLY"))
        {
            CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
        }
        auto all=WorldDatabase.Query("SELECT guid FROM creature ORDER BY guid");Require(bool(all),"Combat creature closure empty");
        do{auto guid=all->Fetch()[0].GetUInt64();auto data=sObjectMgr->GetCreatureData(guid);Require(data,"Combat native cache missing");sObjectMgr->RemoveCreatureFromGrid(guid,data);}while(all->NextRow());
#ifdef DUNGEON_SIC_BASELINE
        auto rows=WorldDatabase.Query("SELECT MIN(guid),id,map FROM creature WHERE guid BETWEEN 900902000 AND 900902214 AND id=102788 GROUP BY id,map ORDER BY map,id");
#elif defined(DUNGEON_DARK_HUNT_ONLY)
        auto rows=WorldDatabase.Query("SELECT MIN(guid),id,map FROM creature WHERE guid BETWEEN 900902000 AND 900902214 AND id=99365 GROUP BY id,map ORDER BY map,id");
#else
        auto rows=WorldDatabase.Query("SELECT MIN(guid),id,map FROM creature WHERE guid BETWEEN 900902000 AND 900902214 GROUP BY id,map ORDER BY map,id");
#endif
        Require(bool(rows),"Actual extra AI representatives absent");
        unsigned contexts=0,interrupts=0,repulls=0,damageContexts=0;
        do
        {
            auto f=rows->Fetch();uint64 guid=f[0].GetUInt64();uint32 entry=f[1].GetUInt32(),mapId=f[2].GetUInt32();auto data=sObjectMgr->GetCreatureData(guid);
#ifdef DUNGEON_DARK_HUNT_ONLY
            for(auto difficulty:{DIFFICULTY_NORMAL,DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE})for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})
#else
            for(auto difficulty:{DIFFICULTY_MYTHIC_KEYSTONE})for(uint8 race:{uint8(RACE_HUMAN)})
#endif
            {
            MapInstanced parent(mapId,300);InstanceMap map(mapId,300,9936000+contexts,difficulty,&parent);map.CreateInstanceData(nullptr);
            std::unique_ptr<DungeonShip> ship;if(mapId==1492)ship=std::make_unique<DungeonShip>(map);
            for(float x:{-96.f,0.f,96.f})for(float y:{-96.f,0.f,96.f})map.LoadGrid(data->posX+x,data->posY+y);
            Position pose{data->posX,data->posY,data->posZ,data->orientation};LifecyclePlayer owner(map,971,pose),member(map,972,pose);
            for(Player* player:{static_cast<Player*>(&owner),static_cast<Player*>(&member)}){player->SetMaxHealth(1000000000);player->SetHealth(1000000000);}
            for(Player* player:{static_cast<Player*>(&owner),static_cast<Player*>(&member)}){player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->setFactionForRace(race);auto info=sChrRacesStore.LookupEntry(race);player->SetDisplayId(info->MaleDisplayID);player->SetNativeDisplayId(info->MaleDisplayID);}
            MapChallengeModeEntry const* route=nullptr;for(auto row:sMapChallengeModeStore)if(row->MapID==mapId){route=row;break;}Require(route,"Combat native route missing");
            auto key=owner.Key(*route);owner.m_challengeKeyInfo.Affix=Affixes::Teeming;LifecycleGroup party(owner,&member,*route,*key);
            party.group.m_affixes={uint32(Affixes::Teeming),0,0};
            auto scenario=sScenarioMgr->AddScenario(&map,1175);Require(scenario,"Combat native scenario unavailable");if(difficulty==DIFFICULTY_MYTHIC_KEYSTONE)scenario->CreateChallenge(&owner);
            {
                NativeAIActor actor;
#ifdef DUNGEON_DARK_HUNT_ONLY
                if(difficulty!=DIFFICULTY_MYTHIC_KEYSTONE){Require(actor.Create(190650,&map,1,99365,0,ALLIANCE,pose.GetPositionX(),pose.GetPositionY(),pose.GetPositionZ(),pose.GetOrientation()),"Native non-Teeming Stalker template create failed");actor.SetHomePosition(pose);Require(map.AddToMap(static_cast<Creature*>(&actor)),"Native non-Teeming Stalker publication failed");}
                else
#endif
                    Require(actor.LoadCreatureFromDB(guid,&map,true),"Real combat actor admission failed");
#ifdef DUNGEON_DARK_HUNT_ONLY
                std::cout<<"DARK_PROFILE mode="<<unsigned(difficulty)<<" race="<<unsigned(race)<<" actualDbSpawn="<<(difficulty==DIFFICULTY_MYTHIC_KEYSTONE)<<" controlledPoseFrom="<<guid<<'\n';
                DarkHuntReceipts(actor,map,owner,member);
#else
                std::vector<std::unique_ptr<NativeAIActor>> bats;
                if(entry==102788)for(unsigned i=0;i<3;++i)
                {
                    auto bat=std::make_unique<NativeAIActor>();Require(bat->Create(190500+i,&map,1,102781,0,ALLIANCE,pose.GetPositionX(),pose.GetPositionY(),pose.GetPositionZ(),pose.GetOrientation()),"Native bat create failed");
                    bat->SetHomePosition(*bat);Require(map.AddToMap(static_cast<Creature*>(bat.get())),"Native bat publication failed");
#ifdef DUNGEON_SIC_BASELINE
                    Require(bat->AIM_Initialize(new SicBatBaseline(bat.get())),"Pinned original bat AI installation failed");
#endif
                    bat->AI()->AttackStart(&owner);bat->AddThreat(&member,1.f);bats.push_back(std::move(bat));
                }
                actor.SetHealth(actor.GetMaxHealth()/2);
                HazardReceipts(entry,actor,map,owner,member);
                TC_LOG_INFO("spells","COMBAT_CASE BEGIN entry=%u guid=%u map=%u",entry,actor.GetGUIDLow(),mapId);
                std::cout<<"CASE entry="<<entry<<" map="<<mapId<<" guid="<<actor.GetGUIDLow()<<" ai="<<typeid(*actor.AI()).name()<<'\n';
                Require(actor.canStartAttack(&owner,true),"Native controlled combat seed inaccessible");actor.AI()->AttackStart(&owner);actor.AddThreat(&member,1.f);member.SetInCombatWith(&actor);
                if(entry==102253||entry==98792)Check(DamageThresholdReceipt(entry,actor,owner,map,member),"Actual first DamageTaken threshold did not cast its native ability");
                bool attemptedInterrupt=false;uint64 health=owner.GetHealth()+member.GetHealth();
#ifdef DUNGEON_SIC_BASELINE
                constexpr unsigned duration=0;
#else
                constexpr unsigned duration=90000;
#endif
                for(unsigned elapsed=0;elapsed<duration;elapsed+=100)
                {
                    bool hadOwnFrenzy=actor.HasAura(201061,actor.GetGUID());
                    Tick(map,owner,member,100);
                    if(NativeFrenzyDeath(actor,hadOwnFrenzy))break;
                    if(!attemptedInterrupt)if(auto cast=actor.GetCurrentSpell(CURRENT_GENERIC_SPELL))if(cast->GetCastTime()>0)
                    {
                        auto id=cast->GetSpellInfo()->Id;auto result=owner.CastSpell(&actor,2139,TriggerCastFlags(TRIGGERED_FULL_MASK&~TRIGGERED_DONT_REPORT_CAST_ERROR));
                        TC_LOG_INFO("spells","COMBAT_INTERRUPT entry=%u spell=%u result=%u stillCasting=%u",entry,id,unsigned(result),bool(actor.GetCurrentSpell(CURRENT_GENERIC_SPELL)));
                        attemptedInterrupt=true;++interrupts;
                    }
                    if(elapsed==30000)actor.SetHealth(actor.GetMaxHealth()/4);
                    Check(actor.IsInWorld()&&actor.IsAlive(),"Combat actor unexpectedly died/unpublished without incoming damage");
                }
                if(owner.GetHealth()+member.GetHealth()<health)++damageContexts;
                TC_LOG_INFO("spells","COMBAT_CASE FIGHT_END entry=%u ownerAlive=%u memberAlive=%u",entry,owner.IsAlive(),member.IsAlive());
                Check(owner.IsAlive()&&member.IsAlive(),"Controlled high-health combat party died;rotation coverage incomplete");
                if(!bats.empty())
                {
                    actor.CastStop();Require(actor.CastSpell(&owner,203163,true)==SPELL_CAST_OK,"Native first Sic Bats cast failed");
                    for(unsigned t=0;t<500;t+=100)Tick(map,owner,member,100,&actor);
#ifdef DUNGEON_SIC_BASELINE
                    for(auto const& bat:bats)Check(bat->getVictim()==&owner,"Pinned original first Sic Bats target changed");
#else
                    for(auto const& bat:bats)Check(bat->AI()->GetGUID(203163)==owner.GetGUID()&&bat->getVictim()==&owner,"Actual first Sic Bats target receipt/attack lost");
#endif
                    Require(actor.CastSpell(&member,203163,true)==SPELL_CAST_OK,"Native repeated Sic Bats cast failed");
                    for(unsigned t=0;t<500;t+=100)Tick(map,owner,member,100,&actor);
#ifdef DUNGEON_SIC_BASELINE
                    for(auto const& bat:bats)Check(bat->getVictim()==&owner,"Pinned original repeated Sic Bats no longer retains stale player");
                    std::cout<<"COUNTERFACTUAL pinned bbeed94c5 bat body ignores second actual Sic Bats command;three bats retain the earlier marked player;not successful gameplay\n";
#else
                    for(auto const& bat:bats)Check(bat->AI()->GetGUID(203163)==member.GetGUID()&&bat->getVictim()==&member,"Repeated Sic Bats retained an earlier marked player");
                    member.RemoveAurasDueToSpell(203163,actor.GetGUID());
                    for(unsigned t=0;t<1500;t+=100)Tick(map,owner,member,100,&actor);
                    for(auto const& bat:bats)Check(bat->AI()->GetGUID(203163).IsEmpty()&&bat->GetReactState()==REACT_AGGRESSIVE,"Expired latest command recovered a stale older mark");
                    for(auto const& bat:bats){bat->AI()->EnterEvadeMode();Check(bat->AI()->GetGUID(203163).IsEmpty(),"Bat evade retained prior command target");}
#endif
                    bats.clear();
#ifndef DUNGEON_SIC_BASELINE
                    std::cout<<"SIC_BATS native three-bat first/repeated cast,actual mark expiry and evade receipts PASS\n";
#endif
                }
                if(!actor.IsAlive()){actor.Respawn(true,0);actor.Respawn(false,0);Check(actor.IsAlive()&&actor.IsInWorld(),"Native Frenzy corpse/respawn failed before the independent evade check");}
                // Real AI evade and native HomeMovementGenerator acknowledgements.
                actor.AI()->EnterEvadeMode();owner.CombatStop(true);member.CombatStop(true);
                for(unsigned elapsed=0;elapsed<30000&&(actor.IsInEvadeMode()||actor.getVictim()||actor.isInCombat());elapsed+=100)Tick(map,owner,member,100);
                std::cout<<"EVADE entry="<<entry<<" state="<<actor.GetUnitState()<<" combat="<<actor.isInCombat()<<" home="<<actor.GetExactDist(actor.GetHomePosition())<<'\n';
                Check(!actor.getVictim()&&!actor.isInCombat()&&!actor.IsInEvadeMode(),"Actual AI/native home return failed to finish evade");
                ClearEphemeral(map,owner);owner.RemoveAllAuras();member.RemoveAllAuras();
                actor.AI()->AttackStart(&owner);Check(actor.getVictim()==&owner&&actor.isInCombat(),"Actual AI failed a second pull after native evade");++repulls;
                if(entry==102253||entry==98792)Check(DamageThresholdReceipt(entry,actor,owner,map,member),"Native DamageTaken threshold did not rearm after evade/re-pull");
                for(unsigned elapsed=0;elapsed<15000;elapsed+=100){bool hadOwnFrenzy=actor.HasAura(201061,actor.GetGUID());Tick(map,owner,member,100);if(NativeFrenzyDeath(actor,hadOwnFrenzy))break;}
                actor.CombatStop(true);actor.CastStop();actor.RemoveAllAuras();actor.m_Events.KillAllEvents(true);
                ClearEphemeral(map,owner);owner.CombatStop(true);member.CombatStop(true);owner.CastStop();member.CastStop();owner.RemoveAllAuras();member.RemoveAllAuras();
                TC_LOG_INFO("spells","COMBAT_CASE END entry=%u",entry);
#endif
            }
            Check(map.CreatureGroupHolder.empty(),"Native combat teardown retained formation pointers");
            ClearEphemeral(map,owner);map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());++contexts;
            }
        }while(rows->NextRow());
#ifdef DUNGEON_SIC_BASELINE
        Check(contexts==1&&repulls==1,"Pinned bat regression scope changed");
        std::cout<<"COUNTERFACTUAL "<<checked<<" native pinned-original bat checks;repeat targeting is expected broken,not a passing ordinary run\n";
#elif defined(DUNGEON_DARK_HUNT_ONLY)
        Check(contexts==8,"Focused actual Stalker scope changed");std::cout<<"PASS "<<checked<<" native Dark Hunt actual timer/pair/lifecycle checks across4 difficulties and Human/Orc;6 template Create contexts plus2 real Teeming DB actors,controlled native pose,not client traversal\n";
#else
        Check(contexts==103&&repulls==103,"Actual timed combat template/repull coverage changed");
        std::cout<<"PASS "<<checked<<" native timed combat checks;contexts="<<contexts<<" interrupt attempts="<<interrupts<<" repulls="<<repulls<<" damage contexts="<<damageContexts<<";up to90s rotation/native Frenzy self-death,actual corpse/respawn+evade+up to15s repull,stationary synthetic party/health checkpoints,not whole dungeon/client gameplay\n";
#endif
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
