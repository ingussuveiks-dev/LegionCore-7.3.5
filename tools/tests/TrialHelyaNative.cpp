// Continue the actual Odyn/Guarm/26-trash progression through Helya's phases.
#define TRIAL_GARM_SUPPORT_ONLY
#include "TrialGarmNative.cpp"
#include "AreaTrigger.h"
static std::vector<AreaTrigger*> HelyaAreas(InstanceMap& map,bool combatOnly=false)
{
    std::vector<AreaTrigger*> result;
    for(uint64 guid=1;guid<sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->GetNextAfterMaxUsed();++guid)
        if(auto at=HashMapHolder<AreaTrigger>::FindLow(guid))if(at->IsInWorld()&&at->GetMap()==&map&&(!combatOnly||(at->GetSpellId()!=228104&&at->GetSpellId()!=234199)))result.push_back(at);
    return result;
}
static void PumpHelya(InstanceMap& map,uint32 duration)
{
    for(uint32 elapsed=0;elapsed<duration;elapsed+=10)
    {for(auto const& ref:map.GetPlayers()){auto p=ref.getSource();if(p->IsAlive())p->SetHealth(p->GetMaxHealth());}if(auto pet=HashMapHolder<Pet>::FindLow(200))if(pet->GetMap()==&map&&pet->IsAlive())pet->SetHealth(pet->GetMaxHealth());for(auto c:Actors(map))c->Update(10);for(auto at:HelyaAreas(map))at->Update(10);map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();}
}
static void LongPhase(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    std::map<uint32,std::set<ObjectGuid>> seen;std::set<ObjectGuid> waves;unsigned minions55=0;
    for(unsigned sec=0;sec<180;++sec)
    {
        PumpHelya(map,1000);
        for(auto entry:{115941u,114709u,114809u,114568u,116335u})for(auto c:Actors(map,entry))seen[entry].insert(c->GetGUID());
        for(auto at:HelyaAreas(map))if(at->GetCasterGUID()==helya.GetGUID()&&at->GetSpellId()==228027){waves.insert(at->GetGUID());}
        if(sec==54)minions55=unsigned(Actors(map,114568).size());
        // A raid must kill/interrupt these Mythic casters. Do not let the
        // survivability fixture infinitely empower Helya instead.
        for(auto c:Actors(map,116335))if(c->IsAlive())p.Kill(c,false);
        Require(p.IsAlive()&&helya.AI()->GetData(2)==2,"Long phase fixture lost player or phase two");
    }
    std::cout<<"LONG waves="<<waves.size()<<" boats="<<seen[115941].size()<<" grim="<<seen[114709].size()<<" marine="<<seen[114809].size()<<" minions="<<seen[114568].size()<<" minions55="<<minions55<<" mythicCasters="<<seen[116335].size()<<std::endl;
    Require(waves.size()>=3&&seen[115941].size()>=6&&seen[114709].size()>=3&&seen[114809].size()>=3&&seen[114568].size()>=20,"Actual repeated Fury/boats/add waves missing");
    Require(map.GetDifficultyID()==DIFFICULTY_MYTHIC_RAID?!seen[116335].empty():seen[116335].empty(),"Mythic casters crossed difficulty gate");
#ifdef TRIAL_HELYA_BASELINE
    Require(minions55==10,"Pinned overwritten first skeleton event changed");
#else
    Require(minions55==20,"First Fury cycle lost one ten-minion wave");
#endif
}
static void CheckHelya(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    auto script=map.GetInstanceScript();Require(helya.AI()->GetData(2)==1&&helya.IsAlive(),"Actual Helya phase one missing");
    p.GetOrInitPetStable();LootPet pet(p,map);pet.setFaction(p.getFaction());
    // Keep the fixture's actors alive while fast-forwarding encounter timers;
    // this probe verifies phase/progression logic, not raid damage tuning.
    pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(pet.GetMaxHealth());
    Unit* attacker=p.getRace()==RACE_ORC?static_cast<Unit*>(&pet):static_cast<Unit*>(&p);
    auto raw=uint32(std::min<double>(UINT32_MAX-4096,std::ceil(1.5*helya.GetHealth(attacker)*helya.GetHealthMultiplierForTarget(attacker))));
    Require(raw/helya.GetHealthMultiplierForTarget(attacker)>helya.GetHealth(attacker),"Fixture hit is not a lethal overkill");
    std::cout<<"HEALTH base="<<helya.GetHealth()<<" relative="<<helya.GetHealth(attacker)<<" multiplier="<<helya.GetHealthMultiplierForTarget(attacker)<<" raw="<<raw<<std::endl;
    attacker->DealDamage(&helya,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
    std::cout<<"AFTER phase="<<helya.AI()->GetData(2)<<" alive="<<helya.IsAlive()<<" health="<<helya.GetHealth()<<std::endl;
    Require(helya.IsAlive()&&helya.AI()->GetData(2)==2&&helya.GetHealth()==helya.CountPctFromMaxHealth(65),"Actual scaled lethal hit bypassed Helya phase two");
    auto tentacles=Actors(map,114900);Require(tentacles.size()==9&&Actors(map,114901).size()==4&&Actors(map,114905).size()==5,"Actual phase two summon group missing tentacles/helpers");
    auto health=helya.GetHealth();attacker->DealDamage(&helya,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(helya.GetHealth()==health,"Native phase two damage guard failed");
    LongPhase(p,map,helya);Require(helya.AI()->GetData(2)==2&&helya.GetHealth()==health,"Native phase-two delayed introduction lost encounter");
    helya.RemoveAura(228847);attacker->DealDamage(&helya,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(helya.GetHealth()==health,"Phase-two fallback allowed damage without immunity aura");
    attacker->Kill(Actors(map,114901).front(),false);Require(helya.GetHealth()==health&&helya.AI()->GetData(2)==2,"Visual helper death wounded Helya");
    ClearCombat(p,helya);PumpHelya(map,15000);Require(script->GetBossState(2)==NOT_STARTED&&helya.GetHealth()==helya.GetMaxHealth()&&Actors(map,114900).empty()&&Actors(map,114901).empty()&&Actors(map,114905).empty()&&Actors(map,114709).empty()&&Actors(map,114809).empty()&&Actors(map,114568).empty()&&HelyaAreas(map,true).empty(),"Phase-two evade left stale summons/health/progress/effects");
    // Wipe after the delayed Fury is queued but before it fires. Ordinary
    // callbacks used to survive this reset and cast the barrage out of combat.
    map.PlayerRelocation(&p,helya.GetPositionX(),helya.GetPositionY(),helya.GetPositionZ(),helya.GetOrientation());helya.SetInCombatWith(&p);attacker->DealDamage(&helya,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);PumpHelya(map,9000);ClearCombat(p,helya);PumpHelya(map,12000);
    bool heroic=map.GetDifficultyID()==DIFFICULTY_HEROIC_RAID||map.GetDifficultyID()==DIFFICULTY_MYTHIC_RAID;
    std::cout<<"EARLY_RESET lateFury="<<helya.HasAura(228300)<<" phase="<<helya.AI()->GetData(2)<<std::endl;
#ifdef TRIAL_HELYA_BASELINE
    // Whether this old timer has queued depends on the remaining alternate
    // power/current cast. Record it; the fresh phase-three case below is the
    // positive reproduction of the missing phase guard.
    Require(helya.AI()->GetData(2)==0,"Pinned native reset did not return to phase zero");
    // Native second reset removes the reproduced stale barrage before retry.
    ClearCombat(p,helya);PumpHelya(map,15000);
#else
    Require(!helya.HasAura(228300)&&!helya.HasAura(228546)&&HelyaAreas(map,true).empty()&&helya.AI()->GetData(2)==0,"Wiped phase-two callback restarted Fury/regen outside combat");
#endif
    Require(!helya.HasUnitState(UNIT_STATE_EVADE),"Native Helya home return did not finish");map.PlayerRelocation(&p,helya.GetPositionX(),helya.GetPositionY(),helya.GetPositionZ(),helya.GetOrientation());helya.SetInCombatWith(&p);Require(helya.AI()->GetData(2)==1,"Helya retry did not restart phase one");
    attacker->DealDamage(&helya,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);tentacles=Actors(map,114900);Require(helya.AI()->GetData(2)==2&&tentacles.size()==9&&Actors(map,114901).size()==4&&Actors(map,114905).size()==5,"Helya retry lost/duplicated native tentacles");health=helya.GetHealth();
    PumpHelya(map,p.getRace()==RACE_HUMAN?9000:20000);
    std::set<ObjectGuid> retainedAdds;for(auto entry:{114709u,114809u})for(auto c:Actors(map,entry))if(c->IsAlive())retainedAdds.insert(c->GetGUID());
    if(p.getRace()==RACE_ORC)Require(retainedAdds.size()==2,"Phase-three retention probe lacks actual phase-two adds");
    unsigned dead=0;
    for(auto tentacle:tentacles)
    {
        Require(attacker->IsAlive()&&tentacle->IsAlive()&&tentacle->IsAIEnabled&&tentacle->ToTempSummon()&&tentacle->ToTempSummon()->GetSummoner()==&helya,"Native tentacle actor/AI/summoner missing");
        attacker->Kill(tentacle,false);++dead;auto expected=health-dead*helya.CountPctFromMaxHealth(2);
        Require(helya.GetHealth()==expected&&helya.AI()->GetData(2)==(dead==9?3:2),"Native tentacle death lost wound or phase transition");
        attacker->Kill(tentacle,false);Require(helya.GetHealth()==expected,"Repeated native tentacle kill duplicated wound");
    }
    Require(std::abs(helya.GetHealthPct()-47.0f)<0.01f,"Nine native tentacles did not leave Helya at47 percent");
    // Let the actual phase-three delayed return run before the final blow.
    PumpHelya(map,7000);std::cout<<"RETURN phase="<<helya.AI()->GetData(2)<<" state="<<script->GetBossState(2)<<" threat="<<helya.getThreatManager().getThreatList().size()<<" playerAlive="<<p.IsAlive()<<" staleLong="<<helya.HasAura(228300)<<std::endl;Require(helya.AI()->GetData(2)==3&&script->GetBossState(2)==IN_PROGRESS&&helya.GetReactState()==REACT_AGGRESSIVE&&!helya.HasAura(228847)&&helya.GetExactDist(524.031f,699.055f,5.17631f)<1,"Native delayed return lost phase-three combat/home position");
#ifdef TRIAL_HELYA_BASELINE
    Require(helya.HasAura(228300)==(heroic&&p.getRace()==RACE_HUMAN),"Pinned late phase-two Fury behavior changed");
#else
    Require(!helya.HasAura(228300),"Phase-two Fury restarted after phase three began");
#endif
    for(auto guid:retainedAdds){auto c=map.GetCreature(guid);Require(c&&c->IsAlive(),"Phase-three transition removed an existing phase-two add");}
    Require(attacker->IsAlive()&&p.IsAlive(),"Fixture attacker died before final Helya hit");helya.ResetPlayerDamageReq();helya.LowerPlayerDamageReq(1);helya.SetLootRecipient(&p);attacker->DealDamage(&helya,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);attacker->Kill(&helya,false);Tick(p,2000);
    Require(helya.isDead(false)&&script->GetBossState(2)==DONE&&p.GetQuestStatus(44868)==QUEST_STATUS_COMPLETE&&p.GetQuestObjectiveData(44868,55482)==1,"Actual Helya victory lost native quest credit");
    Checkpoint(map,group,7);PumpHelya(map,15000);Require(HelyaAreas(map,true).empty()&&Actors(map,114709).empty()&&Actors(map,114809).empty()&&Actors(map,114568).empty()&&Actors(map,115941).empty(),"Victory left combat summons or area effects");auto actors=Actors(map,116760);Require(actors.size()==1&&actors[0]->GetExactDist(498.595f,640.212f,1.8432f)<2,"Native victory Odyn missing/not landed");
    map.PlayerRelocation(&p,actors[0]->GetPositionX(),actors[0]->GetPositionY(),actors[0]->GetPositionZ(),actors[0]->GetOrientation());p.PrepareQuestMenu(actors[0]->GetGUID());Require(p.PlayerTalkClass->GetQuestMenu().HasItem(44868),"Actual victory Odyn turn-in menu missing");
    p.SaveToDB();FlushWrites();WorldPacket wire(CMSG_QUEST_GIVER_CHOOSE_REWARD);wire<<actors[0]->GetGUID()<<int32(44868)<<int32(0);WorldPackets::Quest::QuestGiverChooseReward finish(std::move(wire));finish.Read();p.GetSession()->HandleQuestGiverChooseReward(finish);Tick(p,200);FlushWrites();
    Require(p.GetQuestRewardStatus(44868)&&p.GetItemCount(142534)==1,"Actual Helya progression reward failed");Checkpoint(p,44868,142534,true);p.GetSession()->HandleQuestGiverChooseReward(finish);Tick(p,200);FlushWrites();Require(p.GetItemCount(142534)==1,"Repeated Helya reward packet duplicated item");Checkpoint(p,44868,142534,true);
    std::cout<<"PASS: native scaled hit ->65 percent ->nine real tentacle deaths ->47 percent ->phase three/victory/quest/Odyn/reward; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunGarm(argc,argv,CheckHelya);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
