#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "CreatureTextMgr.h"
#include <filesystem>
#include <cstdlib>

int main(int argc,char** argv)
{
    try
    {
        std::cout<<std::unitbuf;
        Require(argc==2,"Expected native dbc directory");
        auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private native config missing");
        auto directory=std::filesystem::path(argv[1]).lexically_normal();
        if(directory.filename().empty())directory=directory.parent_path();
        std::string runtime=directory.parent_path().parent_path().string()+'/';
        char* arguments[]={argv[0],config,runtime.data()};InitializeNative(3,arguments);
        sCreatureTextMgr->LoadCreatureTexts();
        bool live=std::getenv("LEGION_STUDENT_VERIFY_LIVE")&&std::string(std::getenv("LEGION_STUDENT_VERIFY_LIVE"))=="1";
        Require(sCreatureTextMgr->TextExist(98074,0)==live,"Original/live native default group differs");
        // This public const view refers to the mutable loaded native store.
        // Only this isolated process's in-memory fixture is changed: no SQL
        // or production manager implementation/access specifier is altered.
        auto& holder=const_cast<CreatureTextMap&>(sCreatureTextMgr->GetTextMap()).at(98074);
        auto original=holder;auto& timed=holder.at(3);
        Require(timed.size()==2,"Original two timed student rows missing");
        for(auto& row:timed)
        {
            Require(row.BroadcastTextID==99968||row.BroadcastTextID==99970,"Unexpected timed student text");
            auto broadcast=sBroadcastTextStore.LookupEntry(row.BroadcastTextID);
            Require(broadcast&&!broadcast->SoundEntriesID[0]&&!broadcast->SoundEntriesID[1],"Native reply unexpectedly voiced");
            Require(row.sound==((!live&&row.BroadcastTextID==99968)?62044u:0u),"Original/live student sound differs");
            if(row.BroadcastTextID==99968)row.sound=0;
        }
        auto defaults=live?holder.at(0):timed;
        Require(defaults.size()==2,"Native default fixture needs two replies");
        std::sort(defaults.begin(),defaults.end(),[](CreatureTextEntry const& a,CreatureTextEntry const& b){return a.BroadcastTextID<b.BroadcastTextID;});
        for(uint32 i=0;i<defaults.size();++i)
        {
            auto& row=defaults[i];
            if(live)
            {
                Require(row.BroadcastTextID==(i?99970u:99968u)&&row.group==0&&row.id==i&&!row.MinTimer&&!row.MaxTimer&&!row.sound&&!row.duration,"Actual loaded native default row differs");
                auto retained=std::find_if(timed.begin(),timed.end(),[&](CreatureTextEntry const& item){return item.BroadcastTextID==row.BroadcastTextID;});
                Require(retained!=timed.end()&&retained->text==row.text&&retained->type==row.type&&retained->lang==row.lang&&retained->emote==row.emote&&retained->probability==row.probability,"Default changed original native text/emote/probability");
            }
            else{row.group=0;row.id=uint8(i);row.MinTimer=row.MaxTimer=0;}
            // Both real rows have duration0, which is ambiguous with failure.
            // A controlled sentinel observes the actual native dispatch and
            // repeat selection; public SQL tests separately retain duration0.
            row.duration=101*(i+1);
        }
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1014,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1014,{}}});
        {
            Map map(1014,300,9999987,DIFFICULTY_N_SCENARIO);
            for(uint32 id=0;id<4;++id)
            {
                Actor student(&map,98074,70001+id,{4034.16f,1658.45f,925.31f,1.14584f});
                student.SetPhaseId({7000},false);map.LoadGrid(student.GetPositionX(),student.GetPositionY());
                Require(map.AddToMap(static_cast<Creature*>(&student)),"Native student admission failed");
                if(!live&&id==0)
                {
                    Require(sCreatureTextMgr->SendChat(&student,0)==0,"Missing native group must not emit another group");
                    std::cout<<"COUNTERFACTUAL actual native TextExist/SendChat: existing98074 has no group0; authored default response cannot be selected. Not a client traversal.\n";
                }
                holder[0]=defaults;
                Require(sCreatureTextMgr->TextExist(98074,0),"Native default group not admitted");
                Require(!sCreatureTextMgr->GetTextGroup(98074,0),"Scripted default accidentally became a timed group");
                auto first=sCreatureTextMgr->SendChat(&student,0),second=sCreatureTextMgr->SendChat(&student,0);
                Require((first==101||first==202)&&(second==101||second==202)&&first!=second,"Native default dispatch/repeat exclusion failed");
                auto third=sCreatureTextMgr->SendChat(&student,0);Require(third==101||third==202,"Native repeat cycle failed");
                std::cout<<"NATIVE_REPLY\t"<<id<<'\t'<<first<<'\t'<<second<<'\t'<<third<<'\n';
            }
        }
        auto const* retained=sCreatureTextMgr->GetTextGroup(98074,3);Require(retained&&retained->size()==2,"Native timed group was consumed");
        for(auto const& before:original.at(3))
        {
            auto after=std::find_if(retained->begin(),retained->end(),[&](CreatureTextEntry const& row){return row.BroadcastTextID==before.BroadcastTextID;});
            Require(after!=retained->end()&&after->MinTimer==before.MinTimer&&after->MaxTimer==before.MaxTimer&&after->text==before.text&&after->group==3&&after->id==before.id&&!after->sound,"Original timed text/deadline changed beyond wrong sound");
        }
        holder=original;
        std::cout<<"PASS actual native CreatureTextMgr loader/TextExist/SendChat/default selection/repeat cycle for four real98074 actors; no default-as-timer or Chen voice override, native timed text/deadlines retained. In-memory fixture uses duration sentinels; no SQL or character mutation/Players; actual client playback remains separate. Live="<<live<<'\n';
        WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
    }
    catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
