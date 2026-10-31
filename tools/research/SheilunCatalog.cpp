#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <string>

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<GameObjectsEntry> objects("GameObjects.db2",GameobjectsLoadInfo::Instance());
    if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!spells.Load(argv[1],0)||
       !effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!objects.Load(argv[1],0))return 5;
    steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();objects.LoadFromDB();
    for(auto step:steps)if(step->ScenarioID==1007)
    {
        std::cout<<"STEP\t"<<step->ScenarioID<<'\t'<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\t'<<step->Title->Str[0]<<'\t'<<step->Description->Str[0]<<'\n';
        std::set<uint32> todo{uint32(step->Criteriatreeid)},seen;
        while(!todo.empty())
        {
            auto id=*todo.begin();todo.erase(id);if(!seen.insert(id).second)continue;
            if(auto tree=trees.LookupEntry(id))
            {
                if(auto c=criteria.LookupEntry(tree->CriteriaID))std::cout<<"CRITERION\t"<<unsigned(step->OrderIndex)<<'\t'<<tree->ID<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';
                for(auto child:trees)if(child->Parent==id)todo.insert(child->ID);
            }
        }
    }
    std::set<uint32> selected{202435,202438,194151,204779,204773,204237};
    for(auto effect:effects)if((effect->Effect==61&&(effect->EffectMiscValue[0]==48495||effect->EffectMiscValue[0]==48646))||
       (effect->Effect==90&&(effect->EffectMiscValue[0]==101880||effect->EffectMiscValue[0]==102479))||
       effect->EffectMiscValue[0]==256913||effect->EffectMiscValue[0]==102479||effect->EffectMiscValue[0]==102505)selected.insert(effect->SpellID);
    for(unsigned depth=0;depth<3;++depth)for(auto effect:effects)if(selected.count(effect->SpellID)&&effect->EffectTriggerSpell)selected.insert(effect->EffectTriggerSpell);
    for(auto id:selected)
    {
        if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)
            std::cout<<"EFFECT\t"<<id<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';
        for(auto m:misc)if(m->SpellID==id&&!m->DifficultyID)
            if(auto d=durations.LookupEntry(m->DurationIndex))std::cout<<"DURATION\t"<<id<<'\t'<<d->Duration<<'\t'<<d->MaxDuration<<'\n';
    }
    for(auto object:objects)
    {
        std::string name=object->Name->Str[0];
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(object->OwnerID!=1541&&object->DisplayID!=9806&&name.find("sheilun")==std::string::npos&&name.find("sheylun")==std::string::npos&&name.find("shay-lun")==std::string::npos)continue;
        std::cout<<"CLIENT_GO\t"<<object->ID<<'\t'<<object->Name->Str[0]<<'\t'<<object->OwnerID<<'\t'<<unsigned(object->TypeID)<<'\t'<<object->DisplayID<<'\t'<<object->Position.X<<'\t'<<object->Position.Y<<'\t'<<object->Position.Z<<'\t'<<object->PhaseID;
        for(auto value:object->PropValue)std::cout<<'\t'<<value;
        std::cout<<'\n';
    }
    std::cout<<"EOF\tsheilun\n";HotfixDatabase.Close();MySQL::Library_End();
}
