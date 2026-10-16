// Native owner/reward/credit/quest handlers; world, timing and storage fixtures.
#include "BlightFixture.inc"
#include "NativeRewardPlayer.inc"
struct Source {uint32 entry,credit1,credit2,faction,npc,flags,flags2,display;};
std::map<uint32,Source> sources;
void Victim(Creature& victim,uint32 entry,uint64 serial){auto s=sources.at(entry);victim.entry=entry;victim.guid={serial};victim.data.Entry=entry;victim.data.KillCredit[0]=s.credit1;victim.data.KillCredit[1]=s.credit2;creatures[victim.guid]=&victim;}
void Kill(Player& p,uint32 entry,bool petKill,uint64 serial){
    Creature victim;Victim(victim,entry,serial);Unit pet;pet.ownerPlayer=&p;
    auto resolved=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&victim);assert(resolved.first==&p&&resolved.second);
    auto before=p.packets;KillRewarder reward;reward._victim=&victim;reward._RewardPlayer(&p,false);
    assert(!p.events.empty()&&p.delays.back()==50&&p.packets==before);Deliver(p);
    assert(victim.IsPlayerRewarded(p.GetGUID()));reward._RewardPlayer(&p,false);assert(p.events.empty()); // Native rewarded-player admission, not a count1 cap.
    creatures.erase(victim.guid);
}
int main(int argc,char** argv){
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;auto& q=store.quests[38361];q.id=38361;unsigned objectives=0,spells=0;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,type,level,flagsEx,item,reward,next;in>>id>>type>>level>>q.flags>>flagsEx>>q.AllowableRaces>>item>>reward>>q.PrevQuestID>>next>>q.special;assert(id==38361&&type==2&&level==98&&!item&&!reward&&!next&&q.PrevQuestID==38332);}
        if(kind=="objective"){uint32 id;int index;QuestObjective o;in>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);assert(id==277874&&o.Type==0&&o.ObjectID==91581&&o.Amount==15&&!index&&!o.Flags);q.objectives.push_back(o);++objectives;}
        if(kind=="actor"){Source s;std::string ai,script;in>>s.entry>>s.credit1>>s.credit2>>s.faction>>s.npc>>s.flags>>s.flags2; // Read trailing display from the original tab-delimited line; two empty strings must stay empty.
            auto end=line.find_last_of('\t');s.display=std::stoul(line.substr(end+1));sources.emplace(s.entry,s);}
        if(kind=="spell"){uint32 entry,id,mask;std::string difficulty;in>>entry>>id>>mask>>difficulty;assert(!mask&&difficulty.empty());CreatureSpell spell;assert(spell.CanUseInDifficulty(0));++spells;}
    }
    assert(objectives==1&&spells==10&&sources.size()==4);q.prevQuests={38332};q.special|=QUEST_SPECIAL_FLAGS_KILL;store.quests[38332].id=38332;
    assert(sources.at(91414).credit1==91581&&sources.at(91571).credit1==91581&&!sources.at(91581).credit1);
    assert((sources.at(91414).flags&0x300)==0x300); // Installed feign-death props are not asserted to be killable.
    assert(!(sources.at(91581).flags&0x02000382)&&!(sources.at(91571).flags&0x02000382));
    Creature giver;giver.entry=91158;giver.guid={340422};giver.fields[UNIT_FIELD_NPC_FLAGS]=2;creatures[giver.guid]=&giver;SmartAI giverAI(&giver);
    unsigned hordeRaces=0;for(unsigned bit=0;bit<32;++bit){Player p;p.team=HORDE;p.race=uint64(1)<<bit;p.m_RewardedQuests.insert(38332);bool allowed=bool(q.AllowableRaces&p.race);assert(p.CanTakeQuest(&q,false)==allowed);if(allowed)++hordeRaces;}assert(hordeRaces==9);
    {Player p;p.team=HORDE;p.race=2;assert(!p.CanTakeQuest(&q,false));p.Accept(38332);p.CompleteQuest(38332);assert(!p.CanTakeQuest(&q,false));p.quests.clear();p.m_RewardedQuests.insert(38332);assert(p.CanTakeQuest(&q,false));}
    for(bool petKill:{false,true})for(int variant=0;variant<3;++variant){
        Player p;p.guid={100+unsigned(petKill)*10+unsigned(variant)};p.team=HORDE;p.race=2;p.m_RewardedQuests.insert(38332);assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&giver);
        assert(!p.CanCompleteQuest(38361)&&!p.CanRewardQuest(&q,false)&&!p.packets);p.KilledMonsterCredit(91158);p.KilledMonsterCredit(92224);assert(!p.packets);
        WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{giver.guid,38361};session.HandleQuestGiverRequestReward(request);assert(!p.menu.offers);
        for(unsigned n=1;n<=15;++n){uint32 entry=variant==0?91581:variant==1?91571:(n%2?91581:91571);Kill(p,entry,petKill,10000+n);
            assert(p.GetQuestObjectiveData(38361,91581)==n&&p.packets==n&&p.completions==unsigned(n==15));if(n<15)assert(!p.CanCompleteQuest(38361));}
        assert(p.CanRewardQuest(&q,false));Kill(p,91581,petKill,20000);assert(p.packets==15&&p.completions==1);
        session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);giver.near=false;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);giver.near=true;
        assert(!p.GetNextQuest(giver.guid,&q));p.quests.clear();p.m_RewardedQuests.insert(38361);assert(!p.CanTakeQuest(&q,false));Kill(p,91581,petKill,20001);assert(p.packets==15);
    }
    {Player p;p.Accept(38361);p.guid={300};Creature hypotheticalRider;Victim(hypotheticalRider,91414,30000);p.KilledMonster(&hypotheticalRider.data,hypotheticalRider.guid);assert(p.GetQuestObjectiveData(38361,91581)==1);creatures.erase(hypotheticalRider.guid);} // Template substitution only; protected live91414 is preserved.
    {Player p;p.Accept(38361);for(unsigned n=0;n<7;++n)Kill(p,91581,true,40000+n);p.quests.clear();p.Accept(38361);assert(!p.GetQuestObjectiveData(38361,91581));for(unsigned n=0;n<15;++n)Kill(p,91581,true,41000+n);assert(p.completions==1);}
    {Player p;p.guid={400};p.Accept(38361);p.data[{38361,91581}]=14;Kill(p,91581,false,42000);assert(p.GetQuestStatus(38361)==QUEST_STATUS_COMPLETE);} // Reconstructed counter, not a DB login test.
    {Player p;p.guid={401};Creature victim;Victim(victim,91581,43000);KillRewarder reward;reward._victim=&victim;reward._RewardPlayer(&p,false);Deliver(p);assert(!p.packets);victim.rewarded.clear();p.Accept(38361);reward._RewardPlayer(&p,false);p.quests.clear();Deliver(p);assert(!p.packets);victim.rewarded.clear();p.Accept(38361);reward._RewardPlayer(&p,false);creatures.erase(victim.guid);Deliver(p);assert(!p.packets);}
    {Player killer,near,far,released,offline;unsigned guid=500;for(auto p:{&killer,&near,&far,&released,&offline}){p->guid={guid++};p->Accept(38361);}far.rewardRange=false;released.alive=false;released.corpse=true;offline.contact=false;
        GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},b{&near,&c},a{&killer,&b};Group group;group.first=&a;Creature victim;Victim(victim,91581,44000);KillRewarder reward;reward._victim=&victim;reward._group=&group;reward.SelectGroup();
        for(auto p:{&killer,&near,&far,&released,&offline})Deliver(*p);assert(killer.packets==1&&near.packets==1&&!far.packets&&!released.packets&&!offline.packets);creatures.erase(victim.guid);}
    std::cout<<"PASS:38361 native prerequisite/race admission, owner/player/pet kill reward, template credit,15-count boundary, repeated-victim admission, cap, saved14, abandon/retry, group range/life/contact and Nathanos reward request. Nine allowed Horde races pass; unsupported Alliance races rejected. Live protected riders unchanged; no whole-client or database-login verification.\n";
}
