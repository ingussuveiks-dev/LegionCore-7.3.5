// Actual Challenge Quaking admission, native aura/trigger/area damage paths.
#define CHALLENGE_DEATH_SUPPORT_ONLY
#include "LegionChallengeDeathAffixesNative.cpp"

static void InitializeQuaking()
{
    sObjectMgr->LoadItemTemplates(); sObjectMgr->LoadItemTemplateAddon(); sObjectMgr->LoadScenarioData();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    for (auto table : {"spell_proc_event", "spell_proc_check", "spell_bonus_data"})
    {
        std::string name = table;
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE quake_" + name + " AS SELECT * FROM " + name + " WHERE entry IN(240447,240448,133)").c_str());
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE quake_" + name).c_str());
        WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM quake_" + name).c_str());
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellProcEvents(); sSpellMgr->LoadSpellPrcoCheck(); sSpellMgr->LoadSpellBonusess();
    auto scripts = WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(240447,240448,133)");
    Require(!scripts, "Load changed Quaking native script bindings before this run");
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17,24) AND SourceEntry IN(240447,240448,133)");
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "Load changed Quaking conditions before this run");
}

static void Metadata()
{
    for (uint32 id : {240447u,240448u,133u})
    {
        auto info=sSpellMgr->GetSpellInfo(id);auto row=sSpellStore.LookupEntry(id);
        if(!info||!row){std::cout<<"META absent="<<id<<'\n';continue;}
        std::cout<<"META spell="<<id<<" name="<<row->Name->Str[LOCALE_enUS]<<" description="<<row->Description->Str[LOCALE_enUS]
            <<" duration="<<info->GetDuration(8)<<" cast="<<info->CalcCastTime()<<" interrupt="<<info->AuraInterruptFlags[0]<<'\n';
        for(unsigned i=0;i<MAX_SPELL_EFFECTS;++i){auto e=info->GetEffect(static_cast<SpellEffIndex>(i),8);if(e&&e->IsEffect())
            std::cout<<"META effect="<<i<<" type="<<e->Effect<<" aura="<<e->ApplyAuraName<<" base="<<e->BasePoints<<" period="<<e->ApplyAuraPeriod
                <<" trigger="<<e->TriggerSpell<<" targetA="<<e->TargetA.GetTarget()<<" targetB="<<e->TargetB.GetTarget()<<" radius="<<e->CalcRadius()<<'\n';}
    }
    if(auto links=WorldDatabase.Query("SELECT spell_trigger,spell_effect,type FROM spell_linked_spell WHERE ABS(spell_trigger) IN(240447,240448,240449) OR ABS(spell_effect) IN(240447,240448,240449)"))
        do{auto f=links->Fetch();std::cout<<"META link="<<f[0].GetInt32()<<'|'<<f[1].GetInt32()<<'|'<<f[2].GetUInt32()<<'\n';}while(links->NextRow());
}

struct QuakingContext
{
    MapInstanced parent; InstanceMap map; LifecyclePlayer owner,member; Item* key; LifecycleGroup group; DeathAffixScenario progress;
    static InstanceMap& Ready(InstanceMap& map){map.CreateInstanceData(nullptr);return map;}
    static MapChallengeModeEntry const& Route(){auto row=sMapChallengeModeStore.LookupEntry(208);Require(row,"Native Maw route missing");return *row;}
    LifecyclePlayer& Configure(uint8 race)
    {
        for(auto player:{&owner,&member})
        {
            player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->setFactionForRace(race);
            auto display=sChrRacesStore.LookupEntry(race)->MaleDisplayID;player->SetDisplayId(display);player->SetNativeDisplayId(display);
            player->SetFloatValue(UNIT_FIELD_MOD_CASTING_SPEED,1.f);player->SetFloatValue(UNIT_FIELD_MOD_TIME_RATE,1.f);
            player->SetFloatValue(UNIT_FIELD_BOUNDING_RADIUS,DEFAULT_WORLD_OBJECT_SIZE);player->SetFloatValue(UNIT_FIELD_COMBAT_REACH,DEFAULT_COMBAT_REACH);
        }
        group.group.m_affixes={0,uint32(Affixes::Quaking),0};return owner;
    }
    QuakingContext(uint8 race,unsigned id):parent(1492,300),map(1492,300,9942100+id,DIFFICULTY_MYTHIC_KEYSTONE,&parent),
        owner(Ready(map),791,Position{2934.49f,715.757f,554.286f,1.55255f}),member(map,792,Position{2937.49f,715.757f,554.286f,1.55255f}),
        key(owner.Key(Route())),group(owner,&member,Route(),*key),progress(map,Configure(race))
    {auto challenge=Challenge();challenge->_checkStart=false;challenge->_run=true;challenge->_affixQuakingTimer=20000;}
    ::Challenge* Challenge(){return progress.scenario->GetChallenge();}
    void Pair(uint32 diff){owner.Unit::Update(diff);member.Unit::Update(diff);}
    void Admit()
    {
        auto challenge=Challenge();challenge->Update(19999);
        Check(!owner.HasAura(240447)&&!member.HasAura(240447),"Quaking appeared before native20-second deadline");
        challenge->Update(1);Check(owner.HasAura(240447)&&member.HasAura(240447),"Native active Quaking admission failed");
        Check(owner.GetAura(240447)->GetCasterGUID()==owner.GetGUID()&&member.GetAura(240447)->GetCasterGUID()==member.GetGUID(),"Quaking source ownership differs");
        Check(challenge->_affixQuakingTimer==20000,"Native Quaking timer did not rearm once");
    }
};

