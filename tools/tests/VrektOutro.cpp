#include <unordered_set>
#include <Windows.h>
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2Stores.h"
#include "DB2LoadInfo.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Creature.h"
#include "PassiveAI.h"
#include "Conversation.h"
#include "ConversationData.h"
#include "World.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "GameTables.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "CreatureAIRegistry.h"
#include "VMapFactory.h"
#include "VMapManager2.h"
#include "MMapFactory.h"
#include "MMapManager.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

static void Require(bool value,char const*reason){if(!value)throw std::runtime_error(reason);}

// A real Creature is the caster surrogate. This executes native spell and
// conversation creation, but does not establish a Player's visibility/packets.
struct Actor:Creature
{
    explicit Actor(Map*map)
    {
        Require(Create(10000,map,1,115846,0,ALLIANCE,1816.98f,734.469f,85.565f,0),"Native caster creation failed");
        SetHomePosition(*this);
    }
    void AddToWorld()override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)),"Native idle AI failed");
    }
    void RemoveFromWorld()override
    {
        Unit::RemoveFromWorld();
        sObjectAccessor->RemoveObject(static_cast<Creature*>(this));
    }
    ~Actor()
    {
        CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);
        if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);
        ResetMap();
    }
};

int Run(int argc,char**argv)
{
    Require(argc==3,"Expected private configuration and runtime directory");
    std::string error;
    Require(sConfigMgr->LoadInitial(argv[1],{},error),"Private configuration failed");
    sLog->Initialize(nullptr);MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
    Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Hotfix pool failed");
    Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements(),"World pool failed");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly=WorldDatabase.Query("SELECT @@session.transaction_read_only");
    Require(readOnly&&readOnly->Fetch()[0].GetBool(),"World pool is not read-only");
    sDB2Manager.LoadStores(argv[2],LOCALE_enUS);LoadGameTables(argv[2]);
    sSpellMgr->LoadSpellInfoStore();sSpellMgr->LoadTalentSpellInfo();
    sSpellMgr->LoadSkillLineAbilityMap();sSpellMgr->LoadSpellCustomAttr();
    sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
    sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();
    sObjectMgr->LoadCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();
    sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();
    sConversationDataStore->LoadConversationData();AIRegistry::Initialize();UnitAI::FillAISpellInfo();
    sWorld->LoadConfigSettings();
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220,{}}});
    HashMapHolder<Creature>::SetSize(20000);HashMapHolder<Conversation>::SetSize(20000);

    auto lines=sConversationDataStore->GetConversationData(uint32(3982));
    auto actors=sConversationDataStore->GetConversationActor(3982);
    Require(lines&&lines->size()==1&&actors&&actors->size()==1,"Vrekt outro SQL data missing");
    auto line=sConversationLineStore.LookupEntry(lines->front().id);
    auto text=line?sBroadcastTextStore.LookupEntry(line->BroadcastTextID):nullptr;
    Require(line&&line->ID==9085&&text&&text->Text&&text->Text->Str[0][0],"Native outro line/text missing");
    Require(!line->NextConversationLineID,"Native outro gained another line; audit timing");
    auto display=sCreatureDisplayInfoStore.LookupEntry(actors->front().displayId);
    Require(display&&sCreatureModelDataStore.LookupEntry(display->ModelID),"Outro actor display missing");
    // SoundKit/entry are not global server stores; use the same native typed
    // reader against the canonical runtime, without guessed packed offsets.
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2",SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> files("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    auto dbc=std::string(argv[2])+"dbc/enUS/";
    Require(sounds.Load(dbc.c_str(),0),"Native SoundKit catalog failed");
    auto info=SoundKitEntryLoadInfo::Instance();
    Require(info->Meta==SoundKitEntryMeta::Instance(),"SoundKitEntry uses another table's metadata");
    DB2Header header{};std::ifstream raw(dbc+"SoundKitEntry.db2",std::ios::binary);
    Require(bool(raw.read(reinterpret_cast<char*>(&header),sizeof(header)))&&
        header.LayoutHash==info->Meta->LayoutHash,"Native header/layout mismatch");
    // Reproduce the exact pre-fix metadata substitution without inventing a
    // packed row decoder or changing the canonical file.
    DB2LoadInfo wrong(info->Fields,info->FieldCount,SoundKitMeta::Instance(),info->Statement);
    DB2Storage<SoundKitEntryEntry> broken("SoundKitEntry.db2",&wrong);
    Require(!broken.Load(dbc.c_str(),0),"Old SoundKit metadata no longer reproduces rejection");
    bool filesLoaded=files.Load(dbc.c_str(),0);
    Require(filesLoaded,"Native SoundKitEntry reader rejected the canonical file");
    bool soundFound=false,fileFound=false;
    for(auto sound:text->SoundEntriesID)if(sound&&sounds.LookupEntry(sound))
    {
        soundFound=true;
        for(auto file:files)if(file->SoundKitID==sound&&file->FileDataID)
        {
            fileFound=true;
            std::cout<<"OUTROFILE "<<file->ID<<" kit="<<file->SoundKitID<<" file="<<file->FileDataID
                <<" frequency="<<uint32(file->Frequency)<<" volume="<<file->Volume<<'\n';
        }
    }
    Require(soundFound&&fileFound,"Outro broadcast has no native sound kit/file link");
    std::cout<<"OUTRO SoundKitEntry reader accepted="<<filesLoaded<<" linked-file="<<fileFound
        <<"; physical audio payload and client playback are separate checks\n";
    std::cout<<"OUTRO line="<<line->ID<<" broadcast="<<text->ID<<" sound="<<text->SoundEntriesID[0]
        <<" actor="<<actors->front().creatureId<<" display="<<actors->front().displayId<<'\n';
    {
        Map map(1220,300,9999999,DIFFICULTY_NONE);Actor caster(&map);
        map.LoadGrid(caster.GetPositionX(),caster.GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(&caster)),"Caster map admission failed");
        auto first=sObjectMgr->GetGenerator<HighGuid::Conversation>()->GetNextAfterMaxUsed();
        Require(caster.CastSpell(&caster,230157,true)==SPELL_CAST_OK,"Native outro cast rejected");
        for(uint32 elapsed=0;elapsed<1000;elapsed+=10)caster.Unit::Update(10);
        auto next=sObjectMgr->GetGenerator<HighGuid::Conversation>()->GetNextAfterMaxUsed();
        Require(next==first+1,"Native spell did not create exactly one conversation");
        auto conversation=ObjectAccessor::GetConversation(caster,ObjectGuid::Create<HighGuid::Conversation>(1220,3982,first));
        Require(conversation&&conversation->IsInWorld()&&conversation->GetCaster()==&caster,"Native conversation was not admitted/bound");
        auto actualLine=conversation->GetLine(0);auto actualActor=conversation->GetActor(0);
        Require(actualLine&&actualLine->id==9085&&actualLine->textId==lines->front().textId&&
            actualLine->unk1==lines->front().unk1&&actualActor&&actualActor->actorId==actors->front().actorId&&
            actualActor->creatureId==actors->front().creatureId&&actualActor->displayId==actors->front().displayId,
            "Native dynamic conversation payload differs from live SQL");
        Require(conversation->GetDuration()==int32(actors->front().duration+5000)&&
            conversation->GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME)==uint32(conversation->GetDuration()),
            "Native outro duration changed");
        std::cout<<"OUTRO native spell230157 -> conversation3982, duration="<<conversation->GetDuration()<<"ms\n";
        conversation->Remove();map.RemoveAllObjectsInRemoveList();
        Require(!ObjectAccessor::GetConversation(caster,ObjectGuid::Create<HighGuid::Conversation>(1220,3982,first)),"Conversation cleanup failed");
    }
    HotfixDatabase.Close();WorldDatabase.Close();
    std::cout<<"PASS: linked native effect219, conversation creation, live actors/lines, typed DB2 text/sound-kit/model, map binding and cleanup. No Player/client or physical audio playback claim.\n";
    return 0;
}
int main(int argc,char**argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    try{return Run(argc,argv);}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
}
