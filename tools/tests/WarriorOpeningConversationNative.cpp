#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include <filesystem>
#include <cstdlib>

int main(int argc,char**argv)
{
    try
    {
        Require(argc==2,"Expected native dbc directory");
        auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
        Require(config,"Private native configuration missing");
        auto directory=std::filesystem::path(argv[1]).lexically_normal();
        if(directory.filename().empty()) directory=directory.parent_path();
        std::string runtime=directory.parent_path().parent_path().string()+'/';
        char* arguments[]={argv[0],config,runtime.data()};
        InitializeNative(3,arguments);
        sConversationDataStore->LoadConversationData();
        Require(!sConversationDataStore->GetConversationData(uint32(9001386)),"Negative native conversation ID now exists");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1500,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1500,{}}});
        {
            Map map(1500,300,9999993,DIFFICULTY_N_SCENARIO);
            Actor ragnvald(&map,97167,10001,{-1923.29f,-1218.93f,0.90809f,0.0f});
            Actor ally(&map,97166,10002,{-1918.47f,-1227.38f,0.908083f,0.0f});
            Actor danica(&map,103608,10003,{-1980.04f,-1251.96f,4.83f,0.0f});
            map.LoadGrid(ragnvald.GetPositionX(),ragnvald.GetPositionY());
            Require(map.AddToMap(static_cast<Creature*>(&ragnvald)),"Native Ragnvald map admission failed");
            auto create=[&](uint32 id,Actor* caster)
            {
                auto result=new Conversation;
                if(!result->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),id,caster,nullptr,ragnvald))
                {
                    Require(!result->IsInWorld(),"Failed native conversation entered world");
                    delete result;
                    return static_cast<Conversation*>(nullptr);
                }
                return result;
            };
            Require(!create(1386,nullptr),"Native null caster accepted");
            Require(!create(9001386,&ragnvald),"Native missing data accepted");
            Require(!create(1386,&ragnvald),"Native1386 did not require its actual living97166 participant");
            std::cout<<"COUNTERFACTUAL native1386 rejects its missing97166 actor; no successful factory result or battle is asserted.\n";
            Require(map.AddToMap(static_cast<Creature*>(&ally)),"Native97166 map admission failed");
            Require(map.AddToMap(static_cast<Creature*>(&danica)),"Native103608 map admission failed");
            for(uint32 id:{770u,1386u,1405u,1412u})
            {
                Actor* caster=id==1405?&danica:&ragnvald;
                Conversation* result=create(id,caster);
                Require(result&&result->IsInWorld(),"Native original conversation construction failed after actor admission");
                uint32 expected=id==770?8250:id==1386?15400:id==1405?13000:10250;
                Require(result->GetDuration()==expected,"Native conversation lifetime differs from actual actor duration plus5000");
                auto guid=result->GetGUID();
                Require(ObjectAccessor::GetConversation(*caster,guid)==result,"Native original map lookup failed");
                if(id==770)
                    Require(result->GetActors().size()==1&&result->GetActor(0)->creatureId==97167&&result->GetActor(0)->displayId==67919,"Native770 model actor changed");
                else
                {
                    Require(result->GetCreatures().size()==(id==1412?1u:2u),"Native original creature actor count changed");
                    auto first=id==1386?&ally:id==1405?&danica:&ragnvald;
                    Require(result->GetCreature(0)->GuidLow==first->GetGUID().GetLowPart()&&result->GetCreature(0)->GuidHight==first->GetGUID().GetHighPart(),"Native actor slot0 does not resolve actual live GUID");
                    if(id!=1412)
                        Require(result->GetCreature(1)->GuidLow==ragnvald.GetGUID().GetLowPart()&&result->GetCreature(1)->GuidHight==ragnvald.GetGUID().GetHighPart(),"Native actor slot1 does not resolve original Ragnvald GUID");
                }
                std::cout<<"CONVERSATION\t"<<id<<'\t'<<result->GetDuration()<<'\t'<<result->GetLines().size()<<'\n';
                for(auto const& line:result->GetLines())
                {
                    auto native=sConversationLineStore.LookupEntry(line.id);
                    Require(native,"Native conversation line ID missing from matching26972");
                    auto text=sBroadcastTextStore.LookupEntry(native->BroadcastTextID);
                    Require(text,"Native line BroadcastText missing");
                    std::cout<<"LINE\t"<<id<<'\t'<<line.id<<'\t'<<line.textId<<'\t'<<line.unk1<<'\t'<<line.unk2<<'\t'<<native->BroadcastTextID<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
                }
                result->Remove();
                Require(!ObjectAccessor::GetConversation(*caster,guid),"Removed original conversation still resolves");
                map.RemoveAllObjectsInRemoveList();
            }
            std::cout<<"PASS native770 model actor and1386/1405/1412 actual live actor GUIDs/factory acknowledgements, map-local removal, matching26972 line/BroadcastText/sound IDs and8250/15400/13000/10250 lifetimes. Missing97166 baseline paired with admitted actors. Isolated NullCreatureAI real map/Creature casters, no Players or character saves; not original Player/group personal visibility, full933 progression, audio or client traversal.\n";
        }
        WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
    }
    catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}