static void CheckPulse(uint8 race,bool outside,bool oneSource,bool lag,unsigned id)
{
    auto storage=std::make_unique<QuakingContext>(race,id);auto& c=*storage;
    if(outside)c.map.PlayerRelocation(&c.member,2949.49f,715.757f,554.286f,1.55255f);
    DungeonPet pet(c.owner,c.map);DungeonGuardian guardian(c.owner,c.map);
    c.Admit();if(oneSource)c.member.RemoveAurasDueToSpell(240447);
    Check(!pet.HasAura(240447)&&!guardian.HasAura(240447),"Quaking warning was assigned to a pet or guardian");
    if(lag)c.Pair(10000);
    else
    {
        for(unsigned i=0;i<49;++i)c.Pair(50);c.Pair(49);
        Check(c.owner.GetHealth()==1000000&&c.member.GetHealth()==1000000,"Quaking dealt damage before2500ms");
        c.Pair(1);
    }
    unsigned ownerExpected=outside||oneSource?800000:600000;
    unsigned memberExpected=oneSource?(outside?1000000:800000):(outside?800000:600000);
    Check(c.owner.GetHealth()==ownerExpected&&c.member.GetHealth()==memberExpected,"Native Quake dose/overlap/radius/lag result differs");
    c.Pair(1);Check(!c.owner.HasAura(240447)&&!c.member.HasAura(240447),"Quaking warning did not expire after its one native tick");
    c.Pair(10000);Check(c.owner.GetHealth()==ownerExpected&&c.member.GetHealth()==memberExpected,"Native Quaking repeated damage after expiry");
    std::cout<<"QUAKING race="<<unsigned(race)<<" outside="<<outside<<" single="<<oneSource<<" lag="<<lag
        <<" health="<<c.owner.GetHealth()<<'/'<<c.member.GetHealth()<<" pet/guardian="<<pet.GetHealth()<<'/'<<guardian.GetHealth()<<'\n';
}

static void CheckInterrupt(uint8 race,bool outside,unsigned id)
{
    auto storage=std::make_unique<QuakingContext>(race,id);auto& c=*storage;DeathAffixActor enemy(c.map,97043,176901);
    if(outside)c.map.PlayerRelocation(&c.member,2949.49f,715.757f,554.286f,1.55255f);
    c.Admit();c.member.RemoveAurasDueToSpell(240447);for(unsigned i=0;i<40;++i)c.Pair(50);
    auto info=sSpellMgr->GetSpellInfo(133);Require(info&&info->CalcCastTime()>500&&!(info->InterruptFlags&SPELL_INTERRUPT_FLAG_ABORT_ON_DMG),"Native interrupt fixture is not a long non-abort-on-damage cast");
    auto flags=TriggerCastFlags(TRIGGERED_IGNORE_GCD|TRIGGERED_IGNORE_POWER_AND_REAGENT_COST);
    for(auto player:{&c.owner,&c.member})
    {
        // Set the fixture's initial pose; SetFacingToObject would launch a
        // real movement spline and legitimately reject a moving Fireball.
        player->SetOrientation(player->GetAngle(&enemy));
        Require(!player->isMoving()&&player->HasInArc(float(M_PI),&enemy),"Native stationary/facing cast preconditions absent");
        Require(info->CalcCastTime(player)>500,"Actual fixture cast-speed/time-rate fields are not native");
        auto result=player->CastSpell(&enemy,133,flags);
        if(!player->HasUnitState(UNIT_STATE_CASTING))std::cout<<"INSPECT nativeCastResult="<<unsigned(result)<<" castTime="<<info->CalcCastTime(player)<<'\n';
        Check(player->HasUnitState(UNIT_STATE_CASTING),"Actual native non-instant interrupt fixture did not start");
    }
    for(unsigned i=0;i<10;++i)c.Pair(50);
    Check(!c.owner.HasUnitState(UNIT_STATE_CASTING)&&c.owner.HasSpellCooldown(133),"Own native Quake did not interrupt/lock its current spell");
    Check(c.member.HasUnitState(UNIT_STATE_CASTING)==outside&&c.member.HasSpellCooldown(133)!=outside,"Nearby/outside native Quake interrupt admission differs");
    Check(c.owner.GetSpellCooldownDelay(133)>4&&c.owner.GetSpellCooldownDelay(133)<=5,"Native5-second Quake school lock differs");
    c.owner.CastStop();c.member.CastStop();
    std::cout<<"QUAKING race="<<unsigned(race)<<" outside="<<outside<<" actualCastInterrupt/schoolLock=pass\n";
}

