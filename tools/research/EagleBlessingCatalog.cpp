#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <algorithm>
#include <cctype>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!readOnly||!readOnly->Fetch()[0].GetBool())return 10;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!times.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();times.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
    DB2Storage<CreatureEntry> creatures("Creature.db2",CreatureLoadInfo::Instance());
    DB2Storage<QuestObjectiveEntry> objectives("QuestObjective.db2",QuestObjectiveLoadInfo::Instance());
    DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
    if(!creatures.Load(argv[1],0)||!objectives.Load(argv[1],0)||!lines.Load(argv[1],0))return 6;
    creatures.LoadFromDB();objectives.LoadFromDB();lines.LoadFromDB();
    DB2Storage<CreatureDifficultyEntry> difficulties("CreatureDifficulty.db2",CreatureDifficultyLoadInfo::Instance());
    if(!difficulties.Load(argv[1],0))return 7;difficulties.LoadFromDB();
    for(auto d:difficulties)if(d->CreatureID==110685||d->CreatureID==110821||d->CreatureID==100967||d->CreatureID==100990||d->CreatureID==110841||d->CreatureID==111982)std::cout<<"DIFFICULTY\t"<<d->ID<<'\t'<<d->CreatureID<<'\t'<<d->FactionID<<'\t'<<int(d->MinLevel)<<'\t'<<int(d->MaxLevel)<<'\t'<<d->Flags[0]<<'\t'<<d->Flags[1]<<'\t'<<d->Flags[2]<<'\n';
    for(uint32 id:{110685u,110821u,110936u,110937u,110938u,110901u,110985u,100967u,100990u,110841u,111982u})if(auto c=creatures.LookupEntry(id))std::cout<<"CREATURE\t"<<id<<'\t'<<c->Name->Str[0]<<'\t'<<unsigned(c->CreatureType)<<'\t'<<c->DisplayID[0]<<'\t'<<c->DisplayID[1]<<'\n';else std::cout<<"MISSING_CREATURE\t"<<id<<'\n';
    for(auto o:objectives)if(o->QuestID==39427)std::cout<<"OBJECTIVE\t"<<o->ID<<'\t'<<unsigned(o->OrderIndex)<<'\t'<<unsigned(o->Type)<<'\t'<<o->ObjectID<<'\t'<<o->Amount<<'\t'<<o->Description->Str[0]<<'\n';
    for(auto l:lines)if(l->QuestLineID==220)std::cout<<"QUEST_LINE\t"<<unsigned(l->OrderIndex)<<'\t'<<l->QuestID<<'\n';
    // Older sniff/BFA donor candidates are identities to verify in26972,
    // not an endorsement of their scripts, timings or quest shortcuts.
    std::set<uint32> pending{226319,198950,198944,198945,198946,9739,223019,220554,220557},seen;
    for(auto spell:spells){std::string name=spell->Name->Str[0];std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
      if(name.find("spiritwatch")!=std::string::npos||name.find("eagle spirit")!=std::string::npos||name.find("ohn'ahra")!=std::string::npos||name.find("highmountain hippogryph")!=std::string::npos||name.find("eagle blessing")!=std::string::npos)pending.insert(spell->ID);}
    for(auto e:effects)if((e->Effect==90&&(e->EffectMiscValue[0]==110685||e->EffectMiscValue[0]==110936||e->EffectMiscValue[0]==110937||e->EffectMiscValue[0]==110938))||(e->Effect==16&&e->EffectMiscValue[0]==39427)||(e->Effect==28&&(e->EffectMiscValue[0]==110685||e->EffectMiscValue[0]==110821)))pending.insert(e->SpellID);
    for(auto spell:spells)if(spell->ID>=220600&&spell->ID<=220810)pending.insert(spell->ID);
    DB2Storage<PhaseEntry> phases("Phase.db2",PhaseLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<TaxiNodesEntry> taxiNodes("TaxiNodes.db2",TaxiNodesLoadInfo::Instance());
    DB2Storage<TaxiPathEntry> taxiPaths("TaxiPath.db2",TaxiPathLoadInfo::Instance());
    DB2Storage<TaxiPathNodeEntry> taxiPoints("TaxiPathNode.db2",TaxiPathNodeLoadInfo::Instance());
    if(!phases.Load(argv[1],0)||!summons.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!taxiNodes.Load(argv[1],0)||!taxiPaths.Load(argv[1],0)||!taxiPoints.Load(argv[1],0))return 9;
    phases.LoadFromDB();summons.LoadFromDB();vehicles.LoadFromDB();taxiNodes.LoadFromDB();taxiPaths.LoadFromDB();taxiPoints.LoadFromDB();
    if(auto p=phases.LookupEntry(7088))std::cout<<"PHASE\t"<<p->ID<<'\t'<<p->Flags<<'\n';
    for(uint32 id:{61u,3292u,2972u})if(auto p=summons.LookupEntry(id))std::cout<<"SUMMON\t"<<p->ID<<'\t'<<p->Control<<'\t'<<p->Title<<'\t'<<p->Flags<<'\t'<<p->Slot<<'\n';
    if(auto v=vehicles.LookupEntry(4535))for(unsigned seat=0;seat<8;++seat)if(v->SeatID[seat])std::cout<<"VEHICLE_SEAT\t"<<v->ID<<'\t'<<seat<<'\t'<<v->SeatID[seat]<<'\n';
    std::set<uint32> nodes;
    for(auto n:taxiNodes)if(n->ContinentID==1220){std::string name=n->Name->Str[0];if(name.find("Spiritwatch")!=std::string::npos||name.find("Talonclaw")!=std::string::npos||name.find("Hunter")!=std::string::npos||name.find("Apata")!=std::string::npos||(n->Pos.X>-1000&&n->Pos.X<-700&&n->Pos.Y>4100&&n->Pos.Y<4450)){nodes.insert(n->ID);std::cout<<"TAXI_NODE\t"<<n->ID<<'\t'<<name<<'\t'<<n->Pos.X<<'\t'<<n->Pos.Y<<'\t'<<n->Pos.Z<<'\n';}}
    for(auto p:taxiPaths)if(nodes.count(p->FromTaxiNode)||nodes.count(p->ToTaxiNode)){uint32 count=0;for(auto point:taxiPoints)if(point->PathID==p->ID)++count;std::cout<<"TAXI_PATH\t"<<p->ID<<'\t'<<p->FromTaxiNode<<'\t'<<p->ToTaxiNode<<'\t'<<count<<'\n';}
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    if(!texts.Load(argv[1],0))return 8;texts.LoadFromDB();
    for(auto t:texts){std::string text=t->Text->Str[0];if(text.find("stubborn spirit")!=std::string::npos||text.find("Talonclaw has been lost")!=std::string::npos||text.find("Bloodtotem!")!=std::string::npos||text.find("receive her blessing")!=std::string::npos||text.find("Ohn'ahra gave her blessing")!=std::string::npos||text.find("Highmountain affairs")!=std::string::npos||text.find("my people's sacred heirloom")!=std::string::npos||text.find("wants you to follow")!=std::string::npos)std::cout<<"TEXT\t"<<t->ID<<'\t'<<text<<'\t'<<t->Text1->Str[0]<<'\t'<<t->SoundEntriesID[0]<<'\t'<<t->SoundEntriesID[1]<<'\n';}
    while(!pending.empty()){
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!seen.insert(id).second)continue;if(seen.size()>1000)return 5;
        auto s=spells.LookupEntry(id);if(!s){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id){std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\n';if(e->EffectTriggerSpell)pending.insert(e->EffectTriggerSpell);}
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);auto t=times.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\t'<<(t?t->Base:0)<<'\n';}
    }
    HotfixDatabase.Close();
}