static void CheckCancellation(uint8 race,unsigned mode,unsigned id)
{
    auto storage=std::make_unique<QuakingContext>(race,id);auto& c=*storage;c.Admit();c.member.RemoveAurasDueToSpell(240447);c.Pair(2000);
    if(mode==0)c.owner.RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP);
    else if(mode==1)c.owner.RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED);
    else {c.member.Kill(&c.owner);Check(!c.owner.IsAlive(),"Native Quaking player death did not occur");}
    Check(!c.owner.HasAura(240447),"Native death/map/teleport cleanup retained the warning");
    c.Pair(5000);Check(c.member.GetHealth()==1000000,"Canceled Quaking warning delivered delayed group damage");
    if(mode==2){c.Challenge()->Update(20000);Check(!c.owner.HasAura(240447)&&c.member.HasAura(240447),"Dead-player Quaking admission or surviving player rearm differs");}
    else Check(c.owner.GetHealth()==1000000,"Canceled own Quake delivered delayed self damage");
    std::cout<<"QUAKING race="<<unsigned(race)<<" cancellation="<<mode<<" delayedDamage=0\n";
}

static void CheckAdmission(uint8 race,unsigned id)
{
    auto storage=std::make_unique<QuakingContext>(race,id);auto& c=*storage;
    c.owner.SetFlag(UNIT_FIELD_FLAGS_2,UNIT_FLAG2_NO_ACTIONS);c.Challenge()->Update(20000);
    Check(!c.owner.HasAura(240447)&&c.member.HasAura(240447),"Native no-actions gate differs");
    c.owner.RemoveFlag(UNIT_FIELD_FLAGS_2,UNIT_FLAG2_NO_ACTIONS);c.Challenge()->Update(20000);
    Check(c.owner.HasAura(240447)&&c.member.GetAuraCount(240447)==1,"Native re-admission or duplicate warning guard differs");
    for(auto player:{&c.owner,&c.member})player->RemoveAurasDueToSpell(240447);
    c.Challenge()->_run=false;c.Challenge()->Update(20000);Check(!c.owner.HasAura(240447),"Stopped run admitted Quaking");
    c.Challenge()->_run=true;c.Challenge()->_complete=true;c.Challenge()->Update(20000);Check(!c.owner.HasAura(240447),"Completed run admitted a new Quaking warning");
    std::cout<<"QUAKING race="<<unsigned(race)<<" controller=noActions/duplicate/stopped/complete=guarded\n";
}

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);InitializeQuaking();Metadata();
        unsigned contexts=0;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})
        {
            for(bool outside:{false,true})for(bool single:{false,true})for(bool lag:{false,true})CheckPulse(race,outside,single,lag,contexts++);
            for(bool outside:{false,true})CheckInterrupt(race,outside,contexts++);
            for(unsigned mode=0;mode<3;++mode)CheckCancellation(race,mode,contexts++);
            CheckAdmission(race,contexts++);
        }
        Check(contexts==28,"Native Quaking context scope changed");
        std::cout<<"PASS "<<checked<<" native Quaking checks:28 contexts,race/radius/overlap/lag,real spell interruption/school lock,native warning cancellation/death and controller admission. Controlled publication/health/run latch;actual client traversal remains separate\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}
